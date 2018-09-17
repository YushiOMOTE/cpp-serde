#ifndef SERDE_H_
#define SERDE_H_

#include <deque>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "map.h"

/// Local macro helpers
#define SERDE_CHECK_N(x, n, ...) n
#define SERDE_CHECK(...) SERDE_CHECK_N(__VA_ARGS__, 0, )
#define SERDE_PROBE(x) x, 1,

#define SERDE_IS_PAREN(x) SERDE_CHECK(SERDE_IS_PAREN_PROBE x)
#define SERDE_IS_PAREN_PROBE(...) SERDE_PROBE(~)

#define SERDE_CAT(a, ...) SERDE_PRIMITIVE_CAT(a, __VA_ARGS__)
#define SERDE_PRIMITIVE_CAT(a, ...) a##__VA_ARGS__

#define SERDE_IIF(c) SERDE_PRIMITIVE_CAT(SERDE_IIF_, c)
#define SERDE_IIF_0(t, ...) __VA_ARGS__
#define SERDE_IIF_1(t, ...) t

namespace serde {

namespace internal {
template <typename T, typename = int>
struct is_serde_struct : std::false_type {};
template <typename T>
struct is_serde_struct<T, decltype((void)T::is_serde_struct, 0)>
    : std::true_type {};

template <typename T> struct CoreHandler;
template <typename T> struct is_serde_specialized : std::false_type {};
template <typename T>
struct is_serde_specialized<CoreHandler<T>> : std::true_type {};
} // namespace internal

/// True for struct with SERDE_DEFINE
template <typename T>
inline constexpr bool is_serde_struct_v = internal::is_serde_struct<T>::value;

/// True for struct/enum with SERDE_ADD_STRUCT/SERDE_ADD_ENUM
template <typename T>
inline constexpr bool is_serde_specialized_v =
    internal::is_serde_specialized<T>::value;

/// True for struct/enum with SERDE_DEFINE/SERDE_ADD_STRUCT/SERDE_ADD_ENUM
template <typename T>
inline constexpr bool is_serde_compatible_v =
    is_serde_struct_v<T> || is_serde_specialized_v<T>;

/// Language implementer throws this exception
class Exception : public std::exception {
public:
  explicit Exception(const char *msg) : m_msg(msg) {}

  explicit Exception(const std::string &msg) : m_msg(msg) {}

  virtual ~Exception() throw() {}

  virtual const char *what() const throw() { return m_msg.c_str(); }

protected:
  std::string m_msg;
};

/// Helper class that represents a member of struct or enum
template <typename T> struct Member {
  explicit Member(const char *name, const std::optional<T> &value)
      : name(name), value(value) {}

  const char *name;
  std::optional<T> value;
};

/// Language implementer specializes this struct
template <typename Lang> struct Node;

/// Language implementer specializes this struct
///
/// These methods are required:
///   * static Node<MsgPack> from_string(const std::string &str);
///   * static std::string to_string(const Node<MsgPack> &node);
///   * template <typename T> static T unpack(const Node<MsgPack> &node);
///   * template <typename T> static Node<MsgPack> pack(const T &obj);
///   * template <typename T, typename... Args>
///     static T unpack_struct(const Node<MsgPack> &node, Member<Args>... args);
///   * template <typename T, typename... Args>
///     static Node<MsgPack> pack_struct(Member<Args>... args);
template <typename Lang> struct LangHandler;

template <typename T> struct CoreHandler {
  template <typename Lang> static T unpack(const Node<Lang> &node) {
    if constexpr (is_serde_struct_v<T>) {
      return T::template serde_unpack<Lang, T>(node);
    } else {
      return LangHandler<Lang>::template unpack<T>(node);
    }
  }

  template <typename Lang> static Node<Lang> pack(const T &obj) {
    if constexpr (is_serde_struct_v<T>) {
      return T::template serde_pack<Lang, T>(obj);
    } else {
      return LangHandler<Lang>::template pack<T>(obj);
    }
  }
};

/// Language implementer calls these methods whenever needed
struct Core {
  template <typename Lang>
  static Node<Lang> from_string(const std::string &str) {
    return LangHandler<Lang>::from_string(str);
  }

