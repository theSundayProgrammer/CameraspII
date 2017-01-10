/****************************************
*Copyright Joseph Mariadassou
* theSundayProgrammer@gmail.com
* adapted from https://github.com/eidheim/Simple-Web-Server
*  http_examples.cpp
****************************************/
#include <camerasp/server_http.hpp>

#include <json/reader.h>
#include <camerasp/utils.hpp>
#include <camerasp/timer.hpp>

#include <boost/filesystem.hpp>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>
using namespace std;
namespace spd=spdlog;
typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

std::shared_ptr<spdlog::logger> console;
void default_resource_send(const HttpServer &server,
                           const shared_ptr<HttpServer::Response> &response,
                           const shared_ptr<ifstream> &ifs);

#ifdef RASPICAM_MOCK
const std::string config_path = "./";
#else
const std::string config_path = "/srv/camerasp/";
#endif
string get_image()
{
  stringstream content_stream;
  ifstream ifs("/home/chakra/data/test.jpg");
         
  content_stream << ifs.rdbuf();
  return content_stream.str();
}
int main() {
    //HTTP-server at port 8080 using 1 thread
    //Unless you do more heavy non-threaded processing in the resources,
    //1 thread is usually faster than several threads
    HttpServer server;
    unsigned port_number=8088;
    auto root = camerasp::get_DOM(config_path + "options.json");
    auto server_config = root["Server"];
    if (!server_config.empty())
      port_number = server_config["port"].asInt();
    auto log_config = root["Logging"];
    auto json_path = log_config["path"];
    auto logpath = json_path.asString();
    auto size_mega_bytes = log_config["size"].asInt();
    auto count_files = log_config["count"].asInt();
    server.config.port=port_number;
    server.config.thread_pool_size=2;
    
    console = spd::rotating_logger_mt("console", logpath, 1024 * 1024 * size_mega_bytes, count_files);
    //console = spd::stdout_color_mt("console");
    console->set_level(spd::level::debug);
    auto io_service=std::make_shared<asio::io_service>();
    server.io_service = io_service;
    camerasp::periodic_frame_grabber timer(*io_service, root["Data"]);
    //For instance a request GET /image?prev=123 will receive: 123
    server.resource["^/image\\?prev=([0-9]+)$"]["GET"]=[&timer](
                    shared_ptr<HttpServer::Response> response,
                    shared_ptr<HttpServer::Request> request)
    {
        string number=request->path_match[1];
        int k = atoi(number.c_str());
        auto data = timer.get_image(k);
        console->debug("size of data sent= {0}", data.size());
        *response <<  "HTTP/1.1 200 OK\r\n" 
                  <<  "Content-Length: " << data.size()<< "\r\n"
                  <<  "Content-type: " << "image/jpeg" <<"\r\n"
                   << "\r\n"
                   << data;
    };
    //restart capture
    server.resource["^/start$"]["GET"]=[&timer](
                    shared_ptr<HttpServer::Response> response,
                    shared_ptr<HttpServer::Request> request)
    {
        //find length of content_stream (length received using content_stream.tellp())
        string success("Succeeded");
        *response <<  "HTTP/1.1 200 OK\r\n" 
                  <<  "Content-Length: " << success.size()<< "\r\n"
                  <<  "Content-type: " << "image/jpeg" <<"\r\n"
                   << "\r\n"
                   << success;
    };
    //stop capture
    server.resource["^/stop$"]["GET"]=[&timer](
                    shared_ptr<HttpServer::Response> response,
                    shared_ptr<HttpServer::Request> request)
    {
        //find length of content_stream (length received using content_stream.tellp())
        string success("Succeeded");
        *response <<  "HTTP/1.1 200 OK\r\n" 
                  <<  "Content-Length: " << success.size()<< "\r\n"
                  <<  "Content-type: " << "image/jpeg" <<"\r\n"
                   << "\r\n"
                   << success;
    };
    //Responds with request-information
    server.resource["^/image$"]["GET"]=[&timer](
                    shared_ptr<HttpServer::Response> response,
                    shared_ptr<HttpServer::Request> request)
    {
        //find length of content_stream (length received using content_stream.tellp())
        int k = 0;
        auto data = timer.get_image(k);
        *response <<  "HTTP/1.1 200 OK\r\n" 
                  <<  "Content-Length: " << data.size()<< "\r\n"
                  <<  "Content-type: " << "image/jpeg" <<"\r\n"
                   << "\r\n"
                   << data;
    };
    
    
    //Default GET-example. If no other matches, this anonymous function will be called. 
    //Will respond with content in the web/-directory, and its subdirectories.
    //Default file: index.html
    //Can for instance be used to retrieve an HTML 5 client that uses REST-resources on this server
    server.default_resource["GET"]=[&server](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
        try {
            auto web_root_path=boost::filesystem::canonical("/home/chakra/data/web");
            auto path=boost::filesystem::canonical(web_root_path/request->path);
            //Check if path is within web_root_path
            if(distance(web_root_path.begin(), web_root_path.end())>distance(path.begin(), path.end()) ||
               !equal(web_root_path.begin(), web_root_path.end(), path.begin()))
                throw invalid_argument("path must be within root path");
            if(boost::filesystem::is_directory(path))
                path/="index.html";
            if(!(boost::filesystem::exists(path) && boost::filesystem::is_regular_file(path)))
                throw invalid_argument("file does not exist");

            std::string cache_control, etag;

            // Uncomment the following line to enable Cache-Control
            // cache_control="Cache-Control: max-age=86400\r\n";

            auto ifs=make_shared<ifstream>();
            ifs->open(path.string(), ifstream::in | ios::binary | ios::ate);
            
            if(*ifs) {
                auto length=ifs->tellg();
                ifs->seekg(0, ios::beg);
                
                *response << "HTTP/1.1 200 OK\r\n" << cache_control << etag << "Content-Length: " << length << "\r\n\r\n";
                default_resource_send(server, response, ifs);
            }
            else
                throw invalid_argument("could not read file");
        }
        catch(const exception &e) {
            string content="Could not open path "+request->path+": "+e.what();
            *response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
        }
    };
    
    timer.start_capture();
    server.start();
    return 0;
}

void default_resource_send(const HttpServer &server, const shared_ptr<HttpServer::Response> &response,
                           const shared_ptr<ifstream> &ifs) {
    //read and send 128 KB at a time
    static vector<char> buffer(131072); // Safe when server is running on one thread
    streamsize read_length;
    if((read_length=ifs->read(&buffer[0], buffer.size()).gcount())>0) {
        response->write(&buffer[0], read_length);
        if(read_length==static_cast<streamsize>(buffer.size())) {
            server.send(response, [&server, response, ifs](const std::error_code &ec) {
                if(!ec)
                    default_resource_send(server, response, ifs);
                else
                    cerr << "Connection interrupted" << endl;
            });
        }
    }
}
