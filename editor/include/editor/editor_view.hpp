#pragma once

#include <duorou/ui/runtime.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

namespace duorou::ui::editor {

inline ViewNode panel(std::string title, ViewNode content, float width) {
  return view("Box")
      .prop("width", width)
      .prop("bg", 0xFF1B1B1B)
      .prop("border", 0xFF3A3A3A)
      .prop("border_width", 1.0)
      .children({
          view("Column")
              .prop("padding", 12.0)
              .prop("spacing", 10.0)
              .prop("cross_align", "stretch")
              .children({
                  view("Text")
                      .prop("value", std::move(title))
                      .prop("font_size", 14.0)
                      .prop("color", 0xFFE0E0E0)
                      .build(),
                  view("Divider").prop("thickness", 1.0).prop("color", 0xFF2A2A2A).build(),
                  std::move(content),
              })
              .build(),
      })
      .build();
}

inline bool starts_with(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

inline bool is_container_type(std::string_view type) {
  return type == "Column" || type == "Row" || type == "Box" || type == "Overlay" ||
         type == "Grid" || type == "ScrollView";
}

inline ViewNode *find_node_by_key_mut(ViewNode &root, std::string_view key) {
  if (key.empty()) {
    return nullptr;
  }
  if (root.key == key) {
    return &root;
  }
  for (auto &c : root.children) {
    if (auto *r = find_node_by_key_mut(c, key)) {
      return r;
    }
  }
  return nullptr;
}

inline bool find_path_by_key_impl(const ViewNode &root, std::string_view key,
                                 std::vector<std::size_t> &path) {
  if (key.empty()) {
    return false;
  }
  if (root.key == key) {
    return true;
  }
  for (std::size_t i = 0; i < root.children.size(); ++i) {
    path.push_back(i);
    if (find_path_by_key_impl(root.children[i], key, path)) {
      return true;
    }
    path.pop_back();
  }
  return false;
}

inline std::vector<std::size_t> find_path_by_key(const ViewNode &root,
                                                 std::string_view key) {
  std::vector<std::size_t> path;
  (void)find_path_by_key_impl(root, key, path);
  return path;
}

inline ViewNode *node_at_path_mut(ViewNode &root,
                                  const std::vector<std::size_t> &path) {
  ViewNode *cur = &root;
  for (const auto idx : path) {
    if (idx >= cur->children.size()) {
      return nullptr;
    }
    cur = &cur->children[idx];
  }
  return cur;
}

inline ViewInstance *active_instance() {
  if (!::duorou::ui::detail::active_dispatch_context) {
    return nullptr;
  }
  return ::duorou::ui::detail::active_dispatch_context->instance;
}

inline std::optional<RectF> active_layout_frame_by_key(const std::string &key) {
  if (auto *inst = active_instance()) {
    return inst->layout_frame_by_key(key);
  }
  return std::nullopt;
}

struct InsertPlan {
  bool valid{};
  std::string where;
  std::string axis;
  std::string container_key;
  std::string parent_key;
  std::int64_t index{};
  RectF container_frame{};
};

inline InsertPlan compute_insert_plan(const ViewNode &design_root, float x, float y) {
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

  auto *root_mut = const_cast<ViewNode *>(&design_root);
  auto *container = root_mut;
  std::vector<std::size_t> container_path = path;
  if (design_root.key != target_key) {
    container = node_at_path_mut(*root_mut, container_path);
  }
  while (container && !is_container_type(container->type)) {
    if (container_path.empty()) {
      break;
    }
    container_path.pop_back();
    container = node_at_path_mut(*root_mut, container_path);
  }
  if (!container || !is_container_type(container->type) || container->key.empty()) {
    return out;
  }

  out.valid = true;
  out.container_key = container->key;
  out.where = "inside";
  out.axis = "v";
  out.parent_key.clear();
  out.index = static_cast<std::int64_t>(container->children.size());

  if (const auto fr = active_layout_frame_by_key(out.container_key)) {
    out.container_frame = *fr;
  }

  if (out.container_key == "design:root") {
    return out;
  }

  if (!container_path.empty()) {
    const auto container_idx = container_path.back();
    auto parent_path = container_path;
    parent_path.pop_back();
    const auto *parent =
        parent_path.empty() ? &design_root : node_at_path_mut(*root_mut, parent_path);
    if (parent && !parent->key.empty()) {
      out.parent_key = parent->key;
      out.axis = parent->type == "Row" ? "h" : "v";
      out.index = static_cast<std::int64_t>(container_idx + 1);
      out.where = "after";
      if (out.container_frame.w > 0.0f && out.container_frame.h > 0.0f) {
        const float primary = out.axis == "h" ? out.container_frame.w : out.container_frame.h;
        const float t = std::max(6.0f, std::min(12.0f, primary * 0.25f));
        if (out.axis == "h") {
          if (x < out.container_frame.x + t) {
            out.where = "before";
            out.index = static_cast<std::int64_t>(container_idx);
          } else if (x > out.container_frame.x + out.container_frame.w - t) {
            out.where = "after";
            out.index = static_cast<std::int64_t>(container_idx + 1);
          } else {
            out.where = "inside";
            out.index = static_cast<std::int64_t>(container->children.size());
          }
        } else {
          if (y < out.container_frame.y + t) {
            out.where = "before";
            out.index = static_cast<std::int64_t>(container_idx);
          } else if (y > out.container_frame.y + out.container_frame.h - t) {
            out.where = "after";
            out.index = static_cast<std::int64_t>(container_idx + 1);
          } else {
            out.where = "inside";
            out.index = static_cast<std::int64_t>(container->children.size());
          }
        }
      } else {
        out.where = "inside";
        out.index = static_cast<std::int64_t>(container->children.size());
      }
    }
  }

  return out;
}

inline ViewNode make_default_node(TextureHandle demo_tex_handle,
                                 std::string_view type, std::string key) {
  if (type == "Button") {
    return view("Button").key(std::move(key)).prop("title", "Button").build();
  }
  if (type == "Text") {
    return view("Text").key(std::move(key)).prop("value", "Text").build();
  }
  if (type == "TextField") {
    return view("TextField")
        .key(std::move(key))
        .prop("value", "")
        .prop("placeholder", "Input")
        .prop("width", 260.0)
        .build();
  }
  if (type == "Image") {
    return view("Image")
        .key(std::move(key))
        .prop("texture", static_cast<std::int64_t>(demo_tex_handle))
        .prop("width", 64.0)
        .prop("height", 64.0)
        .build();
  }
  if (type == "Row") {
    return view("Row")
        .key(std::move(key))
        .prop("spacing", 10.0)
        .prop("cross_align", "center")
        .children({})
        .build();
  }
  if (type == "Box") {
    return view("Box")
        .key(std::move(key))
        .prop("padding", 12.0)
        .prop("bg", 0xFF202020)
        .prop("border", 0xFF3A3A3A)
        .prop("border_width", 1.0)
        .children({})
        .build();
  }
  return view("Column")
      .key(std::move(key))
      .prop("spacing", 12.0)
      .prop("cross_align", "start")
      .children({})
      .build();
}

inline void push_history(StateHandle<std::vector<ViewNode>> history,
                         StateHandle<std::int64_t> history_idx,
                         const ViewNode &root) {
  auto h = history.get();
  auto idx = history_idx.get();
  if (idx >= 0 && idx < static_cast<std::int64_t>(h.size())) {
    h.resize(static_cast<std::size_t>(idx + 1));
  } else if (!h.empty() && idx != static_cast<std::int64_t>(h.size() - 1)) {
    idx = static_cast<std::int64_t>(h.size() - 1);
  }
  h.push_back(root);
  history.set(std::move(h));
  history_idx.set(static_cast<std::int64_t>(history.get().size() - 1));
}

inline ViewNode tree_panel(TextureHandle demo_tex_handle,
                           StateHandle<ViewNode> design_root,
                           StateHandle<std::string> selected_key,
                           StateHandle<std::vector<ViewNode>> history,
                           StateHandle<std::int64_t> history_idx) {
  auto drag_active = local_state<bool>("editor:lib_drag_active", false);
  auto drag_type = local_state<std::string>("editor:lib_drag_type", "");
  auto drag_x = local_state<double>("editor:lib_drag_x", 0.0);
  auto drag_y = local_state<double>("editor:lib_drag_y", 0.0);
  auto node_counter = local_state<std::int64_t>("editor:node_counter", 1);
  auto insert_show = local_state<bool>("editor:insert_show", false);
  auto insert_mode = local_state<std::string>("editor:insert_mode", "");
  auto insert_axis = local_state<std::string>("editor:insert_axis", "");
  auto insert_fx = local_state<double>("editor:insert_fx", 0.0);
  auto insert_fy = local_state<double>("editor:insert_fy", 0.0);
  auto insert_fw = local_state<double>("editor:insert_fw", 0.0);
  auto insert_fh = local_state<double>("editor:insert_fh", 0.0);

  auto add_node = [demo_tex_handle, design_root, selected_key, history, history_idx,
                   node_counter](std::string type, InsertPlan plan) mutable {
    if (type.empty()) {
      return;
    }
    if (!plan.valid) {
      return;
    }

    auto root = design_root.get();
    if (root.type.empty()) {
      root = view("Column")
                 .key("design:root")
                 .prop("spacing", 12.0)
                 .prop("cross_align", "start")
                 .children({})
                 .build();
    }

    const auto next_id = node_counter.get();
    node_counter.set(next_id + 1);
    std::string new_key = "design:n" + std::to_string(static_cast<long long>(next_id));
    auto node = make_default_node(demo_tex_handle, type, new_key);

    if (plan.where == "inside") {
      ViewNode *container = find_node_by_key_mut(root, plan.container_key);
      if (!container || !is_container_type(container->type)) {
        container = find_node_by_key_mut(root, "design:root");
      }
      if (!container) {
        return;
      }
      const auto idx = std::clamp<std::int64_t>(
          plan.index, 0, static_cast<std::int64_t>(container->children.size()));
      container->children.insert(container->children.begin() + static_cast<std::ptrdiff_t>(idx),
                                 std::move(node));
    } else {
      ViewNode *parent = find_node_by_key_mut(root, plan.parent_key);
      if (!parent || !is_container_type(parent->type)) {
        parent = find_node_by_key_mut(root, "design:root");
      }
      if (!parent) {
        return;
      }
      const auto idx = std::clamp<std::int64_t>(
          plan.index, 0, static_cast<std::int64_t>(parent->children.size()));
      parent->children.insert(parent->children.begin() + static_cast<std::ptrdiff_t>(idx),
                              std::move(node));
    }
    push_history(history, history_idx, root);
    design_root.set(std::move(root));
    selected_key.set(new_key);
  };

  auto lib_item = [&](std::string type) -> ViewNode {
    auto node = view("Box")
                    .prop("padding", 10.0)
                    .prop("bg", 0xFF151515)
                    .prop("border", 0xFF2A2A2A)
                    .prop("border_width", 1.0)
                    .children({view("Text").prop("value", type).build()})
                    .build();

    node = DragGesture(
        std::move(node), "editor:lib:" + type,
        [drag_active, drag_type, drag_x, drag_y, insert_show, insert_mode, insert_axis,
         insert_fx, insert_fy, insert_fw, insert_fh, design_root, type](DragGestureValue v) mutable {
          drag_active.set(true);
          drag_type.set(type);
          drag_x.set(static_cast<double>(v.x));
          drag_y.set(static_cast<double>(v.y));
          auto plan = compute_insert_plan(design_root.get(), v.x, v.y);
          insert_show.set(plan.valid);
          insert_mode.set(plan.where);
          insert_axis.set(plan.axis);
          insert_fx.set(static_cast<double>(plan.container_frame.x));
          insert_fy.set(static_cast<double>(plan.container_frame.y));
          insert_fw.set(static_cast<double>(plan.container_frame.w));
          insert_fh.set(static_cast<double>(plan.container_frame.h));
        },
        [drag_active, drag_type, drag_x, drag_y, insert_show, insert_mode, insert_axis,
         insert_fx, insert_fy, insert_fw, insert_fh, add_node, design_root, type](DragGestureValue v) mutable {
          drag_active.set(false);
          drag_type.set("");
          drag_x.set(static_cast<double>(v.x));
          drag_y.set(static_cast<double>(v.y));
          auto plan = compute_insert_plan(design_root.get(), v.x, v.y);
          add_node(type, std::move(plan));
          insert_show.set(false);
          insert_mode.set("");
          insert_axis.set("");
          insert_fx.set(0.0);
          insert_fy.set(0.0);
          insert_fw.set(0.0);
          insert_fh.set(0.0);
        },
        4.0f);
    return node;
  };

  auto root = design_root.get();

  auto tree_list = view("Column")
                       .prop("spacing", 6.0)
                       .prop("cross_align", "start")
                       .children([&](auto &c) {
                         auto add_tree = [&](auto &&self, const ViewNode &n, int depth) -> void {
                           if (n.key.empty()) {
                             for (const auto &ch : n.children) {
                               self(self, ch, depth);
                             }
                             return;
                           }
                           std::string label;
                           label.reserve(static_cast<std::size_t>(depth * 2 + 64));
                           for (int i = 0; i < depth; ++i) {
                             label += "  ";
                           }
                           label += n.type;
                           const bool is_sel = (n.key == selected_key.get());
                           auto row =
                               view("Text")
                                   .prop("value", label)
                                   .prop("font_size", 13.0)
                                   .prop("color", is_sel ? 0xFF80A0FF : 0xFFE0E0E0)
                                   .build();
                           row = onTapGesture(std::move(row),
                                              [selected_key, k = n.key]() mutable {
                                                selected_key.set(k);
                                              });
                           c.add(std::move(row));
                           for (const auto &ch : n.children) {
                             self(self, ch, depth + 1);
                           }
                         };
                         if (!root.type.empty()) {
                           add_tree(add_tree, root, 0);
                         }
                       })
                       .build();

  return view("ScrollView")
      .prop("clip", true)
      .prop("default_width", 260.0)
      .prop("default_height", 600.0)
      .children({
          view("Column")
              .prop("spacing", 10.0)
              .prop("cross_align", "stretch")
              .children({
                  view("Text").prop("value", "Library").prop("font_size", 12.0).prop("color", 0xFFB0B0B0).build(),
                  view("Column")
                      .prop("spacing", 8.0)
                      .prop("cross_align", "stretch")
                      .children({
                          lib_item("Button"),
                          lib_item("Text"),
                          lib_item("TextField"),
                          lib_item("Image"),
                          lib_item("Column"),
                          lib_item("Row"),
                          lib_item("Box"),
                      })
                      .build(),
                  view("Divider").prop("thickness", 1.0).prop("color", 0xFF2A2A2A).build(),
                  view("Text").prop("value", "Tree").prop("font_size", 12.0).prop("color", 0xFFB0B0B0).build(),
                  std::move(tree_list),
              })
              .build(),
      })
      .build();
}

inline ViewNode preview_panel(TextureHandle demo_tex, float width, float height,
                              int layout_id,
                              StateHandle<std::string> selected_key) {
  auto content = [&]() -> ViewNode {
    if (layout_id == 1) {
      return view("Column")
          .prop("spacing", 12.0)
          .prop("cross_align", "start")
          .children({
              view("Text")
                  .prop("value", "Login Form")
                  .prop("font_size", 16.0)
                  .prop("color", 0xFFE0E0E0)
                  .build(),
              onTapGesture(
                  view("TextField")
                      .key("preview:form:username")
                      .prop("value", "")
                      .prop("placeholder", "Username")
                      .prop("width", 320.0)
                      .build(),
                  [selected_key]() mutable { selected_key.set("preview:form:username"); }),
              onTapGesture(
                  view("TextField")
                      .key("preview:form:password")
                      .prop("value", "")
                      .prop("placeholder", "Password")
                      .prop("width", 320.0)
                      .build(),
                  [selected_key]() mutable { selected_key.set("preview:form:password"); }),
              view("Row")
                  .prop("spacing", 10.0)
                  .prop("cross_align", "center")
                  .children({
                      onTapGesture(
                          view("Button")
                              .key("preview:form:submit")
                              .prop("title", "Sign In")
                              .prop("variant", "primary")
                              .build(),
                          [selected_key]() mutable { selected_key.set("preview:form:submit"); }),
                      onTapGesture(
                          view("Button")
                              .key("preview:form:cancel")
                              .prop("title", "Cancel")
                              .build(),
                          [selected_key]() mutable { selected_key.set("preview:form:cancel"); }),
                  })
                  .build(),
          })
          .build();
    }
    if (layout_id == 2) {
      return view("Column")
          .prop("spacing", 12.0)
          .prop("cross_align", "start")
          .children({
              view("Text")
                  .prop("value", "Grid Layout")
                  .prop("font_size", 16.0)
                  .prop("color", 0xFFE0E0E0)
                  .build(),
              view("Grid")
                  .prop("columns", 3)
                  .prop("spacing", 10.0)
                  .children({
                      onTapGesture(
                          view("Button").key("preview:grid:1").prop("title", "One").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:1"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:2").prop("title", "Two").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:2"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:3").prop("title", "Three").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:3"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:4").prop("title", "Four").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:4"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:5").prop("title", "Five").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:5"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:6").prop("title", "Six").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:6"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:7").prop("title", "Seven").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:7"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:8").prop("title", "Eight").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:8"); }),
                      onTapGesture(
                          view("Button").key("preview:grid:9").prop("title", "Nine").build(),
                          [selected_key]() mutable { selected_key.set("preview:grid:9"); }),
                  })
                  .build(),
          })
          .build();
    }

    return view("Row")
        .prop("spacing", 12.0)
        .prop("cross_align", "center")
        .children({
            onTapGesture(
                view("Image")
                    .key("preview:image")
                    .prop("texture", static_cast<std::int64_t>(demo_tex))
                    .prop("width", 64.0)
                    .prop("height", 64.0)
                    .build(),
                [selected_key]() mutable { selected_key.set("preview:image"); }),
            view("Column")
                .prop("spacing", 8.0)
                .prop("cross_align", "start")
                .children({
                    view("Row")
                        .prop("spacing", 10.0)
                        .prop("cross_align", "center")
                        .children({
                            onTapGesture(
                                view("Button")
                                    .key("preview:button")
                                    .prop("title", "Button")
                                    .prop("variant", "primary")
                                    .build(),
                                [selected_key]() mutable { selected_key.set("preview:button"); }),
                            onTapGesture(
                                view("Text")
                                    .key("preview:text")
                                    .prop("value", "Text")
                                    .build(),
                                [selected_key]() mutable { selected_key.set("preview:text"); }),
                        })
                        .build(),
                    onTapGesture(
                        view("TextField")
                            .key("preview:input")
                            .prop("value", "")
                            .prop("placeholder", "Input")
                            .prop("width", 260.0)
                            .build(),
                        [selected_key]() mutable { selected_key.set("preview:input"); }),
                })
                .build(),
        })
        .build();
  }();

  return view("Box")
      .key("preview:panel")
      .prop("width", width)
      .prop("height", height)
      .prop("bg", 0xFF101010)
      .prop("border", 0xFF2A2A2A)
      .prop("border_width", 1.0)
      .children({
          view("Column")
              .prop("padding", 16.0)
              .prop("spacing", 12.0)
              .prop("cross_align", "start")
              .children({
                  view("Text")
                      .prop("value", "Preview (GPU)")
                      .prop("font_size", 16.0)
                      .prop("color", 0xFFE0E0E0)
                      .build(),
                  std::move(content),
              })
              .build(),
      })
      .build();
}

