#pragma once

#include <duorou/ui/node.hpp>

#include <duorou/ui/layout.hpp>

#include <duorou/ui/render.hpp>

#include <duorou/ui/style_parser.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <limits>
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

#if defined(_WIN32)
#if !defined(NOMINMAX)
#define NOMINMAX 1
#endif
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#if defined(DrawText)
#undef DrawText
#endif
#endif

namespace duorou::ui {

class StateBase;
class ViewInstance;

inline double now_ms();

struct AnimationSpec {
  double duration_ms{200.0};
  double delay_ms{0.0};
  std::string curve{"easeInOut"};
};

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
  virtual ~StateBase() = default;
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

class ObservableObject : public StateBase {
public:
  void notify() { notify_changed(); }
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
inline thread_local ViewInstance *active_build_instance = nullptr;
inline thread_local std::optional<AnimationSpec> active_animation_spec = std::nullopt;

void request_animation(const AnimationSpec &spec);

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

std::optional<RectF> target_frame();

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
    if (detail::active_animation_spec) {
      detail::request_animation(*detail::active_animation_spec);
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

inline BindingId bind(const StateHandle<std::string> &h) {
  const auto p = reinterpret_cast<std::intptr_t>(h.base());
  return BindingId{static_cast<std::int64_t>(p)};
}

inline std::string binding_get(BindingId id) {
  if (id.raw == 0) {
    return {};
  }
  auto *base = reinterpret_cast<StateBase *>(static_cast<std::intptr_t>(id.raw));
  auto *s = dynamic_cast<State<std::string> *>(base);
  return s ? s->get() : std::string{};
}

inline void binding_set(BindingId id, std::string v) {
  if (id.raw == 0) {
    return;
  }
  auto *base = reinterpret_cast<StateBase *>(static_cast<std::intptr_t>(id.raw));
  auto *s = dynamic_cast<State<std::string> *>(base);
  if (!s) {
    return;
  }
  s->set(std::move(v));
}

constexpr int KEY_SPACE = 32;
constexpr int KEY_BACKSPACE = 259;
constexpr int KEY_DELETE = 261;
constexpr int KEY_ENTER = 257;
constexpr int KEY_KP_ENTER = 335;
constexpr int KEY_LEFT = 263;
constexpr int KEY_RIGHT = 262;
constexpr int KEY_UP = 265;
constexpr int KEY_DOWN = 264;
constexpr int KEY_HOME = 268;
constexpr int KEY_END = 269;

inline std::int64_t utf8_count(std::string_view s) {
  std::int64_t n = 0;
  for (std::size_t i = 0; i < s.size();) {
    const auto b = static_cast<std::uint8_t>(s[i]);
    std::size_t adv = 1;
    if ((b & 0x80) == 0x00) {
      adv = 1;
    } else if ((b & 0xE0) == 0xC0) {
      adv = 2;
    } else if ((b & 0xF0) == 0xE0) {
      adv = 3;
    } else if ((b & 0xF8) == 0xF0) {
      adv = 4;
    }
    if (i + adv > s.size()) {
      adv = 1;
    }
    i += adv;
    ++n;
  }
  return n;
}

inline std::size_t utf8_byte_offset_from_char(std::string_view s,
                                              std::int64_t char_index) {
  if (char_index <= 0) {
    return 0;
  }
  std::int64_t n = 0;
  std::size_t i = 0;
  while (i < s.size() && n < char_index) {
    const auto b = static_cast<std::uint8_t>(s[i]);
    std::size_t adv = 1;
    if ((b & 0x80) == 0x00) {
      adv = 1;
    } else if ((b & 0xE0) == 0xC0) {
      adv = 2;
    } else if ((b & 0xF0) == 0xE0) {
      adv = 3;
    } else if ((b & 0xF8) == 0xF0) {
      adv = 4;
    }
    if (i + adv > s.size()) {
      adv = 1;
    }
    i += adv;
    ++n;
  }
  return i;
}

inline void utf8_erase_prev_char(std::string &s, std::int64_t &caret) {
  const auto len = utf8_count(s);
  caret = std::max<std::int64_t>(0, std::min(caret, len));
  if (caret <= 0) {
    return;
  }
  const auto end = utf8_byte_offset_from_char(s, caret);
  const auto start = utf8_byte_offset_from_char(s, caret - 1);
  if (start <= end && end <= s.size()) {
    s.erase(start, end - start);
    --caret;
  }
}

inline void utf8_insert_at_char(std::string &s, std::int64_t &caret,
                                std::string_view insert) {
  const auto len = utf8_count(s);
  caret = std::max<std::int64_t>(0, std::min(caret, len));
  const auto pos = utf8_byte_offset_from_char(s, caret);
  s.insert(pos, insert.data(), insert.size());
  caret += utf8_count(insert);
}

inline void utf8_erase_at_char(std::string &s, std::int64_t caret) {
  const auto len = utf8_count(s);
  caret = std::max<std::int64_t>(0, std::min(caret, len));
  if (caret >= len) {
    return;
  }
  const auto start = utf8_byte_offset_from_char(s, caret);
  const auto end = utf8_byte_offset_from_char(s, caret + 1);
  if (start <= end && end <= s.size()) {
    s.erase(start, end - start);
  }
}

inline bool utf8_erase_range(std::string &s, std::int64_t &caret,
                             std::int64_t a, std::int64_t b) {
  const auto len = utf8_count(s);
  a = std::max<std::int64_t>(0, std::min(a, len));
  b = std::max<std::int64_t>(0, std::min(b, len));
  const auto start_c = std::min(a, b);
  const auto end_c = std::max(a, b);
  if (end_c <= start_c) {
    return false;
  }
  const auto start_b = utf8_byte_offset_from_char(s, start_c);
  const auto end_b = utf8_byte_offset_from_char(s, end_c);
  if (!(start_b <= end_b && end_b <= s.size())) {
    return false;
  }
  s.erase(start_b, end_b - start_b);
  caret = start_c;
  return true;
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

  void set_env_value(std::string key, PropValue value) {
    env_values_.insert_or_assign(std::move(key), std::move(value));
  }

  const PropValue *env_value(const std::string &key) const {
    const auto it = env_values_.find(key);
    return it == env_values_.end() ? nullptr : &it->second;
  }

  void set_env_object(std::string key, std::shared_ptr<void> obj) {
    env_objects_.insert_or_assign(std::move(key), std::move(obj));
  }

  std::shared_ptr<void> env_object(const std::string &key) const {
    const auto it = env_objects_.find(key);
    return it == env_objects_.end() ? std::shared_ptr<void>{} : it->second;
  }

  void invoke_handler(std::uint64_t handler_id) {
    if (handler_id == 0) {
      return;
    }
    const auto it = handlers_.find(handler_id);
    if (it != handlers_.end() && it->second) {
      it->second();
    }
  }

  std::optional<RectF>
  layout_frame_at_path(const std::vector<std::size_t> &path) const {
    const auto *ln = layout_at_path(layout_, path);
    if (!ln) {
      return std::nullopt;
    }
    return ln->frame;
  }

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
    pointers_down_.insert(pointer);
    return dispatch_pointer("pointer_down", pointer, x, y);
  }

  bool dispatch_pointer_up(int pointer, float x, float y) {
    const bool handled = dispatch_pointer("pointer_up", pointer, x, y);
    pointers_down_.erase(pointer);
    captures_.erase(pointer);
    return handled;
  }

  bool dispatch_pointer_move(int pointer, float x, float y) {
    if (pointers_down_.find(pointer) == pointers_down_.end() &&
        captures_.find(pointer) == captures_.end()) {
      return false;
    }
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

  bool dispatch_scroll(float x, float y, float delta_y_px) {
    const auto hit = hit_test(tree_, layout_, x, y);
    if (!hit) {
      return false;
    }
    const auto sv_path = scrollview_path_from_hit(hit->path);
    if (!sv_path) {
      return false;
    }

    auto *vn = node_at_path_mut(tree_, *sv_path);
    if (!vn || vn->type != "ScrollView") {
      return false;
    }

    double cur = prop_as_float(vn->props, "scroll_y", 0.0f);
    if (!find_prop(vn->props, "scroll_y") && !vn->key.empty()) {
      if (const auto it = scroll_offsets_.find(vn->key);
          it != scroll_offsets_.end()) {
        cur = it->second;
      }
    }

    float max_scroll = 0.0f;
    if (const auto *ln = layout_at_path(layout_, *sv_path)) {
      max_scroll = ln->scroll_max_y;
    }

    double next = cur + static_cast<double>(delta_y_px);
    if (next < 0.0) {
      next = 0.0;
    }
    if (next > static_cast<double>(max_scroll)) {
      next = static_cast<double>(max_scroll);
    }

    vn->props.insert_or_assign("scroll_y", PropValue{next});
    if (!vn->key.empty()) {
      scroll_offsets_.insert_or_assign(vn->key, next);
    }
    layout_ = layout_tree(tree_, viewport_);
    render_ops_ = build_render_ops(tree_, layout_);
    return true;
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
    const double now = now_ms();

    bool any_timeline = false;
    for (auto &kv : timelines_) {
      auto &t = kv.second;
      if (!(t.interval_ms > 0.0)) {
        continue;
      }
      if (t.last_ms <= 0.0) {
        t.last_ms = now;
        continue;
      }
      if (now - t.last_ms >= t.interval_ms) {
        t.last_ms = now;
        auto slot = local_state<double>(t.key + ":timeline_now", now);
        slot.set(now);
        any_timeline = true;
      }
    }

    if (dirty_ || deps_changed()) {
      return rebuild();
    }

    if (!anims_.empty()) {
      const bool changed = step_animations(now);
      if (changed) {
        render_ops_ = build_render_ops(tree_, layout_);
        return UpdateResult{false, {}, false, true};
      }
    }

    if (any_timeline) {
      return rebuild();
    }

    return UpdateResult{false, {}, false, false};
  }

  void set_viewport(SizeF viewport) {
    viewport_ = viewport;
    layout_ = layout_tree(tree_, viewport_);
    render_ops_ = build_render_ops(tree_, layout_);
  }

  void set_pending_animation(AnimationSpec spec) {
    pending_animation_ = std::move(spec);
  }

  void register_timeline(std::string key, double interval_ms) {
    auto &t = timelines_[key];
    t.key = std::move(key);
    t.interval_ms = interval_ms;
    if (t.last_ms < 0.0) {
      t.last_ms = 0.0;
    }
  }

  template <typename T> StateHandle<T> local_state(std::string key, T initial) {
    const auto it = local_states_.find(key);
    if (it != local_states_.end()) {
      if (auto p = std::dynamic_pointer_cast<State<T>>(it->second)) {
        return StateHandle<T>{std::move(p)};
      }
    }
    auto p = std::make_shared<State<T>>(std::move(initial));
    local_states_.insert_or_assign(std::move(key), p);
    return StateHandle<T>{std::move(p)};
  }

private:
  struct PropAnim {
    std::vector<std::size_t> path;
    std::string prop_key;
    PropValue from;
    PropValue to;
    double start_ms{};
    double duration_ms{};
    double delay_ms{};
  };

  struct TimelineReg {
    std::string key;
    double interval_ms{};
    double last_ms{};
  };

  struct MatchedGeomEntry {
    std::vector<std::size_t> path;
    RectF frame;
  };

  static bool prop_is_color_key(const std::string &k) {
    return k == "bg" || k == "border" || k == "color" || k == "tint" ||
           k == "track" || k == "fill" || k == "scrollbar_track" ||
           k == "scrollbar_thumb";
  }

  static bool prop_is_animatable_key(const std::string &k) {
    if (prop_is_color_key(k)) {
      return true;
    }
    return k == "opacity" || k == "render_offset_x" || k == "render_offset_y" ||
           k == "border_width";
  }

  static bool prop_can_interpolate(const std::string &key, const PropValue &a,
                                  const PropValue &b) {
    if (prop_is_color_key(key)) {
      const auto ca = [&]() -> bool {
        if (std::holds_alternative<std::int64_t>(a)) {
          return true;
        }
        if (std::holds_alternative<double>(a)) {
          return true;
        }
        return false;
      }();
      const auto cb = [&]() -> bool {
        if (std::holds_alternative<std::int64_t>(b)) {
          return true;
        }
        if (std::holds_alternative<double>(b)) {
          return true;
        }
        return false;
      }();
      return ca && cb;
    }

    double da = 0.0;
    double db = 0.0;
    return prop_as_double_any(a, da) && prop_as_double_any(b, db);
  }

  static bool prop_as_double_any(const PropValue &v, double &out) {
    if (const auto *d = std::get_if<double>(&v)) {
      out = *d;
      return true;
    }
    if (const auto *i = std::get_if<std::int64_t>(&v)) {
      out = static_cast<double>(*i);
      return true;
    }
    if (const auto *b = std::get_if<bool>(&v)) {
      out = *b ? 1.0 : 0.0;
      return true;
    }
    return false;
  }

  static std::uint32_t pack_color_u32(ColorU8 c) {
    return static_cast<std::uint32_t>(c.r) |
           (static_cast<std::uint32_t>(c.g) << 8) |
           (static_cast<std::uint32_t>(c.b) << 16) |
           (static_cast<std::uint32_t>(c.a) << 24);
  }

  static PropValue interpolate_prop(const std::string &key, const PropValue &a,
                                    const PropValue &b, double t) {
    t = std::max(0.0, std::min(1.0, t));
    if (prop_is_color_key(key)) {
      const auto ca = [&]() -> std::optional<ColorU8> {
        if (const auto *i = std::get_if<std::int64_t>(&a)) {
          return color_from_u32(static_cast<std::uint32_t>(
              static_cast<std::uint64_t>(*i) & 0xFFFFFFFFull));
        }
        if (const auto *d = std::get_if<double>(&a)) {
          return color_from_u32(static_cast<std::uint32_t>(
              static_cast<std::uint64_t>(*d) & 0xFFFFFFFFull));
        }
        return std::nullopt;
      }();
      const auto cb = [&]() -> std::optional<ColorU8> {
        if (const auto *i = std::get_if<std::int64_t>(&b)) {
          return color_from_u32(static_cast<std::uint32_t>(
              static_cast<std::uint64_t>(*i) & 0xFFFFFFFFull));
        }
        if (const auto *d = std::get_if<double>(&b)) {
          return color_from_u32(static_cast<std::uint32_t>(
              static_cast<std::uint64_t>(*d) & 0xFFFFFFFFull));
        }
        return std::nullopt;
      }();
      if (ca && cb) {
        auto lerp_u8 = [&](std::uint8_t x, std::uint8_t y) -> std::uint8_t {
          const double v = static_cast<double>(x) +
                           (static_cast<double>(y) - static_cast<double>(x)) * t;
          return clamp_u8(static_cast<int>(std::lround(v)));
        };
        ColorU8 c;
        c.r = lerp_u8(ca->r, cb->r);
        c.g = lerp_u8(ca->g, cb->g);
        c.b = lerp_u8(ca->b, cb->b);
        c.a = lerp_u8(ca->a, cb->a);
        return PropValue{static_cast<std::int64_t>(pack_color_u32(c))};
      }
    }

    double da = 0.0;
    double db = 0.0;
    if (prop_as_double_any(a, da) && prop_as_double_any(b, db)) {
      return PropValue{da + (db - da) * t};
    }

    return b;
  }

  static std::optional<AnimationSpec> animation_spec_from_node(const ViewNode &v) {
    if (!prop_as_bool(v.props, "animation_enabled", false)) {
      return std::nullopt;
    }
    AnimationSpec s;
    s.duration_ms = prop_as_float(v.props, "animation_duration_ms", 200.0f);
    s.delay_ms = prop_as_float(v.props, "animation_delay_ms", 0.0f);
    s.curve = prop_as_string(v.props, "animation_curve", "easeInOut");
    return s;
  }

  std::optional<AnimationSpec>
  animation_spec_for_path(const ViewNode &root,
                          const std::vector<std::size_t> &path) const {
    std::vector<std::size_t> cur = path;
    for (;;) {
      if (const auto *vn = node_at_path(root, cur)) {
        if (auto s = animation_spec_from_node(*vn)) {
          return s;
        }
      }
      if (cur.empty()) {
        break;
      }
      cur.pop_back();
    }
    return std::nullopt;
  }

  bool step_animations(double now) {
    bool changed = false;
    std::vector<PropAnim> keep;
    keep.reserve(anims_.size());
    for (auto &a : anims_) {
      if (a.path.empty() && a.prop_key.empty()) {
        continue;
      }
      const double t0 = a.start_ms + a.delay_ms;
      if (now < t0) {
        keep.push_back(a);
        continue;
      }
      const double denom = std::max(1e-6, a.duration_ms);
      const double t = (now - t0) / denom;
      auto *vn = node_at_path_mut(tree_, a.path);
      if (!vn) {
        continue;
      }
      const auto next = interpolate_prop(a.prop_key, a.from, a.to, t);
      vn->props.insert_or_assign(a.prop_key, next);
      changed = true;
      if (t < 1.0) {
        keep.push_back(a);
      } else {
        vn->props.insert_or_assign(a.prop_key, a.to);
      }
    }
    anims_ = std::move(keep);
    return changed;
  }

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

  static ViewNode flatten_groups(ViewNode node) {
    std::vector<ViewNode> out_children;
    out_children.reserve(node.children.size());

    for (auto &c : node.children) {
      auto cc = flatten_groups(std::move(c));
      if (cc.type == "Group") {
        for (auto &gc : cc.children) {
          out_children.push_back(std::move(gc));
        }
      } else {
        out_children.push_back(std::move(cc));
      }
    }

    node.children = std::move(out_children);
    return node;
  }

  static ViewNode normalize_root(ViewNode node) {
    if (node.type != "Group") {
      return node;
    }
    if (node.children.empty()) {
      return view("Spacer").build();
    }
    if (node.children.size() == 1) {
      return std::move(node.children[0]);
    }
    auto children = std::move(node.children);
    auto b = view("Box");
    b.children([&](auto &c) {
      for (auto &ch : children) {
        c.add(std::move(ch));
      }
    });
    return std::move(b).build();
  }

  static bool contains(const RectF &r, float x, float y) {
    return x >= r.x && y >= r.y && x < (r.x + r.w) && y < (r.y + r.h);
  }

  static const LayoutNode *
  layout_at_path(const LayoutNode &root, const std::vector<std::size_t> &path) {
    const LayoutNode *cur = &root;
    for (auto idx : path) {
      if (idx >= cur->children.size()) {
        return nullptr;
      }
      cur = &cur->children[idx];
    }
    return cur;
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
    if (v.type == "TextField" || v.type == "TextEditor") {
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

  static ViewNode *node_at_path_mut(ViewNode &root,
                                    const std::vector<std::size_t> &path) {
    ViewNode *cur = &root;
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
    if ((event_name == "pointer_move" || event_name == "pointer_up") &&
        update_scroll_from_drag(event_name, pointer, x, y)) {
      return true;
    }

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

    if (event_name == "pointer_down") {
      if (auto sv_path = scrollview_path_from_hit(hit->path)) {
        const auto *sv = node_at_path(tree_, *sv_path);
        if (sv) {
          ScrollDrag d;
          d.path = *sv_path;
          d.key = sv->key;
          d.start_x = x;
          d.start_y = y;
          d.last_y = y;
          double sy = prop_as_float(sv->props, "scroll_y", 0.0f);
          if (!find_prop(sv->props, "scroll_y") && !sv->key.empty()) {
            if (const auto it = scroll_offsets_.find(sv->key);
                it != scroll_offsets_.end()) {
              sy = it->second;
            }
          }
          d.start_scroll_y = sy;
          d.activated = false;
          scroll_drags_.insert_or_assign(pointer, std::move(d));
        }
      }
    }

    return dispatch_bubble(event_name, ctx, hit->path);
  }

  struct ScrollDrag {
    std::vector<std::size_t> path;
    std::string key;
    float start_x{};
    float start_y{};
    float last_y{};
    double start_scroll_y{};
    bool activated{};
  };

  std::optional<std::vector<std::size_t>>
  scrollview_path_from_hit(std::vector<std::size_t> hit_path) {
    for (;;) {
      const auto *vn = node_at_path(tree_, hit_path);
      if (vn && vn->type == "ScrollView" &&
          prop_as_bool(vn->props, "scroll_enabled", true)) {
        return hit_path;
      }
      if (hit_path.empty()) {
        break;
      }
      hit_path.pop_back();
    }
    return std::nullopt;
  }

  void restore_scroll_offsets(ViewNode &root) {
    if (root.type == "ScrollView" && !root.key.empty()) {
      if (!find_prop(root.props, "scroll_y")) {
        const auto it = scroll_offsets_.find(root.key);
        if (it != scroll_offsets_.end()) {
          root.props.insert_or_assign("scroll_y", PropValue{it->second});
        }
      }
    }
    for (auto &c : root.children) {
      restore_scroll_offsets(c);
    }
  }

  bool update_scroll_from_drag(const std::string &event_name, int pointer,
                               float x, float y) {
    const auto it = scroll_drags_.find(pointer);
    if (it == scroll_drags_.end()) {
      return false;
    }

    auto path = resolve_target_path(it->second.path, it->second.key);
    if (!path) {
      scroll_drags_.erase(pointer);
      return false;
    }

    auto *vn = node_at_path_mut(tree_, *path);
    if (!vn || vn->type != "ScrollView") {
      scroll_drags_.erase(pointer);
      return false;
    }

    if (event_name == "pointer_move") {
      it->second.last_y = y;
      const float dy = y - it->second.start_y;
      if (!it->second.activated) {
        if (std::abs(dy) < 3.0f) {
          return false;
        }
        it->second.activated = true;
        capture_pointer_internal(pointer, *path, vn->key);
      }

      const double next_scroll = it->second.start_scroll_y + static_cast<double>(dy);
      vn->props.insert_or_assign("scroll_y", PropValue{next_scroll});
      if (!vn->key.empty()) {
        scroll_offsets_.insert_or_assign(vn->key, next_scroll);
      }
      layout_ = layout_tree(tree_, viewport_);
      render_ops_ = build_render_ops(tree_, layout_);
      return true;
    }

    if (event_name == "pointer_up") {
      const bool was_drag = it->second.activated;
      scroll_drags_.erase(pointer);
      if (was_drag) {
        release_pointer_internal(pointer);
        return true;
      }
      return false;
    }

    return false;
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
    auto old_layout = layout_;

    detail::EventCollector event_collector;

    env_values_.clear();
    env_objects_.clear();
    timelines_.clear();

    detail::DependencyCollector collector;
    detail::active_collector = &collector;
    detail::active_event_collector = &event_collector;
    detail::active_build_instance = this;
    auto new_tree = fn_();
    if (const auto *pv = env_value("style.toml")) {
      if (const auto *s = std::get_if<std::string>(pv)) {
        if (*s != style_toml_cache_) {
          style_toml_cache_ = *s;
          style_rules_cache_ = detail::parse_stylesheet_toml(style_toml_cache_);
        }
        if (!style_rules_cache_.empty()) {
          detail::apply_styles_to_tree(new_tree, style_rules_cache_);
        }
      }
    }

    auto apply_text_bindings = [&](auto &&self, ViewNode &node) -> void {
      for (auto &ch : node.children) {
        self(self, ch);
      }

      if (node.type != "TextField" && node.type != "TextEditor") {
        return;
      }

      auto binding_raw = prop_as_i64_opt(node.props, "binding");
      if (!binding_raw) {
        binding_raw = prop_as_i64_opt(node.props, "value");
      }
      if (!binding_raw || *binding_raw == 0) {
        return;
      }

      const BindingId binding{*binding_raw};

      std::string state_key = node.key;
      if (state_key.empty()) {
        state_key = std::string{"node:"} + std::to_string(node.id);
      }

      auto focused = local_state<bool>(state_key + ":focused", false);
      auto caret = local_state<std::int64_t>(state_key + ":caret", 0);
      auto sel_anchor = local_state<std::int64_t>(state_key + ":sel_anchor", 0);
      auto sel_end = local_state<std::int64_t>(state_key + ":sel_end", 0);

      const auto padding = prop_as_float(node.props, "padding", 10.0f);
      const auto font_px = prop_as_float(node.props, "font_size", 16.0f);

      {
        auto s = binding_get(binding);
        node.props.insert_or_assign("value", PropValue{s});
        node.props.insert_or_assign("focused", PropValue{focused.get()});
        node.props.insert_or_assign("caret", PropValue{caret.get()});
        node.props.insert_or_assign("sel_start", PropValue{sel_anchor.get()});
        node.props.insert_or_assign("sel_end", PropValue{sel_end.get()});
      }

      if (auto it = node.events.find("focus");
          it == node.events.end() || it->second == 0) {
        node.events.insert_or_assign(
            "focus", event_collector.add([focused, caret, sel_anchor, sel_end,
                                          binding]() mutable {
              focused.set(true);
              const auto next = utf8_count(binding_get(binding));
              caret.set(next);
              sel_anchor.set(next);
              sel_end.set(next);
            }));
      }
      if (auto it = node.events.find("blur");
          it == node.events.end() || it->second == 0) {
        node.events.insert_or_assign(
            "blur", event_collector.add([focused]() mutable { focused.set(false); }));
      }

      if (auto it = node.events.find("pointer_down");
          it == node.events.end() || it->second == 0) {
        if (node.type == "TextField") {
          node.events.insert_or_assign(
              "pointer_down",
              event_collector.add([caret, sel_anchor, sel_end, binding, padding,
                                   font_px]() mutable {
                auto r = target_frame();
                if (!r) {
                  return;
                }
                const float char_w = font_px * 0.5f;
                const float local_x = pointer_x() - (r->x + padding);
                const auto len = utf8_count(binding_get(binding));
                auto pos = static_cast<std::int64_t>(
                    std::round(char_w > 0.0f ? (local_x / char_w) : 0.0f));
                pos = std::max<std::int64_t>(0, std::min(pos, len));
                caret.set(pos);
                sel_anchor.set(pos);
                sel_end.set(pos);
                capture_pointer();
              }));
        } else {
          node.events.insert_or_assign(
              "pointer_down",
              event_collector.add([caret, sel_anchor, sel_end, binding, padding,
                                   font_px]() mutable {
                auto r = target_frame();
                if (!r) {
                  return;
                }
                const float char_w = font_px * 0.5f;
                const float line_h = font_px * 1.2f;

                const float local_x = pointer_x() - (r->x + padding);
                const float local_y = pointer_y() - (r->y + padding);
                const std::int64_t col = static_cast<std::int64_t>(
                    std::max(0.0f, std::round(char_w > 0.0f ? (local_x / char_w)
                                                           : 0.0f)));
                const std::int64_t row = static_cast<std::int64_t>(
                    std::max(0.0f, std::floor(line_h > 0.0f ? (local_y / line_h)
                                                           : 0.0f)));

                auto s = binding_get(binding);
                struct Line {
                  std::int64_t start{};
                  std::int64_t len{};
                };
                std::vector<Line> lines;
                lines.reserve(8);
                {
                  std::int64_t start = 0;
                  std::int64_t cur = 0;
                  for (std::size_t i = 0; i < s.size();) {
                    if (s[i] == '\n') {
                      lines.push_back(Line{start, cur - start});
                      ++cur;
                      ++i;
                      start = cur;
                      continue;
                    }
                    const auto b0 = static_cast<std::uint8_t>(s[i]);
                    std::size_t adv = 1;
                    if ((b0 & 0x80) == 0x00) {
                      adv = 1;
                    } else if ((b0 & 0xE0) == 0xC0) {
                      adv = 2;
                    } else if ((b0 & 0xF0) == 0xE0) {
                      adv = 3;
                    } else if ((b0 & 0xF8) == 0xF0) {
                      adv = 4;
                    }
                    if (i + adv > s.size()) {
                      adv = 1;
                    }
                    i += adv;
                    ++cur;
                  }
                  lines.push_back(Line{start, cur - start});
                }

                const auto total_len = utf8_count(s);
                const auto rrow = std::max<std::int64_t>(
                    0, std::min<std::int64_t>(row,
                                              static_cast<std::int64_t>(lines.size()) -
                                                  1));
                const auto line_start = lines[static_cast<std::size_t>(rrow)].start;
                const auto line_len = lines[static_cast<std::size_t>(rrow)].len;
                const auto next = line_start + std::min(col, line_len);
                const auto pos =
                    std::max<std::int64_t>(0, std::min(next, total_len));
                caret.set(pos);
                sel_anchor.set(pos);
                sel_end.set(pos);
                capture_pointer();
              }));
        }
      }

      if (auto it = node.events.find("pointer_move");
          it == node.events.end() || it->second == 0) {
        if (node.type == "TextField") {
          node.events.insert_or_assign(
              "pointer_move",
              event_collector.add([caret, sel_end, binding, padding,
                                   font_px]() mutable {
                auto r = target_frame();
                if (!r) {
                  return;
                }
                const float char_w = font_px * 0.5f;
                const float local_x = pointer_x() - (r->x + padding);
                const auto len = utf8_count(binding_get(binding));
                auto pos = static_cast<std::int64_t>(
                    std::round(char_w > 0.0f ? (local_x / char_w) : 0.0f));
                pos = std::max<std::int64_t>(0, std::min(pos, len));
                caret.set(pos);
                sel_end.set(pos);
              }));
        } else {
          node.events.insert_or_assign(
              "pointer_move",
              event_collector.add([caret, sel_end, binding, padding,
                                   font_px]() mutable {
                auto r = target_frame();
                if (!r) {
                  return;
                }
                const float char_w = font_px * 0.5f;
                const float line_h = font_px * 1.2f;

                const float local_x = pointer_x() - (r->x + padding);
                const float local_y = pointer_y() - (r->y + padding);
                const std::int64_t col = static_cast<std::int64_t>(
                    std::max(0.0f, std::round(char_w > 0.0f ? (local_x / char_w)
                                                           : 0.0f)));
                const std::int64_t row = static_cast<std::int64_t>(
                    std::max(0.0f, std::floor(line_h > 0.0f ? (local_y / line_h)
                                                           : 0.0f)));

                auto s = binding_get(binding);
                struct Line {
                  std::int64_t start{};
                  std::int64_t len{};
                };
                std::vector<Line> lines;
                lines.reserve(8);
                {
                  std::int64_t start = 0;
                  std::int64_t cur = 0;
                  for (std::size_t i = 0; i < s.size();) {
                    if (s[i] == '\n') {
                      lines.push_back(Line{start, cur - start});
                      ++cur;
                      ++i;
                      start = cur;
                      continue;
                    }
                    const auto b0 = static_cast<std::uint8_t>(s[i]);
                    std::size_t adv = 1;
                    if ((b0 & 0x80) == 0x00) {
                      adv = 1;
                    } else if ((b0 & 0xE0) == 0xC0) {
                      adv = 2;
                    } else if ((b0 & 0xF0) == 0xE0) {
                      adv = 3;
                    } else if ((b0 & 0xF8) == 0xF0) {
                      adv = 4;
                    }
                    if (i + adv > s.size()) {
                      adv = 1;
                    }
                    i += adv;
                    ++cur;
                  }
                  lines.push_back(Line{start, cur - start});
                }

                const auto total_len = utf8_count(s);
                const auto rrow = std::max<std::int64_t>(
                    0, std::min<std::int64_t>(row,
                                              static_cast<std::int64_t>(lines.size()) -
                                                  1));
                const auto line_start = lines[static_cast<std::size_t>(rrow)].start;
                const auto line_len = lines[static_cast<std::size_t>(rrow)].len;
                const auto next = line_start + std::min(col, line_len);
                const auto pos =
                    std::max<std::int64_t>(0, std::min(next, total_len));
                caret.set(pos);
                sel_end.set(pos);
              }));
        }
      }

      if (auto it = node.events.find("pointer_up");
          it == node.events.end() || it->second == 0) {
        node.events.insert_or_assign(
            "pointer_up", event_collector.add([]() mutable { release_pointer(); }));
      }

      if (auto it = node.events.find("key_down");
          it == node.events.end() || it->second == 0) {
        if (node.type == "TextField") {
          node.events.insert_or_assign(
              "key_down",
              event_collector.add([binding, caret, sel_anchor, sel_end]() mutable {
                auto c = caret.get();
                auto a = sel_anchor.get();
                auto b = sel_end.get();
                auto s = binding_get(binding);
                const auto len = utf8_count(s);
                c = std::max<std::int64_t>(0, std::min(c, len));
                a = std::max<std::int64_t>(0, std::min(a, len));
                b = std::max<std::int64_t>(0, std::min(b, len));

                if (key_code() == KEY_LEFT) {
                  c = std::max<std::int64_t>(0, c - 1);
                } else if (key_code() == KEY_RIGHT) {
                  c = std::min<std::int64_t>(len, c + 1);
                } else if (key_code() == KEY_HOME) {
                  c = 0;
                } else if (key_code() == KEY_END) {
                  c = len;
                } else if (key_code() == KEY_BACKSPACE) {
                  if (a != b) {
                    if (utf8_erase_range(s, c, a, b)) {
                      binding_set(binding, std::move(s));
                      a = c;
                      b = c;
                    }
                  } else {
                    utf8_erase_prev_char(s, c);
                    binding_set(binding, std::move(s));
                  }
                } else if (key_code() == KEY_DELETE) {
                  if (a != b) {
                    if (utf8_erase_range(s, c, a, b)) {
                      binding_set(binding, std::move(s));
                      a = c;
                      b = c;
                    }
                  } else {
                    utf8_erase_at_char(s, c);
                    binding_set(binding, std::move(s));
                  }
                }

                caret.set(c);
                sel_anchor.set(c);
                sel_end.set(c);
              }));
        } else {
          node.events.insert_or_assign(
              "key_down",
              event_collector.add([binding, caret, sel_anchor, sel_end]() mutable {
                auto c = caret.get();
                auto a = sel_anchor.get();
                auto b = sel_end.get();
                auto s = binding_get(binding);
                const auto total_len = utf8_count(s);
                c = std::max<std::int64_t>(0, std::min(c, total_len));
                a = std::max<std::int64_t>(0, std::min(a, total_len));
                b = std::max<std::int64_t>(0, std::min(b, total_len));

                struct Line {
                  std::int64_t start{};
                  std::int64_t len{};
                };
                std::vector<Line> lines;
                lines.reserve(8);
                {
                  std::int64_t start = 0;
                  std::int64_t cur = 0;
                  for (std::size_t i = 0; i < s.size();) {
                    if (s[i] == '\n') {
                      lines.push_back(Line{start, cur - start});
                      ++cur;
                      ++i;
                      start = cur;
                      continue;
                    }
                    const auto b0 = static_cast<std::uint8_t>(s[i]);
                    std::size_t adv = 1;
                    if ((b0 & 0x80) == 0x00) {
                      adv = 1;
                    } else if ((b0 & 0xE0) == 0xC0) {
                      adv = 2;
                    } else if ((b0 & 0xF0) == 0xE0) {
                      adv = 3;
                    } else if ((b0 & 0xF8) == 0xF0) {
                      adv = 4;
                    }
                    if (i + adv > s.size()) {
                      adv = 1;
                    }
                    i += adv;
                    ++cur;
                  }
                  lines.push_back(Line{start, cur - start});
                }

                std::size_t line_idx = 0;
                for (std::size_t i = 0; i < lines.size(); ++i) {
                  if (c <= lines[i].start + lines[i].len) {
                    line_idx = i;
                    break;
                  }
                }
                const std::int64_t col = c - lines[line_idx].start;

                if (key_code() == KEY_LEFT) {
                  c = std::max<std::int64_t>(0, c - 1);
                } else if (key_code() == KEY_RIGHT) {
                  c = std::min<std::int64_t>(total_len, c + 1);
                } else if (key_code() == KEY_HOME) {
                  c = lines[line_idx].start;
                } else if (key_code() == KEY_END) {
                  c = lines[line_idx].start + lines[line_idx].len;
                } else if (key_code() == KEY_UP) {
                  if (line_idx > 0) {
                    const auto prev = lines[line_idx - 1];
                    c = prev.start + std::min(col, prev.len);
                  }
                } else if (key_code() == KEY_DOWN) {
                  if (line_idx + 1 < lines.size()) {
                    const auto next = lines[line_idx + 1];
                    c = next.start + std::min(col, next.len);
                  }
                } else if (key_code() == KEY_BACKSPACE) {
                  if (a != b) {
                    if (utf8_erase_range(s, c, a, b)) {
                      binding_set(binding, std::move(s));
                      a = c;
                      b = c;
                    }
                  } else {
                    utf8_erase_prev_char(s, c);
                    binding_set(binding, std::move(s));
                  }
                } else if (key_code() == KEY_DELETE) {
                  if (a != b) {
                    if (utf8_erase_range(s, c, a, b)) {
                      binding_set(binding, std::move(s));
                      a = c;
                      b = c;
                    }
                  } else {
                    utf8_erase_at_char(s, c);
                    binding_set(binding, std::move(s));
                  }
                } else if (key_code() == KEY_ENTER || key_code() == KEY_KP_ENTER) {
                  if (a != b) {
                    if (utf8_erase_range(s, c, a, b)) {
                      a = c;
                      b = c;
                    }
                  }
                  utf8_insert_at_char(s, c, "\n");
                  binding_set(binding, std::move(s));
                }

                caret.set(c);
                sel_anchor.set(c);
                sel_end.set(c);
              }));
        }
      }

      if (auto it = node.events.find("text_input");
          it == node.events.end() || it->second == 0) {
        node.events.insert_or_assign(
            "text_input",
            event_collector.add([binding, caret, sel_anchor, sel_end]() mutable {
              auto c = caret.get();
              auto a = sel_anchor.get();
              auto b = sel_end.get();
              auto s = binding_get(binding);
              if (a != b) {
                if (utf8_erase_range(s, c, a, b)) {
                  a = c;
                  b = c;
                }
              }
              utf8_insert_at_char(s, c, text_input());
              binding_set(binding, std::move(s));
              caret.set(c);
              sel_anchor.set(c);
              sel_end.set(c);
            }));
      }
    };

    apply_text_bindings(apply_text_bindings, new_tree);
    new_tree = normalize_root(flatten_groups(std::move(new_tree)));
    restore_scroll_offsets(new_tree);

    auto resolve_geometry_readers = [&](ViewNode &root) {
      for (int iter = 0; iter < 4; ++iter) {
        const auto layout0 = layout_tree(root, viewport_);
        bool changed = false;

        auto walk = [&](auto &&self, ViewNode &v, const LayoutNode &l) -> void {
          if (v.type == "GeometryReader") {
            if (auto raw = prop_as_i64_opt(v.props, "content_fn");
                raw && *raw != 0) {
              auto *fn = reinterpret_cast<std::function<ViewNode(SizeF)> *>(
                  static_cast<std::intptr_t>(*raw));
              const auto padding = prop_as_float(v.props, "padding", 0.0f);
              const SizeF size{
                  std::max(0.0f, l.frame.w - padding * 2.0f),
                  std::max(0.0f, l.frame.h - padding * 2.0f),
              };
              auto child = (*fn)(size);
              delete fn;
              v.props.erase("content_fn");
              v.children.clear();
              v.children.push_back(std::move(child));
              changed = true;
            }
          }

          const auto n = std::min(v.children.size(), l.children.size());
          for (std::size_t i = 0; i < n; ++i) {
            self(self, v.children[i], l.children[i]);
          }
        };

        walk(walk, root, layout0);
        if (!changed) {
          break;
        }
      }
    };

    resolve_geometry_readers(new_tree);

    new_tree = normalize_root(flatten_groups(std::move(new_tree)));
    restore_scroll_offsets(new_tree);
    apply_text_bindings(apply_text_bindings, new_tree);

    detail::active_collector = nullptr;
    detail::active_event_collector = nullptr;
    detail::active_build_instance = nullptr;

    auto patches = old_tree.type.empty() ? std::vector<PatchOp>{}
                                         : diff_tree(old_tree, new_tree);

    const double anim_start_ms = now_ms();
    std::vector<PropAnim> next_anims;
    std::optional<AnimationSpec> override_spec = pending_animation_;
    pending_animation_ = std::nullopt;

    auto spec_for = [&](const std::vector<std::size_t> &path)
        -> std::optional<AnimationSpec> {
      if (override_spec) {
        return override_spec;
      }
      return animation_spec_for_path(new_tree, path);
    };

    auto schedule_prop_anim = [&](const std::vector<std::size_t> &path,
                                  std::string prop_key, PropValue from,
                                  PropValue to, const AnimationSpec &spec) {
      if (auto *vn = node_at_path_mut(new_tree, path)) {
        vn->props.insert_or_assign(prop_key, from);
      }
      PropAnim a;
      a.path = path;
      a.prop_key = std::move(prop_key);
      a.from = std::move(from);
      a.to = std::move(to);
      a.start_ms = anim_start_ms;
      a.duration_ms = spec.duration_ms;
      a.delay_ms = spec.delay_ms;
      next_anims.push_back(std::move(a));
    };

    for (const auto &p : patches) {
      std::visit(
          [&](const auto &op) {
            using T = std::decay_t<decltype(op)>;
            if constexpr (std::is_same_v<T, PatchSetProp>) {
              if (!prop_is_animatable_key(op.key)) {
                return;
              }
              const auto *old_node = node_at_path(old_tree, op.path);
              if (!old_node) {
                return;
              }
              const auto *from_pv = find_prop(old_node->props, op.key);
              if (!from_pv) {
                return;
              }
              if (!prop_can_interpolate(op.key, *from_pv, op.value)) {
                return;
              }
              const auto s = spec_for(op.path);
              if (!s) {
                return;
              }
              schedule_prop_anim(op.path, op.key, *from_pv, op.value, *s);
            } else if constexpr (std::is_same_v<T, PatchInsertChild>) {
              auto child_path = op.parent_path;
              child_path.push_back(op.index);
              auto *vn = node_at_path_mut(new_tree, child_path);
              if (!vn) {
                return;
              }
              const auto tr = prop_as_string(vn->props, "transition", "");
              if (tr != "opacity") {
                return;
              }
              const auto s = spec_for(child_path);
              if (!s) {
                return;
              }
              const double to_op = static_cast<double>(
                  prop_as_float(vn->props, "opacity", 1.0f));
              schedule_prop_anim(child_path, "opacity", PropValue{0.0},
                                 PropValue{to_op}, *s);
            }
          },
          p);
    }

    anims_ = std::move(next_anims);
    tree_ = std::move(new_tree);

    handlers_ = std::move(event_collector.handlers);

    layout_ = layout_tree(tree_, viewport_);

    std::unordered_map<std::string, RectF> old_frames;
    {
      if (!old_tree.type.empty() && !old_layout.type.empty()) {
        auto collect_matched = [&](auto &&self, const ViewNode &v,
                                   const LayoutNode &l,
                                   std::unordered_map<std::string, RectF> &out)
            -> void {
          const auto ns = prop_as_string(v.props, "matched_geom_ns", "");
          const auto id = prop_as_string(v.props, "matched_geom_id", "");
          if (!ns.empty() && !id.empty()) {
            out.insert_or_assign(ns + "|" + id, l.frame);
          }
          const auto n = std::min(v.children.size(), l.children.size());
          for (std::size_t i = 0; i < n; ++i) {
            self(self, v.children[i], l.children[i], out);
          }
        };
        collect_matched(collect_matched, old_tree, old_layout, old_frames);
      }
    }

    auto apply_matched = [&](auto &&self, ViewNode &v, const LayoutNode &l,
                             std::vector<std::size_t> &path) -> void {
      const auto ns = prop_as_string(v.props, "matched_geom_ns", "");
      const auto id = prop_as_string(v.props, "matched_geom_id", "");
      if (!ns.empty() && !id.empty()) {
        const auto it_old = old_frames.find(ns + "|" + id);
        if (it_old != old_frames.end()) {
          const auto dx = static_cast<double>(it_old->second.x - l.frame.x);
          const auto dy = static_cast<double>(it_old->second.y - l.frame.y);
          if (dx != 0.0 || dy != 0.0) {
            const auto s = [&]() -> std::optional<AnimationSpec> {
              if (override_spec) {
                return override_spec;
              }
              return animation_spec_for_path(tree_, path);
            }();
            if (s) {
              const auto base_x =
                  static_cast<double>(prop_as_float(v.props, "render_offset_x", 0.0f));
              const auto base_y =
                  static_cast<double>(prop_as_float(v.props, "render_offset_y", 0.0f));

              const double from_x = base_x + dx;
              const double from_y = base_y + dy;
              v.props.insert_or_assign("render_offset_x", PropValue{from_x});
              v.props.insert_or_assign("render_offset_y", PropValue{from_y});

              PropAnim ax;
              ax.path = path;
              ax.prop_key = "render_offset_x";
              ax.from = PropValue{from_x};
              ax.to = PropValue{base_x};
              ax.start_ms = anim_start_ms;
              ax.duration_ms = s->duration_ms;
              ax.delay_ms = s->delay_ms;
              anims_.push_back(std::move(ax));

              PropAnim ay;
              ay.path = path;
              ay.prop_key = "render_offset_y";
              ay.from = PropValue{from_y};
              ay.to = PropValue{base_y};
              ay.start_ms = anim_start_ms;
              ay.duration_ms = s->duration_ms;
              ay.delay_ms = s->delay_ms;
              anims_.push_back(std::move(ay));
            }
          }
        }
      }

      const auto n = std::min(v.children.size(), l.children.size());
      for (std::size_t i = 0; i < n; ++i) {
        path.push_back(i);
        self(self, v.children[i], l.children[i], path);
        path.pop_back();
      }
    };

    {
      std::vector<std::size_t> path;
      apply_matched(apply_matched, tree_, layout_, path);
    }

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
  std::unordered_set<int> pointers_down_{};
  std::unordered_map<std::string, double> scroll_offsets_{};
  std::unordered_map<int, ScrollDrag> scroll_drags_{};
  std::optional<FocusTarget> focus_{};
  std::unordered_map<std::string, std::shared_ptr<StateBase>> local_states_{};
  std::unordered_map<std::string, PropValue> env_values_{};
  std::unordered_map<std::string, std::shared_ptr<void>> env_objects_{};
  std::string style_toml_cache_{};
  std::vector<detail::StyleRule> style_rules_cache_{};
  std::optional<AnimationSpec> pending_animation_{};
  std::vector<PropAnim> anims_{};
  std::unordered_map<std::string, TimelineReg> timelines_{};
  bool dirty_{true};
};

namespace detail {
inline void request_animation(const AnimationSpec &spec) {
  ViewInstance *inst = nullptr;
  if (active_dispatch_context) {
    inst = active_dispatch_context->instance;
  }
  if (!inst) {
    inst = active_build_instance;
  }
  if (!inst) {
    return;
  }
  inst->set_pending_animation(spec);
}
} // namespace detail

template <typename T> StateHandle<T> local_state(std::string key, T initial) {
  if (detail::active_build_instance) {
    return detail::active_build_instance->local_state<T>(std::move(key),
                                                         std::move(initial));
  }
  return state<T>(std::move(initial));
}

template <typename T> class ObservedObjectHandle {
public:
  explicit ObservedObjectHandle(std::shared_ptr<T> ptr) : ptr_{std::move(ptr)} {
    static_assert(std::is_base_of_v<StateBase, T>);
  }

  bool valid() const { return static_cast<bool>(ptr_); }

  std::shared_ptr<T> get() const {
    detail::record_dependency(ptr_.get());
    return ptr_;
  }

  T *operator->() const {
    detail::record_dependency(ptr_.get());
    return ptr_.get();
  }

  T &operator*() const {
    detail::record_dependency(ptr_.get());
    return *ptr_;
  }

private:
  std::shared_ptr<T> ptr_;
};

template <typename T>
inline ObservedObjectHandle<T> ObservedObject(std::shared_ptr<T> obj) {
  static_assert(std::is_base_of_v<StateBase, T>);
  return ObservedObjectHandle<T>{std::move(obj)};
}

template <typename T, typename... Args>
inline ObservedObjectHandle<T> StateObject(std::string key, Args &&...args) {
  static_assert(std::is_base_of_v<StateBase, T>);
  auto slot = local_state<std::shared_ptr<T>>(std::move(key),
                                              std::shared_ptr<T>{});
  auto obj = slot.get();
  if (!obj) {
    obj = std::make_shared<T>(std::forward<Args>(args)...);
    slot.set(obj);
  }
  return ObservedObjectHandle<T>{std::move(obj)};
}

template <typename T> inline StateHandle<T> FocusState(std::string key, T initial) {
  return local_state<T>(std::move(key), std::move(initial));
}

inline void provide_environment(std::string key, PropValue value) {
  if (!detail::active_build_instance) {
    return;
  }
  detail::active_build_instance->set_env_value(std::move(key), std::move(value));
}

inline void provide_environment(std::string key, const char *value) {
  provide_environment(std::move(key), PropValue{std::string{value}});
}

inline void provide_environment(std::string key, std::string value) {
  provide_environment(std::move(key), PropValue{std::move(value)});
}

inline void provide_environment(std::string key, std::int64_t value) {
  provide_environment(std::move(key), PropValue{value});
}

inline void provide_environment(std::string key, double value) {
  provide_environment(std::move(key), PropValue{value});
}

inline void provide_environment(std::string key, bool value) {
  provide_environment(std::move(key), PropValue{value});
}

template <typename Int,
          typename = std::enable_if_t<
              std::is_integral_v<std::remove_reference_t<Int>> &&
              !std::is_same_v<std::remove_reference_t<Int>, bool> &&
              !std::is_same_v<std::remove_reference_t<Int>, std::int64_t>>>
inline void provide_environment(std::string key, Int value) {
  provide_environment(std::move(key), static_cast<std::int64_t>(value));
}

template <typename T> inline T Environment(const std::string &key, T fallback) {
  if (!detail::active_build_instance) {
    return fallback;
  }
  const auto *pv = detail::active_build_instance->env_value(key);
  if (!pv) {
    return fallback;
  }

  if constexpr (std::is_same_v<T, PropValue>) {
    return *pv;
  } else if constexpr (std::is_same_v<T, std::string>) {
    if (const auto *s = std::get_if<std::string>(pv)) {
      return *s;
    }
    return fallback;
  } else if constexpr (std::is_same_v<T, bool>) {
    if (const auto *b = std::get_if<bool>(pv)) {
      return *b;
    }
    if (const auto *i = std::get_if<std::int64_t>(pv)) {
      return *i != 0;
    }
    if (const auto *d = std::get_if<double>(pv)) {
      return *d != 0.0;
    }
    return fallback;
  } else if constexpr (std::is_integral_v<T>) {
    if (const auto *i = std::get_if<std::int64_t>(pv)) {
      return static_cast<T>(*i);
    }
    if (const auto *d = std::get_if<double>(pv)) {
      return static_cast<T>(*d);
    }
    if (const auto *b = std::get_if<bool>(pv)) {
      return static_cast<T>(*b ? 1 : 0);
    }
    return fallback;
  } else if constexpr (std::is_floating_point_v<T>) {
    if (const auto *d = std::get_if<double>(pv)) {
      return static_cast<T>(*d);
    }
    if (const auto *i = std::get_if<std::int64_t>(pv)) {
      return static_cast<T>(*i);
    }
    if (const auto *b = std::get_if<bool>(pv)) {
      return static_cast<T>(*b ? 1 : 0);
    }
    return fallback;
  } else {
    return fallback;
  }
}

template <typename Key>
inline const std::string &StyleKeyName() {
  static const std::string s = std::string{Key::key};
  return s;
}

template <typename Key>
inline typename Key::Value StyleValue(typename Key::Value fallback = Key::default_value()) {
  return Environment(StyleKeyName<Key>(), std::move(fallback));
}

template <typename Key>
inline void provide_style(typename Key::Value value) {
  provide_environment(StyleKeyName<Key>(), std::move(value));
}

inline void provide_style_toml(std::string toml) {
  provide_environment("style.toml", std::move(toml));
}

inline void provide_style_toml(const char *toml) {
  provide_environment("style.toml", std::string{toml ? toml : ""});
}

inline std::optional<std::string> load_text_file(const std::string &path) {
  std::ifstream f(path, std::ios::in | std::ios::binary);
  if (!f) {
    return std::nullopt;
  }
  f.seekg(0, std::ios::end);
  const auto size = f.tellg();
  if (size <= 0) {
    return std::string{};
  }
  std::string out;
  out.resize(static_cast<std::size_t>(size));
  f.seekg(0, std::ios::beg);
  if (!f.read(out.data(), static_cast<std::streamsize>(out.size()))) {
    return std::nullopt;
  }
  return out;
}

inline bool provide_style_toml_file(const std::string &path) {
  auto s = load_text_file(path);
  if (!s) {
    return false;
  }
  provide_style_toml(std::move(*s));
  return true;
}

inline ViewNode style_class(ViewNode node, std::string cls) {
  auto cur = prop_as_string(node.props, "class", "");
  if (cur.empty()) {
    node.props.insert_or_assign("class", PropValue{std::move(cls)});
    return node;
  }
  cur.push_back(' ');
  cur += cls;
  node.props.insert_or_assign("class", PropValue{std::move(cur)});
  return node;
}

template <typename T>
inline void provide_environment_object(std::string key, std::shared_ptr<T> obj) {
  if (!detail::active_build_instance) {
    return;
  }
  detail::active_build_instance->set_env_object(std::move(key),
                                                std::static_pointer_cast<void>(
                                                    std::move(obj)));
}

template <typename T>
inline ObservedObjectHandle<T> EnvironmentObject(const std::string &key) {
  static_assert(std::is_base_of_v<StateBase, T>);
  if (!detail::active_build_instance) {
    return ObservedObjectHandle<T>{std::shared_ptr<T>{}};
  }
  auto p = detail::active_build_instance->env_object(key);
  return ObservedObjectHandle<T>{std::static_pointer_cast<T>(std::move(p))};
}

template <typename Fn>
inline std::uint64_t chain_handler(std::uint64_t prev_handler_id, Fn fn);

inline ViewNode focusable(ViewNode node, StateHandle<std::string> focus,
                          std::string id) {
  if (node.key.empty() && !id.empty()) {
    node.key = id;
  }
  const std::string focus_id = node.key.empty() ? std::move(id) : node.key;

  const auto prev_focus = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("focus"); it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();
  const auto prev_blur = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("blur"); it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();

  node.props.insert_or_assign("focused", focus.get() == focus_id);

  node.events.insert_or_assign(
      "focus", chain_handler(prev_focus, [focus, focus_id]() mutable {
        focus.set(focus_id);
      }));

  node.events.insert_or_assign(
      "blur", chain_handler(prev_blur, [focus, focus_id]() mutable {
        if (focus.get() == focus_id) {
          focus.set("");
        }
      }));

  return node;
}

inline bool open_url(const std::string &url) {
  if (url.empty()) {
    return false;
  }
#if defined(_WIN32)
  const auto r = reinterpret_cast<std::intptr_t>(
      ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
  return r > 32;
#elif defined(__APPLE__)
  std::string cmd = "open \"";
  for (const char ch : url) {
    if (ch == '\"') {
      cmd.push_back('\\');
    }
    cmd.push_back(ch);
  }
  cmd.push_back('\"');
  return std::system(cmd.c_str()) == 0;
#else
  std::string cmd = "xdg-open \"";
  for (const char ch : url) {
    if (ch == '\"') {
      cmd.push_back('\\');
    }
    cmd.push_back(ch);
  }
  cmd.push_back('\"');
  return std::system(cmd.c_str()) == 0;
#endif
}

inline bool set_clipboard_text(std::string text) {
  if (text.empty()) {
    return false;
  }
#if defined(_WIN32)
  if (!OpenClipboard(nullptr)) {
    return false;
  }
  struct Close {
    ~Close() { CloseClipboard(); }
  } close;

  if (!EmptyClipboard()) {
    return false;
  }

  const int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
  if (wlen <= 0) {
    return false;
  }

  const std::size_t bytes = static_cast<std::size_t>(wlen) * sizeof(wchar_t);
  HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
  if (!mem) {
    return false;
  }

  auto *buf = static_cast<wchar_t *>(GlobalLock(mem));
  if (!buf) {
    GlobalFree(mem);
    return false;
  }
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, buf, wlen);
  GlobalUnlock(mem);

  if (!SetClipboardData(CF_UNICODETEXT, mem)) {
    GlobalFree(mem);
    return false;
  }
  return true;
#else
  return false;
#endif
}

inline std::optional<std::string> open_file_dialog(std::string title,
                                                   bool images_only = false) {
#if defined(_WIN32)
  auto to_wide = [](const std::string &s) -> std::wstring {
    if (s.empty()) {
      return {};
    }
    const int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (wlen <= 0) {
      return {};
    }
    std::wstring out;
    out.resize(static_cast<std::size_t>(wlen - 1));
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), wlen);
    return out;
  };

  auto to_utf8 = [](const wchar_t *ws) -> std::string {
    if (!ws || *ws == 0) {
      return {};
    }
    const int len =
        WideCharToMultiByte(CP_UTF8, 0, ws, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) {
      return {};
    }
    std::string out;
    out.resize(static_cast<std::size_t>(len - 1));
    WideCharToMultiByte(CP_UTF8, 0, ws, -1, out.data(), len, nullptr, nullptr);
    return out;
  };

  std::wstring filter;
  if (images_only) {
    filter.append(L"Images");
    filter.push_back(L'\0');
    filter.append(L"*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.webp");
    filter.push_back(L'\0');
    filter.append(L"All Files");
    filter.push_back(L'\0');
    filter.append(L"*.*");
    filter.push_back(L'\0');
    filter.push_back(L'\0');
  } else {
    filter.append(L"All Files");
    filter.push_back(L'\0');
    filter.append(L"*.*");
    filter.push_back(L'\0');
    filter.push_back(L'\0');
  }

  std::wstring wtitle = to_wide(title);

  wchar_t file_buf[4096]{};
  OPENFILENAMEW ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = nullptr;
  ofn.lpstrFile = file_buf;
  ofn.nMaxFile = static_cast<DWORD>(std::size(file_buf));
  ofn.lpstrTitle = wtitle.empty() ? nullptr : wtitle.c_str();
  ofn.lpstrFilter = filter.c_str();
  ofn.nFilterIndex = 1;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

  if (!GetOpenFileNameW(&ofn)) {
    return std::nullopt;
  }

  auto out = to_utf8(file_buf);
  if (out.empty()) {
    return std::nullopt;
  }
  return out;
#else
  (void)title;
  (void)images_only;
  return std::nullopt;
#endif
}

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

inline void call_handler(std::uint64_t handler_id) {
  if (!detail::active_dispatch_context ||
      !detail::active_dispatch_context->instance) {
    return;
  }
  detail::active_dispatch_context->instance->invoke_handler(handler_id);
}

inline double now_ms() {
  using clock = std::chrono::steady_clock;
  static const auto t0 = clock::now();
  const auto dt = clock::now() - t0;
  return std::chrono::duration<double, std::milli>(dt).count();
}

template <typename Fn>
inline void withAnimation(AnimationSpec spec, Fn &&fn) {
  const auto prev = detail::active_animation_spec;
  detail::active_animation_spec = std::move(spec);
  fn();
  detail::active_animation_spec = prev;
}

template <typename Fn> inline void withAnimation(Fn &&fn) {
  withAnimation(AnimationSpec{}, std::forward<Fn>(fn));
}

inline ViewNode animation(ViewNode node, AnimationSpec spec) {
  node.props.insert_or_assign("animation_enabled", PropValue{true});
  node.props.insert_or_assign("animation_duration_ms", PropValue{spec.duration_ms});
  node.props.insert_or_assign("animation_delay_ms", PropValue{spec.delay_ms});
  node.props.insert_or_assign("animation_curve", PropValue{std::move(spec.curve)});
  return node;
}

inline ViewNode animation(ViewNode node, bool enabled) {
  node.props.insert_or_assign("animation_enabled", PropValue{enabled});
  if (!enabled) {
    node.props.erase("animation_duration_ms");
    node.props.erase("animation_delay_ms");
    node.props.erase("animation_curve");
  }
  return node;
}

template <typename ContentFn>
inline ViewNode TimelineView(std::string key, double interval_ms, ContentFn fn) {
  if (detail::active_build_instance) {
    detail::active_build_instance->register_timeline(key, interval_ms);
  }
  auto now = local_state<double>(key + ":timeline_now", now_ms());
  return fn(now.get());
}

inline ViewNode matchedGeometryEffect(ViewNode node, std::string ns, std::string id) {
  node.props.insert_or_assign("matched_geom_ns", PropValue{std::move(ns)});
  node.props.insert_or_assign("matched_geom_id", PropValue{std::move(id)});
  return node;
}

inline ViewNode Transition(ViewNode node, std::string type = "opacity") {
  node.props.insert_or_assign("transition", PropValue{std::move(type)});
  return node;
}

inline std::optional<RectF> target_frame() {
  if (!detail::active_dispatch_context ||
      !detail::active_dispatch_context->instance) {
    return std::nullopt;
  }
  return detail::active_dispatch_context->instance->layout_frame_at_path(
      detail::active_dispatch_context->target_path);
}

template <typename Fn>
inline std::uint64_t chain_handler(std::uint64_t prev_handler_id, Fn fn) {
  return on_click([prev_handler_id, fn = std::move(fn)]() mutable {
    if (prev_handler_id != 0) {
      call_handler(prev_handler_id);
    }
    fn();
  });
}

inline ViewNode onTapGesture(ViewNode node, std::function<void()> fn) {
  const auto prev = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("pointer_up");
        it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();
  node.events.insert_or_assign(
      "pointer_up",
      chain_handler(prev, [fn = std::move(fn)]() mutable {
        if (fn) {
          fn();
        }
      }));
  return node;
}

struct DragGestureValue {
  float start_x{};
  float start_y{};
  float x{};
  float y{};
  float dx{};
  float dy{};
};

inline ViewNode DragGesture(ViewNode node, std::string key,
                            std::function<void(DragGestureValue)> on_changed = {},
                            std::function<void(DragGestureValue)> on_ended = {},
                            float min_distance = 0.0f) {
  if (node.key.empty() && !key.empty()) {
    node.key = key;
  }
  const std::string state_key = node.key.empty() ? std::move(key) : node.key;

  auto active = local_state<bool>(state_key + ":drag:active", false);
  auto started = local_state<bool>(state_key + ":drag:started", false);
  auto start_x = local_state<double>(state_key + ":drag:start_x", 0.0);
  auto start_y = local_state<double>(state_key + ":drag:start_y", 0.0);

  const auto prev_down = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("pointer_down");
        it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();
  const auto prev_move = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("pointer_move");
        it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();
  const auto prev_up = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("pointer_up");
        it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();

  node.events.insert_or_assign(
      "pointer_down",
      chain_handler(prev_down, [active, started, start_x, start_y]() mutable {
        active.set(true);
        started.set(false);
        start_x.set(pointer_x());
        start_y.set(pointer_y());
        capture_pointer();
      }));

  node.events.insert_or_assign(
      "pointer_move",
      chain_handler(prev_move, [active, started, start_x, start_y, min_distance,
                               on_changed]() mutable {
        if (!active.get()) {
          return;
        }
        const float sx = static_cast<float>(start_x.get());
        const float sy = static_cast<float>(start_y.get());
        const float x = pointer_x();
        const float y = pointer_y();
        const float dx = x - sx;
        const float dy = y - sy;

        if (!started.get()) {
          if (std::sqrt(dx * dx + dy * dy) < min_distance) {
            return;
          }
          started.set(true);
        }

        if (on_changed) {
          on_changed(DragGestureValue{sx, sy, x, y, dx, dy});
        }
      }));

  node.events.insert_or_assign(
      "pointer_up",
      chain_handler(prev_up, [active, started, start_x, start_y, on_ended]() mutable {
        if (!active.get()) {
          return;
        }
        active.set(false);
        const float sx = static_cast<float>(start_x.get());
        const float sy = static_cast<float>(start_y.get());
        const float x = pointer_x();
        const float y = pointer_y();
        const float dx = x - sx;
        const float dy = y - sy;
        if (started.get() && on_ended) {
          on_ended(DragGestureValue{sx, sy, x, y, dx, dy});
        }
        started.set(false);
        release_pointer();
      }));

  return node;
}

inline ViewNode onLongPressGesture(ViewNode node, std::string key,
                                  std::function<void()> fn,
                                  double minimum_duration_ms = 500.0,
                                  float maximum_distance = 6.0f) {
  if (node.key.empty() && !key.empty()) {
    node.key = key;
  }
  const std::string state_key = node.key.empty() ? std::move(key) : node.key;

  auto pressed = local_state<bool>(state_key + ":lp:pressed", false);
  auto start_t = local_state<double>(state_key + ":lp:start_t", 0.0);
  auto start_x = local_state<double>(state_key + ":lp:start_x", 0.0);
  auto start_y = local_state<double>(state_key + ":lp:start_y", 0.0);

  const auto prev_down = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("pointer_down");
        it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();
  const auto prev_move = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("pointer_move");
        it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();
  const auto prev_up = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("pointer_up");
        it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();

  node.events.insert_or_assign(
      "pointer_down",
      chain_handler(prev_down, [pressed, start_t, start_x, start_y]() mutable {
        pressed.set(true);
        start_t.set(now_ms());
        start_x.set(pointer_x());
        start_y.set(pointer_y());
        capture_pointer();
      }));

  node.events.insert_or_assign(
      "pointer_move",
      chain_handler(prev_move, [pressed, start_x, start_y, maximum_distance]() mutable {
        if (!pressed.get()) {
          return;
        }
        const float dx = pointer_x() - static_cast<float>(start_x.get());
        const float dy = pointer_y() - static_cast<float>(start_y.get());
        if (std::sqrt(dx * dx + dy * dy) > maximum_distance) {
          pressed.set(false);
          release_pointer();
        }
      }));

  node.events.insert_or_assign(
      "pointer_up",
      chain_handler(prev_up, [pressed, start_t, minimum_duration_ms, fn = std::move(fn)]() mutable {
        if (pressed.get()) {
          const double dt = now_ms() - start_t.get();
          if (dt >= minimum_duration_ms) {
            if (fn) {
              fn();
            }
          }
        }
        pressed.set(false);
        release_pointer();
      }));

  return node;
}

struct MagnificationGestureValue {
  double magnification{1.0};
  double delta{0.0};
};

inline ViewNode MagnificationGesture(
    ViewNode node, std::string key,
    std::function<void(MagnificationGestureValue)> on_changed = {},
    std::function<void(MagnificationGestureValue)> on_ended = {},
    float sensitivity = 240.0f) {
  if (node.key.empty() && !key.empty()) {
    node.key = key;
  }
  const std::string state_key = node.key.empty() ? std::move(key) : node.key;

  auto active = local_state<bool>(state_key + ":mag:active", false);
  auto start_y = local_state<double>(state_key + ":mag:start_y", 0.0);
  auto last = local_state<double>(state_key + ":mag:last", 1.0);

  const auto prev_down = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("pointer_down");
        it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();
  const auto prev_move = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("pointer_move");
        it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();
  const auto prev_up = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("pointer_up");
        it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();

  node.events.insert_or_assign(
      "pointer_down",
      chain_handler(prev_down, [active, start_y, last]() mutable {
        active.set(true);
        start_y.set(pointer_y());
        last.set(1.0);
        capture_pointer();
      }));

  node.events.insert_or_assign(
      "pointer_move",
      chain_handler(prev_move, [active, start_y, last, sensitivity, on_changed]() mutable {
        if (!active.get()) {
          return;
        }
        const double dy = static_cast<double>(pointer_y()) - start_y.get();
        const double m = std::exp(sensitivity != 0.0f ? (dy / static_cast<double>(sensitivity)) : 0.0);
        const double d = m - last.get();
        last.set(m);
        if (on_changed) {
          on_changed(MagnificationGestureValue{m, d});
        }
      }));

  node.events.insert_or_assign(
      "pointer_up",
      chain_handler(prev_up, [active, last, on_ended]() mutable {
        if (!active.get()) {
          return;
        }
        active.set(false);
        if (on_ended) {
          on_ended(MagnificationGestureValue{last.get(), 0.0});
        }
        last.set(1.0);
        release_pointer();
      }));

  return node;
}

struct RotationGestureValue {
  double radians{0.0};
  double delta{0.0};
};

inline ViewNode RotationGesture(
    ViewNode node, std::string key,
    std::function<void(RotationGestureValue)> on_changed = {},
    std::function<void(RotationGestureValue)> on_ended = {},
    float sensitivity = 240.0f) {
  if (node.key.empty() && !key.empty()) {
    node.key = key;
  }
  const std::string state_key = node.key.empty() ? std::move(key) : node.key;

  auto active = local_state<bool>(state_key + ":rot:active", false);
  auto start_x = local_state<double>(state_key + ":rot:start_x", 0.0);
  auto last = local_state<double>(state_key + ":rot:last", 0.0);

  const auto prev_down = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("pointer_down");
        it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();
  const auto prev_move = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("pointer_move");
        it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();
  const auto prev_up = [&]() -> std::uint64_t {
    if (const auto it = node.events.find("pointer_up");
        it != node.events.end()) {
      return it->second;
    }
    return 0;
  }();

  node.events.insert_or_assign(
      "pointer_down",
      chain_handler(prev_down, [active, start_x, last]() mutable {
        active.set(true);
        start_x.set(pointer_x());
        last.set(0.0);
        capture_pointer();
      }));

  node.events.insert_or_assign(
      "pointer_move",
      chain_handler(prev_move, [active, start_x, last, sensitivity, on_changed]() mutable {
        if (!active.get()) {
          return;
        }
        const double dx = static_cast<double>(pointer_x()) - start_x.get();
        const double r = sensitivity != 0.0f ? (dx / static_cast<double>(sensitivity)) : 0.0;
        const double d = r - last.get();
        last.set(r);
        if (on_changed) {
          on_changed(RotationGestureValue{r, d});
        }
      }));

  node.events.insert_or_assign(
      "pointer_up",
      chain_handler(prev_up, [active, last, on_ended]() mutable {
        if (!active.get()) {
          return;
        }
        active.set(false);
        if (on_ended) {
          on_ended(RotationGestureValue{last.get(), 0.0});
        }
        last.set(0.0);
        release_pointer();
      }));

  return node;
}

template <typename ApplyGestureFn>
inline ViewNode gesture(ViewNode node, ApplyGestureFn fn) {
  return fn(std::move(node));
}

inline ViewNode Link(std::string title, std::string url) {
  auto b = view("Text");
  b.prop("value", std::move(title));
  b.prop("color", 0xFF80A0FF);
  b.event("pointer_up", on_pointer_up([url = std::move(url)]() mutable {
            open_url(url);
          }));
  return std::move(b).build();
}

inline ViewNode WebView(std::string url, double default_width = 520.0,
                        double default_height = 320.0) {
#if !defined(DUOROU_ENABLE_WEBVIEW) || !DUOROU_ENABLE_WEBVIEW
  const std::string key = std::string{"webview_disabled:"} + url;
  auto node = Canvas(
      key,
      [url](RectF frame, std::vector<RenderOp> &out) {
        out.push_back(DrawRect{frame, ColorU8{28, 28, 30, 255}});
        const RectF title_r{frame.x + 10.0f, frame.y + 10.0f,
                            std::max(0.0f, frame.w - 20.0f), 22.0f};
        out.push_back(
            DrawText{title_r, std::string{"WebView disabled"},
                     ColorU8{220, 220, 220, 255}, 14.0f, 0.0f, 0.0f});
        const RectF url_r{frame.x + 10.0f, frame.y + 34.0f,
                          std::max(0.0f, frame.w - 20.0f),
                          std::max(0.0f, frame.h - 44.0f)};
        out.push_back(DrawText{url_r, url, ColorU8{140, 180, 255, 255}, 12.0f,
                               0.0f, 0.0f});
      },
      default_width, default_height);
  return node;
#else
  const std::string key = std::string{"webview:"} + url;
  auto node = Canvas(
      key,
      [url](RectF frame, std::vector<RenderOp> &out) {
        out.push_back(DrawRect{frame, ColorU8{28, 28, 30, 255}});
        const RectF title_r{frame.x + 10.0f, frame.y + 10.0f,
                            std::max(0.0f, frame.w - 20.0f), 22.0f};
        out.push_back(
            DrawText{title_r, std::string{"WebView (placeholder)"},
                     ColorU8{220, 220, 220, 255}, 14.0f, 0.0f, 0.0f});
        const RectF url_r{frame.x + 10.0f, frame.y + 34.0f,
                          std::max(0.0f, frame.w - 20.0f),
                          std::max(0.0f, frame.h - 44.0f)};
        out.push_back(DrawText{url_r, url, ColorU8{140, 180, 255, 255}, 12.0f,
                               0.0f, 0.0f});
      },
      default_width, default_height);
  node = onTapGesture(std::move(node), [url = std::move(url)]() mutable {
    open_url(url);
  });
  return node;
#endif
}

inline ViewNode VideoPlayer(std::string source, double default_width = 520.0,
                            double default_height = 320.0) {
#if !defined(DUOROU_ENABLE_FFMPEG) || !DUOROU_ENABLE_FFMPEG
  const std::string key = std::string{"videoplayer_disabled:"} + source;
  auto node = Canvas(
      key,
      [source](RectF frame, std::vector<RenderOp> &out) {
        out.push_back(DrawRect{frame, ColorU8{20, 20, 22, 255}});
        const RectF title_r{frame.x + 10.0f, frame.y + 10.0f,
                            std::max(0.0f, frame.w - 20.0f), 22.0f};
        out.push_back(
            DrawText{title_r, std::string{"VideoPlayer disabled"},
                     ColorU8{220, 220, 220, 255}, 14.0f, 0.0f, 0.0f});
        const RectF url_r{frame.x + 10.0f, frame.y + 34.0f,
                          std::max(0.0f, frame.w - 20.0f),
                          std::max(0.0f, frame.h - 44.0f)};
        out.push_back(DrawText{url_r, source, ColorU8{200, 200, 200, 255}, 12.0f,
                               0.0f, 0.0f});
      },
      default_width, default_height);
  return node;
#else
  const std::string key = std::string{"videoplayer:"} + source;
  auto node = Canvas(
      key,
      [source](RectF frame, std::vector<RenderOp> &out) {
        out.push_back(DrawRect{frame, ColorU8{20, 20, 22, 255}});
        const RectF title_r{frame.x + 10.0f, frame.y + 10.0f,
                            std::max(0.0f, frame.w - 20.0f), 22.0f};
        out.push_back(
            DrawText{title_r, std::string{"VideoPlayer (placeholder)"},
                     ColorU8{220, 220, 220, 255}, 14.0f, 0.0f, 0.0f});
        const RectF url_r{frame.x + 10.0f, frame.y + 34.0f,
                          std::max(0.0f, frame.w - 20.0f),
                          std::max(0.0f, frame.h - 44.0f)};
        out.push_back(DrawText{url_r, source, ColorU8{200, 200, 200, 255}, 12.0f,
                               0.0f, 0.0f});
      },
      default_width, default_height);
  return node;
#endif
}

inline ViewNode ShareLink(std::string title, std::string url,
                          bool open_after_copy = false) {
  auto b = view("Text");
  b.prop("value", std::move(title));
  b.prop("color", 0xFF80A0FF);
  b.event("pointer_up",
          on_pointer_up([url = std::move(url), open_after_copy]() mutable {
            set_clipboard_text(url);
            if (open_after_copy) {
              open_url(url);
            }
          }));
  return std::move(b).build();
}

inline ViewNode PhotosPicker(StateHandle<std::string> selection,
                             std::string title = "Pick Photo") {
  auto b = view("Text");
  b.prop("value", std::move(title));
  b.prop("color", 0xFF80A0FF);
  b.event("pointer_up", on_pointer_up([selection]() mutable {
            if (auto p = open_file_dialog("Pick Photo", true)) {
              selection.set(*p);
            }
          }));
  return std::move(b).build();
}

inline ViewNode TextField(StateHandle<std::string> value, std::string key,
                          std::string placeholder = {}) {
  auto focused = local_state<bool>(key + ":focused", false);
  auto caret = local_state<std::int64_t>(key + ":caret", 0);

  auto b = view("TextField");
  b.key(key);
  b.prop("value", value.get());
  b.prop("caret", caret.get());
  b.prop("focused", focused.get());
  if (!placeholder.empty()) {
    b.prop("placeholder", std::move(placeholder));
  }

  b.event("focus",
          on_focus([focused, value, caret]() mutable {
            focused.set(true);
            caret.set(utf8_count(value.get()));
          }));
  b.event("blur", on_blur([focused]() mutable { focused.set(false); }));

  b.event("pointer_down", on_pointer_down([value, caret]() mutable {
            auto r = target_frame();
            if (!r) {
              return;
            }
            const float padding = 10.0f;
            const float font_px = 16.0f;
            const float char_w = font_px * 0.5f;
            const float local_x = pointer_x() - (r->x + padding);
            const auto len = utf8_count(value.get());
            auto pos = static_cast<std::int64_t>(
                std::round(char_w > 0.0f ? (local_x / char_w) : 0.0f));
            pos = std::max<std::int64_t>(0, std::min(pos, len));
            caret.set(pos);
          }));

  b.event("key_down", on_key_down([value, caret]() mutable {
            auto c = caret.get();
            auto s = value.get();
            const auto len = utf8_count(s);
            c = std::max<std::int64_t>(0, std::min(c, len));

            if (key_code() == KEY_LEFT) {
              c = std::max<std::int64_t>(0, c - 1);
            } else if (key_code() == KEY_RIGHT) {
              c = std::min<std::int64_t>(len, c + 1);
            } else if (key_code() == KEY_HOME) {
              c = 0;
            } else if (key_code() == KEY_END) {
              c = len;
            } else if (key_code() == KEY_BACKSPACE) {
              utf8_erase_prev_char(s, c);
              value.set(std::move(s));
            } else if (key_code() == KEY_DELETE) {
              utf8_erase_at_char(s, c);
              value.set(std::move(s));
            }

            caret.set(c);
          }));

  b.event("text_input", on_text_input([value, caret]() mutable {
            auto c = caret.get();
            auto s = value.get();
            utf8_insert_at_char(s, c, text_input());
            value.set(std::move(s));
            caret.set(c);
          }));

  return std::move(b).build();
}

inline ViewNode SecureField(StateHandle<std::string> value, std::string key,
                            std::string placeholder = {}) {
  auto b = view("TextField");
  b.key(key);
  b.prop("secure", true);
  b.prop("value", value.get());
  if (!placeholder.empty()) {
    b.prop("placeholder", std::move(placeholder));
  }

  auto focused = local_state<bool>(key + ":focused", false);
  auto caret = local_state<std::int64_t>(key + ":caret", 0);
  b.prop("caret", caret.get());
  b.prop("focused", focused.get());

  b.event("focus",
          on_focus([focused, value, caret]() mutable {
            focused.set(true);
            caret.set(utf8_count(value.get()));
          }));
  b.event("blur", on_blur([focused]() mutable { focused.set(false); }));

  b.event("pointer_down", on_pointer_down([value, caret]() mutable {
            auto r = target_frame();
            if (!r) {
              return;
            }
            const float padding = 10.0f;
            const float font_px = 16.0f;
            const float char_w = font_px * 0.5f;
            const float local_x = pointer_x() - (r->x + padding);
            const auto len = utf8_count(value.get());
            auto pos = static_cast<std::int64_t>(
                std::round(char_w > 0.0f ? (local_x / char_w) : 0.0f));
            pos = std::max<std::int64_t>(0, std::min(pos, len));
            caret.set(pos);
          }));

  b.event("key_down", on_key_down([value, caret]() mutable {
            auto c = caret.get();
            auto s = value.get();
            const auto len = utf8_count(s);
            c = std::max<std::int64_t>(0, std::min(c, len));

            if (key_code() == KEY_LEFT) {
              c = std::max<std::int64_t>(0, c - 1);
            } else if (key_code() == KEY_RIGHT) {
              c = std::min<std::int64_t>(len, c + 1);
            } else if (key_code() == KEY_HOME) {
              c = 0;
            } else if (key_code() == KEY_END) {
              c = len;
            } else if (key_code() == KEY_BACKSPACE) {
              utf8_erase_prev_char(s, c);
              value.set(std::move(s));
            } else if (key_code() == KEY_DELETE) {
              utf8_erase_at_char(s, c);
              value.set(std::move(s));
            }

            caret.set(c);
          }));

  b.event("text_input", on_text_input([value, caret]() mutable {
            auto c = caret.get();
            auto s = value.get();
            utf8_insert_at_char(s, c, text_input());
            value.set(std::move(s));
            caret.set(c);
          }));

  return std::move(b).build();
}

inline ViewNode TextEditor(StateHandle<std::string> value, std::string key) {
  auto focused = local_state<bool>(key + ":focused", false);
  auto caret = local_state<std::int64_t>(key + ":caret", 0);

  auto b = view("TextEditor");
  b.key(key);
  b.prop("value", value.get());
  b.prop("caret", caret.get());
  b.prop("focused", focused.get());

  b.event("focus",
          on_focus([focused, value, caret]() mutable {
            focused.set(true);
            caret.set(utf8_count(value.get()));
          }));
  b.event("blur", on_blur([focused]() mutable { focused.set(false); }));

  b.event("pointer_down", on_pointer_down([value, caret]() mutable {
            auto r = target_frame();
            if (!r) {
              return;
            }
            const float padding = 10.0f;
            const float font_px = 16.0f;
            const float char_w = font_px * 0.5f;
            const float line_h = font_px * 1.2f;

            const float local_x = pointer_x() - (r->x + padding);
            const float local_y = pointer_y() - (r->y + padding);
            const std::int64_t col = static_cast<std::int64_t>(
                std::max(0.0f, std::round(char_w > 0.0f ? (local_x / char_w)
                                                       : 0.0f)));
            const std::int64_t row = static_cast<std::int64_t>(
                std::max(0.0f, std::floor(line_h > 0.0f ? (local_y / line_h)
                                                       : 0.0f)));

            auto s = value.get();
            struct Line {
              std::int64_t start{};
              std::int64_t len{};
            };
            std::vector<Line> lines;
            lines.reserve(8);
            {
              std::int64_t start = 0;
              std::int64_t cur = 0;
              for (std::size_t i = 0; i < s.size();) {
                if (s[i] == '\n') {
                  lines.push_back(Line{start, cur - start});
                  ++cur;
                  ++i;
                  start = cur;
                  continue;
                }
                const auto b0 = static_cast<std::uint8_t>(s[i]);
                std::size_t adv = 1;
                if ((b0 & 0x80) == 0x00) {
                  adv = 1;
                } else if ((b0 & 0xE0) == 0xC0) {
                  adv = 2;
                } else if ((b0 & 0xF0) == 0xE0) {
                  adv = 3;
                } else if ((b0 & 0xF8) == 0xF0) {
                  adv = 4;
                }
                if (i + adv > s.size()) {
                  adv = 1;
                }
                i += adv;
                ++cur;
              }
              lines.push_back(Line{start, cur - start});
            }

            const auto total_len = utf8_count(s);
            const auto rrow = std::max<std::int64_t>(
                0, std::min<std::int64_t>(row,
                                          static_cast<std::int64_t>(lines.size()) -
                                              1));
            const auto line_start = lines[static_cast<std::size_t>(rrow)].start;
            const auto line_len = lines[static_cast<std::size_t>(rrow)].len;
            const auto next = line_start + std::min(col, line_len);
            caret.set(std::max<std::int64_t>(0, std::min(next, total_len)));
          }));

  b.event("key_down", on_key_down([value, caret]() mutable {
            auto c = caret.get();
            auto s = value.get();
            const auto total_len = utf8_count(s);
            c = std::max<std::int64_t>(0, std::min(c, total_len));

            struct Line {
              std::int64_t start{};
              std::int64_t len{};
            };
            std::vector<Line> lines;
            lines.reserve(8);
            {
              std::int64_t start = 0;
              std::int64_t cur = 0;
              for (std::size_t i = 0; i < s.size();) {
                if (s[i] == '\n') {
                  lines.push_back(Line{start, cur - start});
                  ++cur;
                  ++i;
                  start = cur;
                  continue;
                }
                const auto b0 = static_cast<std::uint8_t>(s[i]);
                std::size_t adv = 1;
                if ((b0 & 0x80) == 0x00) {
                  adv = 1;
                } else if ((b0 & 0xE0) == 0xC0) {
                  adv = 2;
                } else if ((b0 & 0xF0) == 0xE0) {
                  adv = 3;
                } else if ((b0 & 0xF8) == 0xF0) {
                  adv = 4;
                }
                if (i + adv > s.size()) {
                  adv = 1;
                }
                i += adv;
                ++cur;
              }
              lines.push_back(Line{start, cur - start});
            }

            std::size_t line_idx = 0;
            for (std::size_t i = 0; i < lines.size(); ++i) {
              if (c <= lines[i].start + lines[i].len) {
                line_idx = i;
                break;
              }
            }
            const std::int64_t col = c - lines[line_idx].start;

            if (key_code() == KEY_LEFT) {
              c = std::max<std::int64_t>(0, c - 1);
            } else if (key_code() == KEY_RIGHT) {
              c = std::min<std::int64_t>(total_len, c + 1);
            } else if (key_code() == KEY_HOME) {
              c = lines[line_idx].start;
            } else if (key_code() == KEY_END) {
              c = lines[line_idx].start + lines[line_idx].len;
            } else if (key_code() == KEY_UP) {
              if (line_idx > 0) {
                const auto prev = lines[line_idx - 1];
                c = prev.start + std::min(col, prev.len);
              }
            } else if (key_code() == KEY_DOWN) {
              if (line_idx + 1 < lines.size()) {
                const auto next = lines[line_idx + 1];
                c = next.start + std::min(col, next.len);
              }
            } else if (key_code() == KEY_BACKSPACE) {
              utf8_erase_prev_char(s, c);
              value.set(std::move(s));
            } else if (key_code() == KEY_DELETE) {
              utf8_erase_at_char(s, c);
              value.set(std::move(s));
            } else if (key_code() == KEY_ENTER || key_code() == KEY_KP_ENTER) {
              utf8_insert_at_char(s, c, "\n");
              value.set(std::move(s));
            }

            caret.set(c);
          }));

  b.event("text_input", on_text_input([value, caret]() mutable {
            auto c = caret.get();
            auto s = value.get();
            utf8_insert_at_char(s, c, text_input());
            value.set(std::move(s));
            caret.set(c);
          }));

  return std::move(b).build();
}

} // namespace duorou::ui
