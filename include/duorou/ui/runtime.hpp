#pragma once

#include <duorou/ui/node.hpp>

#include <duorou/ui/layout.hpp>

#include <duorou/ui/render.hpp>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace duorou::ui {

class StateBase;
class ViewInstance;

class Subscription {
public:
  Subscription() = default;
  Subscription(const Subscription &) = delete;
  Subscription &operator=(const Subscription &) = delete;

  Subscription(Subscription &&other) noexcept
      : state_{std::exchange(other.state_, nullptr)},
        id_{std::exchange(other.id_, 0)} {}

  Subscription &operator=(Subscription &&other) noexcept {
    if (this == &other) {
      return *this;
    }
    reset();
    state_ = std::exchange(other.state_, nullptr);
    id_ = std::exchange(other.id_, 0);
    return *this;
  }

  ~Subscription() { reset(); }

  void reset();

private:
  friend class StateBase;
  explicit Subscription(StateBase *state, std::uint64_t id)
      : state_{state}, id_{id} {}

  StateBase *state_{};
  std::uint64_t id_{};
};

class StateBase {
public:
  using Callback = std::function<void()>;

  std::uint64_t version() const noexcept {
    return version_.load(std::memory_order_relaxed);
  }

  Subscription subscribe(Callback cb) {
    std::lock_guard<std::mutex> lock{mutex_};
    const auto id = ++next_id_;
    callbacks_.emplace(id, std::move(cb));
    return Subscription{this, id};
  }

private:
  friend class Subscription;
  void unsubscribe(std::uint64_t id) {
    std::lock_guard<std::mutex> lock{mutex_};
    callbacks_.erase(id);
  }

protected:
  void notify_changed() {
    version_.fetch_add(1, std::memory_order_relaxed);
    std::vector<Callback> callbacks;
    {
      std::lock_guard<std::mutex> lock{mutex_};
      callbacks.reserve(callbacks_.size());
      for (auto &kv : callbacks_) {
        callbacks.push_back(kv.second);
      }
    }
    for (auto &cb : callbacks) {
      if (cb) {
        cb();
      }
    }
  }

private:
  std::atomic<std::uint64_t> version_{0};
  std::mutex mutex_{};
  std::uint64_t next_id_{0};
  std::unordered_map<std::uint64_t, Callback> callbacks_{};
};

inline void Subscription::reset() {
  if (!state_ || id_ == 0) {
    return;
  }
  state_->unsubscribe(id_);
  state_ = nullptr;
  id_ = 0;
}

namespace detail {

struct DependencyCollector {
  std::unordered_set<StateBase *> states;

  void add(StateBase *s) {
    if (!s) {
      return;
    }
    states.insert(s);
  }
};

inline thread_local DependencyCollector *active_collector = nullptr;

inline void record_dependency(StateBase *state) {
  if (active_collector) {
    active_collector->add(state);
  }
}

struct EventCollector {
  std::uint64_t next_id{0};
  std::unordered_map<std::uint64_t, std::function<void()>> handlers;

  std::uint64_t add(std::function<void()> fn) {
    const auto id = ++next_id;
    handlers.emplace(id, std::move(fn));
    return id;
  }
};

inline thread_local EventCollector *active_event_collector = nullptr;

struct EventDispatchContext {
  int pointer_id{};
  float x{};
  float y{};
  int key{};
  int scancode{};
  int action{};
  int mods{};
  std::string text;
  ViewInstance *instance{};
  std::vector<std::size_t> target_path;
  std::string target_key;
};

inline thread_local EventDispatchContext *active_dispatch_context = nullptr;

} // namespace detail

inline std::uint64_t on_click(std::function<void()> fn) {
  if (!detail::active_event_collector) {
    return 0;
  }
  return detail::active_event_collector->add(std::move(fn));
}

inline std::uint64_t on_pointer_down(std::function<void()> fn) {
  return on_click(std::move(fn));
}

inline std::uint64_t on_pointer_up(std::function<void()> fn) {
  return on_click(std::move(fn));
}

inline std::uint64_t on_pointer_move(std::function<void()> fn) {
  return on_click(std::move(fn));
}

