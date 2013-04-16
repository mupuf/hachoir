#ifndef SENSINGCLIENT_H
#define SENSINGCLIENT_H

#include <boost/asio.hpp>
#include <list>

class SensingClient
{
public:
	struct freqInterest
	{
		uint64_t centralFreq;
		uint32_t bandwidth;
		uint8_t error;
	};

	SensingClient(boost::asio::io_service &ios);

	boost::asio::ip::tcp::socket &socket() { return _socket; }
	bool wantsFullSensing() const { return _fullSensing; }
	std::list<freqInterest> &frequencies() { return _frequencies; }

	void socketIsConnected();

	bool send(const char *buf, size_t len);

private:
	boost::asio::ip::tcp::socket _socket;
	bool _fullSensing;

	std::list<freqInterest> _frequencies;

	bool readNBytes(char *buf, size_t size);
};

#endif // SENSINGCLIENT_H
