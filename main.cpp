// VISA-Local-Daemon.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <sstream>
#include <thread>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocketServer.h>
#include <ixwebsocket/IXHttpServer.h>

int main()
{
  ix::initNetSystem();

  std::thread *thread1 = new std::thread([]() {
    ix::HttpServer server(8000, "127.0.0.1");

    auto res = server.listen();
    if (!res.first)
    {
      std::cerr << res.second << std::endl;
      return 1;
    }

    server.setOnConnectionCallback(
        [&server](ix::HttpRequestPtr request,
                  std::shared_ptr<ix::ConnectionState>) {
          // Build a string for the response
          std::stringstream ss;
          ss << request->method
             << " "
             << request->uri;

          std::string content = ss.str();

          return std::make_shared<ix::HttpResponse>(200, "OK",
                                                    ix::HttpErrorCode::Ok,
                                                    ix::WebSocketHttpHeaders(),
                                                    content);
        });

    server.start();
    server.wait();

    return 0;
  });

  std::thread *thread2 = new std::thread([]() {
    ix::WebSocketServer server(8080);

    server.setOnConnectionCallback(
        [&server](std::shared_ptr<ix::WebSocket> webSocket,
                  std::shared_ptr<ix::ConnectionState> connectionState) {
          webSocket->setOnMessageCallback(
              [webSocket, connectionState, &server](const ix::WebSocketMessagePtr &msg) {
                if (msg->type == ix::WebSocketMessageType::Open)
                {
                  std::cerr << "New connection" << std::endl;

                  // A connection state object is available, and has a default id
                  // You can subclass ConnectionState and pass an alternate factory
                  // to override it. It is useful if you want to store custom
                  // attributes per connection (authenticated bool flag, attributes, etc...)
                  std::cerr << "id: " << connectionState->getId() << std::endl;

                  // The uri the client did connect to.
                  std::cerr << "Uri: " << msg->openInfo.uri << std::endl;

                  std::cerr << "Headers:" << std::endl;
                  for (auto it : msg->openInfo.headers)
                  {
                    std::cerr << it.first << ": " << it.second << std::endl;
                  }
                }
                else if (msg->type == ix::WebSocketMessageType::Message)
                {
                  // For an echo server, we just send back to the client whatever was received by the server
                  // All connected clients are available in an std::set. See the broadcast cpp example.
                  // Second parameter tells whether we are sending the message in binary or text mode.
                  // Here we send it in the same mode as it was received.
                  webSocket->send(msg->str, msg->binary);
                }
              });
        });

    auto res = server.listen();
    if (!res.first)
    {
      std::cout << res.second;
      // Error handling
      return 1;
    }

    // Run the server in the background. Server can be stoped by calling server.stop()
    server.start();

    // Block until server.stop() is called.
    server.wait();

    return 0;
  });

  thread1->join();
  thread2->join();
  std::cout << "Hello World!\n";

  // getchar();
  return 0;
}
