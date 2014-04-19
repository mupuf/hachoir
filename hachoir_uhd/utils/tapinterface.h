#ifndef TAPINTERFACE_H
#define TAPINTERFACE_H

#include <string>

#include <utils/message.h>

class TapInterface
{
	int _tapFd;
	std::string _ifName;
public:
	TapInterface(const char *name, bool persistent = false);
	~TapInterface();

	bool isReady() const;
	bool removePersistent();

	Message readNextMessage();
};

#endif // TAPINTERFACE_H
