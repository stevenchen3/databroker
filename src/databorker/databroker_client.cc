#include <iostream>
#include <string>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

#include <grpc/grpc.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include "helper.h"
#include "databroker.grpc.pb.h"
#include "config.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using databroker::Databroker;
using databroker::PutDataRequest;
using databroker::PutDataReply;
using databroker::GetDataRequest;
using databroker::GetDataReply;
using databroker::DataBlock;

class DatabrokerClient {
public:
  DatabrokerClient(std::shared_ptr<Channel> channel)
    : stub_(Databroker::NewStub(channel)) {
    }

  void Get(const int id, const std::string path) {
    GetDataRequest request;
    request.set_id(id);
    ClientContext context;
    std::unique_ptr<ClientReader<GetDataReply>> reader(stub_->Get(&context, request));
    GetDataReply reply;
    while (reader->Read(&reply)) {
      // Concate the file and store to disk
      if (reply.id() != id) {
        std::cout << "Not found: " << id << std::endl;
        return;
      }
      DataBlock block = reply.file();
      databroker::Write(block.data(), block.length(), path);
    }
    Status status = reader->Finish();
    if (status.ok()) {
      std::cout << "Data has been stored in '" << path << "'\n";
    } else {
      std::cout << "Failed to get file: " << status.error_message() << std::endl;
    }
  }

  void Put(std::string path) {
    ClientContext context;
    PutDataReply reply;
    std::unique_ptr<ClientWriter<PutDataRequest>> writer(stub_->Put(&context, &reply));
    long total_size = databroker::GetFileSize(path);
    std::ifstream file(path, std::ios::in | std::ios::binary);
    char *buffer = new char[BLOCK_SIZE];
    std::string name = databroker::getFileName(path);
    std::cout << "filename⇒" << name << ", total length⇒" << total_size << " bytes\n";
    if (file.is_open()) {
      file.seekg(0, file.beg);
      while(!file.eof()) {
        int start_pos = file.tellg();
        if (start_pos == total_size) {
          break;
        }
        int length = BLOCK_SIZE;
        if (total_size - start_pos < length) {
          length = total_size - start_pos;
        }
        file.read(buffer, length);

        DataBlock *block = new DataBlock();
        block->set_offset(start_pos);
        block->set_length(length);
        block->set_block_size(BLOCK_SIZE);
        block->set_data(buffer, length);

        PutDataRequest request;
        request.set_name(name);
        request.set_allocated_file(block);

        if (!writer->Write(request)) {
          break;
        }
      }
      file.close();
    }
    delete[] buffer;
    writer->WritesDone();
    Status status = writer->Finish();
    if (status.ok()) {
      std::cout << "id⇒" << reply.id() << ", name⇒" << reply.name() << ", total⇒" << reply.size() << " bytes\n";
    } else {
      std::cout << "Failed to send data to server: " << status.error_message() << std::endl;
    }
  }

private:
  std::unique_ptr<Databroker::Stub> stub_;
};

int main(int argc, char** argv) {
  // for simplicity, assume the command options as follows:
  // PORT, [GET ID DST_PATH | PUT PATH]
  //
  // Ignore input validation at this moment
  std::string uri = "localhost:" + std::string(argv[1]);
  DatabrokerClient client(grpc::CreateChannel(uri, grpc::InsecureChannelCredentials()));
  if (argc > 4) {
    std::cout << "Get file from server: id⇒" << argv[3] << ", dst⇒" << argv[4] << std::endl;
    if (databroker::Exists(argv[4])) {
      std::cout << "File exists '" << argv[4] << "'\n";
      return 1;
    }
    client.Get(std::atoi(argv[3]), argv[4]);
  } else if (argc == 4) {
    std::cout << "Put file to server from '" << argv[3] << "'\n";
    client.Put(argv[3]);
  } else {
    std::cout << "Invalid arguments\n";
  }
  return 0;
}
