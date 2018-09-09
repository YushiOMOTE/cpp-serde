#ifndef SERDE_CBOR_H_
#define SERDE_CBOR_H_

#include <serde/json.h>

namespace serde {
struct CBOR;

template <> struct Node<CBOR> { json j; };

template <> struct LangHandler<CBOR> {
  static Node<CBOR> from_string(const std::string &str) {
    return Node<CBOR>{json::from_cbor(str)};
  }

  static std::string to_string(const Node<CBOR> &node) {
    auto vec = nlohmann::json::to_cbor(node.j);
    return std::string(vec.begin(), vec.end());
  }

  template <typename T> static T unpack(const Node<CBOR> &node) {
    auto jnode = Node<JSON>{node.j};
    return LangHandler<JSON>::template unpack<T>(jnode);
  }

  template <typename T> static Node<CBOR> pack(const T &obj) {
    return Node<CBOR>{LangHandler<JSON>::template pack<T>(obj).j};
  }

  template <typename T, typename... Args>
  static T unpack_struct(const Node<CBOR> &node, Member<Args>... args) {
    auto jnode = Node<JSON>{node.j};
    return LangHandler<JSON>::template unpack_struct<T>(jnode, args...);
  }

  template <typename T, typename... Args>
  static Node<CBOR> pack_struct(Member<Args>... args) {
    return Node<CBOR>{LangHandler<JSON>::template pack_struct<T>(args...).j};
  }
};

} // namespace serde

#endif // SERDE_CBOR_H_
