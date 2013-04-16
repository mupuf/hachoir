#ifndef SENSINGCLIENT_H
#define SENSINGCLIENT_H

#include <boost/asio.hpp>

class SensingClient
{
	boost::asio::ip::tcp::socket _socket;

	bool _fullSensing;

public:
	SensingClient(boost::asio::io_service &ios);

	boost::asio::ip::tcp::socket &socket() { return _socket; }
	bool wantsFullSensing() const { return _fullSensing; }

	void socketIsConnected();

	bool send(const char *buf, size_t len);
};

#endif // SENSINGCLIENT_H
