#include <camerasp/smtp_client.hpp>
#include <memory>
#include <camerasp/logger.hpp>
namespace spd = spdlog;
std::shared_ptr<spdlog::logger> console;
int main(int argc, char* argv[])
{
  console = spd::stdout_color_mt("console");
  console->set_level(spd::level::debug);
  smtp_client smtp;
  smtp.password = argv[2];
  smtp.user= argv[1];
  smtp.url = "smtps://smtp.gmail.com:465";
  smtp.from = std::string("<") + argv[1] + "> camerasp";
  smtp.subject = "Motion detection";
  smtp.recipient_ids.push_back("joseph.mariadassou@outlook.com");
  smtp.recipient_ids.push_back("parama_chakra@yahoo.com");
  if(argc>3)
  {
    smtp.file_name = argv[3];
    console->info("File name={0}", argv[3]);
  }
  smtp.send();
}
