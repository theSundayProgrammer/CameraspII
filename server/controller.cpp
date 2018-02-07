
////////////////////////////////////////////////////////////////////////////// // Copyright 2016-2017 (c) Joseph Mariadassou
// theSundayProgrammer@gmail.com
// Distributed under the Boost Software License, Version 1.0.
//
// http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////
#include <camerasp/server_http.hpp>
#include <json/reader.h>
#include <json/writer.h>
#include <asio.hpp>
#include <spawn.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdarg>
#include <fstream>
#include <camerasp/utils.hpp>
#include <camerasp/ipc.hpp>
#include <boost/filesystem.hpp>
#include <sys/wait.h>
#include <sstream>
#include <gsl/gsl_util>
std::shared_ptr<spdlog::logger> console;
#ifdef RASPICAM_MOCK
const std::string config_path = "./";
char const *home_page = "/home/chakra/data/web";
char const *cmd = "./camerasp";
#else
const std::string config_path = "/srv/camerasp/";
char const *home_page = "/home/pi/data/web";
char const *cmd = "/home/pi/bin/camerasp";
#endif

using asio::ip::udp;
class address_broadcasting_server
{
public:
	address_broadcasting_server(asio::io_context &io_context, unsigned short port)
			: socket_(io_context, udp::endpoint(udp::v4(), port))
	{
	}

	void receive()
	{
		socket_.async_receive_from(
				asio::buffer(data_, max_length), sender_endpoint_,
				[this](std::error_code ec, std::size_t bytes_recvd) {
					if (!ec && bytes_recvd > 0 &&
							send_message == std::string(data_, bytes_recvd))
					{
						send(bytes_recvd);
					}
					else
					{
						receive();
					}
				});
	}

	void send(std::size_t length)
	{
		socket_.async_send_to(
				asio::buffer(recv_message), sender_endpoint_,
				[this](std::error_code /*ec*/, std::size_t /*bytes_sent*/) {
					receive();
				});
	}

private:
	udp::socket socket_;
	udp::endpoint sender_endpoint_;
	enum
	{
		max_length = 1024
	};
	char data_[max_length];
	const std::string send_message{"12068c99-18de-48e1-87b4-3e09bbbd8b15-Camerasp"};
	const std::string recv_message{"ee7f7fc7-9d54-480b-868d-fde1f5a67ab6-Camerasp"};
};

//ToDo: set executable file name in json config
char const *log_folder = "/tmp/frame_grabber.log";
#define ASIO_ERROR_CODE asio::error_code
typedef SimpleWeb::Server<SimpleWeb::HTTP> http_server;
using server_base = SimpleWeb::ServerBase<SimpleWeb::HTTP>;
void default_resource_send(const http_server &server,
													 const std::shared_ptr<http_server::Response> &response,
													 const std::shared_ptr<std::ifstream> &ifs);
enum class process_state
{
	started,
	stop_pending,
	stopped
};

/*****************************************
State Transition Table
----------------------------------------
State| Command| Result State
----------------------------------------
started| stop | stop_pending 
started| start| started
stopped| stop | stopped
stopped| start| started
stop_pending | stop | stop_pending
stop_pending | start| stop_pending
******************************************/

namespace ipc = boost::interprocess;

class web_server
{
public:
	web_server(Json::Value &root)
			: response(RESPONSE_MEMORY_NAME), request(REQUEST_MEMORY_NAME), root_(root)
	{

		// server config
		unsigned port_number = 8088;
		auto server_config = root["Server"];
		if (!server_config.empty())
			port_number = server_config["port"].asInt();
		server.config.port = port_number;
		server.config.thread_pool_size = 2;
	}
	auto send_failure(
			std::shared_ptr<http_server::Response> http_response,
			std::string const &message)
	{
		*http_response << "HTTP/1.1 400 Bad Request\r\n"
									 << "Content-Length: " << message.size() << "\r\n"
									 << "Content-type: "
									 << "application/text"
									 << "\r\n"
									 << "\r\n"
									 << message;
	}
	auto send_success(
			std::shared_ptr<http_server::Response> http_response,
			std::string const &message)
	{
		*http_response << "HTTP/1.1 200 OK\r\n"
									 << "Content-Length: " << message.size() << "\r\n"
									 << "Content-type: "
									 << "application/text"
									 << "\r\n"
									 << "\r\n"
									 << message;
	}

