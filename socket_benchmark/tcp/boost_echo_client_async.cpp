// Asynchronous echo client.

#include <array>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <deque>

#include "boost/asio.hpp"

//#include "utility.h"  // for printing endpoints

using boost::asio::ip::tcp;
const int ksize = 1024 * 1024 * 16;

struct message {
  char *data;
  int   size;
};

inline uint64_t easy_gettimeofday() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return __INT64_C(1000000) * tv.tv_sec + tv.tv_usec;
}

// -----------------------------------------------------------------------------
class Client {
public:
  Client(boost::asio::io_context& io_context, 
    const std::string& host, const std::string& port) 
    : _socket(io_context)/*, 
    //_sending_data(NULL), 
    //_sending_size(0)*/ {
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(tcp::v4(), host, port);
    boost::asio::async_connect(
      _socket,
      endpoints,
      std::bind(&Client::on_onnect, this, std::placeholders::_1, std::placeholders::_2));
  }
    
public:
  void on_onnect(boost::system::error_code ec, tcp::endpoint endpoint) {
    if (ec) {
      std::cout << "Connect failed: " << ec.message() << std::endl;
      _socket.close();
    } else {
      char* send_buf = new char[ksize];
      if (NULL == send_buf) {
        std::cout << "malloc failed" << std::endl;
        return ;
      }
      SetSockOpts();
      message msg;
      msg.data  =   send_buf;
      msg.size  =   ksize;
      async_send_message(msg);
    }
  }
  
  void async_send_message(const message& msg) {
    put_into_pending_queue(msg);
    try_start_send();
  }

  void put_into_pending_queue(const message& msg) {
    _pending_calls.push_back(msg);
  }

  void try_start_send() {
    if (get_from_pending_queue(_sending_message)) {
      //_sending_data = _sending_message.data;
      //_sending_size = _sending_message.size;
      async_write_some(_sending_message, 0);
    }
  }

  bool get_from_pending_queue(message& msg) {
    if (_pending_calls.empty()) {
      std::cout << "Error: _pending_calls is null " << std::endl;
      return false;
    }
    
    msg = _pending_calls.front(); 
    _pending_calls.pop_front();
    return true;
  }

  void async_write_some(message& msg, uint32_t offset) {
    _socket.async_write_some(boost::asio::buffer(msg.data + offset, msg.size - offset),
      std::bind(
        &Client::on_write_some, 
        this, 
        msg,
        offset,
        std::placeholders::_1, std::placeholders::_2));
  }

  void on_write_some(message& msg,
    uint32_t offset,
    const boost::system::error_code& error,
    std::size_t bytes_transferred) {
    if (error) {
      std::cout << "write error " << error.message() << std::endl;
      return ;
    }
    
    if ((static_cast<int>(bytes_transferred) + offset) < msg.size) {
      now = easy_gettimeofday();
      std::cout << "    time: " << now << " bytes_transferred: " << bytes_transferred + offset << std::endl;
      // start sending the remaining data
      async_write_some(msg, bytes_transferred + offset); 
    } else {
      //clear_sending_env();
      now = easy_gettimeofday();
      std::cout << "end time: " << now << " bytes_transferred: " << bytes_transferred << std::endl;
      try_start_send();
    }
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
  message _sending_message; //current sending message
  std::deque<message> _pending_calls;
  //const char* _sending_data; // current sending data, weak ptr
  //int _sending_size; // size of the current sending data
  uint64_t now;
};

// -----------------------------------------------------------------------------

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " <host> <port>" << std::endl;
    return 1;
  }

  std::string host = argv[1];
  std::string port = argv[2];

  boost::asio::io_context io_context;

  Client client(io_context, host, port);

  io_context.run();

  return 0;
}

