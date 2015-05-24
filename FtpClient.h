#pragma once

class FtpClient 
{
public:
	FtpClient(boost::asio::io_service& io_service);
	virtual ~FtpClient(void);
	void start(void);

private:
	enum { max_buf = 512 };
	boost::asio::io_service& io_service_;
	boost::asio::ip::tcp::socket socket_;
	boost::asio::ip::tcp::socket socketAccept_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::streambuf buf_;

	auto createPortCommand() -> const std::string;
	void sendWrite(const std::string& command, const boost::asio::yield_context& yield);
	void recvAll(boost::asio::ip::tcp::socket& socket, boost::asio::streambuf& buf
		, const boost::asio::yield_context& yield, bool check = true);
	void recvFile(boost::asio::ip::tcp::socket& socket, boost::asio::streambuf& buf
		, const std::string fileName, const boost::asio::yield_context& yield);
	void startAccept(const boost::asio::yield_context& yield);
	void commandUser(const std::string& user, const boost::asio::yield_context& yield);
	void commandCwd(const std::string& dir, const boost::asio::yield_context& yield);
	void commandPass(const std::string& pass, const boost::asio::yield_context& yield);
	void commandList(const std::string& dir, const boost::asio::yield_context& yield);
	void commandRetr(const std::string& fileName, const boost::asio::yield_context& yield);
	void commandQuit(const boost::asio::yield_context& yield);
	void commandOpen(const std::string& hostName, const boost::asio::yield_context& yield);

};

