#include "firewall.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

#include "logger.hpp"
#include "net_utils.hpp"
#include "tun_device.hpp"

Firewall::Firewall() {}

void Firewall::ban_words(const std::list<std::string>& words) {
    for (const auto& w : words) banned_words.push_back(w);
}

void Firewall::ban_ip(const std::string& ip) { banned_ips.push_back(ip); }

void Firewall::show_rules() const {
    std::cout << "Banned IPs: ";
    for (const auto& ip : banned_ips) std::cout << ip << std::endl;
    std::cout << "Banned Words: ";
    for (const auto& w : banned_words) std::cout << w << " ";
    std::cout << std::endl;
}

bool Firewall::is_ip_banned(const std::string& ip) const {
    for (const auto& b : banned_ips)
        if (b == ip) return true;
    return false;
}

bool Firewall::is_packet_allowed(const std::string& src_ip, const std::string& payload) const {
    if (is_ip_banned(src_ip)) return false;
    for (const auto& w : banned_words)
        if (!w.empty() && payload.find(w) != std::string::npos) return false;
    return true;
}


void Firewall::run(const std::string& tun_name) {
    TunDevice tun(tun_name);
    if (!tun.open()) {
        Logger::info("TUN", "falha ao criar TUN. Rode como root (sudo).");
        return;
    }
    tun.configure();

    show_rules();
    Logger::info("FW", "ouvindo em " + tun.name() + " (10.0.0.1/24).");

    uint8_t buf[4096];
    while (true) {
        ssize_t n = tun.readPacket(buf, sizeof buf);
        if (n < 0) {
            perror("read tun");
            continue;
        }
        ParsedIp p;
        if (!parseIp(buf, n, p)) continue;
        if (!IpAddr::inSubnet10(p.hdr->daddr)) continue;
        if (!checkBans(p)) continue;
        dispatch(tun, buf, p);
    }
}

bool Firewall::parseIp(uint8_t* buf, ssize_t n, ParsedIp& out) const {
    if (n < (ssize_t)sizeof(IpHdr)) return false;
    IpHdr* ip = (IpHdr*)buf;
    if ((ip->ver_ihl >> 4) != 4) return false;
    size_t ihl = (ip->ver_ihl & 0x0F) * 4;
    if (ihl < 20 || n < (ssize_t)ihl) return false;
    uint16_t tot = ntohs(ip->tot_len);
    if (tot > (uint16_t)n) tot = (uint16_t)n;
    out.hdr = ip;
    out.ihl = ihl;
    out.tot = tot;
    out.src = IpAddr::toString(ip->saddr);
    out.dst = IpAddr::toString(ip->daddr);
    return true;
}

bool Firewall::checkBans(const ParsedIp& p) const {
    if (is_ip_banned(p.src) || is_ip_banned(p.dst)) {
        Logger::info("DROP", "IP bloqueado src=" + p.src + " dst=" + p.dst +
                                 " proto=" + std::to_string((int)p.hdr->proto));
        return false;
    }
    return true;
}

void Firewall::dispatch(TunDevice& tun, uint8_t* buf, const ParsedIp& p) {
    switch (p.hdr->proto) {
        case IP_PROTO_ICMP: handleIcmp(tun, buf, p); break;
        case IP_PROTO_UDP: handleUdp(tun, buf, p); break;
        case IP_PROTO_TCP: handleTcp(tun, buf, p); break;
        default:
            Logger::info("DROP", "proto " + std::to_string((int)p.hdr->proto) + " nao suportado src=" +
                                      p.src + " dst=" + p.dst);
    }
}


void Firewall::handleIcmp(TunDevice& tun, uint8_t* buf, const ParsedIp& p) {
    if (p.tot < p.ihl + 8) return;
    uint8_t* ic = buf + p.ihl;
    if (ic[0] != 8) return; // so Echo Request
    uint16_t id = (ic[4] << 8) | ic[5];
    uint16_t seq = (ic[6] << 8) | ic[7];
    Logger::info("ICMP", "echo-request " + p.src + " -> " + p.dst + " id=" + std::to_string(id) +
                             " seq=" + std::to_string(seq));

    uint8_t out[4096];
    std::memcpy(out, buf, p.tot);
    IpHdr* oip = (IpHdr*)out;
    std::swap(oip->saddr, oip->daddr);
    oip->ttl = 64;
    oip->check = 0;
    oip->check = htons(Checksum::raw(out, p.ihl));

    uint8_t* oic = out + p.ihl;
    oic[0] = 0; // echo reply
    oic[2] = oic[3] = 0;
    uint16_t c = Checksum::raw(oic, p.tot - p.ihl);
    oic[2] = c >> 8;
    oic[3] = c & 0xFF;
    tun.writePacket(out, p.tot);
    Logger::info("ICMP", "echo-reply " + p.dst + " -> " + p.src);
}


