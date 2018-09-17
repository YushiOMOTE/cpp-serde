#include <gtest/gtest.h>

#include <serde/serde_all.h>

template <typename Lang> const char *name();
template <> const char *name<serde::YAML>() { return "YAML"; }
template <> const char *name<serde::JSON>() { return "JSON"; }
template <> const char *name<serde::TOML>() { return "TOML"; }
template <> const char *name<serde::CBOR>() { return "CBOR"; }
template <> const char *name<serde::MsgPack>() { return "MsgPack"; }
template <> const char *name<serde::UBJSON>() { return "UBJSON"; }

template <typename Lang> void print_binary(const std::string &s) {
  std::cout << "*** Pack in " << name<Lang>() << " ***" << std::endl;
  for (auto b : s) {
    std::cout << std::setfill('0') << std::setw(2) << std::hex << (b & 0xff)
              << " ";
  }
  std::cout << std::endl;
}

template <typename Lang> void print(const std::string &s) {
  std::cout << "*** Pack in " << name<Lang>() << " ***" << std::endl;
  std::cout << s << std::endl;
}
template <> void print<serde::CBOR>(const std::string &s) {
  print_binary<serde::CBOR>(s);
}
template <> void print<serde::MsgPack>(const std::string &s) {
  print_binary<serde::MsgPack>(s);
}
template <> void print<serde::UBJSON>(const std::string &s) {
  print_binary<serde::UBJSON>(s);
}

template <typename Lang, typename T> void repack(const T &data) {
  SCOPED_TRACE(name<Lang>());

  auto str = serde::to_string<Lang>(data);
  ASSERT_TRUE(bool(str)) << str.error();
  print<Lang>(str.value());
  auto dat = serde::from_string<Lang, T>(str.value());
  ASSERT_TRUE(bool(dat)) << dat.error();
  EXPECT_EQ(data, dat.value());
}

template <typename T, typename... Args>
inline constexpr bool contains_v = (std::is_same_v<T, Args> || ...);

template <typename... Args, typename T> void repack_except(const T &data) {
  if constexpr (!contains_v<serde::YAML, Args...>) {
    repack<serde::YAML>(data);
  }
  if constexpr (!contains_v<serde::JSON, Args...>) {
    repack<serde::JSON>(data);
  }
  if constexpr (!contains_v<serde::TOML, Args...>) {
    repack<serde::JSON>(data);
  }
  if constexpr (!contains_v<serde::CBOR, Args...>) {
    repack<serde::CBOR>(data);
  }
  if constexpr (!contains_v<serde::MsgPack, Args...>) {
    repack<serde::MsgPack>(data);
  }
  if constexpr (!contains_v<serde::UBJSON, Args...>) {
    repack<serde::UBJSON>(data);
  }
}

template <typename T> void repack_all(const T &data) {
  repack<serde::YAML>(data);
  repack<serde::JSON>(data);
  // FIXME: Fix some issues on TOML
  // repack<serde::TOML>(data);
  repack<serde::CBOR>(data);
  repack<serde::MsgPack>(data);
  repack<serde::UBJSON>(data);
}

struct Primitives {
  bool b;
  char c;
  short s;
  int i;
  long l;
  unsigned char uc;
  unsigned short us;
  unsigned int ui;
  unsigned long ul;
  size_t sz;

  SERDE_DEFINE(b, c, s, i, l, uc, us, ui, ul, sz)
};

inline bool operator==(const Primitives &lhs, const Primitives &rhs) {
  return lhs.b == rhs.b && lhs.c == rhs.c && lhs.s == rhs.s && lhs.i == rhs.i &&
         lhs.l == rhs.l && lhs.uc == rhs.uc && lhs.us == rhs.us &&
         lhs.ui == rhs.ui && lhs.ul == rhs.ul && lhs.sz == rhs.sz;
}

TEST(Basics, Primitives) {
  auto p = Primitives{true, 3,    -1,    -100,     8932,
                      99,   4740, 11111, 18198419, 1940488420};
  repack_all(p);
}

struct String {
  std::string s1;
  std::string s2;
  std::string s3;

  SERDE_DEFINE(s1, s2, s3)
};

inline bool operator==(const String &lhs, const String &rhs) {
  return lhs.s1 == rhs.s1 && lhs.s2 == rhs.s2 && lhs.s3 == rhs.s3;
}

TEST(Basics, String) {
  auto s = String{"foobar", "", "-19392824840"};

  repack_all(s);
}

struct Map {
  std::map<int, std::string> m1;
  std::map<int, int> m2;
  std::map<std::string, std::string> m3;
  std::unordered_map<int, std::string> m4;
  std::unordered_map<int, int> m5;
  std::unordered_map<std::string, std::string> m6;