inline ViewNode edit_panel(BindingId binding, float width, float height) {
  return view("Box")
      .prop("width", width)
      .prop("height", height)
      .prop("bg", 0xFF101010)
      .prop("border", 0xFF2A2A2A)
      .prop("border_width", 1.0)
      .children({
          view("Column")
              .prop("padding", 12.0)
              .prop("spacing", 10.0)
              .prop("cross_align", "stretch")
              .children({
                  view("Text")
                      .prop("value", "Edit")
                      .prop("font_size", 12.0)
                      .prop("color", 0xFFB0B0B0)
                      .build(),
                  view("TextEditor")
                      .key("editor_source")
                      .prop("binding", binding.raw)
                      .prop("padding", 10.0)
                      .prop("font_size", 14.0)
                      .prop("width", std::max(0.0f, width - 24.0f))
                      .prop("height", std::max(0.0f, height - 34.0f))
                      .build(),
              })
              .build(),
      })
      .build();
}

inline const ViewNode *find_node_by_key(const ViewNode &root,
                                        std::string_view key) {
  if (key.empty()) {
    return nullptr;
  }
  if (root.key == key) {
    return &root;
  }
  for (const auto &c : root.children) {
    if (const auto *r = find_node_by_key(c, key)) {
      return r;
    }
  }
  return nullptr;
}

