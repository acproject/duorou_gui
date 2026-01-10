#pragma once

#include <atomic>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace duorou::ui {

using NodeId = std::uint64_t;

using PropValue = std::variant<std::string, std::int64_t, double, bool>;

using Props = std::unordered_map<std::string, PropValue>;

struct ViewNode {
  NodeId id{};
  std::string key;
  std::string type;
  Props props;
  std::unordered_map<std::string, std::uint64_t> events;
  std::vector<ViewNode> children;
};

namespace detail {
inline std::atomic<NodeId> next_node_id{1};
} // namespace detail

class ViewBuilder {
public:
  explicit ViewBuilder(std::string type)
      : node_{allocate_id(), {}, std::move(type), {}, {}, {}} {}

  ViewBuilder &key(std::string k) {
    node_.key = std::move(k);
    return *this;
  }

  ViewBuilder &prop(std::string key, std::string value) {
    node_.props.insert_or_assign(std::move(key), PropValue{std::move(value)});
    return *this;
  }

  ViewBuilder &prop(std::string key, const char *value) {
    return prop(std::move(key), std::string{value});
  }

  ViewBuilder &prop(std::string key, std::int64_t value) {
    node_.props.insert_or_assign(std::move(key), PropValue{value});
    return *this;
  }

  ViewBuilder &prop(std::string key, double value) {
    node_.props.insert_or_assign(std::move(key), PropValue{value});
    return *this;
  }

  ViewBuilder &prop(std::string key, bool value) {
    node_.props.insert_or_assign(std::move(key), PropValue{value});
    return *this;
  }

  template <typename Int,
            typename = std::enable_if_t<
                std::is_integral_v<std::remove_reference_t<Int>> &&
                !std::is_same_v<std::remove_reference_t<Int>, bool> &&
                !std::is_same_v<std::remove_reference_t<Int>, std::int64_t>>>
  ViewBuilder &prop(std::string key, Int value) {
    return prop(std::move(key), static_cast<std::int64_t>(value));
  }

  template <typename F> ViewBuilder &children(F &&fn) {
    ChildCollector collector;
    fn(collector);
    node_.children = std::move(collector.children);
    return *this;
  }

  ViewBuilder &children(std::initializer_list<ViewNode> nodes) {
    node_.children.assign(nodes.begin(), nodes.end());
    return *this;
  }

  ViewBuilder &event(std::string name, std::uint64_t handler_id) {
    node_.events.insert_or_assign(std::move(name), handler_id);
    return *this;
  }

  ViewNode build() const & { return node_; }

  ViewNode build() && { return std::move(node_); }

private:
  struct ChildCollector {
    std::vector<ViewNode> children;

    void add(ViewNode node) { children.push_back(std::move(node)); }
  };

  static NodeId allocate_id() {
    return detail::next_node_id.fetch_add(1, std::memory_order_relaxed);
  }

  ViewNode node_;
};

inline ViewBuilder view(std::string type) {
  return ViewBuilder{std::move(type)};
}

} // namespace duorou::ui

