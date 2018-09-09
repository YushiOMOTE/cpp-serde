#ifndef SERDE_JSON_H_
#define SERDE_JSON_H_

#include <nlohmann/json.hpp>

namespace serde {

struct JSON;

template <typename T, typename> struct JsonSerde;

using json =
    nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int64_t,
                         std::uint64_t, double, std::allocator, JsonSerde>;

template <typename T, typename> struct JsonSerde {
  static void from_json(const json &j, T &t) {
    if constexpr (is_serde_compatible_v<T>) {
      t = Core::unpack<JSON, T>(Node<JSON>{j});
    } else {
      using nlohmann::from_json;
      from_json(j, t);
    }
  }

  static void to_json(json &j, const T &t) {
    if constexpr (is_serde_compatible_v<T>) {
      j = Core::pack<JSON>(t).j;
    } else {
      using nlohmann::to_json;
      to_json(j, t);
    }
  }
};

template <> struct Node<JSON> { json j; };

template <> struct LangHandler<JSON> {
  static Node<JSON> from_string(const std::string &str) {
    return Node<JSON>{json::parse(str)};
  }

  static std::string to_string(const Node<JSON> &node) { return node.j.dump(); }

  template <typename T> static T unpack(const Node<JSON> &node) {
    return node.j.get<T>();
  }

  template <typename T> static Node<JSON> pack(const T &obj) {
    return Node<JSON>{obj};
  }

  template <typename T, typename... Args>
  static T unpack_struct(const Node<JSON> &node, Member<Args>... args) {
    if (!node.j.is_object()) {
      throw Exception("Node is not object");
    }

    auto unpack_member = [&](const auto &mem) {
      if (!node.j[mem.name].is_null()) {
        return node.j[mem.name]
            .template get<std::decay_t<decltype(mem.value.value())>>();
      } else {
        if (mem.value) {
          return mem.value.value();
        } else {
          throw Exception("Node member doesn't have value: " +
                          std::string(mem.name));
        }
      }
    };

    return T{unpack_member(args)...};
  }

  template <typename T, typename... Args>
  static Node<JSON> pack_struct(Member<Args>... args) {
    json j;

    auto push_member = [&](auto &mem) {
      if (!mem.value) {
        throw Exception("Logic error: " + std::string(mem.name));
      }

      j[mem.name] = mem.value.value();
    };

    (push_member(args), ...);

    return Node<JSON>{j};
  }
};

} // namespace serde

#endif // SERDE_JSON_H_