inline std::string format_prop_value(const Props &props, std::string_view k) {
  const auto it = props.find(std::string{k});
  if (it == props.end()) {
    return {};
  }
  if (const auto *s = std::get_if<std::string>(&it->second)) {
    return *s;
  }
  if (const auto *i = std::get_if<std::int64_t>(&it->second)) {
    char buf[64]{};
    std::snprintf(buf, sizeof(buf), "0x%llX",
                  static_cast<unsigned long long>(*i));
    return std::string{buf};
  }
  if (const auto *d = std::get_if<double>(&it->second)) {
    char buf[64]{};
    std::snprintf(buf, sizeof(buf), "%.3f", *d);
    return std::string{buf};
  }
  if (const auto *b = std::get_if<bool>(&it->second)) {
    return *b ? "true" : "false";
  }
  return {};
}

inline ViewNode props_panel(StateHandle<ViewNode> design_root,
                            StateHandle<std::string> selected_key,
                            StateHandle<std::vector<ViewNode>> history,
                            StateHandle<std::int64_t> history_idx) {
  enum class Kind { Str, Num, Color, Bool };
  struct Item {
    const char *label;
    const char *key;
    Kind kind;
  };

  static const Item items[] = {
      {"title", "title", Kind::Str},
      {"value", "value", Kind::Str},
      {"placeholder", "placeholder", Kind::Str},
      {"variant", "variant", Kind::Str},
      {"cross_align", "cross_align", Kind::Str},
      {"padding", "padding", Kind::Num},
      {"spacing", "spacing", Kind::Num},
      {"width", "width", Kind::Num},
      {"height", "height", Kind::Num},
      {"font_size", "font_size", Kind::Num},
      {"opacity", "opacity", Kind::Num},
      {"bg", "bg", Kind::Color},
      {"border", "border", Kind::Color},
      {"border_width", "border_width", Kind::Num},
      {"clip", "clip", Kind::Bool},
  };

  auto props_last_key = local_state<std::string>("editor:props_last_key", "");
  auto selected = selected_key.get();
  const auto root = design_root.get();
  const auto *sel_node = find_node_by_key(root, selected);

  std::vector<StateHandle<std::string>> field_states;
  field_states.reserve(std::size(items));
  for (const auto &it : items) {
    field_states.push_back(
        local_state<std::string>(std::string{"editor:prop:"} + it.key, ""));
  }

  auto stringify = [&](const PropValue &v, Kind k) -> std::string {
    if (const auto *s = std::get_if<std::string>(&v)) {
      return *s;
    }
    if (const auto *b = std::get_if<bool>(&v)) {
      return *b ? "true" : "false";
    }
    if (const auto *d = std::get_if<double>(&v)) {
      char buf[64]{};
      std::snprintf(buf, sizeof(buf), "%.3f", *d);
      return std::string{buf};
    }
    if (const auto *i = std::get_if<std::int64_t>(&v)) {
      if (k == Kind::Color) {
        char buf[64]{};
        std::snprintf(buf, sizeof(buf), "0x%llX",
                      static_cast<unsigned long long>(*i));
        return std::string{buf};
      }
      return std::to_string(static_cast<long long>(*i));
    }
    return {};
  };

  if (props_last_key.get() != selected) {
    for (std::size_t i = 0; i < std::size(items); ++i) {
      const auto &it = items[i];
      std::string v;
      if (sel_node) {
        if (const auto prop_it = sel_node->props.find(std::string{it.key});
            prop_it != sel_node->props.end()) {
          v = stringify(prop_it->second, it.kind);
        }
      }
      field_states[i].set(std::move(v));
    }
    props_last_key.set(selected);
  }

  const auto idx = history_idx.get();
  const auto hist = history.get();
  const bool can_undo = idx > 0;
  const bool can_redo = idx >= 0 && (idx + 1) < static_cast<std::int64_t>(hist.size());

  auto apply = [design_root, selected_key, selected,
                history, history_idx, field_states]() mutable {
    auto root = design_root.get();
    if (root.type.empty()) {
      return;
    }
    auto *n = find_node_by_key_mut(root, selected);
    if (!n) {
      return;
    }

    auto parse_bool = [&](std::string_view s) -> std::optional<bool> {
      if (s == "true" || s == "1") {
        return true;
      }
      if (s == "false" || s == "0") {
        return false;
      }
      return std::nullopt;
    };

    auto trim = [&](std::string &s) -> void {
      while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.erase(s.begin());
      }
      while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
      }
    };

    for (std::size_t i = 0; i < std::size(items); ++i) {
      const auto &it = items[i];
      auto s = field_states[i].get();
      trim(s);
      if (s.empty()) {
        n->props.erase(std::string{it.key});
        continue;
      }
      if (it.kind == Kind::Str) {
        n->props.insert_or_assign(std::string{it.key}, PropValue{std::move(s)});
        continue;
      }
      if (it.kind == Kind::Bool) {
        if (auto b = parse_bool(s)) {
          n->props.insert_or_assign(std::string{it.key}, PropValue{*b});
        }
        continue;
      }
      if (it.kind == Kind::Color) {
        const char *begin = s.c_str();
        int base = 10;
        if (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
          begin += 2;
          base = 16;
        } else if (!s.empty() && s[0] == '#') {
          begin += 1;
          base = 16;
        }
        char *end = nullptr;
        const auto v = std::strtoll(begin, &end, base);
        if (end && end != begin) {
          n->props.insert_or_assign(std::string{it.key}, PropValue{static_cast<std::int64_t>(v)});
        }
        continue;
      }

      char *end = nullptr;
      const auto v = std::strtod(s.c_str(), &end);
      if (end && end != s.c_str()) {
        n->props.insert_or_assign(std::string{it.key}, PropValue{static_cast<double>(v)});
      }
    }

    push_history(history, history_idx, root);
    design_root.set(std::move(root));
    selected_key.set(selected);
  };

  auto undo = [history, history_idx, design_root](bool redo) mutable {
    auto idx = history_idx.get();
    auto h = history.get();
    if (h.empty() || idx < 0 || idx >= static_cast<std::int64_t>(h.size())) {
      return;
    }
    const auto next = redo ? (idx + 1) : (idx - 1);
    if (next < 0 || next >= static_cast<std::int64_t>(h.size())) {
      return;
    }
    history_idx.set(next);
    design_root.set(h[static_cast<std::size_t>(next)]);
  };

  return view("ScrollView")
      .prop("clip", true)
      .prop("default_width", 360.0)
      .prop("default_height", 600.0)
      .children({
          view("Column")
              .prop("spacing", 10.0)
              .prop("cross_align", "stretch")
              .children([&](auto &c) {
                c.add(view("Text")
                          .prop("value", "Props")
                          .prop("font_size", 12.0)
                          .prop("color", 0xFFB0B0B0)
                          .build());

                std::string header = selected.empty() ? std::string{"(none)"} : selected;
                if (sel_node) {
                  header = sel_node->type + "  " + header;
                }
                c.add(view("Text")
                          .prop("value", header)
                          .prop("font_size", 13.0)
                          .prop("color", 0xFFE0E0E0)
                          .build());

                c.add(view("Row")
                          .prop("spacing", 8.0)
                          .prop("cross_align", "center")
                          .children({
                              view("Button")
                                  .prop("title", "Undo")
                                  .prop("disabled", !can_undo)
                                  .event("pointer_up", on_pointer_up([undo]() mutable { undo(false); }))
                                  .build(),
                              view("Button")
                                  .prop("title", "Redo")
                                  .prop("disabled", !can_redo)
                                  .event("pointer_up", on_pointer_up([undo]() mutable { undo(true); }))
                                  .build(),
                              view("Spacer").build(),
                              view("Button")
                                  .prop("title", "Apply")
                                  .prop("variant", "primary")
                                  .event("pointer_up", on_pointer_up([apply]() mutable { apply(); }))
                                  .build(),
                          })
                          .build());

                c.add(view("Divider").prop("thickness", 1.0).prop("color", 0xFF2A2A2A).build());

                for (std::size_t i = 0; i < std::size(items); ++i) {
                  const auto &it = items[i];
                  c.add(view("Text")
                            .prop("value", it.label)
                            .prop("font_size", 12.0)
                            .prop("color", 0xFFB0B0B0)
                            .build());
                  c.add(TextField(field_states[i], std::string{"editor:prop_field:"} + it.key,
                                  std::string{"("} + it.key + ")"));
                }
              })
              .build(),
      })
      .build();
}

