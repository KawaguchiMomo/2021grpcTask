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
#include <vector>
#include <time.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

using helloworld::MessageRequest;
using helloworld::RequestReply;
using helloworld::ReplyRequest;
using helloworld::MessageReply;

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public Greeter::Service {
  MessageRequest serverMessage;
  std::vector<MessageRequest> serverMessageList;

  // メッセージをクライアントから受信
  Status SendRequest(ServerContext* context, const MessageRequest* request,
                        RequestReply* reply) override {

    // char date[64];
    time_t date = time(NULL);
    // strftime(date, sizeof(date), "%Y/%m/%d %a %H:%M:%S", localtime(&t));
    // std::cout << date << std::endl;

    serverMessage.set_date(date);
    serverMessage.set_name(request->name());
    serverMessage.set_message(request->message());
    serverMessageList.push_back(serverMessage);
    return Status::OK;
  }

  // メッセージをクライアントに送信
  Status GetReply(ServerContext* context, const ReplyRequest* request,
                        MessageReply* reply) override {
    // まだデータが一件もない場合は返さない
    if(serverMessageList.size() == 0) { return Status::OK; }

    // 取得した最後のメッセージの日時が最新でない場合返信
    if(request->date() != serverMessageList.back().date()) {
      reply->set_date(serverMessage.date());
      reply->set_name(serverMessage.name());
      reply->set_message(serverMessage.message());
      // テスト用
      // if(serverMessageList.size() >= 2) {
      //   int at = serverMessageList.size()-2;
      //   std::cout << serverMessageList.at(at).message() << std::endl;
      // }

    } else {
      reply->set_date(0);
    }
    return Status::OK;
  }
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  GreeterServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}
