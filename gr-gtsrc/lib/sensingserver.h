#ifndef SENSINGSERVER_H
#define SENSINGSERVER_H

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>

#include <memory>

#include "sensingclient.h"

enum MessageType { MSG_FFT = 1, MSG_RET = 2 };

class SensingServer
{
private:
	uint16_t _port;

	boost::asio::io_service _ios;
	boost::asio::ip::tcp::endpoint _endpoint;
	boost::asio::ip::tcp::acceptor _acceptor;
	boost::thread _io_thread;

	boost::mutex _clientsMutex;
	std::list<SensingClient*> _clients;

	void io_service();
	void incomingConnection(SensingClient *client,
				const boost::system::error_code &error);
public:
	SensingServer(uint16_t port);

	void startListening();
	void stopListening();

	void sendToAll(const char *buf, size_t len);
};

#endif // SENSINGSERVER_H