inline std::string style_src_for(const Props &props, std::string_view k) {
  std::string src_key = "style_src.";
  src_key += k;
  return prop_as_string(props, src_key, "");
}

inline std::string style_chain_for(const Props &props, std::string_view k) {
  std::string chain_key = "style_chain.";
  chain_key += k;
  return prop_as_string(props, chain_key, "");
}

inline std::string style_prev_src_for(const Props &props, std::string_view k) {
  std::string src_key = "style_prev_src.";
  src_key += k;
  return prop_as_string(props, src_key, "");
}

inline std::string lower_ascii(std::string s) {
  for (auto &ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
}

inline bool contains_ci(std::string_view haystack, std::string_view needle) {
  if (needle.empty()) {
    return true;
  }
  return lower_ascii(std::string{haystack}).find(lower_ascii(std::string{needle})) !=
         std::string::npos;
}

inline std::string join_str(const std::vector<std::string> &parts,
                            std::string_view sep) {
  std::string out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) {
      out.append(sep);
    }
    out.append(parts[i]);
  }
  return out;
}

inline std::string unique_name_like(const std::vector<std::string> &names,
                                    std::string base) {
  std::unordered_set<std::string> set;
  set.reserve(names.size() * 2 + 1);
  for (const auto &n : names) {
    set.insert(n);
  }
  if (!set.contains(base)) {
    return base;
  }
  for (int i = 2; i < 10000; ++i) {
    std::string cand = base + " " + std::to_string(i);
    if (!set.contains(cand)) {
      return cand;
    }
  }
  return base + " Copy";
}

inline std::string format_theme_errors(const std::vector<StyleParseError> &errs) {
  if (errs.empty()) {
    return {};
  }
  std::string out;
  const std::size_t max_items = 8;
  const std::size_t take = std::min(max_items, errs.size());
  for (std::size_t i = 0; i < take; ++i) {
    const auto &e = errs[i];
    out += "line ";
    out += std::to_string(e.line);
    out += ":";
    out += std::to_string(e.column);
    out += " ";
    out += e.message;
    if (i + 1 < take) {
      out.push_back('\n');
    }
  }
  if (errs.size() > take) {
    out += "\n...";
  }
  return out;
}

inline std::unordered_map<std::string, PropValue>
flatten_sheet(const StyleSheetModel &sheet) {
  std::unordered_map<std::string, PropValue> out;
  out.reserve(sheet.global.size() + sheet.components.size() * 16);
  for (const auto &kv : sheet.global) {
    out.insert_or_assign("Global." + kv.first, kv.second);
  }
  for (const auto &ckv : sheet.components) {
    const auto &cname = ckv.first;
    const auto &comp = ckv.second;
    for (const auto &kv : comp.props) {
      out.insert_or_assign(cname + "." + kv.first, kv.second);
    }
    for (const auto &skv : comp.states) {
      for (const auto &kv : skv.second) {
        out.insert_or_assign(cname + "." + skv.first + "." + kv.first, kv.second);
      }
    }
    for (const auto &vkv : comp.variants) {
      const auto &vname = vkv.first;
      const auto &var = vkv.second;
      for (const auto &kv : var.props) {
        out.insert_or_assign(cname + "." + vname + "." + kv.first, kv.second);
      }
      for (const auto &skv : var.states) {
        for (const auto &kv : skv.second) {
          out.insert_or_assign(cname + "." + vname + "." + skv.first + "." + kv.first,
                               kv.second);
        }
      }
    }
  }
  return out;
}

inline std::string sheet_diff_summary(const StyleSheetModel &old_sheet,
                                      const StyleSheetModel &new_sheet) {
  const auto old_map = flatten_sheet(old_sheet);
  const auto new_map = flatten_sheet(new_sheet);

  std::size_t added = 0;
  std::size_t removed = 0;
  std::size_t changed = 0;
  std::vector<std::string> changed_keys;
  changed_keys.reserve(16);

  for (const auto &kv : new_map) {
    const auto it = old_map.find(kv.first);
    if (it == old_map.end()) {
      ++added;
      if (changed_keys.size() < 8) {
        changed_keys.push_back(std::string{"+"} + kv.first);
      }
      continue;
    }
    if (it->second != kv.second) {
      ++changed;
      if (changed_keys.size() < 8) {
        changed_keys.push_back(std::string{"~"} + kv.first);
      }
    }
  }
  for (const auto &kv : old_map) {
    if (!new_map.contains(kv.first)) {
      ++removed;
      if (changed_keys.size() < 8) {
        changed_keys.push_back(std::string{"-"} + kv.first);
      }
    }
  }

  std::string out =
      "diff: +" + std::to_string(added) + " -" + std::to_string(removed) +
      " ~" + std::to_string(changed) + " (total " +
      std::to_string(new_map.size()) + ")";
  if (!changed_keys.empty()) {
    out.push_back('\n');
    out += join_str(changed_keys, "\n");
  }
  return out;
}

inline ViewNode style_row(std::string name, std::string value,
                          std::string source) {
  return view("Box")
      .prop("padding", 10.0)
      .prop("bg", 0xFF151515)
      .prop("border", 0xFF2A2A2A)
      .prop("border_width", 1.0)
      .children({
          view("Column")
              .prop("spacing", 6.0)
              .prop("cross_align", "stretch")
              .children({
                  view("Row")
                      .prop("spacing", 10.0)
                      .prop("cross_align", "center")
                      .children({
                          view("Text").prop("value", std::move(name)).build(),
                          view("Spacer").build(),
                          view("Text")
                              .prop("value", std::move(value))
                              .prop("color", 0xFFB0B0B0)
                              .build(),
                      })
                      .build(),
                  view("Text")
                      .prop("value", std::move(source))
                      .prop("font_size", 12.0)
                      .prop("color", 0xFFB0B0B0)
                      .build(),
              })
              .build(),
      })
      .build();
}

inline ViewNode style_panel(std::string selected_key, const ViewNode *selected) {
  auto query = local_state<std::string>("editor:style_query", "");
  auto src_filter = local_state<std::string>("editor:style_src_filter", "");

  std::vector<std::string> style_keys;
  if (selected) {
    style_keys.reserve(selected->props.size());
    for (const auto &kv : selected->props) {
      const std::string_view k = kv.first;
      constexpr std::string_view prefix = "style_src.";
      if (!k.starts_with(prefix)) {
        continue;
      }
      std::string base_key{k.substr(prefix.size())};
      if (base_key.empty()) {
        continue;
      }
      style_keys.push_back(std::move(base_key));
    }
    std::sort(style_keys.begin(), style_keys.end());
    style_keys.erase(std::unique(style_keys.begin(), style_keys.end()),
                     style_keys.end());
  }

  return view("ScrollView")
      .prop("clip", true)
      .prop("default_width", 360.0)
      .prop("default_height", 600.0)
      .children({
          view("Column")
              .prop("spacing", 10.0)
              .prop("cross_align", "stretch")
              .children([&](auto &c) {
                c.add(view("Text")
                          .prop("value", "Property")
                          .prop("font_size", 12.0)
                          .prop("color", 0xFFB0B0B0)
                          .build());
                c.add(view("Text")
                          .prop("value",
                                selected_key.empty()
                                    ? std::string{"Selected: (none)"}
                                    : (std::string{"Selected: "} + selected_key))
                          .prop("font_size", 12.0)
                          .prop("color", 0xFFB0B0B0)
                          .build());
                c.add(view("Text")
                          .prop("value", "Search")
                          .prop("font_size", 12.0)
                          .prop("color", 0xFFB0B0B0)
                          .build());
                c.add(TextField(query, "editor:style_query_input", "prop name"));
                c.add(TextField(src_filter, "editor:style_src_filter_input",
                                "source filter (e.g. Global / Inline / Button.primary.hover)"));

                if (!selected || style_keys.empty()) {
                  c.add(view("Text")
                            .prop("value", "Click a component to inspect")
                            .prop("font_size", 12.0)
                            .prop("color", 0xFFB0B0B0)
                            .build());
                  return;
                }

                const auto q = query.get();
                const auto sf = src_filter.get();
                for (const auto &k : style_keys) {
                  if (!q.empty() && !contains_ci(k, q)) {
                    continue;
                  }

                  std::string info;
                  std::string filter_blob;
                  {
                    const auto src = style_src_for(selected->props, k);
                    if (!src.empty()) {
                      info = "from ";
                      info += src;
                    }
                    filter_blob += src;
                  }
                  {
                    const auto chain = style_chain_for(selected->props, k);
                    if (!chain.empty()) {
                      if (!info.empty()) {
                        info.push_back('\n');
                      }
                      info += "chain: ";
                      info += chain;
                    }
                    if (!filter_blob.empty()) {
                      filter_blob.push_back('\n');
                    }
                    filter_blob += chain;
                  }
                  {
                    std::string prev_key = "style_prev.";
                    prev_key += k;
                    const auto prev = format_prop_value(selected->props, prev_key);
                    if (!prev.empty()) {
                      if (!info.empty()) {
                        info.push_back('\n');
                      }
                      info += "overridden: ";
                      info += prev;
                      const auto prev_src = style_prev_src_for(selected->props, k);
                      if (!prev_src.empty()) {
                        info += " (from ";
                        info += prev_src;
                        info.push_back(')');
                      }
                      if (!filter_blob.empty()) {
                        filter_blob.push_back('\n');
                      }
                      filter_blob += prev_src;
                    }
                  }

                  if (!sf.empty() && !contains_ci(filter_blob, sf)) {
                    continue;
                  }
                  c.add(style_row(k, format_prop_value(selected->props, k), std::move(info)));
                }
              })
              .build(),
      })
      .build();
}

