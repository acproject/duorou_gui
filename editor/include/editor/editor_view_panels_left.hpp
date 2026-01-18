#pragma once

#include <editor/editor_view_common.hpp>
#include <editor/editor_view_history.hpp>
#include <editor/editor_view_insert.hpp>
#include <editor/editor_view_tree_ops.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace duorou::ui::editor {

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
  auto insert_kind = local_state<std::string>("editor:insert_kind", "");
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
        [drag_active, drag_type, drag_x, drag_y, insert_show, insert_mode, insert_axis, insert_kind,
         insert_fx, insert_fy, insert_fw, insert_fh, design_root, type](DragGestureValue v) mutable {
          drag_active.set(true);
          drag_type.set(type);
          drag_x.set(static_cast<double>(v.x));
          drag_y.set(static_cast<double>(v.y));
          auto plan = compute_insert_plan(design_root.get(), v.x, v.y);
          insert_show.set(plan.valid);
          insert_mode.set(plan.where);
          insert_axis.set(plan.axis);
          insert_kind.set(plan.indicator_kind);
          insert_fx.set(static_cast<double>(plan.indicator_rect.x));
          insert_fy.set(static_cast<double>(plan.indicator_rect.y));
          insert_fw.set(static_cast<double>(plan.indicator_rect.w));
          insert_fh.set(static_cast<double>(plan.indicator_rect.h));
        },
        [drag_active, drag_type, drag_x, drag_y, insert_show, insert_mode, insert_axis, insert_kind,
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
          insert_kind.set("");
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
    return view("Column")
        .prop("spacing", 12.0)
        .prop("cross_align", "start")
        .children({
            view("Text")
                .prop("value", "Basic")
                .prop("font_size", 16.0)
                .prop("color", 0xFFE0E0E0)
                .build(),
            view("Row")
                .prop("spacing", 10.0)
                .prop("cross_align", "center")
                .children({
                    onTapGesture(
                        view("Button")
                            .key("preview:button")
                            .prop("title", "Button")
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

} // namespace duorou::ui::editor

