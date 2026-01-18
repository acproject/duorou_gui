#pragma once

#include <editor/editor_view_common.hpp>
#include <editor/editor_view_tree_ops.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace duorou::ui::editor {

struct InsertPlan {
  bool valid{};
  std::string where;
  std::string axis;
  std::string container_key;
  std::string parent_key;
  std::int64_t index{};
  RectF container_frame{};
  std::string indicator_kind;
  RectF indicator_rect{};
};

inline InsertPlan compute_insert_plan(const ViewNode &design_root, float x, float y,
                                      std::string_view ignore_key = {}) {
  InsertPlan out;

  const auto hit = ::duorou::ui::hit_key_at(x, y);
  if (hit.empty()) {
    return out;
  }

  std::string target_key;
  if (hit == "editor:canvas") {
    target_key = "design:root";
  } else if (starts_with(hit, "design:")) {
    target_key = hit;
  } else {
    return out;
  }

  auto path = find_path_by_key(design_root, target_key);
  if (design_root.key != target_key && path.empty()) {
    return out;
  }

  if (!ignore_key.empty() && target_key == ignore_key && target_key != design_root.key) {
    if (!path.empty()) {
      path.pop_back();
      if (path.empty()) {
        target_key = design_root.key;
      } else if (auto *p = node_at_path_mut(*const_cast<ViewNode *>(&design_root), path)) {
        target_key = p->key;
      }
    }
  }

  auto *root_mut = const_cast<ViewNode *>(&design_root);
  ViewNode *target = root_mut;
  if (design_root.key != target_key) {
    target = node_at_path_mut(*root_mut, path);
  }
  if (!target || target->key.empty()) {
    return out;
  }

  auto get_parent = [&](const std::vector<std::size_t> &p) -> ViewNode * {
    if (p.empty()) {
      return nullptr;
    }
    auto pp = p;
    pp.pop_back();
    return pp.empty() ? root_mut : node_at_path_mut(*root_mut, pp);
  };

  auto compute_inside_index = [&](const ViewNode &c, std::string axis,
                                  RectF container_frame) -> std::pair<std::int64_t, RectF> {
    const bool h = (axis == "h");
    std::int64_t idx = 0;
    std::optional<RectF> prev;
    std::optional<RectF> next;
    std::optional<RectF> first;
    std::optional<RectF> last;

    const auto n = static_cast<std::int64_t>(c.children.size());
    for (std::int64_t i = 0; i < n; ++i) {
      const auto &ch = c.children[static_cast<std::size_t>(i)];
      if (ch.key.empty()) {
        continue;
      }
      const auto fr = active_layout_frame_by_key(ch.key);
      if (!fr) {
        continue;
      }
      if (!first) {
        first = *fr;
      }
      last = *fr;

      const float mid = h ? (fr->x + fr->w * 0.5f) : (fr->y + fr->h * 0.5f);
      const float p = h ? x : y;
      if (p < mid) {
        idx = i;
        next = *fr;
        break;
      }
      prev = *fr;
      idx = i + 1;
    }

    RectF line{};
    const float t = 2.0f;
    if (h) {
      float line_x = container_frame.x;
      if (idx == 0 && first) {
        line_x = first->x;
      } else if (idx == n && last) {
        line_x = last->x + last->w;
      } else if (prev && next) {
        line_x = (prev->x + prev->w + next->x) * 0.5f;
      }
      line = RectF{line_x - t * 0.5f, container_frame.y, t, container_frame.h};
    } else {
      float line_y = container_frame.y;
      if (idx == 0 && first) {
        line_y = first->y;
      } else if (idx == n && last) {
        line_y = last->y + last->h;
      } else if (prev && next) {
        line_y = (prev->y + prev->h + next->y) * 0.5f;
      }
      line = RectF{container_frame.x, line_y - t * 0.5f, container_frame.w, t};
    }
    return {idx, line};
  };

  auto set_inside_for_container = [&](const ViewNode &c) {
    out.valid = true;
    out.where = "inside";
    out.parent_key.clear();
    out.container_key = c.key;
    out.axis = c.type == "Row" ? "h" : "v";
    out.index = static_cast<std::int64_t>(c.children.size());
    if (const auto cf = active_layout_frame_by_key(c.key)) {
      out.container_frame = *cf;
      if (c.type == "Column" || c.type == "Row" || c.type == "ScrollView" || c.type == "Grid") {
        if (!c.children.empty()) {
          const auto [idx, line] = compute_inside_index(c, out.axis, out.container_frame);
          out.index = idx;
          out.indicator_kind = out.axis == "h" ? "line_v" : "line_h";
          out.indicator_rect = line;
          return;
        }
      }
      out.indicator_kind = "box";
      out.indicator_rect = out.container_frame;
    }
  };

  const auto target_frame_opt = active_layout_frame_by_key(target->key);

  auto *direct_parent = get_parent(path);

  if (is_container_type(target->type)) {
    if (direct_parent && is_container_type(direct_parent->type) && !path.empty() && target_frame_opt) {
      const auto axis_parent = direct_parent->type == "Row" ? "h" : "v";
      const bool h = (axis_parent == "h");
      const RectF fr = *target_frame_opt;
      const float primary = h ? fr.w : fr.h;
      const float edge = std::max(6.0f, std::min(12.0f, primary * 0.25f));
      const float p = h ? x : y;
      const float start = h ? fr.x : fr.y;
      const float end = start + primary;
      const auto idx = path.back();

      if (p < start + edge) {
        out.valid = true;
        out.where = "before";
        out.axis = axis_parent;
        out.parent_key = direct_parent->key;
        out.index = static_cast<std::int64_t>(idx);
        out.indicator_kind = h ? "line_v" : "line_h";
        out.indicator_rect = h ? RectF{start - 1.0f, fr.y, 2.0f, fr.h}
                               : RectF{fr.x, start - 1.0f, fr.w, 2.0f};
        return out;
      }
      if (p > end - edge) {
        out.valid = true;
        out.where = "after";
        out.axis = axis_parent;
        out.parent_key = direct_parent->key;
        out.index = static_cast<std::int64_t>(idx + 1);
        out.indicator_kind = h ? "line_v" : "line_h";
        out.indicator_rect = h ? RectF{end - 1.0f, fr.y, 2.0f, fr.h}
                               : RectF{fr.x, end - 1.0f, fr.w, 2.0f};
        return out;
      }
    }

    set_inside_for_container(*target);
    return out;
  }

  if (direct_parent && is_container_type(direct_parent->type) && !path.empty() && target_frame_opt) {
    const auto axis_parent = direct_parent->type == "Row" ? "h" : "v";
    const bool h = (axis_parent == "h");
    const RectF fr = *target_frame_opt;
    const float primary = h ? fr.w : fr.h;
    const float p = h ? x : y;
    const float start = h ? fr.x : fr.y;
    const float mid = start + primary * 0.5f;
    const auto idx = path.back();

    out.valid = true;
    out.axis = axis_parent;
    out.parent_key = direct_parent->key;
    out.indicator_kind = h ? "line_v" : "line_h";
    if (p < mid) {
      out.where = "before";
      out.index = static_cast<std::int64_t>(idx);
      out.indicator_rect = h ? RectF{start - 1.0f, fr.y, 2.0f, fr.h}
                             : RectF{fr.x, start - 1.0f, fr.w, 2.0f};
    } else {
      out.where = "after";
      out.index = static_cast<std::int64_t>(idx + 1);
      out.indicator_rect = h ? RectF{(start + primary) - 1.0f, fr.y, 2.0f, fr.h}
                             : RectF{fr.x, (start + primary) - 1.0f, fr.w, 2.0f};
    }
    return out;
  }

  std::vector<std::size_t> container_path = path;
  if (!container_path.empty()) {
    container_path.pop_back();
  }
  ViewNode *container = container_path.empty() ? root_mut : node_at_path_mut(*root_mut, container_path);
  while (container && !is_container_type(container->type)) {
    if (container_path.empty()) {
      container = nullptr;
      break;
    }
    container_path.pop_back();
    container = container_path.empty() ? root_mut : node_at_path_mut(*root_mut, container_path);
  }
  if (container && is_container_type(container->type) && !container->key.empty()) {
    set_inside_for_container(*container);
  }
  return out;
}

