// Copyright 2020 Your Name <your_email>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/thread/pthread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>
#include <header.hpp>
#include <iostream>

using boost::asio::io_service;
using boost::asio::ip::tcp;

boost::recursive_mutex mutex;
static io_service service;

class talk_to_client {
 private:
  tcp::socket socket;
  std::chrono::system_clock::time_point create_time;
  std::string name;

 public:
  std::string& GetName() { return name; }

  void SetName(std::string& nm) { name = nm; }

  tcp::socket& GetSocket() { return socket; }

  bool CheckTime() {
    if ((std::chrono::system_clock::now() - create_time).count() >= 5)
      return true;
    return false;
  }

  talk_to_client()
      : socket(service), create_time(std::chrono::system_clock::now()) {}
};

std::vector<std::shared_ptr<talk_to_client>> clients;

std::string GetAllUsers() {
  std::string ret;
  for (auto& client : clients) {
    ret += client->GetName() + " ";
  }
  ret += '\n';
  return ret;
}

unsigned short port = 8001;

void accept_thread() {
  tcp::acceptor acceptor{service, tcp::endpoint{tcp::v4(), port}};
  while (true) {
    BOOST_LOG_TRIVIAL(info)
        << "starting wait \n Thread id: " << std::this_thread::get_id() << "\n";

    auto client = std::make_shared<talk_to_client>();
    acceptor.accept(client->GetSocket());

    boost::recursive_mutex::scoped_lock lock{mutex};
    clients.push_back(client);
  }
}

void handle_clients_thread() {
  while (true) {
    boost::this_thread::sleep(boost::posix_time::millisec(1));
    boost::recursive_mutex::scoped_lock lock{mutex};
    for (auto& client : clients) {
      if(client->GetSocket().available()) {
        std::string com;
        boost::asio::streambuf buffer;
        boost::asio::read_until(client->GetSocket(), buffer, '\n');
        std::cout<<client->GetSocket().available();
        std::istream input(&buffer);
        std::getline(input, com);
        if (com == "client_list") {
          client->GetSocket().write_some(boost::asio::buffer(GetAllUsers()));
          client->GetSocket().write_some(boost::asio::buffer("Ping Ok\n"));
          BOOST_LOG_TRIVIAL(info)
              << "Answered to client \n username: " << client->GetName()
              << "\n";
        } else if (com == "Ping") {
          client->GetSocket().write_some(boost::asio::buffer("Ping Ok\n"));
          BOOST_LOG_TRIVIAL(info)
              << "Answered to client \n username: " << client->GetName()
              << "\n";
        } else if (com.empty()) {
          client->GetSocket().write_some(
              boost::asio::buffer("logging failed\n", 20));

          BOOST_LOG_TRIVIAL(info) << "Logging failed \n Thread id: \n"
                                  << std::this_thread::get_id() << "\n";
        } else {
          client->SetName(com);
          client->GetSocket().write_some(
              (boost::asio::buffer("success logging \n")));
          BOOST_LOG_TRIVIAL(info)
              << "Success logging \n username " << com << "\n";
        }
      }

    }
  }
}

int main(int argc, char* argv[]) {
  if (argc >1) {
    try {
      port = std::stoi(argv[1]);
    } catch (std::exception& e) {
      BOOST_LOG_TRIVIAL(error) << "Error " << e.what() << ::std::endl;
      return 1;
    }
  }
  boost::thread_group threads;
  threads.create_thread(accept_thread);
  threads.create_thread(handle_clients_thread);
  threads.join_all();
  return 0;
}