  template <typename Lang>
  static std::string to_string(const Node<Lang> &node) {
    return LangHandler<Lang>::to_string(node);
  }

  template <typename Lang, typename T> static T unpack(const Node<Lang> &node) {
    return CoreHandler<T>::template unpack<Lang>(node);
  }

  template <typename Lang, typename T> static Node<Lang> pack(const T &obj) {
    return CoreHandler<T>::template pack<Lang>(obj);
  }

private:
  template <typename T> friend struct CoreHandler;

  template <typename Lang, typename T, typename... Args>
  static T unpack_enum(const Node<Lang> &node, Member<Args>... args) {
    auto enum_str = CoreHandler<std::string>::template unpack<Lang>(node);

    std::optional<T> result;

    auto check_member = [&](auto &mem) {
      if (result) {
        return;
      }
      if (mem.name == enum_str) {
        if (!mem.value) {
          throw Exception("Library bug: " + std::string(mem.name) +
                          " is missing value");
        }
        result = mem.value.value();
      }
    };

    (check_member(args), ...);

    if (!result) {
      throw Exception("Bad enum value: " + enum_str);
    }

    return result.value();
  }

  template <typename Lang, typename T, typename... Args>
  static Node<Lang> pack_enum(const T &value, Member<Args>... args) {
    std::optional<Node<Lang>> result;

    auto check_member = [&](auto &mem) {
      if (result) {
        return;
      }

      if (!mem.value) {
        throw Exception("Library bug: " + std::string(mem.name) +
                        " is missing value");
      }

      if (mem.value.value() == value) {
        result = CoreHandler<std::string>::template pack<Lang>(mem.name);
      }
    };

    (check_member(args), ...);

    if (!result) {
      throw Exception(
          "Bad enum value: " +
          std::to_string(static_cast<std::underlying_type_t<T>>(value)));
    }

    return result.value();
  }
};

} // namespace serde

/// Construct a member variable object
#define SERDE_MEM_PACK(X) ::serde::Member<decltype(X)>(#X, obj.X)

/// Pack a member variable (emitter doesn't care default)
#define SERDE_MEM_PACK_WITH_DEFAULT(X, ...) SERDE_MEM_PACK(X)

/// Construct a member variable object
#define SERDE_MEM_UNPACK(X) ::serde::Member<decltype(X)>(#X, std::nullopt)

