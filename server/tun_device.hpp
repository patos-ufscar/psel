#ifndef TUN_DEVICE_H_
#define TUN_DEVICE_H_

#include <cstdint>
#include <string>

class TunDevice {
public:
    explicit TunDevice(const std::string& wanted_name);
    ~TunDevice();

    TunDevice(const TunDevice&) = delete;
    TunDevice& operator=(const TunDevice&) = delete;

    bool open();
    void close();
    bool isOpen() const { return fd_ >= 0; }
    int fd() const { return fd_; }
    std::string name() const { return name_; }

    void configure() const;

    ssize_t readPacket(uint8_t* buf, size_t len) const;
    bool writePacket(const uint8_t* buf, size_t len) const;

private:
    std::string wanted_;
    std::string name_;
    int fd_ = -1;
};

#endif // TUN_DEVICE_H_
