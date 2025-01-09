#pragma once

#include "log/log.h"
#include <cstdarg>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include "serialize.hpp"

std::string file_path(size_t n, ...) {
  va_list valist;
  va_start(valist, n);
  std::string ret;
  for (size_t i = 1; i < n; ++i) {
    ret += va_arg(valist, char*);
    if (ret.back() != '/') ret += '/';
  }
  ret += va_arg(valist, char*);
  return ret;
}

// save a serialized file
template <class T>
bool save_file(const std::string& filename, const T& data) {
  log_info("saving file '%s'", filename.c_str());
  std::size_t delpos = filename.find_last_of("/");
  if (delpos !=  std::string::npos) {
    std::string cmd = "mkdir -p " + filename.substr(0, delpos);
    std::system(cmd.c_str());
  }

  std::ofstream file(filename, std::ios::binary);
  if (file.eof() || file.fail()) {
    log_error("failed to save file '%s'", filename.c_str());
    return false;
  }

  __serialize_detail::stream res;
  serialize(data, res);
  file.write(reinterpret_cast<char*>(&res[0]), res.size());
  file.close();
  res.clear();
  log_info("file '%s' saved", filename.c_str());
  return true;
}

// load a serialized file
template <class T>
T load_file(const std::string& filename) {
  log_info("loading file '%s'", filename.c_str());

  std::ifstream file(filename, std::ios::binary);
  if (file.eof() || file.fail()) {
    log_fatal("cannot open file '%s'", filename.c_str());
    exit(1);
  }

  file.seekg(0, std::ios_base::end);
  const std::streampos size = file.tellg();
  file.seekg(0, std::ios_base::beg);
  std::vector<uint8_t> res(size);
  file.read(reinterpret_cast<char*>(&res[0]), size);
  file.close();
  T ret = deserialize<T>(res);
  res.clear();
  log_info("file '%s' loaded", filename.c_str());
  return ret;
}
