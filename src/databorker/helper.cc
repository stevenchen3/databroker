#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include "databroker.grpc.pb.h"

namespace databroker {

// Create directory if parent directories do not exist, create them
bool MakeDirectories(const std::string path) {
  boost::filesystem::path dir(path.c_str());
  return boost::filesystem::create_directories(dir);
}

// Remove a directory
bool RemoveDirectory(const std::string path) {
  boost::filesystem::path dir(path.c_str());
  return boost::filesystem::remove(dir);
}

std::string getFileName(const std::string path) {
  boost::filesystem::path dir(path.c_str());
  return dir.filename().c_str();
}

// Get a list of filenames of a given directory
std::vector<std::string> GetFileNames(const std::string path) {
  boost::filesystem::path dir(path.c_str());
  std::vector<std::string> result;
  if (boost::filesystem::is_directory(dir)) {
    for (auto &entry : boost::make_iterator_range(
             boost::filesystem::directory_iterator(dir), {})) {
      std::string str(entry.path().filename().c_str());
      if (str != "_metadata") {
        result.push_back(str);
      }
    }
  }
  std::sort(result.begin(), result.end());
  return result;
}

bool Write(const std::string path, const char *value, const int length) {
  std::ofstream file(path, std::ios::out | std::ios::binary);
  if (file.is_open()) {
    file.write(value, length);
    file.close();
    return true;
  }
  return false;
}

bool Write(const std::string path, const int value) {
  return Write(path, reinterpret_cast<const char *>(&value), sizeof(value));
}

bool Write(const std::string path, const std::string value) {
  return Write(path, reinterpret_cast<const char *>(&value), sizeof(value));
}

bool Write(const std::string& data, const int& length, const std::string& path) {
  std::ofstream output(path.c_str(), std::ios::out | std::ios::app | std::ios::binary);
  if (output.is_open()) {
    output.write(data.c_str(), length);
    output.close();
    return true;
  }
  return false;
}

// Read metadata from disk
bool Read(const std::string path, char *result, const int length) {
  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (file.is_open()) {
    file.read(result, length);
    file.close();
    return true;
  }
  return false;
}

// Get binding port number from command
// Does not support invalid input validation
int GetBindingPort(int argc, char **argv) {
  std::string port_str;
  std::string arg_str("--port");
  if (argc > 1) {
    std::string argv_1 = argv[1];
    size_t start_position = argv_1.find(arg_str);
    if (start_position != std::string::npos) {
      start_position += arg_str.size();
      if (argv_1[start_position] == ' ' || argv_1[start_position] == '=') {
        port_str = argv_1.substr(start_position + 1);
      }
    }
  } else {
    port_str = "50051";
  }
  return std::atoi(port_str.c_str());
}

std::ifstream::pos_type GetFileSize(const std::string filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

bool Persist(const std::string &data, const int &length,
             const std::string &path) {
  std::ofstream output(path.c_str(),
                       std::ios::out | std::ios::app | std::ios::binary);
  if (output.is_open()) {
    output.write(data.c_str(), length);
    output.close();
    return true;
  }
  return false;
}

bool Exists(const std::string path) {
  boost::filesystem::path file(path.c_str());
  return boost::filesystem::exists(file);
}

} // namespace databroker

