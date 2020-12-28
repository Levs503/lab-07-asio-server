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

void accept_thread() {
  tcp::acceptor acceptor{service, tcp::endpoint{tcp::v4(), 8001}};
  while (true) {
    BOOST_LOG_TRIVIAL(info)
        << "starting \n Thread id: " << std::this_thread::get_id() << "\n";

    auto client = std::make_shared<talk_to_client>();
    std::cout << "READDY" << std::endl;
    acceptor.accept(client->GetSocket());
    BOOST_LOG_TRIVIAL(info) << "Logging failed \n Thread id: \n"
                            << std::this_thread::get_id() << "\n";
    std::string name;
    client->GetSocket().read_some(boost::asio::buffer(name));
    if (name.empty()) {
      client->GetSocket().write_some(
          boost::asio::buffer("logging failed\n", 20));

      BOOST_LOG_TRIVIAL(info) << "Logging failed \n Thread id: \n"
                              << std::this_thread::get_id() << "\n";
    } else {
      client->SetName(name);
      client->GetSocket().write_some(
          (boost::asio::buffer("success logging \n")));
      BOOST_LOG_TRIVIAL(info) << "Success logging \n username \n"
                              << name << "\n";
    }
    boost::recursive_mutex::scoped_lock lock{mutex};
    clients.push_back(client);
  }
}

void handle_clients_thread() {
  while (true) {
    boost::this_thread::sleep(boost::posix_time::millisec(1));
    boost::recursive_mutex::scoped_lock lock{mutex};
    for (auto& client : clients) {
      std::string com;
      client->GetSocket().read_some(boost::asio::buffer(com, 256));
      if (com == "client_list_chaned") {
        client->GetSocket().write_some(boost::asio::buffer(GetAllUsers()));
      }
      client->GetSocket().write_some(boost::asio::buffer("Ping Ok\n"));
      BOOST_LOG_TRIVIAL(info)
          << "Answered to client \n username: " << client->GetName() << "\n";
    }
  }
}

int main(/*int argc, char* argv[]*/) {
  boost::thread_group threads;
  threads.create_thread(accept_thread);
  threads.create_thread(handle_clients_thread);
  threads.join_all();
}