#ifndef TCP_TRACKER_H_
#define TCP_TRACKER_H_

#include <cstdint>
#include <map>
#include <string>

struct TcpConn {
    uint32_t client_ip = 0; // network order
    uint32_t server_ip = 0;
    uint16_t client_port = 0; // network order
    uint16_t server_port = 0;
    uint32_t server_next = 0; // host order
    bool established = false;
};

class TcpTracker {
public:
    static std::string key(const std::string& src, uint16_t sport_h, const std::string& dst,
                           uint16_t dport_h) {
        return src + ":" + std::to_string(sport_h) + ">" + dst + ":" + std::to_string(dport_h);
    }

    bool contains(const std::string& k) const { return conns_.count(k) > 0; }
    TcpConn* find(const std::string& k) {
        auto it = conns_.find(k);
        return it == conns_.end() ? nullptr : &it->second;
    }
    void put(const std::string& k, const TcpConn& c) { conns_[k] = c; }
    void remove(const std::string& k) { conns_.erase(k); }

private:
    std::map<std::string, TcpConn> conns_;
};

#endif // TCP_TRACKER_H_
