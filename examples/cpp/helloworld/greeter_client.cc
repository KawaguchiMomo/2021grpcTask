/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

using helloworld::MessageRequest;
using helloworld::RequestReply;
using helloworld::ReplyRequest;
using helloworld::MessageReply;

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  // メッセージをサーバーに送信
  std::string SendRequest(const std::string& date, const std::string& name, const std::string& message) {
    // Data we are sending to the server.
    MessageRequest request;
    request.set_date(date);
    request.set_name(name);
    request.set_message(message);

    // Container for the data we expect from the server.
    RequestReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->SendRequest(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "SendRequest RPC failed";
    }
  }

  // メッセージの受信をリクエストする
  std::string GetReply(const std::string& date) {
    // Data we are sending to the server.
    ReplyRequest request;
    request.set_date(date);

    // Container for the data we expect from the server.
    MessageReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->GetReply(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "GetReply RPC failed";
    }
  }

 private:
  std::unique_ptr<Greeter::Stub> stub_;
};

// エラーコード12が修正でき次第スレッドで実装する　今は使ってない
void ThreadFunc(GreeterClient* greeter) {
  // メッセージの受信を模倣
  // 10秒間、1秒おきにメッセージを表示
  std::string date = "date";
  for(int i = 0; i < 10; ++i) {
    std::cout << "ThreadFunc " << i << std::endl;
    std::cout << greeter->GetReply(date) << std::endl;
    sleep(1);
  }
}
void exitMethod(std::thread* th1) {
  th1->join();
  exit(0);
}

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  std::string target_str;
  std::string arg_str("--target");
  if (argc > 1) {
    std::string arg_val = argv[1];
    size_t start_pos = arg_val.find(arg_str);
    if (start_pos != std::string::npos) {
      start_pos += arg_str.size();
      if (arg_val[start_pos] == '=') {
        target_str = arg_val.substr(start_pos + 1);
      } else {
        std::cout << "The only correct argument syntax is --target="
                  << std::endl;
        return 0;
      }
    } else {
      std::cout << "The only acceptable argument is --target=" << std::endl;
      return 0;
    }
  } else {
    target_str = "localhost:50051";
  }
  GreeterClient greeter(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  std::vector<HelloReply*> messageList;

  // ユーザーネームを入れる
  std::string username;
  std::cout << "username: ";
  std::cin >> username;
  std::string name(username);

  // メッセージ受信開始
  // std::thread th1(ThreadFunc, &greeter);

  std::string reply;
  std::string prefix;
  while(1){
    std::cin >> prefix;
    greeter.SendRequest("date", "name", "message");
    std::cout << "Greeter received: " << greeter.GetReply("date") << std::endl;

    if(prefix == "exit"){
      // exitMethod(&th1);
    }
  }

  return 0;
}
