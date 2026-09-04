package main

import (
	"fmt"
	"log"
	"net"
	"os"
	"time"

	"golang.org/x/net/icmp"
	"golang.org/x/net/ipv4"
)

func main() {
	if len(os.Args) < 3 {
		fmt.Println("Uso:")
		fmt.Println("  ping <IP>")
		fmt.Println("  udp <IP> <porta> [payload]")
		fmt.Println("  tcp <IP> <porta> [payload]")
		os.Exit(1)
	}

	cmd := os.Args[1]
	switch cmd {
	case "ping":
		if len(os.Args) < 3 {
			log.Fatal("Uso: ping <IP>")
		}
		target := os.Args[2]
		sendPing(target)

	case "udp":
		if len(os.Args) < 4 {
			log.Fatal("Uso: udp <IP> <porta> [payload]")
		}
		target := os.Args[2]
		port := os.Args[3]
		payload := ""
		if len(os.Args) > 4 {
			payload = os.Args[4]
		}
		sendUDP(target, port, payload)

	case "tcp":
		if len(os.Args) < 4 {
			log.Fatal("Uso: tcp <IP> <porta> [payload]")
		}
		target := os.Args[2]
		port := os.Args[3]
		payload := ""
		if len(os.Args) > 4 {
			payload = os.Args[4]
		}
		sendTCP(target, port, payload)

	default:
		log.Fatalf("Comando desconhecido: %s", cmd)
	}
}

// sendPing envia um ICMP Echo Request e aguarda resposta (timeout 3s)
func sendPing(target string) {
	conn, err := icmp.ListenPacket("ip4:icmp", "0.0.0.0")
	if err != nil {
		log.Fatalf("Erro ao criar socket ICMP: %v (precisa de permissões root?)", err)
	}
	defer conn.Close()

	// Monta mensagem ICMP
	msg := icmp.Message{
		Type: ipv4.ICMPTypeEcho,
		Code: 0,
		Body: &icmp.Echo{
			ID:   os.Getpid() & 0xffff,
			Seq:  1,
			Data: []byte("ping-test"),
		},
	}
	raw, err := msg.Marshal(nil)
	if err != nil {
		log.Fatal("Erro ao montar ICMP:", err)
	}

	// Resolve IP
	raddr, err := net.ResolveIPAddr("ip4", target)
	if err != nil {
		log.Fatal("Erro ao resolver IP:", err)
	}

	// Envia
	start := time.Now()
	if _, err := conn.WriteTo(raw, raddr); err != nil {
		log.Fatal("Erro ao enviar ICMP:", err)
	}

	// Aguarda resposta
	reply := make([]byte, 1500)
	conn.SetDeadline(time.Now().Add(3 * time.Second))
	n, peer, err := conn.ReadFrom(reply)
	if err != nil {
		fmt.Printf("❌ Ping para %s: SEM RESPOSTA (timeout ou bloqueado)\n", target)
		return
	}
	elapsed := time.Since(start)

	// Processa resposta
	parsed, err := icmp.ParseMessage(ipv4.ICMPTypeEchoReply.Protocol(), reply[:n])
	if err != nil {
		log.Println("Erro ao parsear resposta:", err)
		return
	}
	switch parsed.Type {
	case ipv4.ICMPTypeEchoReply:
		fmt.Printf("✅ Ping para %s: resposta de %v, tempo %v\n", target, peer, elapsed)
	default:
		fmt.Printf("⚠️ Ping para %s: resposta inesperada tipo %v\n", target, parsed.Type)
	}
}

// sendUDP envia um pacote UDP para target:port com payload opcional
func sendUDP(target, port, payload string) {
	addr := net.JoinHostPort(target, port)
	conn, err := net.DialTimeout("udp", addr, 2*time.Second)
	if err != nil {
		log.Fatalf("Erro ao conectar UDP: %v", err)
	}
	defer conn.Close()

	// Se payload vazio, usa padrão
	if payload == "" {
		payload = "hello from client"
	}

	// Envia
	start := time.Now()
	_, err = conn.Write([]byte(payload))
	if err != nil {
		log.Fatalf("Erro ao enviar UDP: %v", err)
	}

	// Tenta ler resposta (timeout 3s)
	conn.SetReadDeadline(time.Now().Add(3 * time.Second))
	buf := make([]byte, 1024)
	n, err := conn.Read(buf)
	if err != nil {
		fmt.Printf("❌ UDP para %s:%s com payload '%s': SEM RESPOSTA (timeout ou bloqueado)\n", target, port, payload)
		return
	}
	elapsed := time.Since(start)
	fmt.Printf("✅ UDP para %s:%s com payload '%s': resposta '%s' em %v\n", target, port, payload, string(buf[:n]), elapsed)
}

// sendTCP envia um pacote TCP para target:port com payload opcional
func sendTCP(target, port, payload string) {
	addr := net.JoinHostPort(target, port)
	conn, err := net.DialTimeout("tcp", addr, 2*time.Second)
	if err != nil {
		log.Fatalf("Erro ao conectar TCP: %v", err)
	}
	defer conn.Close()

	if payload == "" {
		payload = "hello from client"
	}

	// Envia
	start := time.Now()
	_, err = conn.Write([]byte(payload))
	if err != nil {
		log.Fatalf("Erro ao enviar TCP: %v", err)
	}

	// Tenta ler resposta (timeout 3s)
	conn.SetReadDeadline(time.Now().Add(3 * time.Second))
	buf := make([]byte, 1024)
	n, err := conn.Read(buf)
	if err != nil {
		fmt.Printf("❌ TCP para %s:%s com payload '%s': SEM RESPOSTA (timeout ou bloqueado)\n", target, port, payload)
		return
	}
	elapsed := time.Since(start)
	fmt.Printf("✅ TCP para %s:%s com payload '%s': resposta '%s' em %v\n", target, port, payload, string(buf[:n]), elapsed)
}