inline std::uint64_t on_focus(std::function<void()> fn) {
  return on_click(std::move(fn));
}

inline std::uint64_t on_blur(std::function<void()> fn) {
  return on_click(std::move(fn));
}

inline std::uint64_t on_key_down(std::function<void()> fn) {
  return on_click(std::move(fn));
}

inline std::uint64_t on_key_up(std::function<void()> fn) {
  return on_click(std::move(fn));
}

inline std::uint64_t on_text_input(std::function<void()> fn) {
  return on_click(std::move(fn));
}

inline int pointer_id() {
  return detail::active_dispatch_context
             ? detail::active_dispatch_context->pointer_id
             : 0;
}

inline float pointer_x() {
  return detail::active_dispatch_context ? detail::active_dispatch_context->x
                                         : 0.0f;
}

inline float pointer_y() {
  return detail::active_dispatch_context ? detail::active_dispatch_context->y
                                         : 0.0f;
}

inline int key_code() {
  return detail::active_dispatch_context ? detail::active_dispatch_context->key
                                         : 0;
}

inline int key_scancode() {
  return detail::active_dispatch_context
             ? detail::active_dispatch_context->scancode
             : 0;
}

inline int key_action() {
  return detail::active_dispatch_context
             ? detail::active_dispatch_context->action
             : 0;
}

inline int key_mods() {
  return detail::active_dispatch_context ? detail::active_dispatch_context->mods
                                         : 0;
}

inline std::string_view text_input() {
  return detail::active_dispatch_context ? std::string_view{
                                              detail::active_dispatch_context
                                                  ->text}
                                         : std::string_view{};
}

void capture_pointer();

void release_pointer();

template <typename T> class State final : public StateBase {
public:
  explicit State(T initial) : value_{std::move(initial)} {}

  T get() const {
    detail::record_dependency(const_cast<State *>(this));
    std::lock_guard<std::mutex> lock{value_mutex_};
    return value_;
  }

  void set(T v) {
    {
      std::lock_guard<std::mutex> lock{value_mutex_};
      value_ = std::move(v);
    }
    notify_changed();
  }

private:
  mutable std::mutex value_mutex_{};
  T value_{};
};

template <typename T> class StateHandle {
public:
  explicit StateHandle(std::shared_ptr<State<T>> ptr) : ptr_{std::move(ptr)} {}

  T get() const { return ptr_->get(); }

  void set(T v) { ptr_->set(std::move(v)); }

  StateBase *base() const { return ptr_.get(); }

private:
  std::shared_ptr<State<T>> ptr_;
};

template <typename T> StateHandle<T> state(T initial) {
  return StateHandle<T>{std::make_shared<State<T>>(std::move(initial))};
}

struct PatchSetProp {
  std::vector<std::size_t> path;
  std::string key;
  PropValue value;
};

struct PatchRemoveProp {
  std::vector<std::size_t> path;
  std::string key;
};

struct PatchReplaceNode {
  std::vector<std::size_t> path;
  ViewNode node;
};

struct PatchInsertChild {
  std::vector<std::size_t> parent_path;
  std::size_t index{};
  ViewNode node;
};

struct PatchRemoveChild {
  std::vector<std::size_t> parent_path;
  std::size_t index{};
};

using PatchOp = std::variant<PatchSetProp, PatchRemoveProp, PatchReplaceNode,
                             PatchInsertChild, PatchRemoveChild>;

inline void dump_path(std::ostream &os, const std::vector<std::size_t> &path) {
  os << "[";
  for (std::size_t i = 0; i < path.size(); ++i) {
    if (i != 0) {
      os << ",";
    }
    os << path[i];
  }
  os << "]";
}