inline ViewNode drag_ghost(StateHandle<std::string> drag_type, StateHandle<double> drag_x,
                           StateHandle<double> drag_y) {
  const float x = static_cast<float>(drag_x.get());
  const float y = static_cast<float>(drag_y.get());
  auto bubble =
      view("Box")
          .prop("padding", 10.0)
          .prop("bg", 0xCC202020)
          .prop("border", 0xFF3A3A3A)
          .prop("border_width", 1.0)
          .prop("hit_test", false)
          .children({view("Text")
                         .prop("value", drag_type.get().empty() ? std::string{"(drag)"}
                                                               : drag_type.get())
                         .prop("font_size", 12.0)
                         .build()})
          .build();
  return view("Column")
      .prop("hit_test", false)
      .children([&](auto &c) {
        c.add(view("Spacer").prop("height", y).prop("hit_test", false).build());
        c.add(view("Row")
                  .prop("hit_test", false)
                  .children([&](auto &r) {
                    r.add(view("Spacer").prop("width", x).prop("hit_test", false).build());
                    r.add(std::move(bubble));
                  })
                  .build());
      })
      .build();
}

inline ViewNode insert_indicator(StateHandle<bool> insert_show,
                                 StateHandle<std::string> insert_mode,
                                 StateHandle<std::string> insert_axis,
                                 StateHandle<std::string> insert_kind,
                                 StateHandle<double> insert_fx,
                                 StateHandle<double> insert_fy,
                                 StateHandle<double> insert_fw,
                                 StateHandle<double> insert_fh,
                                 float viewport_w, float viewport_h) {
  if (!insert_show.get()) {
    return view("Spacer").prop("hit_test", false).build();
  }

  const auto mode = insert_mode.get();
  const auto axis = insert_axis.get();
  const auto kind = insert_kind.get();
  const float fx = static_cast<float>(insert_fx.get());
  const float fy = static_cast<float>(insert_fy.get());
  const float fw = static_cast<float>(insert_fw.get());
  const float fh = static_cast<float>(insert_fh.get());
  if (mode.empty() || fw <= 0.0f || fh <= 0.0f) {
    return view("Spacer").prop("hit_test", false).build();
  }

  const RectF r{fx, fy, fw, fh};
  const ColorU8 stroke{128, 160, 255, 255};
  const ColorU8 fill{128, 160, 255, 40};
  const float t = 2.0f;
  const float cap = 6.0f;

  auto node = Canvas(
      "editor:insert_indicator",
      [r, axis, kind, mode, stroke, fill, t, cap](RectF, std::vector<RenderOp> &out) {
        if (kind == "box") {
          out.push_back(DrawRect{RectF{r.x, r.y, r.w, r.h}, fill});
          out.push_back(DrawRect{RectF{r.x, r.y, r.w, t}, stroke});
          out.push_back(DrawRect{RectF{r.x, r.y + r.h - t, r.w, t}, stroke});
          out.push_back(DrawRect{RectF{r.x, r.y, t, r.h}, stroke});
          out.push_back(DrawRect{RectF{r.x + r.w - t, r.y, t, r.h}, stroke});
          return;
        }

        if (kind == "line_v") {
          const float x = r.x + r.w * 0.5f;
          out.push_back(DrawRect{RectF{x - t * 0.5f, r.y, t, r.h}, stroke});
          out.push_back(DrawRect{RectF{x - cap * 0.5f, r.y, cap, cap}, stroke});
          out.push_back(
              DrawRect{RectF{x - cap * 0.5f, r.y + r.h - cap, cap, cap}, stroke});
          return;
        }

        if (kind == "line_h") {
          const float y = r.y + r.h * 0.5f;
          out.push_back(DrawRect{RectF{r.x, y - t * 0.5f, r.w, t}, stroke});
          out.push_back(DrawRect{RectF{r.x, y - cap * 0.5f, cap, cap}, stroke});
          out.push_back(
              DrawRect{RectF{r.x + r.w - cap, y - cap * 0.5f, cap, cap}, stroke});
          return;
        }

        if (mode == "inside") {
          out.push_back(DrawRect{RectF{r.x, r.y, r.w, r.h}, fill});
          out.push_back(DrawRect{RectF{r.x, r.y, r.w, t}, stroke});
          out.push_back(DrawRect{RectF{r.x, r.y + r.h - t, r.w, t}, stroke});
          out.push_back(DrawRect{RectF{r.x, r.y, t, r.h}, stroke});
          out.push_back(DrawRect{RectF{r.x + r.w - t, r.y, t, r.h}, stroke});
          return;
        }

        const bool h = (axis == "h");
        const bool before = (mode == "before");
        if (h) {
          const float x = before ? r.x : (r.x + r.w);
          out.push_back(DrawRect{RectF{x - t * 0.5f, r.y, t, r.h}, stroke});
          out.push_back(DrawRect{RectF{x - cap * 0.5f, r.y, cap, cap}, stroke});
          out.push_back(
              DrawRect{RectF{x - cap * 0.5f, r.y + r.h - cap, cap, cap}, stroke});
        } else {
          const float y = before ? r.y : (r.y + r.h);
          out.push_back(DrawRect{RectF{r.x, y - t * 0.5f, r.w, t}, stroke});
          out.push_back(DrawRect{RectF{r.x, y - cap * 0.5f, cap, cap}, stroke});
          out.push_back(
              DrawRect{RectF{r.x + r.w - cap, y - cap * 0.5f, cap, cap}, stroke});
        }
      },
      viewport_w, viewport_h);
  node.props.insert_or_assign("hit_test", PropValue{false});
  return node;
}

} // namespace duorou::ui::editor
