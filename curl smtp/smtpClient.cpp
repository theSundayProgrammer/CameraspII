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

#define FROM    "<joe.mariadassou@gmail.com>"
#define TO      "<theSundayProgrammer@gmail.com>"
#define CC      "<joseph.mariadassou@outlook.com>"


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

static const char *payload_text[] = {
"Date: Wed, 30 May 2017 08:54:29 +1100\r\n",
  "To: " TO "\r\n",
  "From: " FROM "Chakra\r\n",
  "Message-ID: <dcd7cb3r9-11db-487a-9f3a-e652a9458efd@"
  "theSundayProgrammer:.org>\r\n",
  "Subject: SMTP TLS message\r\n",
  "\r\n", /* empty line to divide headers from body, see RFC5322 */
  "The body of the message starts here.\r\n",
  "\r\n",
  "It could be a lot of lines, could be MIME encoded, whatever.\r\n",
  "Check RFC5322.\r\n"
};
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
    std::vector<string> message;
    CURL *curl;
    CURLcode send()
    {
      curl_easy_setopt(curl, CURLOPT_USERNAME, "joe.mariadassou@gmail.com");
      curl_easy_setopt(curl, CURLOPT_PASSWORD, "954Vnrtz");
      curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.gmail.com:587");
      curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
      curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

      curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM);

      curl_slist *recipients = curl_slist_append(nullptr, TO);
      recipients = curl_slist_append(recipients, CC);
      curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

      curl_easy_setopt(curl, CURLOPT_READFUNCTION, smtp_client::callback);
      curl_easy_setopt(curl, CURLOPT_READDATA, this);
      curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

      start = message.begin();
      finish = message.end();
      CURLcode res = curl_easy_perform(curl);

      if (res != CURLE_OK)
	fprintf(stderr, "curl_easy_perform() failed: %s\n",
	    curl_easy_strerror(res));
      curl_slist_free_all(recipients);

    }
    ~smtp_client()
    {
      curl_easy_cleanup(curl);
    }
  private:
    std::vector<std::string>::iterator start,finish;
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
    

int main(void)
{
  CURLcode res = CURLE_OK;
  smtp_client smtp; 

  for(auto it= std::begin(payload_text); it!=std::end(payload_text); ++it)
    smtp.message.push_back(*it);  
  smtp.send();
  return (int)res;
}