	void exec_cmd(
			std::shared_ptr<http_server::Response> http_response,
			std::shared_ptr<http_server::Request> http_request)

	{
		if (fg_state == process_state::started)
			try
			{
				std::string str = http_request->path_match[0];
				request->set(str);
				camerasp::buffer_t data = response->try_get();
				*http_response << "HTTP/1.1 200 OK\r\n"
											 << "Content-Length: " << data.size() << "\r\n"
											 << "Content-type: "
											 << "application/text"
											 << "\r\n"
											 << "Cache-Control: no-cache, must-revalidate"
											 << "\r\n"
											 << "\r\n"
											 << data;
			}
			catch (std::runtime_error &e)
			{

				std::string success("Frame Grabber hung. Issue start command");
				send_failure(http_response, success);
			}
		else
		{
			std::string success("Frame Grabber not running. Issue start command");
			send_failure(http_response, success);
		}
	}
	auto send_image(
			std::shared_ptr<http_server::Response> http_response,
			std::shared_ptr<http_server::Request> http_request)

	{
		if (fg_state == process_state::started)
			try
			{
				std::string str = http_request->path_match[0];
				request->set(str);
				camerasp::buffer_t data = response->try_get();
				std::string err(data, 0, 4);

				console->error("Getting image");
				if (err == std::string(4, '\0'))
				{
					*http_response << "HTTP/1.1 200 OK\r\n"
												 << "Content-Length: " << data.size() - 4 << "\r\n"
												 << "Content-type: "
												 << "image/jpeg"
												 << "\r\n"
												 << "Cache-Control: no-cache, must-revalidate"
												 << "\r\n"
												 << "\r\n"
												 << std::string(data.begin() + 4, data.end());

					console->error("sending image");
					//ofstream ofs("/home/pi/data/testingsend.jpg");
					//ofs.write(data.begin()+4, data.size()-4);
				}
				else
				{
					console->error("Not sending image");
					send_failure(http_response, err + "No image Available");
				}
			}
			catch (std::runtime_error &e)
			{

				std::string success("Frame Grabber hung. Issue start command");
				send_failure(http_response, success);
			}
		else
		{
			std::string success("Frame Grabber not running. Issue start command");
			send_failure(http_response, success);
		}
	}
	void set_props(
			std::shared_ptr<http_server::Response> http_response,
			std::shared_ptr<http_server::Request> http_request)
	{
		try
		{

			std::ostringstream ostr;
			ostr << http_request->content.rdbuf();
			std::string istr = ostr.str();
			auto camera = root_["Camera"];
			std::regex pat("(\\w+)=(\\d+)");
			for (std::sregex_iterator p(std::begin(istr), std::end(istr), pat);
					 p != std::sregex_iterator{};
					 ++p)
			{
				camera[(*p)[1].str()] = atoi((*p)[2].str().c_str());
			}

			Json::StyledWriter writer;
			root_["Camera"] = camera;
			auto update = writer.write(root_);
			camerasp::write_file_content(config_path + "options.json", update);
			std::string data{"Config file updated. Restart camera"};
			*http_response << "HTTP/1.1 200 OK\r\n"
										 << "Content-Length: " << data.size() << "\r\n"
										 << "Content-type: "
										 << "application/text"
										 << "\r\n"
										 << "Cache-Control: no-cache, must-revalidate"
										 << "\r\n"
										 << "\r\n"
										 << data;
		}
		catch (std::runtime_error &e)
		{
			std::string success("Frame Grabber not running. Issue start command");
			send_failure(http_response, success);
		}
	}
	void stop_camera(
			std::shared_ptr<http_server::Response> http_response,
			std::shared_ptr<http_server::Request> http_request)
	{
		std::string success("Succeeded");
		//
		struct sigaction act;
		memset(&act, '\0', sizeof(act));
		sigaction(SIGCHLD, nullptr, &act);
		console->debug("Mask= {0},{1},{2},{3}", act.sa_mask.__val[0], act.sa_mask.__val[1], act.sa_mask.__val[2], act.sa_mask.__val[3]);
		if (fg_state == process_state::started)
			try
			{
				request->set("exit");
				camerasp::buffer_t data = response->try_get();
				console->info("stop executed");
				int status = 0;
				unsigned n = 0;
				while (n < 20 && 0 <= waitpid(child_pid, &status, WNOHANG))
				{
					usleep(100 * 1000);
					++n;
				}
				if (n == 20)
					kill(child_pid, SIGKILL);
				fg_state = process_state::stopped;
				send_success(http_response, success);
			}
			catch (std::exception &e)
			{
				success = "stop failed - abortng";
				console->info(success);
				kill(child_pid, SIGKILL);
				fg_state = process_state::stop_pending;
				send_success(http_response, success);
			}
		else if (fg_state == process_state::stop_pending)
		{
			success = "Stop Pending. try again later";
			send_failure(http_response, success);
		}
		else
		{
			success = "Not running when command received";
			send_failure(http_response, success);
		}
	}