/// Construct a member variable object with default value
#define SERDE_MEM_UNPACK_WITH_DEFAULT(X, ...)                                  \
  ::serde::Member<decltype(X)>(#X, std::make_optional<decltype(X)>(__VA_ARGS__))

/// Pack a member variable resolving default value macro syntax
#define SERDE_PACK(X)                                                          \
  SERDE_IIF(SERDE_IS_PAREN(X))(SERDE_MEM_PACK_WITH_DEFAULT X, SERDE_MEM_PACK(X))

/// Unpack a member variable resolving default value macro syntax
#define SERDE_UNPACK(X)                                                        \
  SERDE_IIF(SERDE_IS_PAREN(X))                                                 \
  (SERDE_MEM_UNPACK_WITH_DEFAULT X, SERDE_MEM_UNPACK(X))

/// Construct an enum member object
#define SERDE_ENUM_MEM(E) ::serde::Member<EnumType>(#E, EnumType::E)

/// Register a new enum for packing/unpacking
#define SERDE_ADD_ENUM(E, ...)                                                 \
  namespace serde {                                                            \
  template <> struct CoreHandler<E> {                                          \
    template <typename Lang> static E unpack(const Node<Lang> &node) {         \
      using EnumType = E;                                                      \
      return Core::unpack_enum<Lang, E>(                                       \
          node, MAP_LIST(SERDE_ENUM_MEM, __VA_ARGS__));                        \
    }                                                                          \
    template <typename Lang> static Node<Lang> pack(const E &obj) {            \
      using EnumType = E;                                                      \
      return Core::pack_enum<Lang, E>(obj,                                     \
                                      MAP_LIST(SERDE_ENUM_MEM, __VA_ARGS__));  \
    }                                                                          \
  };                                                                           \
  } // namespace serde

/// Register a new structure for packing/unpacking (non-intrusive but slightly
/// verbose)
#define SERDE_ADD_STRUCT(T, ...)                                               \
  namespace serde {                                                            \
  template <> struct CoreHandler<T> {                                          \
    template <typename Lang> static T unpack(const Node<Lang> &node) {         \
      using StructType = T;                                                    \
      return LangHandler<Lang>::template unpack_struct<T>(                     \
          node, MAP_LIST(SERDE_UNPACK, __VA_ARGS__));                          \
    }                                                                          \
    template <typename Lang> static Node<Lang> pack(const T &obj) {            \
      using StructType = T;                                                    \
      return LangHandler<Lang>::template pack_struct<T>(                       \
          MAP_LIST(SERDE_PACK, __VA_ARGS__));                                  \
    }                                                                          \
  };                                                                           \
  } // namespace serde

/// Register a new structure for packing/unpacking (intrusive but slightly
/// simple and can support private members)
#define SERDE_DEFINE(...)                                                      \
  template <typename Lang, typename T>                                         \
  static T serde_unpack(const ::serde::Node<Lang> &node) {                     \
    return ::serde::LangHandler<Lang>::template unpack_struct<T>(              \
        node, MAP_LIST(SERDE_UNPACK, __VA_ARGS__));                            \
  }                                                                            \
  template <typename Lang, typename T>                                         \
  static serde::Node<Lang> serde_pack(const T &obj) {                          \
    return ::serde::LangHandler<Lang>::template pack_struct<T>(                \
        MAP_LIST(SERDE_PACK, __VA_ARGS__));                                    \
  }                                                                            \
  static bool is_serde_struct;                                                 \
  template <typename T> friend struct ::serde::CoreHandler;

/// Syntax for setting default value
#define SERDE_OPT(...) (__VA_ARGS__)

namespace serde::internal {

inline std::optional<std::string> read_file(const std::string &filename) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (!in) {
    return {};
  }
  std::ostringstream contents;
  contents << in.rdbuf();
  in.close();
  return (contents.str());
}

} // namespace serde::internal

namespace serde {

/// Optional value with error reason
template <typename T> class Result {
public:
  template <typename U> static Result<T> value(U &&value) {
    return Result<T>(std::forward<U>(value), "");
  }
  template <typename U> static Result<T> error(U &&error) {
    return Result<T>(std::nullopt, std::forward<U>(error));
  }

  explicit operator bool() const { return bool(m_value); }

  const T &value() const & { return m_value.value(); }

  T &&value() const && { return std::move(m_value.value()); }

  const std::string &error() const & { return m_error; }

  std::string &&error() const && { return std::move(m_error); }

private:
  template <typename U, typename E>
  Result(U &&value, E &&error)
      : m_value(std::forward<U>(value)), m_error(std::forward<E>(error)) {}

  std::optional<T> m_value;
  std::string m_error;
};

/// Parse a file
template <typename Lang, typename T>
Result<T> from_file(const std::string &filename) try {
  const auto str = internal::read_file(filename);
  if (!str) {
    return Result<T>::error("serde: on parsing file: file not found: " +
                            filename);
  }

  const auto node = Core::from_string<Lang>(str.value());
  return Result<T>::value(Core::unpack<Lang, T>(node));
} catch (std::exception &e) {
  return Result<T>::error("serde: on parsing string: " + std::string(e.what()));
}

/// Parse a string
template <typename Lang, typename T>
Result<T> from_string(const std::string &str) try {
  const auto node = Core::from_string<Lang>(str);
  return Result<T>::value(Core::unpack<Lang, T>(node));
} catch (std::exception &e) {
  return Result<T>::error("serde: on parsing string: " + std::string(e.what()));
}

/// Pack into a string
template <typename Lang, typename T>
Result<std::string> to_string(const T &obj) try {
  const auto node = Core::pack<Lang, T>(obj);
  return Result<std::string>::value(Core::to_string<Lang>(node));
} catch (std::exception &e) {
  return Result<std::string>::error("serde: on emitting to string: " +
                                    std::string(e.what()));
}

} // namespace serde

#endif // SERDE_H_
