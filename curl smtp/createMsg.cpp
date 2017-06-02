#include <iostream>
#include <string>
#include <fstream>
#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <sstream> // stringstream
#include <iomanip> // put_time
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
std::string current_GMT_time()
{
    auto now = std::chrono::system_clock::now(); // system time
    auto in_time_t = std::chrono::system_clock::to_time_t(now); //convert to std::time_t

    std::stringstream ss;
    ss << std::put_time(std::gmtime(&in_time_t), "%a, %b %d %Y %X GMT");
    return ss.str();
}


using namespace boost::archive::iterators;
using namespace std;


int main(int argc, char **argv) 
{
  string s;
  std::ostream& ostr = cout;
  ostr << "Content-Type: multipart/mixed; boundary=\"pj+EhsWuSQJxx7pr\"" <<"\n";
  ostr << "Mime-version: 1.0" <<"\n";
  ostr << "" <<"\n";
  ostr << "This is a multi-part message in MIME format." <<"\n";
  ostr << "" <<"\n";
  ostr << "--pj+EhsWuSQJxx7pr" <<"\n";
  ostr << "Content-Type: text/plain; charset=\"us-ascii\"" <<"\n";
  ostr << "Content-Transfer-Encoding: quoted-printable" <<"\n";
  ostr << "" <<"\n";
  ostr << "Hello" <<"\n";
  ostr << "" <<"\n";
  ostr << "--pj+EhsWuSQJxx7pr" <<"\n";
  char *fName = argv[1];
  ifstream ifs(fName);
  ifs.seekg(0,ios::end);
  size_t k = ifs.tellg();
  ifs.seekg(0,ios::beg);
  //ostr << "******************" << k << endl;
  ostr << "Content-Type: application/octet-stream" <<"\n";
  //ostr << "  name=\""<<fName<<"\"" <<"\n";
  ostr << "Content-Transfer-Encoding: base64" <<"\n";
  ostr << "Content-Description: " << fName <<"\n";
  ostr << "Content-Disposition: attachment;" <<"\n";
  ostr << "  filename=\""<<fName<<"\"; size=" << k*4/3  <<";\n";
  ostr << "  creation-date=" <<"\""<< current_GMT_time()<< "\"\n";
  ostr << "  modification-date=" <<"\""<< current_GMT_time()<< "\"\n";

  ostr << "\n";  
  //typedef insert_linebreaks<base64_from_binary<transform_width<string::const_iterator,6,8> >, 72 > it_base64_t;
  typedef base64_from_binary<transform_width<const char*,6,8> > it_base64_t;
  char buffer[54];
  while(ifs.read(buffer,54))
  {
    auto count = ifs.gcount();   
    string base64(it_base64_t(buffer),it_base64_t(buffer+count));
    if(unsigned int writePaddChars = (3-count%3)%3)
    {
      base64.append(writePaddChars,'=');
    }
    ostr << base64 <<"\n";  

  }
  if(auto count = ifs.gcount())   
  {
    string base64(it_base64_t(buffer),it_base64_t(buffer+count));
    if(unsigned int writePaddChars = (3-count%3)%3)
    {
      base64.append(writePaddChars,'=');
    }
    ostr << base64 <<"\n";  

  }
  ostr << "\n";  
  ostr<< "--pj+EhsWuSQJxx7pr--" <<"\n";
  return 0;
}

