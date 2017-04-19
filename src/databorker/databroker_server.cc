#include <algorithm>
#include <iostream>
#include <string>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>
#include "helper.h"
#include "databroker.grpc.pb.h"
#include "config.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using databroker::Databroker;
using databroker::PutDataRequest;
using databroker::PutDataReply;
using databroker::GetDataRequest;
using databroker::GetDataReply;
using databroker::DataBlock;

static std::atomic_int MAX_INDEX;

// Get data path by Id
std::string GetDataPath(const int id) {
  return DATA_DIR + "/" + std::to_string(id);
}

// Initialize metadata to enable id auto-increment
void init(std::atomic_int& index) {
  if (databroker::MakeDirectories(DATA_DIR)) {
    std::cout << "Data directory created\n";
  }
  std::ifstream file(DATA_DIR + "/_index", std::ios::in | std::ios::binary);
  if (file.is_open()) {
    int _index = -1;
    file.read(reinterpret_cast<char *>(&_index), sizeof(_index));
    file.close();
    index = std::max(index.load(), _index);
  } else {
    index = 0;
  }
}

// Get a unique auto-increment request id
// For simplicity, flush the current max id to disk
int GetRequestId() {
  MAX_INDEX++;
  databroker::Write(DATA_DIR + "/_index", MAX_INDEX);
  return MAX_INDEX;
}

// Persist metadata to disk
bool Persist(const int id, const std::string name) {
  databroker::MakeDirectories(GetDataPath(id));
  return databroker::Write(GetDataPath(id) + "/_metadata", name);
}

// Persist a DataBlock to disk
bool Persist(const int id, const DataBlock& block) {
  databroker::MakeDirectories(GetDataPath(id));
  int block_id = block.offset() / (block.block_size() <= 0 ? BLOCK_SIZE : block.block_size());
  std::string filename = GetDataPath(id) + "/blk_" + std::to_string(block_id);
  std::fstream output(filename, std::ios::out | std::ios::binary);
  return block.SerializeToOstream(&output);
}

// Read a block from disk into memory object
bool Read(const std::string path, DataBlock *block) {
  std::fstream input(path, std::ios::in | std::ios::binary);
  return block->ParseFromIstream(&input);
}

bool Clean(const int id) {
  return databroker::RemoveDirectory(GetDataPath(id));
}

class DatabrokerImpl final : public Databroker::Service {
public:
  explicit DatabrokerImpl() {}

  Status Put(ServerContext *context, ServerReader<PutDataRequest> *reader,
             PutDataReply *reply) override {
    int size = 0, request_id = GetRequestId();
    std::string name = "";
    PutDataRequest request;
    bool status = true;
    // Either all blocks are committed or all are aborted
    while (reader->Read(&request)) {
      if (name == "") {
        name = request.name();
      }
      size += request.file().length();
      if (!Persist(request_id, request.file())) {
        status = false;
        break;
      }
    }
    reply->set_id(request_id);
    reply->set_name(name);
    reply->set_size(status ? size : -1);
    status = status & Persist(request_id, name); // Persist metadata
    if (!status) {
      // Clean up partially committed data
      Clean(request_id);
      return Status::CANCELLED;
    }
    return Status::OK;
  }

  Status Get(ServerContext* context,
                      const databroker::GetDataRequest* request,
                      ServerWriter<GetDataReply>* writer) override {
    if (!databroker::Exists(GetDataPath(request->id()))) {
      GetDataReply reply;
      reply.set_id(-1);
      writer->Write(reply);
      return Status::OK;
    }

    std::string name;
    databroker::Read(GetDataPath(request->id()), reinterpret_cast<char*>(&name), sizeof(name));
    std::vector<std::string> files = databroker::GetFileNames(GetDataPath(request->id()));
    std::string base_dir = GetDataPath(request->id());
    for (auto item : files) {
      DataBlock *block = new DataBlock();
      if (Read(base_dir + "/" + item, block)) {
        GetDataReply reply;
        reply.set_id(request->id());
        reply.set_name(name);
        reply.set_allocated_file(block);
        writer->Write(reply);
      }
    }
    return Status::OK;
  }
};

void RunServer(const int port) {
  std::string server_address("0.0.0.0:" + std::to_string(port));
  DatabrokerImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) {
  init(MAX_INDEX);
  RunServer(databroker::GetBindingPort(argc, argv));
  return 0;
}
