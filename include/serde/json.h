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

template <typename T, typename U> struct JsonSerde<std::optional<T>, U> {
  static void from_json(const json &j, std::optional<T> &t) {
    if (j.is_null()) {
      t = std::nullopt;
    } else {
      t = Core::unpack<JSON, T>(Node<JSON>{j});
    }
  }

  static void to_json(json &j, const std::optional<T> &t) {
    if (t) {
      j = Core::pack<JSON>(t.value()).j;
    } else {
      j = nullptr;
    }
  }
};

/// Helper method for std::variant converter
template <typename T, typename U> void try_set(const json &j, U &t, bool &set) {
  if (set) {
    return;
  }
  try {
    t = Core::unpack<JSON, T>(Node<JSON>{j});
    set = true;
  } catch (serde::Exception &) {
  } catch (json::exception &) {
  }
}

template <typename... T, typename U> struct JsonSerde<std::variant<T...>, U> {
  static void from_json(const json &j, std::variant<T...> &t) {
    bool set = false;

    (try_set<T>(j, t, set), ...);

    if (!set) {
      throw serde::Exception("Variant didn't match");
    }
  }

  static void to_json(json &j, const std::variant<T...> &t) {
    std::visit([&](auto &v) { j = Core::pack<JSON>(v).j; }, t);
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
