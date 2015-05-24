#include <iostream>
#include <fstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/regex_actions.hpp>
#include "FtpClient.h"

namespace asio = boost::asio;
using boost::asio::ip::tcp;

FtpClient::FtpClient(asio::io_service& io_service)
	: io_service_(io_service)
	, socket_(io_service)
	, socketAccept_(io_service)
	, acceptor_(io_service_)
{
}

FtpClient::~FtpClient(void)
{
}

void FtpClient::start(void)
{
	asio::spawn(io_service_, [&] (asio::yield_context yield) {
		boost::system::error_code error_code;
		commandOpen("example.com",  yield[error_code]);
		commandUser("anonymous", yield[error_code]);
		commandPass("ftp@example.com", yield[error_code]);
		commandCwd("pub", yield[error_code]);
		commandList("/pub", yield[error_code]);
		commandRetr("README", yield[error_code]);
		commandQuit(yield[error_code]);
	});
}

void FtpClient::commandOpen(const std::string& hostName, const asio::yield_context& yield)
{
	boost::system::error_code error_code;
	asio::ip::tcp::resolver resolver(io_service_);
	asio::ip::tcp::resolver::query query(hostName, "ftp");
	asio::ip::tcp::resolver::iterator it = resolver.async_resolve(query, yield[error_code]);
	std::cout << "connecting to ... " << it->endpoint().address().to_string() << std::endl;
	socket_.async_connect(it->endpoint(), yield[error_code]);
	recvAll(socket_, buf_, yield[error_code]);
}

void FtpClient::commandUser(const std::string& user, const asio::yield_context& yield)
{
	boost::system::error_code error_code;
	sendWrite("USER " + user, yield[error_code]);
	recvAll(socket_, buf_, yield[error_code]);
}

void FtpClient::commandPass(const std::string& pass, const asio::yield_context& yield)
{
	boost::system::error_code error_code;
	sendWrite("PASS " + pass, yield[error_code]);
	recvAll(socket_, buf_, yield[error_code]);
}

void FtpClient::commandCwd(const std::string& dir, const asio::yield_context& yield)
{
	boost::system::error_code error_code;
	sendWrite("CWD " + dir, yield[error_code]);
	recvAll(socket_, buf_, yield[error_code]);
}

void FtpClient::commandList(const std::string& fileName, const asio::yield_context& yield)
{
	boost::system::error_code error_code;
	sendWrite(createPortCommand(), yield[error_code]);
	recvAll(socket_, buf_, yield[error_code]);
	sendWrite("LIST " + fileName, yield[error_code]);
	recvAll(socket_, buf_, yield[error_code]);
	startAccept(yield[error_code]);
	recvAll(socketAccept_, buf_, yield[error_code], false);
	acceptor_.close();
	socketAccept_.close();
	recvAll(socket_, buf_, yield[error_code]);
}

void FtpClient::commandRetr(const std::string& fileName, const asio::yield_context& yield)
{
	boost::system::error_code error_code;
	sendWrite(createPortCommand(), yield[error_code]);
	recvAll(socket_, buf_, yield[error_code]);
	sendWrite("RETR " + fileName, yield[error_code]);
	recvAll(socket_, buf_, yield[error_code]);
	startAccept(yield[error_code]);
	recvFile(socketAccept_, buf_, fileName, yield[error_code]);
	acceptor_.close();
	socketAccept_.close();
	recvAll(socket_, buf_, yield[error_code]);
}

void FtpClient::commandQuit(const asio::yield_context& yield)
{
	boost::system::error_code error_code;
	sendWrite("QUIT", yield[error_code]);
	recvAll(socket_, buf_, yield[error_code]);
}

void FtpClient::startAccept(const asio::yield_context& yield)
{
	boost::system::error_code error_code;
	acceptor_.async_accept(socketAccept_, yield[error_code]);
	if (error_code) {
		std::cout << "error startAccept " << error_code.message() << std::endl;
	}
}

void FtpClient::sendWrite(const std::string& command, const asio::yield_context& yield)
{
	boost::system::error_code error_code;
	std::string sendData = command;
	sendData += "\r\n";
	std::cout << "<< " << command << std::endl;
	asio::async_write(socket_, asio::buffer(sendData, sendData.size()), yield[error_code]);
	if (error_code) {
		std::cout << "error sendWrite " << error_code.message() << std::endl;
	}
}

void FtpClient::recvAll(tcp::socket& socket, asio::streambuf& buf, const asio::yield_context& yield, bool check)
{
	using namespace boost::xpressive;
	boost::system::error_code error_code;
	bool firstFlag = check;
	for (;;) {
		asio::async_read(socket
			, buf
			, asio::transfer_at_least(1)
			, yield[error_code]);
		std::size_t returnSize = buf.size();
		if (!returnSize) {
			break;
		}
		if (error_code && error_code != asio::error::eof) {
			std::cout << "error recvAll " << error_code.message() << std::endl;
			break;
		}
		if (firstFlag) {
			sregex rex = bos 
			>> as_xpr('1') | '2' | '3'
			>> repeat<2>(_d)
			>> _s;
			smatch what;
			std::string str(asio::buffer_cast<const char*>(buf.data()), returnSize);
			if (!regex_search(str, what, rex)) {
				//std::cout << "---" << what[0] << std::endl;
				std::cout << "error return code" << std::endl;
			}
			firstFlag = false;
		}
		std::cout << std::string(asio::buffer_cast<const char*>(buf.data()), returnSize); // << std::endl;
		buf.consume(returnSize);
		if (error_code == asio::error::eof || returnSize < max_buf) {
			break;
		}
	}
}

void FtpClient::recvFile(tcp::socket& socket, asio::streambuf& buf, const std::string fileName
	, const asio::yield_context& yield)
{
	boost::system::error_code error_code;
	std::ofstream ofsFile(fileName, std::ofstream::out | std::ofstream::binary);
	while (asio::async_read(socket, buf, asio::transfer_at_least(1), yield[error_code])) {
		if (error_code && error_code != asio::error::eof) {
			std::cout << "error recvFile " << error_code.message() << std::endl;
			break;
		} else {
			ofsFile << &buf;
		}
	}
	ofsFile.close();
}

auto FtpClient::createPortCommand() -> const std::string
{
	tcp::endpoint endpoint(socket_.local_endpoint().address(), 0);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen(1);
	std::string address = acceptor_.local_endpoint().address().to_string();
	unsigned short port = acceptor_.local_endpoint().port();
	std::vector<std::string> v;
	boost::algorithm::split(v, address, boost::is_any_of("."));
	return (boost::format("PORT %1%,%2%,%3%,%4%,%5%,%6%")
		% v[0] % v[1] % v[2] % v[3] % ((port >> 8) & 0xff) % (port & 0xff)).str();
}

