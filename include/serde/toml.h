#ifndef SERDE_TOML_H_
#define SERDE_TOML_H_

#include <iostream>

#include <cpptoml.h>

namespace serde {

struct TOML;

template <> struct Node<TOML> { std::shared_ptr<cpptoml::base> base; };

using Base = std::shared_ptr<cpptoml::base>;

template <typename T> struct TomlSerde {
  static T decode(Base base) {
    if constexpr (cpptoml::valid_value<T>::value) {
      return base->as<T>()->get();
    } else if constexpr (std::is_floating_point_v<T>) {
      return base->as<double>()->get();
    } else if constexpr (std::is_integral_v<T>) {
      return base->as<int64_t>()->get();
    } else {
      return Core::unpack<TOML, T>(Node<TOML>{base});
    }
  }

  static Base encode(const T &value) {
    if constexpr (cpptoml::valid_value<T>::value) {
      return std::static_pointer_cast<cpptoml::base>(
          cpptoml::make_value(value));
    } else if constexpr (std::is_floating_point_v<T>) {
      return Core::pack<TOML>(static_cast<double>(value)).base;
    } else if constexpr (std::is_integral_v<T>) {
      return Core::pack<TOML>(static_cast<int64_t>(value)).base;
    } else {
      return Core::pack<TOML>(value).base;
    }
  }
};

template <typename T> T dec(Base b) { return TomlSerde<T>::decode(b); }

template <typename T> Base enc(const T &v) { return TomlSerde<T>::encode(v); }

template <typename T> struct TomlSerde<std::vector<T>> {
  static std::vector<T> decode(Base base) {
    std::vector<T> v;

    auto array = base->as_array();
    for (auto b : *array) {
      auto d = dec<T>(b);
      v.emplace_back(d);
    }

    return v;
  }

  static Base encode(const std::vector<T> &v) {
    auto array = cpptoml::make_array();

    for (auto &i : v) {
      auto b = enc<T>(i);
      array->push_back(std::static_pointer_cast<cpptoml::value<T>>(b));
    }

    return std::static_pointer_cast<cpptoml::base>(array);
  }
};

template <typename T, size_t N> struct TomlSerde<std::array<T, N>> {
  static std::array<T, N> decode(Base base) {
    std::array<T, N> v;

    auto i = 0ul;
    auto array = base->as_array();
    for (auto b : *array) {
      v[i] = dec<T>(b);
      ++i;
    }

    return v;
  }

  static Base encode(const std::array<T, N> &v) {
    auto array = cpptoml::make_array();

    for (auto &e : v) {
      auto b = enc<T>(e);
      array->push_back(std::static_pointer_cast<cpptoml::value<T>>(b));
    }

    return std::static_pointer_cast<cpptoml::base>(array);
  }
};

template <typename K, typename V> struct TomlSerde<std::unordered_map<K, V>> {
  static std::unordered_map<K, V> decode(Base base) {
    std::unordered_map<K, V> v;

    auto table = base->as_table();
    for (auto &[k, b] : *table) {
      auto d = dec<V>(b);
      v.emplace(k, d);
    }

    return v;
  }

  static Base encode(const std::unordered_map<K, V> &m) {
    auto table = cpptoml::make_table();

    for (auto &[k, v] : m) {
      auto b = enc(v);
      table->insert(k, b);
    }

    return std::static_pointer_cast<cpptoml::base>(table);
  }
};

template <typename K, typename V> struct TomlSerde<std::map<K, V>> {
  static std::map<K, V> decode(Base base) {
    std::map<K, V> v;

    auto table = base->as_table();
    for (auto &[k, b] : *table) {
      auto d = dec<V>(b);
      v.emplace(k, d);
    }

    return v;
  }

  static Base encode(const std::map<K, V> &m) {
    auto table = cpptoml::make_table();

    for (auto &[k, v] : m) {
      auto b = enc(v);
      table->insert(k, b);
    }

    return std::static_pointer_cast<cpptoml::base>(table);
  }
};

template <> struct LangHandler<TOML> {
  static Node<TOML> from_string(const std::string &str) {
    auto is = std::istringstream(str);
    auto p = cpptoml::parser(is);
    return Node<TOML>{std::static_pointer_cast<cpptoml::base>(p.parse())};
  }

  static std::string to_string(const Node<TOML> &node) {
    std::ostringstream oss;
    oss << *node.base;
    return oss.str();
  }

  template <typename T> static T unpack(const Node<TOML> &node) {
    return dec<T>(node.base);
  }

  template <typename T> static Node<TOML> pack(const T &obj) {
    return Node<TOML>{enc<T>(obj)};
  }

  template <typename T, typename... Args>
  static T unpack_struct(const Node<TOML> &node, Member<Args>... args) {
    auto table = node.base->as_table();

    auto unpack_member = [&](const auto &mem) {
      if (!table->contains(mem.name)) {
        if (mem.value) {
          return mem.value.value();
        } else {
          throw Exception("Node member doesn't have value: " +
                          std::string(mem.name));
        }
      }
      auto b = table->get(mem.name);
      return dec<std::decay_t<decltype(mem.value.value())>>(b);
    };

    return T{unpack_member(args)...};
  }

  template <typename T, typename... Args>
  static Node<TOML> pack_struct(Member<Args>... args) {
    auto table = cpptoml::make_table();

    auto push_member = [&](auto &mem) {
      if (!mem.value) {
        throw Exception("Logic error: " + std::string(mem.name));
      }

      table->insert(mem.name, enc(mem.value.value()));
    };

    (push_member(args), ...);

    return Node<TOML>{std::static_pointer_cast<cpptoml::base>(table)};
  }
};

} // namespace serde

#endif // SERDE_TOML_H_
