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
using helloworld::MessageReplyList;

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public Greeter::Service {
  std::vector<MessageRequest> serverMessageList;

  // メッセージをクライアントから受信
  Status SendRequest(ServerContext* context, const MessageRequest* request,
                        RequestReply* reply) override {

    time_t date = time(NULL);
    MessageRequest serverMessage;
    serverMessage.set_date(date);
    serverMessage.set_name(request->name());
    serverMessage.set_message(request->message());
    serverMessageList.push_back(serverMessage);
    return Status::OK;
  }

  // メッセージをクライアントに送信
  Status GetReply(ServerContext* context, const ReplyRequest* request,
                        MessageReplyList* reply) override {
    // まだデータが一件もない場合は返さない
    if(serverMessageList.size() == 0) { 
      reply->clear_messages();
      return Status::OK;

    }
    // 取得した最後のメッセージの日時が最新の場合返さない
    if(request->date() == serverMessageList.back().date()) {
      reply->clear_messages();
      return Status::OK;
    }

    // サーバーに保存されているメッセージの末尾から、取得した最後のメッセージまでイテレータを巻き戻す
    auto itr = serverMessageList.end()-1;
    for( ; (*itr).date() > request->date(); itr--) {
      if (itr == serverMessageList.begin() ) {
        break;
      }
    }
   if(serverMessageList.size() != 1) {
      itr++;
    }
    // 取得した最後のメッセージ以降の未送信メッセージを配列にする
    for(;itr != serverMessageList.end(); itr++) {
      // MessageReply messageReply;
      auto message = reply->add_messages();
      message->set_date((*itr).date());
      message->set_name((*itr).name());
      message->set_message((*itr).message());
    }

    for(auto& a:serverMessageList){
      std::cout << a.date() << ")" << a.name() << ":" <<a.message() << std::endl;
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
