/***************************************************************************
 *
 * Copyright (C) 1998 - 2016, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 *
 ***************************************************************************/

/* <DESC>
 * SMTP example using TLS
 * </DESC>
 */

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
 /* Note that this example requires libcurl 7.20.0 or above.
 */

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>



struct upload_status {
  std::vector<std::string>::iterator start,finish;
};

    using std::string;
static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
  struct upload_status *upload_ctx = (struct upload_status *)userp;

  if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
    return 0;
  }
  if (upload_ctx->start == upload_ctx->finish)
    return 0; 
  size_t len =upload_ctx->start->length();
  memcpy(ptr, upload_ctx->start->data() , len);
  upload_ctx->start++;
  return len;

}

class smtp_client
{
  public:
    smtp_client()
    { 
      curl = curl_easy_init();
      if (curl == nullptr)
	throw std::runtime_error("Cannot initialise curl");
    }
    string user, password;
    string url;
    string from;
    string body;
    string subject;
    std::vector<string> recipient_ids;
    CURLcode send()
    {
      string date ( "Date: Wed, 30 May 2017 08:54:29 +1100\r\n");
      message.push_back(date);
      string to = "To: ";
      to += *recipient_ids.begin() + "\r\n";
      message.push_back(to);

      std::cerr << "OK Here " << __LINE__ << std::endl;
      message.push_back(string("From: ") + from + "\r\n");


      boost::uuids::random_generator gen;
      boost::uuids::uuid u = gen();
      message.push_back(string("Message-ID: ") + "<" + to_string(u) + "@theSundayProgrammer.com>\r\n");

      message.push_back(string("Subject :") + subject + "\r\n");
      message.push_back(string("\r\n"));
      message.push_back(body);
      message.push_back(string("\r\n"));

      curl_easy_setopt(curl, CURLOPT_USERNAME, user.c_str());
      curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
      curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

      curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str() );


      curl_slist *recipients = nullptr;
      for(auto const& str: recipient_ids)
{
	recipients = curl_slist_append(recipients, str.c_str());
      std::cerr <<str<< std::endl;
}
      curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

      std::cerr << "OK Here " << __LINE__ << std::endl;
      curl_easy_setopt(curl, CURLOPT_READFUNCTION, smtp_client::callback);
      curl_easy_setopt(curl, CURLOPT_READDATA, this);
      curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

      std::cerr << "OK Here " << __LINE__ << std::endl;
      start = message.begin();
      finish = message.end();
      CURLcode res = curl_easy_perform(curl);

      std::cerr << "OK Here " << __LINE__ << std::endl;
      if (res != CURLE_OK)
	fprintf(stderr, "curl_easy_perform() failed: %s\n",
	    curl_easy_strerror(res));
      curl_slist_free_all(recipients);

      std::cerr << "OK Here " << __LINE__ << std::endl;
    }
    ~smtp_client()
    {
      curl_easy_cleanup(curl);
    }
  private:
    std::vector<std::string>::iterator start,finish;
    std::vector<string> message;
    CURL *curl;
    static size_t callback(void *ptr, size_t size, size_t nmemb, void *userp)
    {
      smtp_client *this_= static_cast<smtp_client*>(userp);

      if (this_->start == this_->finish)
	return 0; 
      //std::cerr << *(this_->start);
      if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
	return 0;
      }
      size_t len =this_->start->length();
      memcpy(ptr, this_->start->data() , len);
      this_->start++;
      return len;

    }
};
    
#define TO      "<theSundayProgrammer@gmail.com>"
#define CC      "<joseph.mariadassou@outlook.com>"

int main(void)
{
  CURLcode res = CURLE_OK;
  smtp_client smtp; 

  smtp.user= "joe.mariadassou@gmail.com";
  smtp.password =  "954Vnrtz";
  smtp.url="smtp://smtp.gmail.com:587";
  smtp.from = "<joe.mariadassou@gmail.com> Chakra";
  smtp.subject = "Test Message";
  smtp.body = "This is test message\r\n" 
  "It could be a lot of lines, could be MIME encoded, whatever.\r\n"
  "Check RFC5322.\r\n";
   smtp.recipient_ids.push_back(TO);
   smtp.recipient_ids.push_back(CC);
      std::cerr << "OK Here " << __LINE__ << std::endl;
  smtp.send();

  return (int)res;
}