inline void dump_patches(std::ostream &os,
                         const std::vector<PatchOp> &patches) {
  for (const auto &p : patches) {
    std::visit(
        [&](const auto &op) {
          using T = std::decay_t<decltype(op)>;
          if constexpr (std::is_same_v<T, PatchSetProp>) {
            os << "SetProp ";
            dump_path(os, op.path);
            os << " " << op.key << "=";
            std::visit([&](const auto &v) { os << v; }, op.value);
            os << "\n";
          } else if constexpr (std::is_same_v<T, PatchRemoveProp>) {
            os << "RemoveProp ";
            dump_path(os, op.path);
            os << " " << op.key << "\n";
          } else if constexpr (std::is_same_v<T, PatchReplaceNode>) {
            os << "ReplaceNode ";
            dump_path(os, op.path);
            os << " -> " << op.node.type << "\n";
          } else if constexpr (std::is_same_v<T, PatchInsertChild>) {
            os << "InsertChild ";
            dump_path(os, op.parent_path);
            os << " @" << op.index << " -> " << op.node.type << "\n";
          } else if constexpr (std::is_same_v<T, PatchRemoveChild>) {
            os << "RemoveChild ";
            dump_path(os, op.parent_path);
            os << " @" << op.index << "\n";
          }
        },
        p);
  }
}

inline void diff_nodes(const ViewNode &old_node, const ViewNode &new_node,
                       std::vector<std::size_t> &path,
                       std::vector<PatchOp> &out) {
  if (old_node.type != new_node.type) {
    out.push_back(PatchReplaceNode{path, new_node});
    return;
  }

  for (const auto &kv : new_node.props) {
    const auto it = old_node.props.find(kv.first);
    if (it == old_node.props.end() || it->second != kv.second) {
      out.push_back(PatchSetProp{path, kv.first, kv.second});
    }
  }

  for (const auto &kv : old_node.props) {
    if (!new_node.props.contains(kv.first)) {
      out.push_back(PatchRemoveProp{path, kv.first});
    }
  }

  const auto old_size = old_node.children.size();
  const auto new_size = new_node.children.size();
  const auto min_size = std::min(old_size, new_size);

  for (std::size_t i = 0; i < min_size; ++i) {
    path.push_back(i);
    diff_nodes(old_node.children[i], new_node.children[i], path, out);
    path.pop_back();
  }

  if (new_size > old_size) {
    for (std::size_t i = old_size; i < new_size; ++i) {
      out.push_back(PatchInsertChild{path, i, new_node.children[i]});
    }
  } else if (old_size > new_size) {
    for (std::size_t i = new_size; i < old_size; ++i) {
      out.push_back(PatchRemoveChild{path, new_size});
    }
  }
}

inline std::vector<PatchOp> diff_tree(const ViewNode &old_root,
                                      const ViewNode &new_root) {
  std::vector<PatchOp> out;
  std::vector<std::size_t> path;
  diff_nodes(old_root, new_root, path, out);
  return out;
}

class ViewInstance {
public:
  using ViewFn = std::function<ViewNode()>;

  explicit ViewInstance(ViewFn fn) : fn_{std::move(fn)} { rebuild(); }

  const ViewNode &tree() const { return tree_; }

  const LayoutNode &layout() const { return layout_; }

  const std::vector<RenderOp> &render_ops() const { return render_ops_; }

  bool dispatch_click(float x, float y) {
    const auto hit = hit_test(tree_, layout_, x, y);
    if (!hit) {
      return false;
    }

    auto path = hit->path;
    for (;;) {
      const auto *vn = node_at_path(tree_, path);
      if (!vn) {
        break;
      }

      const auto it = vn->events.find("click");
      if (it != vn->events.end() && it->second != 0) {
        const auto hit_id = it->second;
        const auto h = handlers_.find(hit_id);
        if (h != handlers_.end() && h->second) {
          h->second();
          return true;
        }
      }

      if (path.empty()) {
        break;
      }
      path.pop_back();
    }

    return false;
  }

  bool dispatch_pointer_down(int pointer, float x, float y) {
    return dispatch_pointer("pointer_down", pointer, x, y);
  }

  bool dispatch_pointer_up(int pointer, float x, float y) {
    return dispatch_pointer("pointer_up", pointer, x, y);
  }

  bool dispatch_pointer_move(int pointer, float x, float y) {
    return dispatch_pointer("pointer_move", pointer, x, y);
  }

  bool dispatch_key_down(int key, int scancode, int mods) {
    return dispatch_key("key_down", key, scancode, 1, mods);
  }

