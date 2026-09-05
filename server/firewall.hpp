#ifndef FIREWALL_H_
#define FIREWALL_H_

#include <cstdint>
#include <list>
#include <string>

#include "tcp_tracker.hpp"

class TunDevice;
struct IpHdr;

struct ParsedIp {
    IpHdr* hdr = nullptr;
    size_t ihl = 0;
    uint16_t tot = 0;
    std::string src;
    std::string dst;
};

class Firewall {
public:
    Firewall();
    void run(const std::string& tun_name = "tun0");
    void ban_words(const std::list<std::string>& words);
    void ban_ip(const std::string& ip);
    void show_rules() const;
    bool is_packet_allowed(const std::string& src_ip, const std::string& payload) const;
    bool is_ip_banned(const std::string& ip) const;

private:
    bool parseIp(uint8_t* buf, ssize_t n, ParsedIp& out) const;
    bool checkBans(const ParsedIp& p) const; // true = pode seguir
    void dispatch(TunDevice& tun, uint8_t* buf, const ParsedIp& p);

    void handleIcmp(TunDevice& tun, uint8_t* buf, const ParsedIp& p);
    void handleUdp(TunDevice& tun, uint8_t* buf, const ParsedIp& p);
    void handleTcp(TunDevice& tun, uint8_t* buf, const ParsedIp& p);

    void sendTcpSegment(TunDevice& tun, uint32_t saddr, uint32_t daddr, uint16_t sport,
                        uint16_t dport, uint32_t seq_h, uint32_t ack_h, uint8_t flags,
                        const uint8_t* data, size_t dlen);
    void handleTcpSyn(TunDevice& tun, const ParsedIp& p, uint16_t sport, uint16_t dport,
                      uint32_t seq, const std::string& key);
    void handleTcpFin(TunDevice& tun, const ParsedIp& p, uint16_t sport, uint16_t dport,
                      uint32_t seq, size_t plen, const std::string& key);
    void handleTcpData(TunDevice& tun, const ParsedIp& p, uint16_t sport, uint16_t dport,
                       uint32_t seq, const std::string& payload, const std::string& key);

    std::list<std::string> banned_words;
    std::list<std::string> banned_ips;
    TcpTracker tracker_;
    uint16_t ip_id_ = 0;
    uint32_t iss_base_ = 100000;
};

#endif // FIREWALL_H_