	void do_fallback(
			std::shared_ptr<http_server::Response> http_response,
			std::shared_ptr<http_server::Request> http_request)
	{
		try
		{
			auto web_root_path =
					boost::filesystem::canonical(home_page);
			auto path = boost::filesystem::canonical(web_root_path / http_request->path);
			//Check if path is within web_root_path
			if (std::distance(web_root_path.begin(), web_root_path.end()) >
							std::distance(path.begin(), path.end()) ||
					!std::equal(web_root_path.begin(), web_root_path.end(), path.begin()))
				throw std::invalid_argument("path must be within root path");
			if (boost::filesystem::is_directory(path))
				path /= "index.html";
			if (!(boost::filesystem::exists(path) &&
						boost::filesystem::is_regular_file(path)))
				throw std::invalid_argument("file does not exist");

			std::string cache_control, etag;

			// Uncomment the following line to enable Cache-Control
			// cache_control="Cache-Control: max-age=86400\r\n";

			auto ifs = std::make_shared<std::ifstream>();
			ifs->open(path.string(), std::ifstream::in | std::ios::binary | std::ios::ate);

			if (*ifs)
			{
				auto length = ifs->tellg();
				ifs->seekg(0, std::ios::beg);

				*http_response << "HTTP/1.1 200 OK\r\n"
											 << cache_control << etag
											 << "Content-Length: " << length << "\r\n\r\n";
				default_resource_send(server, http_response, ifs);
			}
			else
				throw std::invalid_argument("could not read file");
		}
		catch (const std::exception &e)
		{
			std::string content = "Could not open path " + http_request->path + ": " + e.what();
			*http_response << "HTTP/1.1 400 Bad Request\r\n"
										 << "Content-Length: " << content.length() << "\r\n"
										 << "\r\n"
										 << content;
		}
	}
	void flip(std::shared_ptr<http_server::Response> http_response,
			 std::shared_ptr<http_server::Request> http_request) {
        try	{
		camerasp::buffer_t data("Success");
		std::ostringstream ostr;
		ostr << http_request->content.rdbuf();
		std::string istr = ostr.str();
		auto camera = root_["Camera"];
		std::regex pat("(\\w+)=(0|1)");
		for (std::sregex_iterator p(std::begin(istr), std::end(istr), pat);
					p != std::sregex_iterator{};
					++p)
		{
			camera[(*p)[1].str()] = atoi((*p)[2].str().c_str());
			if (fg_state == process_state::started)
			{
				request->set(std::string("/flip?") + (*p)[0].str().c_str());
				data = data + response->try_get() + "\r\n";
			}
		}

		Json::StyledWriter writer;
		root_["Camera"] = camera;
		auto update = writer.write(root_);
		camerasp::write_file_content(config_path + "options.json", update);
		*http_response << "HTTP/1.1 200 OK\r\n"
										<< "Content-Length: " << data.size() << "\r\n"
										<< "Content-type: "
										<< "application/text"
										<< "\r\n"
										<< "Cache-Control: no-cache, must-revalidate"
										<< "\r\n"
										<< "\r\n"
										<< data;

		}
		catch (std::runtime_error &e)
		{

			std::string success("Frame Grabber not running. Issue start command");
			send_failure(http_response, success);
		}
	}
	

