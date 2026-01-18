#pragma once

#include <editor/editor_view_common.hpp>
#include <editor/editor_view_history.hpp>
#include <editor/editor_view_insert.hpp>
#include <editor/editor_view_panels_left.hpp>
#include <editor/editor_view_panels_right.hpp>
#include <editor/editor_view_style.hpp>
#include <editor/editor_view_theme_utils.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace duorou::ui::editor {
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

    auto drag_active = local_state<bool>("editor:lib_drag_active", false);
    auto drag_type = local_state<std::string>("editor:lib_drag_type", "");
    auto drag_key = local_state<std::string>("editor:drag_key", "");
    auto drag_x = local_state<double>("editor:lib_drag_x", 0.0);
    auto drag_y = local_state<double>("editor:lib_drag_y", 0.0);
    auto insert_show = local_state<bool>("editor:insert_show", false);
    auto insert_mode = local_state<std::string>("editor:insert_mode", "");
    auto insert_axis = local_state<std::string>("editor:insert_axis", "");
    auto insert_kind = local_state<std::string>("editor:insert_kind", "");
    auto insert_fx = local_state<double>("editor:insert_fx", 0.0);
    auto insert_fy = local_state<double>("editor:insert_fy", 0.0);
    auto insert_fw = local_state<double>("editor:insert_fw", 0.0);
    auto insert_fh = local_state<double>("editor:insert_fh", 0.0);

    auto move_node = [design_root, selected_key, history, history_idx](std::string moving_key,
                                                                       InsertPlan plan) mutable {
      if (moving_key.empty() || moving_key == "design:root") {
        return;
      }
      if (!plan.valid) {
        return;
      }
      auto root = design_root.get();
      if (root.type.empty()) {
        return;
      }

      std::string old_parent_key;
      std::int64_t old_index = -1;
      auto moving = take_node_by_key_mut(root, moving_key, &old_parent_key, &old_index);
      if (!moving) {
        return;
      }

      if ((plan.where == "inside" && node_contains_key(*moving, plan.container_key)) ||
          (plan.where != "inside" && node_contains_key(*moving, plan.parent_key))) {
        return;
      }

      auto insert_into = [&](ViewNode &parent, std::int64_t idx) {
        const auto clamped = std::clamp<std::int64_t>(
            idx, 0, static_cast<std::int64_t>(parent.children.size()));
        parent.children.insert(parent.children.begin() + static_cast<std::ptrdiff_t>(clamped),
                               std::move(*moving));
      };

      if (plan.where == "inside") {
        ViewNode *container = find_node_by_key_mut(root, plan.container_key);
        if (!container || !is_container_type(container->type)) {
          return;
        }
        std::int64_t idx = plan.index;
        if (container->key == old_parent_key && old_index >= 0 && idx > old_index) {
          idx -= 1;
        }
        insert_into(*container, idx);
      } else {
        ViewNode *parent = find_node_by_key_mut(root, plan.parent_key);
        if (!parent || !is_container_type(parent->type)) {
          return;
        }
        std::int64_t idx = plan.index;
        if (parent->key == old_parent_key && old_index >= 0 && idx > old_index) {
          idx -= 1;
        }
        insert_into(*parent, idx);
      }

      push_history(history, history_idx, root);
      design_root.set(std::move(root));
      selected_key.set(moving_key);
    };

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
          const auto k = n.key;
          const auto t = n.type;
          n = onTapGesture(std::move(n), [selected_key, k]() mutable {
            selected_key.set(k);
          });
          if (k != "design:root") {
            n = DragGesture(
                std::move(n), "editor:design:" + k,
                [drag_active, drag_type, drag_key, drag_x, drag_y, insert_show, insert_mode,
                 insert_axis, insert_kind, insert_fx, insert_fy, insert_fw, insert_fh, design_root,
                 k, t](DragGestureValue v) mutable {
                  drag_active.set(true);
                  drag_type.set(t);
                  drag_key.set(k);
                  drag_x.set(static_cast<double>(v.x));
                  drag_y.set(static_cast<double>(v.y));
                  auto plan = compute_insert_plan(design_root.get(), v.x, v.y, k);
                  insert_show.set(plan.valid);
                  insert_mode.set(plan.where);
                  insert_axis.set(plan.axis);
                  insert_kind.set(plan.indicator_kind);
                  insert_fx.set(static_cast<double>(plan.indicator_rect.x));
                  insert_fy.set(static_cast<double>(plan.indicator_rect.y));
                  insert_fw.set(static_cast<double>(plan.indicator_rect.w));
                  insert_fh.set(static_cast<double>(plan.indicator_rect.h));
                },
                [drag_active, drag_type, drag_key, drag_x, drag_y, insert_show, insert_mode,
                 insert_axis, insert_kind, insert_fx, insert_fy, insert_fw, insert_fh, move_node,
                 design_root, k](DragGestureValue v) mutable {
                  drag_active.set(false);
                  drag_type.set("");
                  drag_key.set("");
                  drag_x.set(static_cast<double>(v.x));
                  drag_y.set(static_cast<double>(v.y));
                  auto plan = compute_insert_plan(design_root.get(), v.x, v.y, k);
                  move_node(k, std::move(plan));
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
          }
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

    auto drag_ghost_node = [&]() -> ViewNode { return drag_ghost(drag_type, drag_x, drag_y); };
    auto insert_indicator_node = [&]() -> ViewNode {
      return insert_indicator(insert_show, insert_mode, insert_axis, insert_kind, insert_fx,
                              insert_fy, insert_fw, insert_fh, viewport_w, viewport_h);
    };

    if (!theme_pop_open.get()) {
      if (!drag_active.get() && !insert_show.get()) {
        return main;
      }
      return view("Overlay")
          .children([&](auto &c) {
            c.add(std::move(main));
            c.add(insert_indicator_node());
            c.add(drag_ghost_node());
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
          c.add(insert_indicator_node());
          c.add(std::move(pop));
          if (drag_active.get()) {
            c.add(drag_ghost_node());
          }
        })
        .build();
  });
}

} // namespace duorou::ui::editor
