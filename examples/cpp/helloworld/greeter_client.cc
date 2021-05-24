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
#include <time.h>

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
using helloworld::MessageReplyList;

void PrintMessage(const MessageReply& message);

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  // メッセージをサーバーに送信
  std::string SendRequest(const int& date, const std::string& name, const std::string& message) {
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
  void GetReply(const int date, std::vector<MessageReply>* messageList) {
    // Data we are sending to the server.
    ReplyRequest request;
    request.set_date(date);

    // Container for the data we expect from the server.
    MessageReplyList reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // std::cout << "ｸﾗ" << std::endl;
    // The actual RPC.
    Status status = stub_->GetReply(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      // 新着メッセージがなかった場合は新着がない旨を返す
      if (reply.messages().empty() == true) { return; }
      std::vector<MessageReply> messages;
      for (auto& message : reply.messages()) {

        messageList->push_back(message);

        PrintMessage(message);

      }
      return;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      std::cout << "GetReply RPC failed" << std::endl;
    }
  }  

 private:
  std::unique_ptr<Greeter::Stub> stub_;
};

void PrintMessage(const MessageReply& message) {
  // タイムスタンプを日付表示に変換
  char date[64];
  time_t t = message.date();
  strftime(date, sizeof(date), "(%H:%M)", localtime(&t));

  // 表示メッセージを作成
  std::string text = date + message.name() + ": " + message.message();

  std::cout << text << std::endl;
}
bool ThreadFuncEndFlag = false;

void ThreadFunc(GreeterClient* greeter) {
  std::vector<MessageReply> messageList;
  int date;
  time_t t = time(NULL);
  date = t;

  // メッセージの受信
  while(!ThreadFuncEndFlag) {
    // 一番最後に受信したメッセージのタイムスタンプを取得
    if(messageList.size() == 0) {
      greeter->GetReply(date, &messageList);
    } else {
      int date = messageList.back().date();
      greeter->GetReply(date, &messageList);
    }
    sleep(1);
  }
}

void exitMethod(std::thread* th1) {
  ThreadFuncEndFlag = true;
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

  // ユーザーネームを入れる
  std::string username;
  std::cout << "username: ";
  std::cin >> username;
  std::string name(username);
  std::string message;
  int date = 1;
  
  // メッセージ受信開始
  std::thread th1(ThreadFunc, &greeter);

  while(1){
    // メッセージ送信
    std::cin >> message;
    if(message == "exit"){
      exitMethod(&th1);
    }
    greeter.SendRequest(date, name, message);
    // std::cout << greeter.GetReply(date) << std::endl;
  }
  return 0;
}
