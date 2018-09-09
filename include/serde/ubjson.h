#ifndef SERDE_UBJSON_H_
#define SERDE_UBJSON_H_

#include <serde/json.h>

namespace serde {

struct UBJSON;

template <> struct Node<UBJSON> { json j; };

template <> struct LangHandler<UBJSON> {
  static Node<UBJSON> from_string(const std::string &str) {
    return Node<UBJSON>{json::from_ubjson(str)};
  }

  static std::string to_string(const Node<UBJSON> &node) {
    auto vec = nlohmann::json::to_ubjson(node.j);
    return std::string(vec.begin(), vec.end());
  }

  template <typename T> static T unpack(const Node<UBJSON> &node) {
    auto jnode = Node<JSON>{node.j};
    return LangHandler<JSON>::template unpack<T>(jnode);
  }

  template <typename T> static Node<UBJSON> pack(const T &obj) {
    return Node<UBJSON>{LangHandler<JSON>::template pack<T>(obj).j};
  }

  template <typename T, typename... Args>
  static T unpack_struct(const Node<UBJSON> &node, Member<Args>... args) {
    auto jnode = Node<JSON>{node.j};
    return LangHandler<JSON>::template unpack_struct<T>(jnode, args...);
  }

  template <typename T, typename... Args>
  static Node<UBJSON> pack_struct(Member<Args>... args) {
    return Node<UBJSON>{LangHandler<JSON>::template pack_struct<T>(args...).j};
  }
};

} // namespace serde

#endif // SERDE_UBJSON_H_