	void run(std::shared_ptr<asio::io_context> io_service, char *argv[], char *env[])
	{
		using namespace camerasp;

		//Child process
		//Important: the working directory of the child process
		// is the same as that of the parent process.
		int ret;
		if (ret = posix_spawn_file_actions_init(&child_fd_actions))
			console->error("posix_spawn_file_actions_init"), exit(ret);
		if (ret = posix_spawn_file_actions_addopen(
						&child_fd_actions, 1, log_folder,
						O_WRONLY | O_CREAT | O_TRUNC, 0644))
			console->error("posix_spawn_file_actions_addopen"), exit(ret);
		if (ret = posix_spawn_file_actions_adddup2(&child_fd_actions, 1, 2))
			console->error("posix_spawn_file_actions_adddup2"), exit(ret);
		//
		server.io_service = io_service;
		// The signal set is used to register termination notifications
		asio::signal_set signals_(*io_service);
		signals_.add(SIGINT);
		signals_.add(SIGTERM);
		signals_.add(SIGCHLD);

		// signal handler for SIGCHLD must be set before the process is forked
		// In a service 'spawn' forks a child process
		std::function<void(ASIO_ERROR_CODE, int)> signal_handler =
				[&](ASIO_ERROR_CODE const &error, int signal_number) {
					if (signal_number == SIGCHLD)
					{
						fg_state = process_state::stopped;
						console->debug("Process stopped");
						waitpid(child_pid, NULL, 0);
						signals_.async_wait(signal_handler);
					}
					else
					{
						console->debug("SIGTERM received");
						server.stop();
					}
				};

		if (int ret = posix_spawnp(&child_pid, cmd, &child_fd_actions,
															 NULL, argv, env))
			console->error("posix_spawn"), exit(ret);
		console->info("Child pid: {0}\n", child_pid);
		fg_state = process_state::started;

		// get previous image
		server.resource[R"(^/image\?prev=([0-9]+)$)"]["GET"] = [&](
					std::shared_ptr<http_server::Response> http_response,
					std::shared_ptr<http_server::Request> http_request) {
			send_image(http_response, http_request);
		};

		// replace camera properties
		server.resource["^/camera"]["PUT"] = [&](
							std::shared_ptr<http_server::Response> http_response,
							std::shared_ptr<http_server::Request> http_request) {
			set_props(http_response, http_request);
		};
		// flip horizontal vertical
		server.resource["^/flip"]["PUT"] = [&]( //\\?horizontal=(0|1)$
							std::shared_ptr<http_server::Response> http_response,
							std::shared_ptr<http_server::Request> http_request) {
				flip(http_response,http_request);
			
		};
		server.resource["^/resume$"]["GET"] = [&](
					std::shared_ptr<http_server::Response> http_response,
					std::shared_ptr<http_server::Request> http_request) {
			exec_cmd(http_response, http_request);
		};
		// pause/resume
		server.resource["^/pause$"]["GET"] = [&](
					std::shared_ptr<http_server::Response> http_response,
					std::shared_ptr<http_server::Request> http_request) {
			exec_cmd(http_response, http_request);
		};
		server.resource["^/resume$"]["GET"] = [&](
					std::shared_ptr<http_server::Response> http_response,
					std::shared_ptr<http_server::Request> http_request) {
			exec_cmd(http_response, http_request);
		};
		// get current image
		server.resource["^/image"]["GET"] = [&](
					std::shared_ptr<http_server::Response> http_response,
					std::shared_ptr<http_server::Request> http_request) {
			send_image(http_response, http_request);
		};

		//restart capture
		server.resource["^/config$"]["GET"] = [&](
						std::shared_ptr<http_server::Response> http_response,
						std::shared_ptr<http_server::Request> http_request) {
			Json::Value root = camerasp::get_DOM(config_path + "options.json");

			Json::FastWriter writer;
			auto camera_config = root["Camera"];
			auto response = writer.write(camera_config);
			send_success(http_response, response);
		};
		server.resource["^/start$"]["GET"] = [&](
						std::shared_ptr<http_server::Response> http_response,
						std::shared_ptr<http_server::Request> http_request) {
			std::string success("Succeeded");

			console->info("Resart Child command received");
			if (fg_state == process_state::started)
			{
				success = "Already Running";
				send_failure(http_response, success);
			}
			else if (fg_state == process_state::stop_pending)
			{
				success = "Stop Pending. try again later";
				send_failure(http_response, success);
			}
			else
			{
				if (int ret = posix_spawnp(&child_pid, cmd, &child_fd_actions, NULL, argv, env))
					console->error("posix_spawn"), exit(ret);
				fg_state = process_state::started;
				console->info("Child pid: {0}\n", child_pid);
				send_success(http_response, success);
			}
		};
		auto kill_child = [=]() {
			if (fg_state == process_state::started)
			{
				int status = 0;
				unsigned n = 0;
				kill(child_pid, SIGTERM);
				while (n < 20 && 0 <= waitpid(child_pid, &status, WNOHANG))
				{
					usleep(100 * 1000);
					++n;
				}
				if (n == 20)
					kill(child_pid, SIGKILL);
				fg_state == process_state::stopped;
			}
		};
		auto _1 = gsl::finally([=]() { kill_child(); });
		//stop capture
		server.resource["^/abort$"]["GET"] = [&](
				std::shared_ptr<http_server::Response> http_response,
				std::shared_ptr<http_server::Request> http_request) {
			std::string success("Succeeded");
			//
			if (fg_state == process_state::started)

			{
				kill_child();
				send_success(http_response, success);
			}
			else if (fg_state == process_state::stop_pending)
			{
				success = "Stop Pending. try again later";
				send_failure(http_response, success);
			}
			else
			{
				success = "Not running when command received";
				send_failure(http_response, success);
			}
		};
		//stop capture
		server.resource["^/stop$"]["GET"] = [&](
						std::shared_ptr<http_server::Response> http_response,
						std::shared_ptr<http_server::Request> http_request) {
			stop_camera(http_response, http_request);
		};
		//default page server
		server.default_resource["GET"] = [&](
						std::shared_ptr<http_server::Response> http_response,
						std::shared_ptr<http_server::Request> http_request) {
			do_fallback(http_response, http_request);
		};

		signals_.async_wait(signal_handler);
		//start() returns on SIGTERM
		server.start();
		if (fg_state == process_state::started)
		{
			try
			{
				request->set("exit");
				camerasp::buffer_t data = response->try_get();
			}
			catch (std::runtime_error &e)
			{
			}
		}
	}

private:
	process_state fg_state = process_state::stopped;
	shared_mem_ptr<shared_response_data> response;
	shared_mem_ptr<shared_request_data> request;
	pid_t child_pid;
	posix_spawn_file_actions_t child_fd_actions;
	Json::Value &root_;
	http_server server;
};

