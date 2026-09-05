#ifndef NET_UTILS_H_
#define NET_UTILS_H_

#include <arpa/inet.h>
#include <netinet/in.h>

#include <cstdint>
#include <string>

// Similar ao https://github.com/torvalds/linux/blob/master/include/uapi/linux/ip.h
#pragma pack(push, 1)
struct IpHdr {
    uint8_t ver_ihl;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag;
    uint8_t ttl;
    uint8_t proto;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
};
#pragma pack(pop)

constexpr uint8_t IP_PROTO_ICMP = 1;
constexpr uint8_t IP_PROTO_TCP = 6;
constexpr uint8_t IP_PROTO_UDP = 17;

constexpr uint8_t TCP_FIN = 0x01;
constexpr uint8_t TCP_SYN = 0x02;
constexpr uint8_t TCP_RST = 0x04;
constexpr uint8_t TCP_PSH = 0x08;
constexpr uint8_t TCP_ACK = 0x10;

class Checksum {
public:
    static inline uint16_t raw(const uint8_t* data, size_t len) {
        uint32_t sum = 0;
        size_t i = 0;
        for (; i + 1 < len; i += 2) sum += (uint32_t)((data[i] << 8) | data[i + 1]);
        if (i < len) sum += (uint32_t)(data[i] << 8);
        while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
        return (uint16_t)(~sum & 0xFFFF);
    }

    static inline uint16_t tcpUdp(uint32_t src, uint32_t dst, uint8_t proto,
                                  const uint8_t* seg, size_t seg_len) {
        uint32_t s = ntohl(src), d = ntohl(dst);
        uint32_t sum = 0;
        sum += (s >> 16) & 0xFFFF;
        sum += s & 0xFFFF;
        sum += (d >> 16) & 0xFFFF;
        sum += d & 0xFFFF;
        sum += (uint32_t)proto;
        sum += (uint32_t)seg_len;
        size_t i = 0;
        for (; i + 1 < seg_len; i += 2) sum += (uint32_t)((seg[i] << 8) | seg[i + 1]);
        if (i < seg_len) sum += (uint32_t)(seg[i] << 8);
        while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
        uint16_t c = (uint16_t)(~sum & 0xFFFF);
        return c == 0 ? 0xFFFF : c;     }
};

class IpAddr {
public:
    static inline std::string toString(uint32_t net_order) {
        char b[INET_ADDRSTRLEN];
        struct in_addr a;
        a.s_addr = net_order;
        inet_ntop(AF_INET, &a, b, sizeof b);
        return std::string(b);
    }
// Verifica se o ip esta na subnet 10.0.0.0/2
    static inline bool inSubnet10(uint32_t daddr_net_order) {
        return (ntohl(daddr_net_order) & 0xFFFFFF00u) == 0x0A000000u;
    }
};

class Payload {
public:
    static inline std::string preview(const std::string& p, size_t max_n = 120) {
        std::string out;
        for (size_t i = 0; i < p.size() && i < max_n; ++i) {
            unsigned char c = p[i];
            out += (c >= 32 && c < 127) ? (char)c : '.';
        }
        if (p.size() > max_n) out += "...";
        return out;
    }
};

#endif // NET_UTILS_H_
