#pragma once
#include <vector>
#include <fstream>
#include <iomanip>
#include <string>
#include <curl/curl.h>
class smtp_client
{
  public:
    std::string user, password;
    std::string url;
    std::string from;
    std::string file_name;
    std::ifstream ifs;    
    std::string subject;
    std::vector<std::string> recipient_ids;
    smtp_client();
    CURLcode send();
    ~smtp_client();
  private:
    std::vector<std::string>::iterator start,finish;
    std::vector<std::string> message;
    CURL *curl;
    int call_back_state;

    size_t next_line(void *ptr);
    static size_t callback(void *ptr, size_t size, size_t nmemb, void *userp);
};
void handle_motion(const char* fName, smtp_client& smtp);
