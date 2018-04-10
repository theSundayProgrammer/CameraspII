#include <string>

#include <camerasp/smtp_client.hpp>
#include <camerasp/logger.hpp>
#include <fstream>

#include <iostream>
std::shared_ptr<spdlog::logger> console;
int main(int argc, char *argv[])
{
  try
  {
    if (argc != 5)
    {
      std::cerr << "Usage: client <host> <port> <uid> <pwd>\n";
      return 1;
    }
    console = spdlog::stdout_color_mt("smtp_logger");
    console->set_level(spdlog::level::debug);
    asio::ssl::context ctx(asio::ssl::context::sslv23);
    ctx.load_verify_file("cacert.pem");

    asio::io_service io_service;
    asio::ip::tcp::resolver resolver(io_service);
    asio::ip::tcp::resolver::query query(argv[1], argv[2]);
    asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

    smtp_client c(io_service, ctx);
    c.set_uid(argv[3]);
    c.set_pwd(argv[4]);
    c.set_server("norwestcomputing.com.au");
    c.set_to("joseph.mariadassou@outlook.com");
    c.set_from("theSundayProgrammer@gmail.com");
    c.set_subject("Motion Detected");
    c.set_message("This is the body of the message.");
    std::ostringstream ostr;
    std::ifstream ifs;
    ifs.open("mail.txt", std::ios::binary);
    ostr << ifs.rdbuf();
    for (int idx = 0; idx < 3; ++idx)
    {
      c.add_attachment(std::string("mail") + std::to_string(idx) + ".txt", ostr.str());
    }
    c.send(iterator);
    ifs.close();
    io_service.run();
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
