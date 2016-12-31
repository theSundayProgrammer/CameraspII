//////////////////////////////////////////////////////////////////////////////
// Copyright (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// adapted from Kenneth Baker's via-http
//
// addpted from http://github.com/kenba/vai-http
// original copyright follows
// Copyright (c) 2014-2015 Ken Barker
// (ken dot barker at via-technology dot co dot uk)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
/// @file handler
/// @brief http handlers
//////////////////////////////////////////////////////////////////////////////
#include <via/comms/tcp_adaptor.hpp>
#include <via/http_server.hpp>
//#include <iostream>
#include <json/reader.h>
#include <camerasp/utils.hpp>
#include <camerasp/handler.hpp>
#include <camerasp/timer.hpp>
#include <camerasp/cam_still.hpp>
typedef http_server_type::http_connection_type http_connection;
typedef http_server_type::chunk_type http_chunk_type;
const std::string response_missing_file =
"<html>\r\n"
"<head><title>Camerasp </title></head>\r\n"
"<body><h2>Requested file not found</h2></body>\r\n"
"</html>\r\n";

const std::string response_command_success =
"<html>\r\n"
"<head><title>Camerasp Command Success</title></head>\r\n"
"<body><h2>Requested Command Succeeded</h2></body>\r\n"
"</html>\r\n";


const std::string response_command_fail =
"<html>\r\n"
"<head><title>Camerasp Command Fail</title></head>\r\n"
"<body><h2>Not Implementedi or incorrect parameters</h2></body>\r\n"
"</html>\r\n";

/// A string to send in responses.

//////////////////////////////////////////////////////////////////////////////
namespace camerasp
{

  Handler::Handler(periodic_frame_grabber&  timer) :
    timer_(timer) {}

  /// The stop callback function.
  /// Closes the server and all it's connections leaving io_service.run
  /// with no more work to do.
  /// Called whenever a SIGINT, SIGTERM or SIGQUIT signal is received.
  void Handler::stop(ASIO_ERROR_CODE const&, // error,
                   int, // signal_number,
                   http_server_type& http_server)
  {
    console->debug("Shutting down");
    http_server.shutdown();
  }

  void Handler::handle_image(
    http_connection::shared_pointer connection,
    via::http::rx_request const& request,
      url_parser& parser)
  {
        int k = 0;
        for (auto kv :  parser.queries)
          console->debug("{0} = {1}", kv.first, kv.second);
        auto kv = parser.queries.begin();
        if (kv != parser.queries.end() &&  kv->first == "prev") {
          if (kv->second.empty() || !is_integer(kv->second, &k)) {
            k = 0;
          }
        }
        auto resp = get_image(k);
        connection->send(std::move(resp.first), std::move(resp.second));
  }

  void Handler::handle_hello(
    http_connection::shared_pointer connection,
    via::http::rx_request const& request,
      url_parser& parser)
  {
const std::string response_body
(std::string("<html>\r\n") +
  std::string("<head><title>Accepted</title></head>\r\n") +
  std::string("<body><h1>200 Accepted</h1></body>\r\n") +
  std::string("</html>\r\n"));
    via::http::tx_response response(via::http::response_status::code::OK);
      // add the server and date headers
      response.add_server_header();
      response.add_date_header();
    if ((request.method() == "GET") )
    {
      response.set_status(via::http::response_status::code::OK);
      // send the body in an unbuffered response i.e. in ConstBuffers
      // ok because the response_body is persistent data
      connection->send(std::move(response),
        via::comms::ConstBuffers(1, asio::buffer(response_body)));
    }
    else if (request.method() == "HEAD")
    {
      response.set_status(via::http::response_status::code::OK);
      connection->send(std::move(response));
    }
    else
    {
      response.set_status(via::http::response_status::code::METHOD_NOT_ALLOWED);
      response.add_header(via::http::header_field::id::ALLOW, "GET, HEAD");
      connection->send(std::move(response));
    }
  }

  /// A function to send a response to a request.
  void Handler::respond_to_request(http_connection::weak_pointer weak_ptr)
  {
    http_connection::shared_pointer connection(weak_ptr.lock());
    if (connection)
    {
      // Get the last request on this connection.
      via::http::rx_request const& request(connection->request());

      console->debug("Request: {0}", request.uri());
      url_parser parser(request.uri());
      console->debug("Parsed request: {0}", parser.command);
      if (parser.command == "/hello")
      {
        this->handle_hello(connection, request, parser );
      }
      else if (parser.command == "/image")
      {
        this->handle_image(connection, request, parser );
      }
      else if (parser.command == "/flip")
      {
          auto kv = parser.queries.begin();
          bool set_val = kv->second == "true";
          if (kv->first == "horizontal") {
            timer_.set_horizontal_flip(set_val);
            send_standard_response(connection, response_command_success);
          }
          else if (kv->first == "vertical")
          {
            timer_.set_vertical_flip(set_val);
            send_standard_response(connection, response_command_success);
          }
          else
           {
            send_standard_response(connection, response_command_fail);
          }
      }
      else
      {
        send_standard_response(connection, response_command_fail);
      }
    }
      else
        console->error("Failed to lock http_connection::weak_pointer");
    }
  
