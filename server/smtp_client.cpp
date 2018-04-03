#include <camerasp/smtp_client.hpp>
#include <curl/curl.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <chrono>	// chrono::system_clock
#include <ctime>	 // localtime
#include <sstream> // stringstream
#include <fstream> // stringstream
#include <iomanip> // put_time

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <sstream> // stringstream
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <camerasp/logger.hpp>

std::string current_GMT_time()
{
	auto now = std::chrono::system_clock::now();								// system time
	auto in_time_t = std::chrono::system_clock::to_time_t(now); //convert to std::time_t

	std::stringstream ss;
	ss << std::put_time(std::gmtime(&in_time_t), "%a, %b %d %Y %X GMT");
	return ss.str();
}

std::string current_date_time()
{
	auto now = std::chrono::system_clock::now();								// system time
	auto in_time_t = std::chrono::system_clock::to_time_t(now); //convert to std::time_t

	std::stringstream ss;
	ss << std::put_time(std::localtime(&in_time_t), "%a, %b %d %Y %X %z");
	return ss.str();
}

using namespace boost::archive::iterators;
smtp_client::smtp_client()
{

}
CURLcode smtp_client::send()
{
	curl = curl_easy_init();
	if (curl == nullptr)
		throw std::runtime_error("Cannot initialise curl");
	std::string date("Date: ");
	message.push_back(date + current_date_time() + "\r\n");
	std::string to = "To:";
	to += *recipient_ids.begin() + "\r\n";
	message.push_back(to);

	message.push_back(std::string("From:") + from + "\r\n");

	boost::uuids::random_generator gen;
	boost::uuids::uuid u = gen();
	message.push_back(std::string("Message-ID:") + "<" + to_string(u) + "@theSundayProgrammer.com>\r\n");

	message.push_back(std::string("Subject:") + subject + "\r\n");
	//message.push_back(body);
	//message.push_back(string("\r\n"));

	curl_easy_setopt(curl, CURLOPT_USERNAME, user.c_str());
	curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
	curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

	curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str());

	curl_slist *recipients = nullptr;
	for (auto const &str : recipient_ids)
	{
		recipients = curl_slist_append(recipients, str.c_str());
	}
	if(recipients)
	curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

	curl_easy_setopt(curl, CURLOPT_READFUNCTION, smtp_client::callback);
	curl_easy_setopt(curl, CURLOPT_READDATA, this);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

//	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	start = message.begin();
	finish = message.end();
	ifs.open(file_name);
	call_back_state = 0;
	CURLcode res = curl_easy_perform(curl);
	ifs.close();
	if (res != CURLE_OK)
		console->warn("curl_easy_perform() failed: {0}", curl_easy_strerror(res));
	curl_slist_free_all(recipients);
}
smtp_client::~smtp_client()
{
	curl_easy_cleanup(curl);
}

size_t smtp_client::next_line(void *ptr)
{
	std::ostringstream ostr;
	const char* boundary="pj+EhsWuSQJxx7pr";
	switch (call_back_state)
	{
	case 0:
		ostr << "Content-Type: multipart/mixed; boundary=\"" << boundary <<"\""
				 << "\r\n";
		++call_back_state;
		break;
	case 1:
		ostr << "Mime-version: 1.0"
				 << "\r\n";
		++call_back_state;
		break;
	case 2:
		ostr << ""
				 << "\r\n";
		++call_back_state;
		break;
	case 3:
		ostr << "This is a multi-part message in MIME format."
				 << "\r\n";
		++call_back_state;
		break;
	case 4:
		ostr << ""
				 << "\r\n";
		++call_back_state;
		break;
	case 5:
		ostr << "--"<< boundary
			 << "\r\n";
		++call_back_state;
		break;
	case 6:
		ostr << "Content-Type: text/plain; charset=\"us-ascii\""
				 << "\r\n";
		++call_back_state;
		break;
	case 7:
		ostr << "Content-Transfer-Encoding: quoted-printable"
				 << "\r\n";
		++call_back_state;
		break;
	case 8:
		ostr << ""
				 << "\r\n";
		++call_back_state;
		break;
	case 9:
		ostr << "Picture taken at " 
		     << current_date_time()
				 << "\r\n";
		++call_back_state;
		break;
	case 10:
		ostr << ""
				 << "\r\n";
		++call_back_state;
		break;
	case 11:
		ostr << "--" << boundary
				 << "\r\n";
		++call_back_state;
		break;
	case 12:
		ostr << "Content-Type: application/octet-stream"
				 << "\r\n";
		++call_back_state;
		break;
	case 13:
		ostr << "Content-Transfer-Encoding: base64"
				 << "\r\n";
		++call_back_state;
		break;
	case 14:
		ostr << "Content-Description: " << file_name << ".jpg" << "\r\n";
		++call_back_state;
		break;
	case 15:
		ostr << "Content-Disposition: attachment;"
				 << "\r\n";
		++call_back_state;
		break;
	case 16:
	{
		ifs.seekg(0, std::ios::end);
		size_t k = ifs.tellg();
		ifs.seekg(0, std::ios::beg);
		ostr << "  filename=\"" << file_name << "\"; size=" << k * 4 / 3 << ";\r\n";

		++call_back_state;
	}
	break;
	case 17:
		ostr << "  creation-date="
				 << "\"" << current_GMT_time() << "\"\r\n";
		++call_back_state;
		break;
	case 18:
		ostr << "  modification-date="
				 << "\"" << current_GMT_time() << "\"\r\n";
		++call_back_state;
		break;
	case 19:
		ostr << "\r\n";
		++call_back_state;
		break;
	case 20:
	{
		typedef base64_from_binary<transform_width<const char *, 6, 8>> it_base64_t;
		char buffer[54];
		if (ifs.read(buffer, 54))
		{
			auto count = ifs.gcount();
			std::string base64(it_base64_t(buffer), it_base64_t(buffer + count));
			ostr << base64 << "\r\n";
		}
		else if (auto count = ifs.gcount())
		{
			std::string base64(it_base64_t(buffer), it_base64_t(buffer + count));
			if (unsigned int writePaddChars = (3 - count % 3) % 3)
			{
				base64.append(writePaddChars, '=');
			}
			ostr << base64 << "\r\n";
			++call_back_state;
		}
	}
	break;
	case 21:
		ostr << "\r\n";
		++call_back_state;
		break;
	case 22:
		ostr << "--"<<boundary<<"--"
				 << "\r\n";
		++call_back_state;
		break;
	case 23:
		return 0;
		break;
	}
	std::string str = ostr.str();
	size_t len = str.length();
	memcpy(ptr, str.data(), len);
	return len;
}
size_t smtp_client::callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
	smtp_client *this_ = static_cast<smtp_client *>(userp);

	if ((size == 0) || (nmemb == 0) )
	{
		return 0;
	}
	if (this_->start == this_->finish)
		return this_->next_line(ptr);
	else
	{
		size_t len = this_->start->length();
		memcpy(ptr, this_->start->data(), len);
		this_->start++;
		return len;
	}
}
