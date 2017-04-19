#ifndef GRPC_COMMON_CPP_ROUTE_GUIDE_HELPER_H_
#define GRPC_COMMON_CPP_ROUTE_GUIDE_HELPER_H_

#include <string>
#include <vector>
#include <fstream>

namespace databroker {

bool MakeDirectories(const std::string path);

bool RemoveDirectory(const std::string path);

std::vector<std::string> GetFileNames(const std::string path);

bool Write(const std::string path, const int value);

bool Write(const std::string path, const std::string value);

bool Write(const std::string& data, const int& length, const std::string& path);

bool Read(const std::string path, char* result, const int length);

int GetBindingPort(int argc, char** argv);

std::ifstream::pos_type GetFileSize(const std::string filename);

std::string getFileName(const std::string path);

bool Persist(const std::string &data, const int &length, const std::string &path);

bool Exists(const std::string path);

}  // namespace databroker

#endif  // GRPC_COMMON_CPP_ROUTE_GUIDE_HELPER_H_