  /// The handler for incoming HTTP requests.
  /// Prints the request and determines whether the request is chunked.
  /// If not, it responds with a 200 OK response with some HTML in the body.
  void Handler::request(http_connection::weak_pointer weak_ptr,
                       via::http::rx_request const& request,
                       std::string const& body)
  {
    console->debug ( "Rx request: {0}" , request.to_string());
    console->debug ("Rx headers: {0}", request.headers().to_string());
    console->debug("Rx body: {0}", body);

    // Don't respond to chunked requests until the last chunk is received
    if (!request.is_chunked())
      respond_to_request(weak_ptr);
  }

  /// The handler for incoming HTTP chunks.
  /// Outputs the chunk header and body to console->debug.
  void Handler::chunk(http_connection::weak_pointer weak_ptr,
                     http_chunk_type const& chunk,
                     std::string const& data)
  {
    if (chunk.is_last())
    {
      console->debug("Rx chunk is last, extension: {0}"  " trailers: {1}", chunk.extension(), chunk.trailers().to_string());

      // Only send a response to the last chunk.
      respond_to_request(weak_ptr);
    }
    else
      console->debug("Rx chunk, size: {0} "  " data: {1}", chunk.size() , data);
  }

  /// A handler for HTTP requests containing an "Expect: 100-continue" header.
  /// Outputs the request and determines whether the request is too big.
  /// It either responds with a 100 CONTINUE or a 413 REQUEST_ENTITY_TOO_LARGE
  /// response.
  void Handler::expect_continue(http_connection::weak_pointer weak_ptr,
                               via::http::rx_request const& request,
                               std::string const& /* body */)
  {
    static const auto MAX_LENGTH(1024);

    console->debug ( "expect_continue_handler\n");
    console->debug ( "rx request: {0}" , request.to_string());
    console->debug("rx headers: {0}", request.headers().to_string());

    // Reject the message if it's too big, otherwise continue
    via::http::tx_response response((request.content_length() > MAX_LENGTH) ?
                       via::http::response_status::code::PAYLOAD_TOO_LARGE :
                       via::http::response_status::code::CONTINUE);

    http_connection::shared_pointer connection(weak_ptr.lock());
    if (connection)
      connection->send(std::move(response));
    else
      console->error("Failed to lock http_connection::weak_pointer");
  }

  /// A handler for the signal sent when an invalid HTTP mesasge is received.
  void Handler::invalid_request(http_connection::weak_pointer weak_ptr,
                               via::http::rx_request const&, // request,
                               std::string const& /* body */)
  {
    console->debug("Invalid request from ");
    http_connection::shared_pointer connection(weak_ptr.lock());
    if (connection)
    {
      console->debug("\tremote address {0}", weak_ptr.lock()->remote_address());
      // Send the default response
      connection->send_response();
      // Disconnect the client
      connection->disconnect();
    }
    else
      console->error("\tFailed to lock http_connection::weak_pointer");
  }

  /// A handler for the signal sent when an HTTP socket is connected.
  void Handler::connected(http_connection::weak_pointer weak_ptr)
  {
    console->debug("Connected: {0}", weak_ptr.lock()->remote_address());
  }

  /// A handler for the signal sent when an HTTP socket is disconnected.
  void Handler::disconnected(http_connection::weak_pointer weak_ptr)
  {
    console->debug("Disconnected: {0}", weak_ptr.lock()->remote_address());
  }

  /// A handler for the signal when a message is sent.
  void Handler::message_sent(http_connection::weak_pointer) // weak_ptr)
  {
    console->debug("response sent");
  }
  void Handler::send_standard_response(
    http_connection::shared_pointer connection,
    std::string const& str) {
    via::http::tx_response response(via::http::response_status::code::OK);
    response.add_server_header();
    response.add_date_header();
    response.add_header("Content-Type", "text/html");
    response.add_header("charset", "utf-8");
    response.add_content_length_header(str.size());
    connection->send(response, str);
  }
  std::pair<via::http::tx_response, std::string>
    Handler::get_image(int k)
  {
    // output the request
    via::http::tx_response response(via::http::response_status::code::OK);
    response.add_server_header();
    response.add_date_header();
    console->info("image number={0}", k);
    response.add_header("Content-Type", "image/jpeg");
    auto responsebody = timer_.get_image(k);
    response.add_content_length_header(responsebody.size());
    
    return std::make_pair(response, responsebody);
  }


}
