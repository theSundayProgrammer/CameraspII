#pragma once
#include <string>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <stack>
class smtp_client
{
enum
{
  max_length = 1024
};
public:
  smtp_client(asio::io_service &io_service,
         asio::ssl::context &context);
  void send(asio::ip::tcp::resolver::iterator endpoint_iterator);
  void add_attachment(std::string const& name, std::string const& content);

  private: 
  void send();
  void handle_connect(const asio::error_code &error);
  void on_read_no_reply(void (smtp_client::*write_handle)(),std::string const &request_);
  void on_read(void (smtp_client::*write_handle)(),std::string const &request_);
  void on_write(void (smtp_client::*read_handle)());
  void handle_handshake(const asio::error_code &error);
  void handle_HELO();
  void handle_AUTH();
  void handle_UID();
  void handle_PWD();
  void handle_from();
  void handle_recpt();
  void handle_data();
  void handle_quit0();
  void handle_quit1();
  void handle_quit();
  void handle_finish();
  void handle_file();
  void handle_file_open();
  bool verify_certificate(bool preverified,asio::ssl::verify_context &ctx);
  
public:
  std::string server;
  std::string uid, pwd;
  std::string from, to;
  std::string subject;
  std::string message;

 
private:
  struct attachment { 
    attachment(std::string const& n, std::string const& c):
    filename(n), content(c){}
    std::string filename, content;
    };
  asio::ssl::stream<asio::ip::tcp::socket> socket_;
  asio::ip::tcp::resolver::iterator endpoint;
  char request_[max_length];
  char reply_[max_length];
  const char *boundary = "pj+EhsWuSQJxx7ps";
  size_t file_pos;
  std::stack<attachment> attachments;
};