  bool dispatch_key_up(int key, int scancode, int mods) {
    return dispatch_key("key_up", key, scancode, 0, mods);
  }

  bool dispatch_text_input(std::string text) {
    return dispatch_text("text_input", std::move(text));
  }

  void capture_pointer_internal(int pointer,
                                const std::vector<std::size_t> &path,
                                const std::string &key) {
    CaptureTarget t;
    t.path = path;
    t.key = key;
    captures_.insert_or_assign(pointer, std::move(t));
  }

  void release_pointer_internal(int pointer) { captures_.erase(pointer); }

  struct UpdateResult {
    bool rebuilt{};
    std::vector<PatchOp> patches;
    bool layout_rebuilt{};
    bool render_rebuilt{};
  };

  UpdateResult update() {
    if (!dirty_ && !deps_changed()) {
      return UpdateResult{false, {}, false, false};
    }
    return rebuild();
  }

  void set_viewport(SizeF viewport) {
    viewport_ = viewport;
    layout_ = layout_tree(tree_, viewport_);
    render_ops_ = build_render_ops(tree_, layout_);
  }

private:
  struct CaptureTarget {
    std::vector<std::size_t> path;
    std::string key;
  };

  struct FocusTarget {
    std::vector<std::size_t> path;
    std::string key;
  };

  struct HitResult {
    std::vector<std::size_t> path;
  };

  static bool contains(const RectF &r, float x, float y) {
    return x >= r.x && y >= r.y && x < (r.x + r.w) && y < (r.y + r.h);
  }

  static bool node_hittable(const ViewNode &v) {
    if (!prop_as_bool(v.props, "hit_test", true)) {
      return false;
    }
    if (prop_as_float(v.props, "opacity", 1.0f) <= 0.0f) {
      return false;
    }
    const auto pe = prop_as_string(v.props, "pointer_events", "");
    return pe != "none";
  }

  static bool node_focusable(const ViewNode &v) {
    if (prop_as_bool(v.props, "focusable", false)) {
      return true;
    }
    if (v.type == "Button") {
      return true;
    }
    if (const auto it = v.events.find("key_down");
        it != v.events.end() && it->second != 0) {
      return true;
    }
    if (const auto it = v.events.find("key_up");
        it != v.events.end() && it->second != 0) {
      return true;
    }
    if (const auto it = v.events.find("text_input");
        it != v.events.end() && it->second != 0) {
      return true;
    }
    return false;
  }

  static const ViewNode *node_at_path(const ViewNode &root,
                                      const std::vector<std::size_t> &path) {
    const ViewNode *cur = &root;
    for (auto idx : path) {
      if (idx >= cur->children.size()) {
        return nullptr;
      }
      cur = &cur->children[idx];
    }
    return cur;
  }

  static std::optional<HitResult>
  hit_test_impl(const ViewNode &v, const LayoutNode &l, float x, float y,
                RectF clip, std::vector<std::size_t> &path) {
    const bool clip_self = prop_as_bool(v.props, "clip", false);
    if (clip_self) {
      clip = intersect_rect(clip, l.frame);
    }
    if (!contains(clip, x, y)) {
      return std::nullopt;
    }

    const auto n = std::min(v.children.size(), l.children.size());
    for (std::size_t i = n; i > 0; --i) {
      const auto idx = i - 1;
      path.push_back(idx);
      if (auto hit = hit_test_impl(v.children[idx], l.children[idx], x, y, clip,
                                   path)) {
        return hit;
      }
      path.pop_back();
    }

    if (contains(l.frame, x, y) && node_hittable(v)) {
      return HitResult{path};
    }
    return std::nullopt;
  }

  static std::optional<HitResult> hit_test(const ViewNode &root,
                                           const LayoutNode &layout_root,
                                           float x, float y) {
    std::vector<std::size_t> path;
    return hit_test_impl(root, layout_root, x, y, layout_root.frame, path);
  }

