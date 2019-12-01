#include <cstdlib>  
#include <iostream>  
#include <string>  
#include <boost/bind.hpp>  
#include <boost/asio.hpp>  
#include <boost/asio/ssl.hpp>  

const int ksize = 1024 * 1024 * 16 + 32;

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;  

inline uint64_t easy_gettimeofday() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return __INT64_C(1000000) * tv.tv_sec + tv.tv_usec;
}

class Session {  
public:  
  Session(boost::asio::io_service& io_service, 
    boost::asio::ssl::context& context)  
    : socket_(io_service, context) {  
  }  
  
  ssl_socket::lowest_layer_type& socket() {  
    return socket_.lowest_layer();  
  }  
  
  void start() {  
    set_socket_connected();
    trigger_receive();
  }  

  void set_socket_connected() {
    _receiving_data = new char[ksize];
    _receiving_size = ksize - 32;
  }

  void trigger_receive() {
    handle_handshake();
  }

  void handle_handshake() {
    //async_read_some(_receiving_data, _receiving_size);
    socket_.async_handshake(
      boost::asio::ssl::stream_base::server,  
      boost::bind(&Session::try_start_receive, this, boost::asio::placeholders::error)
    );  
  }
  
  void try_start_receive(const boost::system::error_code& error) {  
    if (!error) {  
      socket_.async_read_some(
        boost::asio::buffer(_receiving_data, _receiving_size),  
        boost::bind(&Session::on_read, this,  
        boost::asio::placeholders::error,  
        boost::asio::placeholders::bytes_transferred));  
    } else {  
      delete this;  
    }  
  }  
  
  void on_read(const boost::system::error_code& error,  
    size_t bytes_transferred) {  
    if (!error) {  
      /*
      std::cout <<"read: " << std::string(data_, bytes_transferred) << std::endl;  
      boost::asio::async_write(
        socket_,  
        boost::asio::buffer(data_, bytes_transferred),  
        boost::bind(&Session::on_write, this, boost::asio::placeholders::error));
        */
      _receiving_data += bytes_transferred;
      _receiving_size -= bytes_transferred;

      now = easy_gettimeofday();
      if (bytes_transferred != 0) {
        std::cout << "now time: " << now << " bytes_transferred: " << bytes_transferred \
          << "\t _receiving_size: " << _receiving_size << std::endl;
      }

      // trigger next receive
      try_start_receive(error);
    } else {  
      std::cout << "read error: " << error.message() << std::endl;
      delete this;  
    }  
  }  
  
  /*
  void on_write(const boost::system::error_code& error) {  
    if (!error) {  
      socket_.async_read_some(
        boost::asio::buffer(data_, ksize),  
        boost::bind(&Session::on_read, this,  
        boost::asio::placeholders::error,  
        boost::asio::placeholders::bytes_transferred));  
    } else {  
      delete this;  
    }  
  }
  */
  
private:  
  ssl_socket socket_;  
  char*   _receiving_data; // weak ptr
  int64_t _receiving_size; 
  uint64_t now;
};  
  
class Server {  
public:  
  Server(boost::asio::io_service& io_service, unsigned short port)  
    : io_service_(io_service), 
    acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),  
    context_(boost::asio::ssl::context::sslv23) {  
    context_.set_options(
      boost::asio::ssl::context::default_workarounds  
        | boost::asio::ssl::context::no_sslv2  
        | boost::asio::ssl::context::single_dh_use);  
    
    context_.set_password_callback(boost::bind(&Server::get_password, this));  
    context_.use_certificate_chain_file("server.pem");  
    context_.use_private_key_file("server.pem", boost::asio::ssl::context::pem);  
    //context_.use_tmp_dh_file("dh512.pem");  
  
    start_accept();  
  }  
  
  std::string get_password() const {  
    return "test";  
  }  
  
  void start_accept() {  
    Session* new_session = new Session(io_service_, context_);  
    acceptor_.async_accept(
      new_session->socket(),  
      boost::bind(&Server::on_accept, this, new_session, boost::asio::placeholders::error)
    );  
  }  
  
  void on_accept(Session* new_session,  
    const boost::system::error_code& error) {  
    if (!error) {  
      new_session->start();  
    } else {  
      delete new_session;  
    }  
  
    start_accept();  
  }  
  
private:  
  boost::asio::io_service& io_service_;  
  boost::asio::ip::tcp::acceptor acceptor_;  
  boost::asio::ssl::context context_;  
};  
  
int main(int argc, char* argv[]) {  
  try {  
    if (argc != 2) {  
      std::cerr << "Usage: server <port>\n";  
      return 1;  
    }  
  
    boost::asio::io_service io_service;  
  
    using namespace std;
    Server s(io_service, atoi(argv[1]));  
    io_service.run();  
  } catch (std::exception& e) {  
    std::cerr << "Exception: " << e.what() << "\n";  
  }  
  
  return 0;  
}  

