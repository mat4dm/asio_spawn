#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/exception/all.hpp>
#include "FtpClient.h"

int main(int argc, char* argv[])
{
	try {
		boost::asio::io_service io_service;
		FtpClient client(io_service);
		client.start();
		io_service.run();
	} catch (...) {
		std::cerr << "error: " << boost::current_exception_diagnostic_information() << std::endl;
		return 1;
	}
	return 0;
}

