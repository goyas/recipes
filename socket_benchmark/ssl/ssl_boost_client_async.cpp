#include <cstdlib>  
#include <iostream>  
#include <boost/bind.hpp>  
#include <boost/asio.hpp>  
#include <boost/asio/ssl.hpp>  
#include <deque>

//enum { max_length = 1024 };  
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

class Client {  
public:  
  Client(boost::asio::io_service& io_service,  
    boost::asio::ssl::context& context,  
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator)  
    : socket_(io_service, context) {  
    socket_.set_verify_mode(boost::asio::ssl::verify_peer);  
    socket_.set_verify_callback(  
      boost::bind(&Client::verify_certificate, this, _1, _2));  
  
    boost::asio::async_connect(
      socket_.lowest_layer(), 
      endpoint_iterator,  
      boost::bind(&Client::on_connect, this, boost::asio::placeholders::error)
    );  
  }  
  
  bool verify_certificate(bool preverified, 
    boost::asio::ssl::verify_context& ctx) {  
    // The verify callback can be used to check whether the certificate that is  
    // being presented is valid for the peer. For example, RFC 2818 describes  
    // the steps involved in doing this for HTTPS. Consult the OpenSSL  
    // documentation for more details. Note that the callback is called once  
    // for each certificate in the certificate chain, starting from the root  
    // certificate authority.  
  
    // In this example we will simply print the certificate's subject name.  
    char subject_name[256];  
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());  
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);  
    std::cout << "Verifying " << subject_name << "\n";  
  
    // return preverified;  
    return true;
  }  
  
  void on_connect(const boost::system::error_code& error) {  
    if (!error) {  
      socket_.async_handshake(
        boost::asio::ssl::stream_base::client,  
        boost::bind(&Client::on_handshake, this, boost::asio::placeholders::error)
      );  
    } else {  
      std::cout << "Connect failed: " << error.message() << "\n";  
    }  
  }  
  
  void on_handshake(const boost::system::error_code& error) { 
    if (error) {
      std::cout << "Handshake failed: " << error.message() << "\n";  
      return ;
    } else {  
      char* send_buf = new char[ksize];
      if (NULL == send_buf) {
        std::cout << "malloc failed" << std::endl;
        return ;
      }
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
    socket_.async_write_some( 
        boost::asio::buffer(msg.data + offset, msg.size - offset),  
        boost::bind(
          &Client::on_write, 
          this,  
          msg,
          offset,
          boost::asio::placeholders::error,  
          boost::asio::placeholders::bytes_transferred)
      );  
  }
  
  void on_write(message& msg,
    uint32_t offset,
    const boost::system::error_code& error,  
    size_t bytes_transferred) {  
    if (error) {
      std::cout << "write error: " << error.message() << std::endl;
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
  
private:  
  boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
  message _sending_message; //current sending message
  std::deque<message> _pending_calls;
  uint64_t now; 
};  
  
int main(int argc, char* argv[]) {  
  try {  
    if (argc != 3) {  
      std::cerr << "Usage: client <host> <port>\n";  
      return 1;  
    }  
  
    boost::asio::io_service io_service;  
  
    boost::asio::ip::tcp::resolver resolver(io_service);  
    boost::asio::ip::tcp::resolver::query query(argv[1], argv[2]);  
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);  
  
    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);  
    ctx.load_verify_file("ca.pem");  
  
    Client c(io_service, ctx, iterator);  
    io_service.run();
  } catch (std::exception& e) {  
    std::cerr << "Exception: " << e.what() << "\n";  
  }  
  
  return 0;  
}  