  static bool find_path_by_key_impl(const ViewNode &v, const std::string &key,
                                    std::vector<std::size_t> &path) {
    if (!key.empty() && v.key == key) {
      return true;
    }
    for (std::size_t i = 0; i < v.children.size(); ++i) {
      path.push_back(i);
      if (find_path_by_key_impl(v.children[i], key, path)) {
        return true;
      }
      path.pop_back();
    }
    return false;
  }

  static std::optional<std::vector<std::size_t>>
  find_path_by_key(const ViewNode &root, const std::string &key) {
    if (key.empty()) {
      return std::nullopt;
    }
    std::vector<std::size_t> path;
    if (!find_path_by_key_impl(root, key, path)) {
      return std::nullopt;
    }
    return path;
  }

  std::optional<std::vector<std::size_t>>
  resolve_target_path(const std::vector<std::size_t> &path,
                      const std::string &key) {
    if (!key.empty()) {
      if (const auto kp = find_path_by_key(tree_, key)) {
        return *kp;
      }
      return std::nullopt;
    }
    return path;
  }

  bool dispatch_bubble(const std::string &event_name,
                       detail::EventDispatchContext &ctx,
                       std::vector<std::size_t> path) {
    detail::active_dispatch_context = &ctx;
    for (;;) {
      const auto *vn = node_at_path(tree_, path);
      if (!vn) {
        break;
      }

      const auto it = vn->events.find(event_name);
      if (it != vn->events.end() && it->second != 0) {
        ctx.target_path = path;
        ctx.target_key = vn->key;
        const auto handle_id = it->second;
        const auto h = handlers_.find(handle_id);
        if (h != handlers_.end() && h->second) {
          h->second();
          detail::active_dispatch_context = nullptr;
          return true;
        }
      }

      if (path.empty()) {
        break;
      }
      path.pop_back();
    }
    detail::active_dispatch_context = nullptr;
    return false;
  }

  std::optional<std::vector<std::size_t>> focus_path() {
    if (!focus_) {
      return std::nullopt;
    }
    if (auto p = resolve_target_path(focus_->path, focus_->key)) {
      return p;
    }
    focus_.reset();
    return std::nullopt;
  }

  void set_focus(std::optional<std::vector<std::size_t>> path) {
    std::optional<FocusTarget> next;
    if (path) {
      const auto *vn = node_at_path(tree_, *path);
      if (!vn) {
        return;
      }
      FocusTarget t;
      t.path = *path;
      t.key = vn->key;
      next = std::move(t);
    }

    auto prev_path = focus_path();
    if (prev_path && next) {
      const auto *prev_vn = node_at_path(tree_, *prev_path);
      if (prev_vn && prev_vn->key == next->key && *prev_path == next->path) {
        return;
      }
    } else if (!prev_path && !next) {
      return;
    }

    if (prev_path) {
      detail::EventDispatchContext ctx;
      ctx.instance = this;
      dispatch_bubble("blur", ctx, *prev_path);
    }

    focus_ = std::move(next);

    if (path) {
      detail::EventDispatchContext ctx;
      ctx.instance = this;
      dispatch_bubble("focus", ctx, *path);
    }
  }

  void focus_from_hit_path(std::optional<std::vector<std::size_t>> hit_path) {
    if (!hit_path) {
      set_focus(std::nullopt);
      return;
    }
    auto path = *hit_path;
    for (;;) {
      const auto *vn = node_at_path(tree_, path);
      if (vn && node_focusable(*vn)) {
        set_focus(path);
        return;
      }
      if (path.empty()) {
        break;
      }
      path.pop_back();
    }
    set_focus(std::nullopt);
  }

