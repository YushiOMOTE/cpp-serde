#include <fstream>
#include <iostream>
#include <sstream>

#include <serde/serde.h>

#include <serde/cbor.h>
#include <serde/json.h>
#include <serde/msgpack.h>
#include <serde/toml.h>
#include <serde/ubjson.h>
#include <serde/yaml.h>

enum class Mode {
  External,
  Internal,
};

SERDE_ADD_ENUM(Mode, External, Internal)

struct Client {
  std::string ip;
  uint16_t port;

  SERDE_DEFINE(ip, port)
};

struct Config {
  Mode mode;
  std::unordered_map<std::string, Client> clients;
  std::vector<std::string> filters;

  SERDE_DEFINE(mode, clients, filters)
};

std::ostream &operator<<(std::ostream &out, Mode mode) {
  switch (mode) {
  case Mode::External:
    out << "External";
    break;
  case Mode::Internal:
    out << "Internal";
    break;
  }
  return out;
}

std::ostream &operator<<(std::ostream &out, Config c) {
  out << std::endl << "--- Config ---" << std::endl;
  out << "mode: " << c.mode << std::endl;
  out << "clients: " << std::endl;
  for (auto [id, cli] : c.clients) {
    out << "   " << id << "=" << cli.ip << ":" << cli.port << std::endl;
  }
  out << "filters: " << std::endl;
  for (auto f : c.filters) {
    out << "   " << f << std::endl;
  }
  out << "-------------" << std::endl;
  return out;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " sample.yaml" << std::endl;
    return -1;
  }

  auto cfg = serde::from_file<serde::YAML, Config>(argv[1]);
  if (!cfg) {
    std::cerr << "yaml: unpack error: " << cfg.error() << std::endl;
    return -1;
  }
  std::cout << "yaml: unpacked: " << cfg.value() << std::endl;

  {
    auto yaml = serde::to_string<serde::YAML>(cfg.value());
    if (!yaml) {
      std::cerr << "yaml: pack error: " << yaml.error() << std::endl;
      return -1;
    }
    std::cout << "yaml: packed: " << std::endl << yaml.value() << std::endl;
  }

  {
    auto mp = serde::to_string<serde::MsgPack>(cfg.value());
    if (!mp) {
      std::cerr << "msgpack: pack error: " << mp.error() << std::endl;
      return -1;
    }

    printf("msgpack: packed: ");
    for (auto c : mp.value()) {
      printf("%02x ", uint8_t(c));
    }
    printf("\n");

    auto cfg = serde::from_string<serde::MsgPack, Config>(mp.value());
    if (!cfg) {
      std::cerr << "msgpack: unpack error: " << cfg.value() << std::endl;
      return -1;
    }

    std::cout << "msgpack: unpacked: " << cfg.value() << std::endl;
  }

  {
    auto json = serde::to_string<serde::JSON>(cfg.value());
    if (!json) {
      std::cerr << "json: pack error: " << json.error() << std::endl;
      return -1;
    }
    std::cout << "json: packed: " << std::endl << json.value() << std::endl;

    auto cfg = serde::from_string<serde::JSON, Config>(json.value());
    if (!cfg) {
      std::cerr << "json: unpack error: " << cfg.error() << std::endl;
      return -1;
    }

    std::cout << "json: unpacked" << cfg.value() << std::endl;
  }

  {
    auto toml = serde::to_string<serde::TOML>(cfg.value());
    if (!toml) {
      std::cerr << "toml: pack error: " << toml.error() << std::endl;
      return -1;
    }
    std::cout << "toml: packed: " << std::endl << toml.value() << std::endl;

    auto cfg = serde::from_string<serde::TOML, Config>(toml.value());
    if (!cfg) {
      std::cerr << "toml: unpack error: " << cfg.error() << std::endl;
      return -1;
    }

    std::cout << "toml: unpacked" << cfg.value() << std::endl;
  }

  {
    auto dat = serde::to_string<serde::CBOR>(cfg.value());
    if (!dat) {
      std::cerr << "cbor: pack error: " << dat.error() << std::endl;
      return -1;
    }

    printf("cbor: packed: ");
    for (auto c : dat.value()) {
      printf("%02x ", uint8_t(c));
    }
    printf("\n");

    auto cfg = serde::from_string<serde::CBOR, Config>(dat.value());
    if (!cfg) {
      std::cerr << "cbor: unpack error: " << cfg.error() << std::endl;
      return -1;
    }

    std::cout << "cbor: unpacked: " << cfg.value() << std::endl;
  }

  {
    auto dat = serde::to_string<serde::UBJSON>(cfg.value());
    if (!dat) {
      std::cerr << "ubjson: pack error: " << dat.error() << std::endl;
      return -1;
    }

    printf("ubjson: packed: ");
    for (auto c : dat.value()) {
      printf("%02x ", uint8_t(c));
    }
    printf("\n");

    auto cfg = serde::from_string<serde::UBJSON, Config>(dat.value());
    if (!cfg) {
      std::cerr << "ubjson: unpack error: " << cfg.error() << std::endl;
      return -1;
    }

    std::cout << "ubjson: unpacked: " << cfg.value() << std::endl;
  }

  return 0;
}