void Firewall::handleUdp(TunDevice& tun, uint8_t* buf, const ParsedIp& p) {
    constexpr size_t UDP_HDR_LEN = 8;
    constexpr size_t MAX_PKT_SIZE = 4096;
    if (p.tot < p.ihl + UDP_HDR_LEN || p.tot > MAX_PKT_SIZE) {
            return;
    }
    uint8_t* u = buf + p.ihl;
    uint16_t sport, dport;
    std::memcpy(&sport, u, 2);
    std::memcpy(&dport, u + 2, 2);
    size_t payload_len = p.tot > p.ihl + 8 ? p.tot - p.ihl - 8 : 0;
    std::string payload(payload_len ? (char*)(u + 8) : "", payload_len);

    std::ostringstream oss;
    oss << "udp " << p.src << ":" << ntohs(sport) << " -> " << p.dst << ":" << ntohs(dport)
        << " len=" << payload_len << " payload='" << Payload::preview(payload) << "'";
    Logger::info("UDP", oss.str());

    if (!is_packet_allowed(p.src, payload)) {
        Logger::info("DROP", "UDP bloqueado (palavra/IP) de " + p.src);
        return;
    }

    uint8_t out[MAX_PKT_SIZE];
    size_t total = p.ihl + 8 +      payload_len;
    std::memcpy(out, buf, total);
    IpHdr* oip = (IpHdr*)out;
    std::swap(oip->saddr, oip->daddr);
    oip->ttl = 64;
    oip->tot_len = htons((uint16_t)total);
    oip->check = 0;
    oip->check = htons(Checksum::raw(out, p.ihl));

    uint8_t* ou = out + p.ihl;
    std::memcpy(ou, u + 2, 2);
    std::memcpy(ou + 2, u, 2);
    uint16_t nl = htons((uint16_t)(8 + payload_len));
    std::memcpy(ou + 4, &nl, 2);
    ou[6] = ou[7] = 0;
    uint16_t c = Checksum::tcpUdp(oip->saddr, oip->daddr, IP_PROTO_UDP, ou, 8 + payload_len);
    uint16_t cb = htons(c);
    std::memcpy(ou + 6, &cb, 2);
    tun.writePacket(out, total);
    Logger::info("UDP", "echo respondido para " + p.src);
}


void Firewall::sendTcpSegment(TunDevice& tun, uint32_t saddr, uint32_t daddr, uint16_t sport,
                              uint16_t dport, uint32_t seq_h, uint32_t ack_h, uint8_t flags,
                              const uint8_t* data, size_t dlen) {
    uint8_t out[4096];
    size_t total = 20 + 20 + dlen;
    if (total > sizeof out) return;
    IpHdr* oip = (IpHdr*)out;
    oip->ver_ihl = 0x45;
    oip->tos = 0;
    oip->tot_len = htons((uint16_t)total);
    oip->id = htons(ip_id_++);
    oip->frag = htons(0x4000);
    oip->ttl = 64;
    oip->proto = IP_PROTO_TCP;
    oip->check = 0;
    oip->saddr = saddr;
    oip->daddr = daddr;
    oip->check = htons(Checksum::raw(out, 20));

    uint8_t* t = out + 20;
    std::memcpy(t, &sport, 2);
    std::memcpy(t + 2, &dport, 2);
    uint32_t sq = htonl(seq_h), aq = htonl(ack_h);
    std::memcpy(t + 4, &sq, 4);
    std::memcpy(t + 8, &aq, 4);
    t[12] = 0x50;
    t[13] = flags;
    uint16_t w = htons(65535);
    std::memcpy(t + 14, &w, 2);
    t[16] = t[17] = t[18] = t[19] = 0;
    if (dlen) std::memcpy(t + 20, data, dlen);
    uint16_t c = Checksum::tcpUdp(saddr, daddr, IP_PROTO_TCP, t, 20 + dlen);
    uint16_t cb = htons(c);
    std::memcpy(t + 16, &cb, 2);
    tun.writePacket(out, total);
}

