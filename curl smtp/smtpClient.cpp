
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <sstream> // stringstream
#include <fstream> // stringstream
#include <iomanip> // put_time

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

std::string current_date_time()
{
    auto now = std::chrono::system_clock::now(); // system time
    auto in_time_t = std::chrono::system_clock::to_time_t(now); //convert to std::time_t

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%a, %b %d %Y %X %z");
    return ss.str();
}


using std::string;

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
      string date ( "Date: ");
      message.push_back(date + current_date_time() + "\r\n");
      string to = "To: ";
      to += *recipient_ids.begin() + "\r\n";
      message.push_back(to);

      message.push_back(string("From: ") + from + "\r\n");


      boost::uuids::random_generator gen;
      boost::uuids::uuid u = gen();
      message.push_back(string("Message-ID: ") + "<" + to_string(u) + "@theSundayProgrammer.com>\r\n");

      message.push_back(string("Subject :") + subject + "\r\n");
      //message.push_back(string("\r\n"));
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
      }
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
    std::vector<string> message;
    CURL *curl;
    static size_t callback(void *ptr, size_t size, size_t nmemb, void *userp)
    {
      smtp_client *this_= static_cast<smtp_client*>(userp);

      if (this_->start == this_->finish)
	return 0; 
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
  std::cout << "Password:";
  std::cin >> smtp.password;
  smtp.user= "joe.mariadassou@gmail.com";
  smtp.url="smtp://smtp.gmail.com:587";
  smtp.from = "<joe.mariadassou@gmail.com> Chakra";
  smtp.subject = "Test Message";
  string s;
  std::ifstream ifs("content.txt");
  while(getline(ifs,s,'\n'))
    smtp.body += s + "\r\n";
   smtp.recipient_ids.push_back(TO);
   smtp.recipient_ids.push_back(CC);
  smtp.send();

  return (int)res;
}



/*****************************

From: steve_ho...@hotmail.com
To: u...@domain.com
Subject: Test Message
Date: Mon, 02 Apr 2012 11:00:00 +0100
Content-Type: multipart/mixed; boundary="pj+EhsWuSQJxx7pr"
Mime-version: 1.0

This is a multi-part message in MIME format.

--pj+EhsWuSQJxx7pr
Content-Type: text/plain; charset="us-ascii"
Content-Transfer-Encoding: quoted-printable

Hello

--pj+EhsWuSQJxx7pr
Content-Type: application/octet-stream;
        name="Test.txt"
Content-Transfer-Encoding: base64
Content-Description: Test.txt
Content-Disposition: attachment;
        filename="Test.txt"; size=400;
        creation-date="Mon, 02 Apr 2012 10:00:00 GMT";
        modification-date="Mon, 02 Apr 2012 10:00:00 GMT"

[base64 encoding text split across multiple lines, each being 76 characters
long]

--pj+EhsWuSQJxx7pr--


*********************************************/
