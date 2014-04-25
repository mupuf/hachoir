#include "tapinterface.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

TapInterface::TapInterface(const char *name, bool persistent) : _tapFd(0)
{
	struct ifreq ifr;
	int fd, err;

	if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
		perror("Opening the tun device");
		return;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP;

	if (name)
		strncpy(ifr.ifr_name, name, IFNAMSIZ);

	err = ioctl(fd, TUNSETIFF, (void *) &ifr);
	if( err < 0 ) {
		perror("Creating the tap interface");
		close(fd);
		return;
	}

	if (persistent) {
		if(ioctl(fd, TUNSETPERSIST, 1) < 0){
			perror("enabling TUNSETPERSIST");
			close(fd);
			return;
		}
	}

	_ifName = ifr.ifr_name;
	_tapFd = fd;
}

TapInterface::~TapInterface()
{
	if (_tapFd > 0)
		close(_tapFd);
}

bool TapInterface::isReady() const
{
	return _tapFd > 0;
}

bool TapInterface::removePersistent()
{
	if(ioctl(_tapFd, TUNSETPERSIST, 0) < 0){
		perror("disabling TUNSETPERSIST");
		return false;
	} else
		return true;
}

Message TapInterface::readNextMessage()
{
	uint8_t buffer[2000];
	Message m;

	int nread = read(_tapFd, buffer, sizeof(buffer));
	if(nread < 0) {
		perror("Reading from interface");
		return m;
	}

	for (int i = 0; i < nread; i++) {
		m.addByte(buffer[i]);
	}

	return m;
}

bool TapInterface::sendMessage(const Message &msg)
{
	uint8_t buf[2000];
	size_t len = sizeof(buf);

	if (!msg.toBuffer(buf, &len))
		return false;

	if (write(_tapFd, buf, len) != (int) len)
		return false;

	return true;
}