  SERDE_DEFINE(m1, m2, m3, m4, m5, m6)
};

inline bool operator==(const Map &lhs, const Map &rhs) {
  return lhs.m1 == rhs.m1 && lhs.m2 == rhs.m2 && lhs.m3 == rhs.m3 &&
         lhs.m4 == rhs.m4 && lhs.m5 == rhs.m5 && lhs.m6 == rhs.m6;
}

TEST(Basics, Map) {
  auto m =
      Map{{
              {1, "Ichi"},
              {2, "Ni"},
              {3, "San"},
          },
          {{22, 4}, {23, 6}, {-88, -64}, {-3, -9}},
          {{"Gomi", "Kami"}, {"Hoge", "Hage"}, {"", "Gachi"}, {"Gochi", ""}},
          {
              {-1, "Gachi"},
          },
          {{-3, -4}, {-5, -6}, {-7, -8}},
          {{"Foo", "Bar"}, {"", "Bar"}, {"Foo", ""}}};

  // At the moment serde toml doesn't support map with non-string keys
  repack_except<serde::TOML>(m);
}

struct MapStrKey {
  std::map<std::string, std::string> m1;
  std::map<std::string, int> m2;
  std::unordered_map<std::string, std::string> m3;
  std::unordered_map<std::string, int> m4;

  SERDE_DEFINE(m1, m2, m3, m4)
};

inline bool operator==(const MapStrKey &lhs, const MapStrKey &rhs) {
  return lhs.m1 == rhs.m1 && lhs.m2 == rhs.m2 && lhs.m3 == rhs.m3 &&
         lhs.m4 == rhs.m4;
}

TEST(Basics, MapStrKey) {
  // FIXME: Current serde toml cannot accept empty string key
  auto m = MapStrKey{
      {
          {"1", "Ichi"},
          {"2", "Ni"},
          {"3", "San"},
      },
      {{"22", 4}, {"23", 6}, {"-88", -64}, {"-3", -9}},
      {{"Gomi", "Kami"}, {"Hoge", "Hage"}, {"FIXME", "Gachi"}, {"Gochi", ""}},
      {{"ichi", 1}, {"nii", 2}, {"FIXME", 3}}};

  repack_all(m);
}

struct Sequence {
  std::vector<int> s1;
  std::list<std::string> s2;
  std::array<char, 4> s3;
  std::pair<int, std::string> s4;
  std::tuple<int, std::string, double, std::vector<bool>> s5;

  SERDE_DEFINE(s1, s2, s3, s4, s5)
};

inline bool operator==(const Sequence &lhs, const Sequence &rhs) {
  return lhs.s1 == rhs.s1 && lhs.s2 == rhs.s2 && lhs.s3 == rhs.s3 &&
         lhs.s4 == rhs.s4 && lhs.s5 == rhs.s5;
}

TEST(Basics, Sequence) {
  auto s = Sequence{
      {1, 2, 3, 4, 5},
      {"Ichi", "Nii", "San", "Shii", "Go"},
      {'o', 't', 't', 'f'},
      {9999, "Yeeaaahh"},
      {1, "Chi", 0.5, {true, true, false, true}},
  };

  repack_all(s);
}

struct Option {
  std::optional<int> o1;
  std::optional<int> o2;
  std::optional<std::string> o3;
  std::optional<std::string> o4;

  SERDE_DEFINE(o1, o2, o3, o4)
};

inline bool operator==(const Option &lhs, const Option &rhs) {
  return lhs.o1 == rhs.o1 && lhs.o2 == rhs.o2 && lhs.o3 == rhs.o3 &&
         lhs.o4 == rhs.o4;
}

TEST(Basics, Option) {
  auto o = Option{{}, 3, {}, "Gomi"};

  repack_all(o);
}

struct Variant {
  std::variant<int, std::vector<int>, std::unordered_map<int, int>> v1;
  std::variant<int, std::vector<int>, std::unordered_map<int, int>> v2;
  std::variant<int, std::vector<int>, std::unordered_map<int, int>> v3;

  SERDE_DEFINE(v1, v2, v3)
};

inline bool operator==(const Variant &lhs, const Variant &rhs) {
  return lhs.v1 == rhs.v1 && lhs.v2 == rhs.v2 && lhs.v3 == rhs.v3;
}

TEST(Basics, Variant) {
  auto v = Variant{int(3), std::vector<int>{1, 2, 3},
                   std::unordered_map<int, int>{{1, 9}, {2, 8}, {3, 7}}};
  repack_all(v);
}