inline ViewNode editor_view(TextureHandle demo_tex_handle) {
  return GeometryReader([demo_tex_handle](SizeF size) {
    auto style_mgr = StateObject<StyleManager>("editor:style_mgr");
    auto style_mgr_ptr = style_mgr.get();
    if (style_mgr_ptr && style_mgr_ptr->theme_count() == 0) {
      const char *light_toml = R"(
[Theme]
name = "Light"

[Global]
bg = 0xFFF4F4F4
color = 0xFF101010
font_size = 14

[Button]
bg = 0xFFE8E8E8
color = 0xFF101010
padding = 10
border = 0xFFCDCDCD
border_width = 1

[Button.primary]
bg = 0xFF2D6BFF
color = 0xFFFFFFFF

[Button.primary.hover]
bg = 0xFF3A7BFF

[Button.primary.active]
bg = 0xFF2458D8

[Button.loading]
opacity = 0.75

[Button.primary.loading]
opacity = 0.75

[Button.disabled]
bg = 0xFFE0E0E0
color = 0xFF8A8A8A

[Text]
color = 0xFF101010
font_size = 14

[TextField]
bg = 0xFFFFFFFF
border = 0xFFCDCDCD
border_width = 1
padding = 10
placeholder_color = 0xFF7A7A7A

[TextField.focused]
border = 0xFF2D6BFF
)";

      const char *dark_toml = R"(
[Theme]
name = "Dark"
base = "Light"

[Global]
bg = 0xFF101010
color = 0xFFE0E0E0
font_size = 14

[Button]
bg = 0xFF202020
color = 0xFFEFEFEF
padding = 10
border = 0xFF3A3A3A
border_width = 1

[Button.primary]
bg = 0xFF2D6BFF
color = 0xFFFFFFFF

[Button.primary.hover]
bg = 0xFF3A7BFF

[Button.primary.active]
bg = 0xFF2458D8

[Button.loading]
opacity = 0.75

[Button.primary.loading]
opacity = 0.75

[Button.disabled]
bg = 0xFF202020
color = 0xFF808080

[Text]
color = 0xFFE0E0E0
font_size = 14

[TextField]
bg = 0xFF151515
border = 0xFF2A2A2A
border_width = 1
padding = 10
placeholder_color = 0xFF808080

[TextField.focused]
border = 0xFF3A7BFF
)";

      const char *hc_toml = R"(
[Theme]
name = "HighContrast"

[Global]
bg = 0xFF000000
color = 0xFFFFFFFF
font_size = 14

[Button]
bg = 0xFF000000
color = 0xFFFFFFFF
padding = 10
border = 0xFFFFFFFF
border_width = 2

[Button.primary]
bg = 0xFFFFFF00
color = 0xFF000000

[Button.primary.hover]
bg = 0xFFFFD000

[Button.primary.active]
bg = 0xFFFFA000

[Button.loading]
opacity = 0.75

[Button.primary.loading]
opacity = 0.75

[Button.disabled]
bg = 0xFF000000
color = 0xFF808080

[Text]
color = 0xFFFFFFFF
font_size = 14

[TextField]
bg = 0xFF000000
border = 0xFFFFFFFF
border_width = 2
padding = 10
placeholder_color = 0xFFB0B0B0

[TextField.focused]
border = 0xFFFFFF00
)";

      style_mgr_ptr->upsert_theme(parse_theme_toml(light_toml).theme);
      style_mgr_ptr->upsert_theme(parse_theme_toml(dark_toml).theme);
      style_mgr_ptr->upsert_theme(parse_theme_toml(hc_toml).theme);
      style_mgr_ptr->set_active_theme("Dark");
    }
    if (style_mgr_ptr) {
      provide_environment_object("style.manager", style_mgr_ptr);
    }

    auto dsl_engine_state =
        StateObject<::duorou::ui::dsl::MiniSwiftEngine>("editor:dsl_engine");
    auto dsl_engine_ptr = dsl_engine_state.get();
    if (dsl_engine_ptr) {
      provide_environment_object("dsl.engine", dsl_engine_ptr);
    }

    const float viewport_w = std::max(320.0f, size.w);
    const float viewport_h = std::max(240.0f, size.h);
    constexpr float left_w = 260.0f;
    constexpr float right_w = 360.0f;
    constexpr float spacing = 12.0f;
    constexpr float padding = 12.0f;

    const float center_w = std::max(
        320.0f, viewport_w - left_w - right_w - spacing * 2.0f - padding * 2.0f);
    const float workspace_h = std::max(120.0f, viewport_h - 88.0f);
    const float preview_h = std::max(120.0f, workspace_h * 0.55f);
    const float edit_h = std::max(120.0f, workspace_h - preview_h - spacing);

  #if defined(DUOROU_HAS_MINISWIFT)
    auto editor_source = local_state<std::string>(
        "editor:source",
        "let root = VStack()\\n"
        "root.addChild(Text(\\\"Hello duorou\\\"))\\n"
        "root.addChild(Button(\\\"Click\\\"))\\n"
        "UIApplication.shared.setRootView(root)\\n");
  #else
    auto editor_source = local_state<std::string>(
        "editor:source",
        "view(\"Column\")\n"
        "  .prop(\"spacing\", 8.0)\n"
        "  .prop(\"cross_align\", \"start\")\n"
        "  .children({\n"
        "    view(\"Text\").prop(\"value\", \"Hello duorou\").build(),\n"
        "    view(\"Button\").prop(\"title\", \"Click\").build(),\n"
        "  })\n"
        "  .build();\n");
  #endif

    auto preview_state = local_state<std::string>("editor:preview_state", "");
    auto preview_layout = local_state<std::int64_t>("editor:preview_layout", 0);
    auto preview_zoom = local_state<double>("editor:preview_zoom", 1.0);

    auto design_root = local_state<ViewNode>(
        "editor:design_root",
        view("Column")
            .key("design:root")
            .prop("spacing", 12.0)
            .prop("cross_align", "start")
            .children({
                view("Text").key("design:text1").prop("value", "Hello duorou").build(),
                view("Button").key("design:btn1").prop("title", "Button").prop("variant", "primary").build(),
            })
            .build());

    auto history = local_state<std::vector<ViewNode>>("editor:history", {});
    auto history_idx = local_state<std::int64_t>("editor:history_idx", -1);
    auto history_init = local_state<bool>("editor:history_init", false);
    if (!history_init.get()) {
      auto h = std::vector<ViewNode>{design_root.get()};
      history.set(std::move(h));
      history_idx.set(0);
      history_init.set(true);
    }

    auto selected_key =
        local_state<std::string>("editor:selected_key", "design:root");

    auto dsl_engine = EnvironmentObject<::duorou::ui::dsl::Engine>("dsl.engine");
    auto dsl_enabled = local_state<bool>("editor:dsl_enabled", false);
    auto dsl_last_ok = local_state<bool>("editor:dsl_last_ok", false);
    auto dsl_error = local_state<std::string>("editor:dsl_error", "");
    auto dsl_root = local_state<ViewNode>("editor:dsl_root", ViewNode{});

    auto dsl_run = [dsl_engine, editor_source, dsl_root, dsl_last_ok,
                    dsl_error]() mutable {
      if (!dsl_engine.valid()) {
        dsl_last_ok.set(false);
        dsl_error.set("dsl engine missing");
        return;
      }
      auto r = dsl_engine->eval_ui(editor_source.get());
      if (!r.ok || r.root.type.empty()) {
        dsl_last_ok.set(false);
        dsl_error.set(r.error.empty() ? std::string{"eval failed"} : r.error);
        return;
      }
      dsl_root.set(std::move(r.root));
      dsl_last_ok.set(true);
      dsl_error.set("");
    };

    auto preview = [&]() -> ViewNode {
      if (dsl_enabled.get() && dsl_last_ok.get()) {
        auto root = dsl_root.get();
        if (!root.type.empty()) {
          return root;
        }
      }
      auto root = design_root.get();
      if (root.type.empty()) {
        root = view("Column").key("design:root").children({}).build();
      }
      const auto sel = selected_key.get();
      auto decorate = [&](auto &&self, ViewNode n) -> ViewNode {
        for (auto &ch : n.children) {
          ch = self(self, std::move(ch));
        }
        if (starts_with(n.key, "design:")) {
          const bool is_sel = (n.key == sel);
          if (is_sel) {
            n.props.insert_or_assign("border", PropValue{static_cast<std::int64_t>(0xFF80A0FF)});
            n.props.insert_or_assign("border_width", PropValue{2.0});
          }
          n = onTapGesture(std::move(n), [selected_key, k = n.key]() mutable {
            selected_key.set(k);
          });
        }
        return n;
      };
      auto canvas_content = decorate(decorate, std::move(root));
      auto canvas = view("Box")
                        .key("editor:canvas")
                        .prop("padding", 16.0)
                        .prop("bg", 0xFF101010)
                        .prop("border", 0xFF2A2A2A)
                        .prop("border_width", 1.0)
                        .children({std::move(canvas_content)})
                        .build();
      canvas = onTapGesture(std::move(canvas), [selected_key]() mutable {
        selected_key.set("design:root");
      });
      (void)preview_layout;
      return canvas;
    }();
    preview.props.insert_or_assign("render_scale", PropValue{preview_zoom.get()});
    auto zoom_root =
        view("Box")
            .key("preview:zoom_root")
            .prop("width", center_w * static_cast<float>(preview_zoom.get()))
            .prop("height", preview_h * static_cast<float>(preview_zoom.get()))
            .children({std::move(preview)})
            .build();
    preview = view("ScrollView")
                  .key("preview:scroll")
                  .prop("clip", true)
                  .prop("scroll_axis", "both")
                  .prop("width", center_w)
                  .prop("height", preview_h)
                  .prop("default_width", center_w)
                  .prop("default_height", preview_h)
                  .children({std::move(zoom_root)})
                  .build();
    {
      const auto st = preview_state.get();
      if (!st.empty()) {
        auto apply_state = [&](auto &&self, ViewNode &n) -> void {
          n.props.insert_or_assign("style_state", PropValue{st});
          for (auto &c : n.children) {
            self(self, c);
          }
        };
        apply_state(apply_state, preview);
      }
    }
    if (style_mgr_ptr) {
      style_mgr_ptr->apply_to_tree(preview);
    }
    auto props_panel_node =
        props_panel(design_root, selected_key, history, history_idx);

    auto theme_pop_open = local_state<bool>("editor:theme_pop_open", false);
    auto theme_pop_x = local_state<double>("editor:theme_pop_x", 0.0);
    auto theme_pop_y = local_state<double>("editor:theme_pop_y", 0.0);
    auto theme_new_name = local_state<std::string>("editor:theme_new_name", "");
    auto theme_new_base = local_state<std::string>("editor:theme_new_base", "");
    auto theme_copy_name = local_state<std::string>("editor:theme_copy_name", "");
    auto hot_theme_enabled = local_state<bool>("editor:hot_theme_enabled", false);
    auto hot_theme_path = local_state<std::string>("editor:hot_theme_path", "");
    auto hot_theme_mtime = local_state<std::int64_t>("editor:hot_theme_mtime", 0);
    auto hot_theme_status = local_state<std::string>("editor:hot_theme_status", "");
    auto hot_theme_error = local_state<std::string>("editor:hot_theme_error", "");

    auto reload_hot_theme = [style_mgr_ptr, hot_theme_path, hot_theme_mtime,
                             hot_theme_status, hot_theme_error](bool force) mutable {
      if (!style_mgr_ptr) {
        return;
      }
      const auto path = hot_theme_path.get();
      if (path.empty()) {
        return;
      }
      const auto set_error = [&](std::string v) {
        if (hot_theme_error.get() != v) {
          hot_theme_error.set(std::move(v));
        }
      };
      const auto clear_error = [&]() {
        if (!hot_theme_error.get().empty()) {
          hot_theme_error.set("");
        }
      };
      const auto set_status = [&](std::string v) {
        if (hot_theme_status.get() != v) {
          hot_theme_status.set(std::move(v));
        }
      };
      std::error_code ec;
      const auto ft = std::filesystem::last_write_time(path, ec);
      if (ec) {
        set_error("stat failed");
        return;
      }
      const auto ticks =
          static_cast<std::int64_t>(ft.time_since_epoch().count());
      if (!force && ticks == hot_theme_mtime.get()) {
        return;
      }

      auto text = ::duorou::ui::load_text_file(path);
      if (!text) {
        set_error("read failed");
        return;
      }

      auto parsed = ::duorou::ui::parse_theme_toml(*text);
      if (!parsed.errors.empty()) {
        set_error(format_theme_errors(parsed.errors));
        return;
      }

      std::string target = style_mgr_ptr->active_theme();
      if (target.empty()) {
        target = "Default";
      }
      StyleSheetModel old_sheet;
      if (const auto *old = style_mgr_ptr->theme(target)) {
        old_sheet = old->sheet;
      }
      parsed.theme.name = target;
      style_mgr_ptr->upsert_theme(parsed.theme);
      style_mgr_ptr->set_active_theme(target);
      hot_theme_mtime.set(ticks);
      clear_error();
      set_status(sheet_diff_summary(old_sheet, parsed.theme.sheet));
    };
    auto reload_hot_theme_auto = [reload_hot_theme]() mutable {
      reload_hot_theme(false);
    };
    auto reload_hot_theme_force = [reload_hot_theme]() mutable {
      reload_hot_theme(true);
    };

    std::vector<std::string> theme_names;
    std::string active_theme;
    std::string chain_str;
    std::string overrides_str;
    bool can_delete_theme = false;
    if (style_mgr_ptr) {
      theme_names = style_mgr_ptr->theme_names();
      active_theme = style_mgr_ptr->active_theme();
      can_delete_theme = style_mgr_ptr->theme_count() > 1;
      const auto chain = style_mgr_ptr->base_chain(active_theme);
      chain_str = chain.empty() ? std::string{} : join_str(chain, " -> ");

      const auto *t = style_mgr_ptr->theme(active_theme);
      if (t && !t->base.empty()) {
        const auto base_sheet = style_mgr_ptr->resolved_sheet_for(t->base);
        std::unordered_set<std::string> seen;
        std::vector<std::string> overrides;
        overrides.reserve(128);

        auto add_key = [&](std::string s) {
          if (seen.insert(s).second) {
            overrides.push_back(std::move(s));
          }
        };

        for (const auto &kv : t->sheet.global) {
          if (base_sheet.global.contains(kv.first)) {
            add_key("Global." + kv.first);
          }
        }

        for (const auto &ckv : t->sheet.components) {
          const auto it_base_comp = base_sheet.components.find(ckv.first);
          if (it_base_comp == base_sheet.components.end()) {
            continue;
          }
          for (const auto &kv : ckv.second.props) {
            if (it_base_comp->second.props.contains(kv.first)) {
              add_key(ckv.first + "." + kv.first);
            }
          }
          for (const auto &skv : ckv.second.states) {
            const auto it_bs = it_base_comp->second.states.find(skv.first);
            if (it_bs == it_base_comp->second.states.end()) {
              continue;
            }
            for (const auto &kv : skv.second) {
              if (it_bs->second.contains(kv.first)) {
                add_key(ckv.first + "." + skv.first + "." + kv.first);
              }
            }
          }
          for (const auto &vkv : ckv.second.variants) {
            const auto it_bv = it_base_comp->second.variants.find(vkv.first);
            if (it_bv == it_base_comp->second.variants.end()) {
              continue;
            }
            for (const auto &kv : vkv.second.props) {
              if (it_bv->second.props.contains(kv.first)) {
                add_key(ckv.first + "." + vkv.first + "." + kv.first);
              }
            }
            for (const auto &sv : vkv.second.states) {
              const auto it_bvs = it_bv->second.states.find(sv.first);
              if (it_bvs == it_bv->second.states.end()) {
                continue;
              }
              for (const auto &kv : sv.second) {
                if (it_bvs->second.contains(kv.first)) {
                  add_key(ckv.first + "." + vkv.first + "." + sv.first + "." +
                          kv.first);
                }
              }
            }
          }
        }

        std::sort(overrides.begin(), overrides.end());
        if (!overrides.empty()) {
          const std::size_t max_items = 24;
          std::vector<std::string> slice;
          const std::size_t take = std::min(max_items, overrides.size());
          slice.reserve(take + 1);
          for (std::size_t i = 0; i < take; ++i) {
            slice.push_back(overrides[i]);
          }
          overrides_str = "Overrides: " + std::to_string(overrides.size());
          if (!slice.empty()) {
            overrides_str += "\n";
            overrides_str += join_str(slice, "\n");
            if (overrides.size() > take) {
              overrides_str += "\n...";
            }
          }
        }
      }
    }

    auto top_bar = view("Box")
                       .prop("padding", 10.0)
                       .prop("bg", 0xFF161616)
                       .prop("border", 0xFF2A2A2A)
                       .prop("border_width", 1.0)
                       .children({
                           view("Row")
                               .prop("spacing", 10.0)
                               .prop("cross_align", "center")
                               .children({
                                   view("Text")
                                       .prop("value", "duorou Editor (GPU)")
                                       .prop("font_size", 14.0)
                                       .build(),
                                   view("Spacer").build(),
                                   view("Text")
                                       .prop("value", "Theme:")
                                       .prop("font_size", 12.0)
                                       .prop("color", 0xFFB0B0B0)
                                       .build(),
                                   view("Button")
                                       .prop("title", active_theme.empty() ? std::string{"(none)"} : active_theme)
                                       .event("pointer_up", on_pointer_up([theme_pop_open, theme_pop_x, theme_pop_y]() mutable {
                                                if (const auto r = target_frame()) {
                                                  theme_pop_x.set(r->x);
                                                  theme_pop_y.set(r->y + r->h);
                                                }
                                                theme_pop_open.set(true);
                                              }))
                                       .build(),
                               })
                               .build(),
                       })
                       .build();

    auto workspace =
        view("Column")
            .prop("spacing", spacing)
            .prop("cross_align", "stretch")
            .children({
                view("Column")
                    .prop("spacing", 10.0)
                    .prop("cross_align", "stretch")
                    .children({
                        view("Column")
                            .prop("spacing", 8.0)
                            .prop("cross_align", "stretch")
                            .children([&](auto &c) {
                              c.add(view("Row")
                                        .prop("spacing", 8.0)
                                        .prop("cross_align", "center")
                                        .children([&](auto &r) {
                                          r.add(view("Text")
                                                    .prop("value", "Hot Theme:")
                                                    .prop("font_size", 12.0)
                                                    .prop("color", 0xFFB0B0B0)
                                                    .build());
                                          auto path_field = TextField(
                                              hot_theme_path,
                                              "editor:hot_theme_path_input",
                                              "theme toml path");
                                          path_field.props.insert_or_assign(
                                              "width", PropValue{420.0});
                                          r.add(std::move(path_field));
                                          r.add(view("Button")
                                                    .prop("title", "Browse")
                                                    .event("pointer_up", on_pointer_up([hot_theme_path, hot_theme_mtime, hot_theme_status, hot_theme_error]() mutable {
                                                             if (auto p = open_file_dialog("Open Theme TOML")) {
                                                               hot_theme_path.set(*p);
                                                               hot_theme_mtime.set(0);
                                                               hot_theme_status.set("");
                                                               hot_theme_error.set("");
                                                             }
                                                           }))
                                                    .build());
                                          r.add(view("Button")
                                                    .prop("title", "Reload")
                                                    .event("pointer_up", on_pointer_up([reload_hot_theme_force]() mutable { reload_hot_theme_force(); }))
                                                    .build());
                                          r.add(view("Button")
                                                    .prop("title", hot_theme_enabled.get() ? "Watch: ON" : "Watch: OFF")
                                                    .event("pointer_up", on_pointer_up([hot_theme_enabled, hot_theme_mtime, reload_hot_theme_force]() mutable {
                                                             const bool next = !hot_theme_enabled.get();
                                                             hot_theme_enabled.set(next);
                                                             hot_theme_mtime.set(0);
                                                             if (next) {
                                                               reload_hot_theme_force();
                                                             }
                                                           }))
                                                    .build());
                                        })
                                        .build());
                              const auto err = hot_theme_error.get();
                              if (!err.empty()) {
                                c.add(view("Text")
                                          .prop("value", err)
                                          .prop("font_size", 12.0)
                                          .prop("color", 0xFFFF8080)
                                          .build());
                              } else {
                                const auto st = hot_theme_status.get();
                                if (!st.empty()) {
                                  c.add(view("Text")
                                            .prop("value", st)
                                            .prop("font_size", 12.0)
                                            .prop("color", 0xFFB0B0B0)
                                            .build());
                                }
                              }
                              if (hot_theme_enabled.get() && !hot_theme_path.get().empty()) {
                                c.add(WatchFile("editor:hot_theme_watch",
                                                hot_theme_path.get(), 250.0,
                                                [reload_hot_theme_auto]() mutable {
                                                  reload_hot_theme_auto();
                                                },
                                                false));
                              }
                            })
                            .build(),
                        view("Column")
                            .prop("spacing", 8.0)
                            .prop("cross_align", "stretch")
                            .children([&](auto &c) {
                              c.add(view("Row")
                                        .prop("spacing", 8.0)
                                        .prop("cross_align", "center")
                                        .children([&](auto &r) {
                                          r.add(view("Text")
                                                    .prop("value", "DSL:")
                                                    .prop("font_size", 12.0)
                                                    .prop("color", 0xFFB0B0B0)
                                                    .build());
                                          r.add(view("Button")
                                                    .prop("title",
                                                          dsl_enabled.get()
                                                              ? "Use DSL: ON"
                                                              : "Use DSL: OFF")
                                                    .event("pointer_up",
                                                           on_pointer_up([dsl_enabled]() mutable {
                                                             dsl_enabled.set(!dsl_enabled.get());
                                                           }))
                                                    .build());
                                          r.add(view("Button")
                                                    .prop("title", "Run DSL")
                                                    .event("pointer_up",
                                                           on_pointer_up([dsl_run]() mutable {
                                                             dsl_run();
                                                           }))
                                                    .build());
                                          r.add(view("Spacer").build());
                                          r.add(view("Text")
                                                    .prop("value",
                                                          dsl_engine.valid()
                                                              ? "Engine: OK"
                                                              : "Engine: missing")
                                                    .prop("font_size", 12.0)
                                                    .prop("color",
                                                          dsl_engine.valid()
                                                              ? 0xFFB0B0B0
                                                              : 0xFFFF8080)
                                                    .build());
                                        })
                                        .build());
                              const auto err = dsl_error.get();
                              if (!err.empty()) {
                                c.add(view("Text")
                                          .prop("value", err)
                                          .prop("font_size", 12.0)
                                          .prop("color", 0xFFFF8080)
                                          .build());
                              }
                            })
                            .build(),
                        view("Row")
                            .prop("spacing", 8.0)
                            .prop("cross_align", "center")
                            .children({
                                view("Text").prop("value", "Preview state:").prop("font_size", 12.0).build(),
                                view("Button")
                                    .prop("title", "Normal")
                                    .event("pointer_up", on_pointer_up([preview_state]() mutable { preview_state.set(""); }))
                                    .build(),
                                view("Button")
                                    .prop("title", "Hover")
                                    .event("pointer_up", on_pointer_up([preview_state]() mutable { preview_state.set("hover"); }))
                                    .build(),
                                view("Button")
                                    .prop("title", "Active")
                                    .event("pointer_up", on_pointer_up([preview_state]() mutable { preview_state.set("active"); }))
                                    .build(),
                                view("Button")
                                    .prop("title", "Disabled")
                                    .event("pointer_up", on_pointer_up([preview_state]() mutable { preview_state.set("disabled"); }))
                                    .build(),
                                view("Button")
                                    .prop("title", "Loading")
                                    .event("pointer_up", on_pointer_up([preview_state]() mutable { preview_state.set("loading"); }))
                                    .build(),
                                view("Spacer").build(),
                                view("Text")
                                    .prop("value", preview_state.get().empty() ? std::string{"(normal)"} : preview_state.get())
                                    .prop("font_size", 12.0)
                                    .build(),
                            })
                            .build(),
                        view("Row")
                            .prop("spacing", 8.0)
                            .prop("cross_align", "center")
                            .children({
                                view("Text").prop("value", "Layout:").prop("font_size", 12.0).build(),
                                view("Button")
                                    .prop("title", "Basic")
                                    .event("pointer_up", on_pointer_up([preview_layout, selected_key]() mutable {
                                             preview_layout.set(0);
                                             selected_key.set("preview:button");
                                           }))
                                    .build(),
                                view("Button")
                                    .prop("title", "Form")
                                    .event("pointer_up", on_pointer_up([preview_layout, selected_key]() mutable {
                                             preview_layout.set(1);
                                             selected_key.set("preview:form:submit");
                                           }))
                                    .build(),
                                view("Button")
                                    .prop("title", "Grid")
                                    .event("pointer_up", on_pointer_up([preview_layout, selected_key]() mutable {
                                             preview_layout.set(2);
                                             selected_key.set("preview:grid:1");
                                           }))
                                    .build(),
                                view("Spacer").build(),
                                view("Text").prop("value", "Zoom:").prop("font_size", 12.0).build(),
                                view("Stepper")
                                    .prop("value", preview_zoom.get())
                                    .prop("width", 140.0)
                                    .event("pointer_up", on_pointer_up([preview_zoom]() mutable {
                                             const auto r = target_frame();
                                             if (!r) {
                                               return;
                                             }
                                             const float local_x = pointer_x() - r->x;
                                             const bool inc = local_x > r->w * 0.5f;
                                             const double step = 0.1;
                                             double next = preview_zoom.get() + (inc ? step : -step);
                                             if (next < 0.5) {
                                               next = 0.5;
                                             }
                                             if (next > 2.0) {
                                               next = 2.0;
                                             }
                                             preview_zoom.set(next);
                                           }))
                                    .build(),
                                view("Text")
                                    .prop("value", std::to_string(static_cast<int>(preview_zoom.get() * 100.0)) + "%")
                                    .prop("font_size", 12.0)
                                    .build(),
                            })
                            .build(),
                        view("Box")
                            .children({
                                std::move(preview),
                            })
                            .build(),
                    })
                    .build(),
                edit_panel(::duorou::ui::bind(editor_source), center_w, edit_h),
            })
            .build();

    auto body =
        view("Row")
            .prop("spacing", spacing)
            .prop("cross_align", "stretch")
            .children({
                panel("Components",
                      tree_panel(demo_tex_handle, design_root, selected_key,
                                 history, history_idx),
                      left_w),
                panel("Workspace", std::move(workspace), center_w),
                panel("Property", std::move(props_panel_node), right_w),
            })
            .build();

    auto main =
        view("Column")
            .prop("padding", padding)
            .prop("spacing", 12.0)
            .prop("cross_align", "stretch")
            .children({std::move(top_bar), std::move(body)})
            .build();

    auto drag_active = local_state<bool>("editor:lib_drag_active", false);
    auto drag_type = local_state<std::string>("editor:lib_drag_type", "");
    auto drag_x = local_state<double>("editor:lib_drag_x", 0.0);
    auto drag_y = local_state<double>("editor:lib_drag_y", 0.0);
    auto insert_show = local_state<bool>("editor:insert_show", false);
    auto insert_mode = local_state<std::string>("editor:insert_mode", "");
    auto insert_axis = local_state<std::string>("editor:insert_axis", "");
    auto insert_fx = local_state<double>("editor:insert_fx", 0.0);
    auto insert_fy = local_state<double>("editor:insert_fy", 0.0);
    auto insert_fw = local_state<double>("editor:insert_fw", 0.0);
    auto insert_fh = local_state<double>("editor:insert_fh", 0.0);

    auto drag_ghost = [&]() -> ViewNode {
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
                             .prop("value", drag_type.get().empty()
                                               ? std::string{"(drag)"}
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
    };

    auto insert_indicator = [&]() -> ViewNode {
      if (!insert_show.get()) {
        return view("Spacer").prop("hit_test", false).build();
      }

      const auto mode = insert_mode.get();
      const auto axis = insert_axis.get();
      const float fx = static_cast<float>(insert_fx.get());
      const float fy = static_cast<float>(insert_fy.get());
      const float fw = static_cast<float>(insert_fw.get());
      const float fh = static_cast<float>(insert_fh.get());
      if (mode.empty() || fw <= 0.0f || fh <= 0.0f) {
        return view("Spacer").prop("hit_test", false).build();
      }

      const RectF r{fx, fy, fw, fh};
      const bool h = (axis == "h");
      const bool before = (mode == "before");
      const ColorU8 stroke{128, 160, 255, 255};
      const ColorU8 fill{128, 160, 255, 40};
      const float t = 2.0f;
      const float cap = 6.0f;

      auto node = Canvas(
          "editor:insert_indicator",
          [r, h, before, mode, stroke, fill, t, cap](RectF, std::vector<RenderOp> &out) {
            if (mode == "inside") {
              out.push_back(DrawRect{RectF{r.x, r.y, r.w, r.h}, fill});
              out.push_back(DrawRect{RectF{r.x, r.y, r.w, t}, stroke});
              out.push_back(DrawRect{RectF{r.x, r.y + r.h - t, r.w, t}, stroke});
              out.push_back(DrawRect{RectF{r.x, r.y, t, r.h}, stroke});
              out.push_back(DrawRect{RectF{r.x + r.w - t, r.y, t, r.h}, stroke});
              return;
            }

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
    };

    if (!theme_pop_open.get()) {
      if (!drag_active.get() && !insert_show.get()) {
        return main;
      }
      return view("Overlay")
          .children([&](auto &c) {
            c.add(std::move(main));
            c.add(insert_indicator());
            c.add(drag_ghost());
          })
          .build();
    }

    const auto close_id =
        on_pointer_up([theme_pop_open]() mutable { theme_pop_open.set(false); });

    auto pop = Popover(
        {view("Column")
             .prop("spacing", 10.0)
             .prop("cross_align", "stretch")
             .children([&](auto &c) {
               c.add(view("Text").prop("value", "Themes").prop("font_size", 14.0).build());
               if (!active_theme.empty()) {
                 c.add(view("Text")
                           .prop("value", std::string{"Active: "} + active_theme)
                           .prop("font_size", 12.0)
                           .prop("color", 0xFFB0B0B0)
                           .build());
               }
               if (!chain_str.empty()) {
                 c.add(view("Text")
                           .prop("value", std::string{"Chain: "} + chain_str)
                           .prop("font_size", 12.0)
                           .prop("color", 0xFFB0B0B0)
                           .build());
               }
               if (!overrides_str.empty()) {
                 c.add(view("Text")
                           .prop("value", overrides_str)
                           .prop("font_size", 12.0)
                           .prop("color", 0xFFB0B0B0)
                           .build());
               }

               c.add(view("Divider").prop("thickness", 1.0).prop("color", 0xFF2A2A2A).build());

               for (const auto &n : theme_names) {
                 const bool is_active = (n == active_theme);
                 const std::string title = is_active ? ("* " + n) : n;
                 c.add(view("Button")
                           .prop("title", title)
                           .event("pointer_up",
                                  on_pointer_up([style_mgr_ptr, theme_pop_open, n]() mutable {
                                    if (style_mgr_ptr) {
                                      style_mgr_ptr->set_active_theme(n);
                                    }
                                    theme_pop_open.set(false);
                                  }))
                           .build());
               }

               c.add(view("Divider").prop("thickness", 1.0).prop("color", 0xFF2A2A2A).build());

               c.add(view("Text").prop("value", "Create").prop("font_size", 12.0).prop("color", 0xFFB0B0B0).build());
               c.add(TextField(theme_new_name, "editor:theme_new_name", "Theme name"));
               c.add(TextField(theme_new_base, "editor:theme_new_base", "Base (optional)"));
               c.add(view("Button")
                         .prop("title", "Create")
                         .event("pointer_up", on_pointer_up([style_mgr_ptr, theme_new_name, theme_new_base]() mutable {
                                  if (!style_mgr_ptr) {
                                    return;
                                  }
                                  auto names = style_mgr_ptr->theme_names();
                                  std::string name = theme_new_name.get();
                                  if (name.empty()) {
                                    name = unique_name_like(names, "Theme");
                                  } else if (std::find(names.begin(), names.end(), name) != names.end()) {
                                    name = unique_name_like(names, name);
                                  }

                                  ThemeModel t;
                                  t.name = name;
                                  std::string base = theme_new_base.get();
                                  if (base.empty()) {
                                    base = style_mgr_ptr->active_theme();
                                  }
                                  if (base == name) {
                                    base.clear();
                                  }
                                  if (!base.empty() && !style_mgr_ptr->theme(base)) {
                                    base.clear();
                                  }
                                  t.base = std::move(base);
                                  style_mgr_ptr->upsert_theme(std::move(t));
                                  style_mgr_ptr->set_active_theme(name);
                                  theme_new_name.set("");
                                  theme_new_base.set("");
                                }))
                         .build());

               c.add(view("Text").prop("value", "Copy Active").prop("font_size", 12.0).prop("color", 0xFFB0B0B0).build());
               c.add(TextField(theme_copy_name, "editor:theme_copy_name", "New theme name"));
               c.add(view("Button")
                         .prop("title", "Copy")
                         .event("pointer_up", on_pointer_up([style_mgr_ptr, theme_copy_name]() mutable {
                                  if (!style_mgr_ptr) {
                                    return;
                                  }
                                  const auto src_name = style_mgr_ptr->active_theme();
                                  const auto *src = style_mgr_ptr->theme(src_name);
                                  if (!src) {
                                    return;
                                  }
                                  auto names = style_mgr_ptr->theme_names();
                                  std::string name = theme_copy_name.get();
                                  if (name.empty()) {
                                    name = unique_name_like(names, src_name + " Copy");
                                  } else if (std::find(names.begin(), names.end(), name) != names.end()) {
                                    name = unique_name_like(names, name);
                                  }
                                  ThemeModel t = *src;
                                  t.name = name;
                                  if (t.base == name) {
                                    t.base.clear();
                                  }
                                  style_mgr_ptr->upsert_theme(std::move(t));
                                  style_mgr_ptr->set_active_theme(name);
                                  theme_copy_name.set("");
                                }))
                         .build());

               c.add(view("Divider").prop("thickness", 1.0).prop("color", 0xFF2A2A2A).build());
               c.add(view("Button")
                         .prop("title", "Delete Active")
                         .prop("disabled", !can_delete_theme)
                         .event("pointer_up", on_pointer_up([style_mgr_ptr]() mutable {
                                  if (style_mgr_ptr && style_mgr_ptr->theme_count() > 1) {
                                    const auto n = style_mgr_ptr->active_theme();
                                    style_mgr_ptr->remove_theme(n);
                                    style_mgr_ptr->set_active_theme("");
                                  }
                                }))
                         .build());
             })
             .build()},
        static_cast<float>(theme_pop_x.get()),
        static_cast<float>(theme_pop_y.get()), 0, close_id);

    return view("Overlay")
        .children([&](auto &c) {
          c.add(std::move(main));
          c.add(insert_indicator());
          c.add(std::move(pop));
          if (drag_active.get()) {
            c.add(drag_ghost());
          }
        })
        .build();
  });
}

} // namespace duorou::ui::editor