int main(int argc, char *argv[], char *env[])
{
	try
	{

		Json::Value root = camerasp::get_DOM(config_path + "options.json");
		//configure console

		auto log_config = root["Logging"];
		auto json_path = log_config["path"];
		auto logpath = json_path.asString();
		auto size_mega_bytes = log_config["size"].asInt();
		auto count_files = log_config["count"].asInt();
		//console = spd::rotating_logger_mt("console", logpath, 1024 * 1024 * size_mega_bytes, count_files);
		console = spdlog::stdout_color_mt("console");
		console->set_level(spdlog::level::debug);
		console->debug("Starting");
		auto io_service = std::make_shared<asio::io_context>();

		address_broadcasting_server broadcaster(*io_service, 52153);
		broadcaster.receive();
		//run web server
		web_server server(root);
		server.run(io_service, argv, env);
	}
	catch (std::exception &e)
	{
		console->error("Exception: {0}", e.what());
	}

	return 0;
}

void default_resource_send(const http_server &server,
													 const std::shared_ptr<http_server::Response> &response,
													 const std::shared_ptr<std::ifstream> &ifs)
{
	//read and send 128 KB at a time
	static std::vector<char> buffer(131072); // Safe when server is running on one thread
	std::streamsize read_length = ifs->read(&buffer[0], buffer.size()).gcount();
	if (read_length > 0)
	{
		response->write(&buffer[0], read_length);
		if (read_length == static_cast<std::streamsize>(buffer.size()))
		{
			server.send(
					response,
					[&server, response, ifs](const std::error_code &ec) {
						if (!ec)
							default_resource_send(server, response, ifs);
						else
							console->warn("Connection interrupted");
					});
		}
	}
}
