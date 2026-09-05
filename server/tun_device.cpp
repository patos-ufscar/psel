#include "tun_device.hpp"

#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

TunDevice::TunDevice(const std::string& wanted_name) : wanted_(wanted_name) {}

TunDevice::~TunDevice() { close(); }

bool TunDevice::open() {
    int fd = ::open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        perror("open /dev/net/tun");
        return false;
    }
    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof ifr);
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    std::snprintf(ifr.ifr_name, IFNAMSIZ, "%s", wanted_.c_str());
    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("ioctl TUNSETIFF");
        ::close(fd);
        return false;
    }
    char actual[IFNAMSIZ];
    std::snprintf(actual, IFNAMSIZ, "%s", ifr.ifr_name);
    name_ = actual;
    fd_ = fd;
    return true;
}

void TunDevice::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

void TunDevice::configure() const {
    std::string up = "ip link set " + name_ + " up >/dev/null 2>&1";
    std::string add = "ip addr add 10.0.0.1/24 dev " + name_ + " >/dev/null 2>&1";
    (void)system(up.c_str());
    (void)system(add.c_str());
    (void)system(up.c_str());
}

ssize_t TunDevice::readPacket(uint8_t* buf, size_t len) const {
    return ::read(fd_, buf, len);
}

bool TunDevice::writePacket(const uint8_t* buf, size_t len) const {
    return ::write(fd_, buf, len) == (ssize_t)len;
}