  bool dispatch_pointer(const std::string &event_name, int pointer, float x,
                        float y) {
    detail::EventDispatchContext ctx;
    ctx.pointer_id = pointer;
    ctx.x = x;
    ctx.y = y;
    ctx.instance = this;

    if (const auto cap_it = captures_.find(pointer);
        cap_it != captures_.end()) {
      auto path = resolve_target_path(cap_it->second.path, cap_it->second.key);
      if (!path) {
        captures_.erase(pointer);
      } else {
        if (event_name == "pointer_down") {
          focus_from_hit_path(*path);
        }

        const auto *vn = node_at_path(tree_, *path);
        if (vn) {
          ctx.target_path = *path;
          ctx.target_key = vn->key;
          detail::active_dispatch_context = &ctx;
          const auto it = vn->events.find(event_name);
          if (it != vn->events.end() && it->second != 0) {
            const auto handle_id = it->second;
            const auto h = handlers_.find(handle_id);
            if (h != handlers_.end() && h->second) {
              h->second();
              detail::active_dispatch_context = nullptr;
              return true;
            }
          }
          detail::active_dispatch_context = nullptr;
          return false;
        }
      }
    }

    const auto hit = hit_test(tree_, layout_, x, y);
    if (event_name == "pointer_down") {
      focus_from_hit_path(hit ? std::optional{hit->path} : std::nullopt);
    }

    if (!hit) {
      return false;
    }

    return dispatch_bubble(event_name, ctx, hit->path);
  }

  bool dispatch_key(const std::string &event_name, int key, int scancode,
                    int action, int mods) {
    auto path = focus_path();
    if (!path) {
      return false;
    }

    detail::EventDispatchContext ctx;
    ctx.instance = this;
    ctx.key = key;
    ctx.scancode = scancode;
    ctx.action = action;
    ctx.mods = mods;
    return dispatch_bubble(event_name, ctx, *path);
  }

  bool dispatch_text(const std::string &event_name, std::string text) {
    auto path = focus_path();
    if (!path) {
      return false;
    }

    detail::EventDispatchContext ctx;
    ctx.instance = this;
    ctx.text = std::move(text);
    return dispatch_bubble(event_name, ctx, *path);
  }

  struct DepEntry {
    StateBase *state{};
    std::uint64_t version{};
    Subscription sub;
  };

  bool deps_changed() const {
    for (const auto &d : deps_) {
      if (!d.state) {
        continue;
      }
      if (d.state->version() != d.version) {
        return true;
      }
    }
    return false;
  }

  UpdateResult rebuild() {
    auto old_tree = tree_;

    detail::EventCollector event_collector;

    detail::DependencyCollector collector;
    detail::active_collector = &collector;
    detail::active_event_collector = &event_collector;
    auto new_tree = fn_();
    detail::active_collector = nullptr;
    detail::active_event_collector = nullptr;

    auto patches = old_tree.type.empty() ? std::vector<PatchOp>{}
                                         : diff_tree(old_tree, new_tree);
    tree_ = std::move(new_tree);

    handlers_ = std::move(event_collector.handlers);

    layout_ = layout_tree(tree_, viewport_);
    render_ops_ = build_render_ops(tree_, layout_);

    deps_.clear();
    deps_.reserve(collector.states.size());
    for (auto *s : collector.states) {
      DepEntry entry;
      entry.state = s;
      entry.version = s->version();
      entry.sub = s->subscribe([this]() { dirty_ = true; });
      deps_.push_back(std::move(entry));
    }

    dirty_ = false;
    return UpdateResult{true, std::move(patches), true, true};
  }

  ViewFn fn_;
  ViewNode tree_{};
  LayoutNode layout_{};
  std::vector<RenderOp> render_ops_{};
  SizeF viewport_{800.0f, 600.0f};
  std::vector<DepEntry> deps_{};
  std::unordered_map<std::uint64_t, std::function<void()>> handlers_{};
  std::unordered_map<int, CaptureTarget> captures_{};
  std::optional<FocusTarget> focus_{};
  bool dirty_{true};
};

inline void capture_pointer() {
  if (!detail::active_dispatch_context ||
      !detail::active_dispatch_context->instance) {
    return;
  }
  detail::active_dispatch_context->instance->capture_pointer_internal(
      detail::active_dispatch_context->pointer_id,
      detail::active_dispatch_context->target_path,
      detail::active_dispatch_context->target_key);
}

inline void release_pointer() {
  if (!detail::active_dispatch_context ||
      !detail::active_dispatch_context->instance) {
    return;
  }
  detail::active_dispatch_context->instance->release_pointer_internal(
      detail::active_dispatch_context->pointer_id);
}

} // namespace duorou::ui