void Firewall::handleTcp(TunDevice& tun, uint8_t* buf, const ParsedIp& p) {
    if (p.tot < p.ihl + 20) return;
    uint8_t* t = buf + p.ihl;
    uint16_t sport, dport;
    uint32_t seq, ack;
    std::memcpy(&sport, t, 2);
    std::memcpy(&dport, t + 2, 2);
    std::memcpy(&seq, t + 4, 4);
    std::memcpy(&ack, t + 8, 4);
    seq = ntohl(seq);
    ack = ntohl(ack);
    size_t doff = (t[12] >> 4) * 4;
    if (doff < 20 || p.tot < p.ihl + doff) return;
    uint8_t flags = t[13];
    size_t plen = p.tot > p.ihl + doff ? p.tot - p.ihl - doff : 0;
    std::string payload(plen ? (char*)(t + doff) : "", plen);

    std::string key =
        TcpTracker::key(p.src, ntohs(sport), p.dst, ntohs(dport));
    Logger::info("TCP", key + " flags=" + std::to_string((int)flags) + " seq=" +
                            std::to_string(seq) + " len=" + std::to_string(plen) + " payload='" +
                            Payload::preview(payload) + "'");

    if (flags & TCP_RST) {
        tracker_.remove(key);
        Logger::info("TCP", "RST, conexao removida " + key);
        return;
    }
    if ((flags & TCP_SYN) && !(flags & TCP_ACK)) {
        handleTcpSyn(tun, p, sport, dport, seq, key);
        return;
    }
    if (flags & TCP_FIN) {
        handleTcpFin(tun, p, sport, dport, seq, plen, key);
        return;
    }
    if (plen > 0) {
        handleTcpData(tun, p, sport, dport, seq, payload, key);
        return;
    }
    if (TcpConn* c = tracker_.find(key); c && !c->established) {
        c->established = true;
        Logger::info("TCP", "conexao estabelecida " + key);
    }
}

void Firewall::handleTcpSyn(TunDevice& tun, const ParsedIp& p, uint16_t sport, uint16_t dport,
                            uint32_t seq, const std::string& key) {
    TcpConn c;
    c.client_ip = p.hdr->saddr;
    c.server_ip = p.hdr->daddr;
    c.client_port = sport;
    c.server_port = dport;
    c.server_next = iss_base_++;
    tracker_.put(key, c);
    sendTcpSegment(tun, p.hdr->daddr, p.hdr->saddr, dport, sport, c.server_next, seq + 1,
                   TCP_SYN | TCP_ACK, nullptr, 0);
    if (TcpConn* s = tracker_.find(key)) s->server_next += 1;
    Logger::info("TCP", "SYN-ACK enviado " + key + " ack=" + std::to_string(seq + 1));
}

void Firewall::handleTcpFin(TunDevice& tun, const ParsedIp& p, uint16_t sport, uint16_t dport,
                            uint32_t seq, size_t plen, const std::string& key) {
    uint32_t sn = tracker_.contains(key) ? tracker_.find(key)->server_next : iss_base_++;
    uint32_t ackn = seq + (uint32_t)plen + 1;
    sendTcpSegment(tun, p.hdr->daddr, p.hdr->saddr, dport, sport, sn, ackn, TCP_ACK, nullptr, 0);
    sendTcpSegment(tun, p.hdr->daddr, p.hdr->saddr, dport, sport, sn, ackn, TCP_FIN | TCP_ACK,
                   nullptr, 0);
    tracker_.remove(key);
    Logger::info("TCP", "FIN respondido " + key);
}

void Firewall::handleTcpData(TunDevice& tun, const ParsedIp& p, uint16_t sport, uint16_t dport,
                             uint32_t seq, const std::string& payload, const std::string& key) {
    if (!is_packet_allowed(p.src, payload)) {
        uint32_t sn = tracker_.contains(key) ? tracker_.find(key)->server_next : iss_base_++;
        sendTcpSegment(tun, p.hdr->daddr, p.hdr->saddr, dport, sport, sn,
                       seq + (uint32_t)payload.size(), TCP_RST | TCP_ACK, nullptr, 0);
        tracker_.remove(key);
        Logger::info("DROP", "TCP bloqueado (palavra/IP), RST enviado " + key + " payload='" +
                                  Payload::preview(payload) + "'");
        return;
    }
    uint32_t sn = tracker_.contains(key) ? tracker_.find(key)->server_next : iss_base_++;
    sendTcpSegment(tun, p.hdr->daddr, p.hdr->saddr, dport, sport, sn,
                   seq + (uint32_t)payload.size(), TCP_PSH | TCP_ACK,
                   (const uint8_t*)payload.data(), payload.size());
    TcpConn* c = tracker_.find(key);
    if (!c) {
        TcpConn nc;
        nc.client_ip = p.hdr->saddr;
        nc.server_ip = p.hdr->daddr;
        nc.client_port = sport;
        nc.server_port = dport;
        nc.server_next = sn + (uint32_t)payload.size();
        nc.established = true;
        tracker_.put(key, nc);
    } else {
        c->server_next = sn + (uint32_t)payload.size();
        c->established = true;
    }
    Logger::info("TCP", "echo (" + std::to_string(payload.size()) + "B) enviado " + key);
}
