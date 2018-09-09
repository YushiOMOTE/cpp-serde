#ifndef SERDE_MSGPACK_H_
#define SERDE_MSGPACK_H_

#include <serde/json.h>

namespace serde {

struct MsgPack;

template <> struct Node<MsgPack> { json j; };

template <> struct LangHandler<MsgPack> {
  static Node<MsgPack> from_string(const std::string &str) {
    return Node<MsgPack>{json::from_msgpack(str)};
  }

  static std::string to_string(const Node<MsgPack> &node) {
    auto vec = nlohmann::json::to_msgpack(node.j);
    return std::string(vec.begin(), vec.end());
  }

  template <typename T> static T unpack(const Node<MsgPack> &node) {
    auto jnode = Node<JSON>{node.j};
    return LangHandler<JSON>::template unpack<T>(jnode);
  }

  template <typename T> static Node<MsgPack> pack(const T &obj) {
    return Node<MsgPack>{LangHandler<JSON>::template pack<T>(obj).j};
  }

  template <typename T, typename... Args>
  static T unpack_struct(const Node<MsgPack> &node, Member<Args>... args) {
    auto jnode = Node<JSON>{node.j};
    return LangHandler<JSON>::template unpack_struct<T>(jnode, args...);
  }

  template <typename T, typename... Args>
  static Node<MsgPack> pack_struct(Member<Args>... args) {
    return Node<MsgPack>{LangHandler<JSON>::template pack_struct<T>(args...).j};
  }
};

} // namespace serde

#endif // SERDE_MSGPACK_H_
