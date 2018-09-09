#ifndef SERDE_YAML_H_
#define SERDE_YAML_H_

#include <chrono>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include <yaml-cpp/yaml.h>

namespace serde {
struct YAML;
} // namespace serde

namespace yamlcpp = ::YAML;

namespace serde::yaml::internal {

/// Helper method for std::variant converter
template <typename T, typename U>
void try_set(const yamlcpp::Node &node, U &rhs, bool &set) {
  if (set) {
    return;
  }
  try {
    rhs = node.template as<T>();
    set = true;
  } catch (yamlcpp::Exception &) {
  }
}

/// Helper method to convert sequence node to tuple
template <typename... T, std::size_t... I>
std::tuple<T...> seq_to_tuple(const yamlcpp::Node &node,
                              std::index_sequence<I...>) {
  return std::make_tuple(node[I].template as<T>()...);
}

/// Helper method to convert tuple to sequence node
template <typename... T, std::size_t... I>
yamlcpp::Node tup_to_seq(const std::tuple<T...> &tup,
                         std::index_sequence<I...>) {
  yamlcpp::Node node(yamlcpp::NodeType::Sequence);
  (node.push_back(std::get<I>(tup)), ...);
  return node;
}

} // namespace serde::yaml::internal

/// YAML-cpp converters
namespace YAML {

/// Converter for std::chrono::duration
template <typename R, typename P> struct convert<std::chrono::duration<R, P>> {
  static bool decode(const Node &node, std::chrono::duration<R, P> &rhs) {
    if (!node.IsScalar()) {
      return false;
    }

    rhs = std::chrono::duration<R, P>(node.template as<R>());

    return true;
  }

  static Node encode(const std::chrono::duration<R, P> &rhs) {
    return Node(rhs.count());
  }
};

/// Converter for std::unordered_map
template <typename K, typename V> struct convert<std::unordered_map<K, V>> {
  static bool decode(const Node &node, std::unordered_map<K, V> &rhs) {
    if (!node.IsMap()) {
      return false;
    }

    rhs.clear();

    for (auto &it : node) {
      rhs[it.first.template as<K>()] = it.second.template as<V>();
    }

    return true;
  }

  static Node encode(const std::unordered_map<K, V> &rhs) {
    Node node(NodeType::Map);

    for (auto &[k, v] : rhs) {
      node.force_insert(k, v);
    }

    return node;
  }
};

/// Converter for std::variant
template <typename... T> struct convert<std::variant<T...>> {
  static bool decode(const Node &node, std::variant<T...> &rhs) {
    bool set = false;

    (serde::yaml::internal::try_set<T>(node, rhs, set), ...);

    return set;
  }

  static Node encode(const std::variant<T...> &rhs) {
    Node node;

    std::visit([&](auto &v) { node = Node(v); }, rhs);

    return node;
  }
};

/// Converter for std::optional
template <typename T> struct convert<std::optional<T>> {
  static bool decode(const Node &node, std::optional<T> &rhs) {
    if (node.IsNull()) {
      rhs = {};
    } else {
      rhs = node.template as<T>();
    }
    return true;
  }

  static Node encode(const std::optional<T> &rhs) {
    if (rhs) {
      return Node(rhs.value());
    } else {
      return Node();
    }
  }
};

/// Converter for std::tuple
template <typename... T> struct convert<std::tuple<T...>> {
  static bool decode(const Node &node, std::tuple<T...> &rhs) {
    if (!node.IsSequence()) {
      return false;
    }
    if (node.size() != sizeof...(T)) {
      return false;
    }
    rhs = serde::yaml::internal::seq_to_tuple<T...>(
        node, std::index_sequence_for<T...>{});
    return true;
  }

  static Node encode(const std::tuple<T...> &rhs) {
    return serde::yaml::internal::tup_to_seq<T...>(
        rhs, std::index_sequence_for<T...>{});
  }
};

/// Converter for serde structures
template <typename T> struct convert {
  static bool decode(const Node &node, T &rhs) {
    rhs = serde::Core::unpack<serde::YAML, T>(serde::Node<serde::YAML>{node});
    return true;
  }
  static Node encode(const T &rhs) {
    return serde::Core::pack<serde::YAML>(rhs).node;
  }
};

} // namespace YAML

namespace serde {

template <> struct Node<YAML> { yamlcpp::Node node; };

template <> struct LangHandler<YAML> {
  static Node<YAML> from_string(const std::string &str) {
    return Node<YAML>{yamlcpp::Load(str)};
  }

  static std::string to_string(const Node<YAML> &node) {
    yamlcpp::Emitter e;
    e << node.node;
    return e.c_str();
  }

  template <typename T> static T unpack(const Node<YAML> &node) {
    return node.node.as<T>();
  }

  template <typename T> static Node<YAML> pack(const T &obj) {
    return Node<YAML>{yamlcpp::Node(obj)};
  }

  template <typename T, typename... Args>
  static T unpack_struct(const Node<YAML> &node, Member<Args>... args) {
    if (!node.node.IsMap()) {
      throw yamlcpp::TypedBadConversion<T>(node.node.Mark());
    }

    auto unpack_member = [&](const auto &mem) {
      if (node.node[mem.name] && !node.node[mem.name].IsNull()) {
        return node.node[mem.name]
            .template as<std::decay_t<decltype(mem.value.value())>>();
      } else {
        if (mem.value) {
          return mem.value.value();
        } else {
          throw yamlcpp::KeyNotFound(node.node.Mark(), std::string(mem.name));
        }
      }
    };

    return T{unpack_member(args)...};
  }

  template <typename T, typename... Args>
  static Node<YAML> pack_struct(Member<Args>... args) {
    yamlcpp::Node node(yamlcpp::NodeType::Map);

    auto push_member = [&](auto &mem) {
      if (!mem.value) {
        // TODO: Throw logic error
        throw yamlcpp::KeyNotFound(node.Mark(), std::string(mem.name));
      }

      node.force_insert(mem.name, mem.value.value());
    };

    (push_member(args), ...);

    return Node<YAML>{node};
  }
};

} // namespace serde

#endif // SERDE_YAML_H_
