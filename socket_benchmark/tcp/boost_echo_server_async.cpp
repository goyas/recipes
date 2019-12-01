// Asynchronous echo server.

#include <array>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "boost/asio.hpp"
#include "boost/core/ignore_unused.hpp"

using boost::asio::ip::tcp;
const int ksize = 1024 * 1024 * 16 + 32;

inline uint64_t easy_gettimeofday() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return __INT64_C(1000000) * tv.tv_sec + tv.tv_usec;
}

// -----------------------------------------------------------------------------
class Session : public std::enable_shared_from_this<Session> {
 public:
  Session(tcp::socket socket) : _socket(std::move(socket)) {
  }

  void Start() {
    SetSockOpts();
    set_socket_connected();
    trigger_receive();
  }

  void set_socket_connected() {
    _receiving_data = new char[ksize];
    _receiving_size = ksize - 32;
  }

  bool trigger_receive() {
    return try_start_receive();
  }

  bool try_start_receive() {
    async_read_some(_receiving_data, _receiving_size);
  }

  void async_read_some(char* data, size_t size) {
    _socket.async_read_some(boost::asio::buffer(data, size),
      std::bind(&Session::on_read_some, 
      shared_from_this(), std::placeholders::_1, std::placeholders::_2));
  }

  void on_read_some(const boost::system::error_code& error,
    std::size_t bytes_transferred) {
    if (error) {
      std::cout << "read error: " << error.message() << std::endl;
      return ;
    }
    
    _receiving_data += bytes_transferred;
    _receiving_size -= bytes_transferred;

    now = easy_gettimeofday();
    if (bytes_transferred != 0) {
      std::cout << "now time: " << now << " bytes_transferred: " << bytes_transferred \
        << "\t _receiving_size: " << _receiving_size << std::endl;
    }

    // trigger next receive
    try_start_receive();
  }

  bool SetSockOpts() {
    bool bSucc = true;
    tcp::no_delay option(true);
    try {
        _socket.set_option(option);
    }
    catch (std::exception& e) {
        //LOG_ERROR("set_option error %s", e.what());
        bSucc = false;
    }

    boost::asio::socket_base::send_buffer_size snd_option_sz(1024 * 16);
    try {
        _socket.set_option(snd_option_sz);
    }
    catch (std::exception& e) {
        //LOG_ERROR("set_option snd buff size error %s", e.what());
        bSucc = false;
    }

    boost::asio::socket_base::receive_buffer_size rc_option_sz(1024 * 16);
    try {
        _socket.set_option(rc_option_sz);
    }
    catch (std::exception& e) {
        //LOG_ERROR("set_option snd buff size error %s", e.what());
        bSucc = false;
    }

    //tmp for debug
    boost::asio::socket_base::send_buffer_size snd_option;
    _socket.get_option(snd_option);
    int snd_size = snd_option.value();
    //LOG_DEBUG("send buff size: %d", snd_size);
    boost::asio::socket_base::receive_buffer_size rc_option;
    _socket.get_option(rc_option);
    int rc_size = rc_option.value();
    //LOG_DEBUG("recv buff size: %d", rc_size);
    boost::asio::socket_base::send_low_watermark snd_lw_option;
    _socket.get_option(snd_lw_option);
    int snd_lw_sz = snd_lw_option.value();
    //LOG_DEBUG("send low watermark size: %d", snd_lw_sz);
    boost::asio::socket_base::receive_low_watermark rc_lw_option;
    _socket.get_option(rc_lw_option);
    int rc_lw_sz = rc_lw_option.value();
    //LOG_DEBUG("recv low watermark size: %d", rc_lw_sz);

    return bSucc;
  }
  
 private:
  tcp::socket _socket;
  char* _receiving_data; // weak ptr
  int64_t _receiving_size; 
  uint64_t now;
};
// -----------------------------------------------------------------------------

class Server {
 public:
  Server(boost::asio::io_context& io_context, std::uint16_t port)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    async_accept();
  }

 private:
  void async_accept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
          if (!ec) {
            std::cout << "recv from client: " 
              << socket.remote_endpoint().address() << std::endl;
            std::make_shared<Session>(std::move(socket))->Start();
          }
          async_accept();
        });
  }

private:
  tcp::acceptor acceptor_;
};

// -----------------------------------------------------------------------------

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
    return 1;
  }

  std::uint16_t port = std::atoi(argv[1]);

  boost::asio::io_context io_context;

  Server server(io_context, port);

  io_context.run();

  return 0;
}


