// entire code of the load balancer

package main

import (
	"bufio"       // wraps stream readers and writers into a buffer
	"errors"      // tells one parse failure apart from another
	"fmt"         // standard library for cli output
	"io"          // copies bytes straight from one connection into another
	"log"         // standard library for console output regarding concurrency and errors
	"math/rand"   // breaks the tie between backends that are equally idle
	"net"         // standard library for TCP/IP connections and domain sockets
	"os"          // read files from disk
	"sort"        // orders the backends by how busy they are
	"strings"     // matches the upload path prefix
	"sync/atomic" // allows Atomic types that make it better for goroutines to read
	"time"        // health check interval and dial timeouts

	"github.com/CalvinOgata/psel_patos_loadBalancer/internal" // the hand-written HTTP parser, shared with the backend
)

// struct of the servers
type backend struct {
	address  string
	alive    atomic.Bool
	inflight atomic.Int64
}

// if the backend doesn't answer in a second, give up on it
const dialTimeout = time.Second

// backend took too long to answer
const backendTimeout = 10 * time.Second

var pool = []*backend{ // all the backend servers available
	{address: "localhost:8081"},
	{address: "localhost:8082"},
	{address: "localhost:8083"},
	{address: "localhost:8084"},
	{address: "localhost:8085"},
}

// start the TCP listener and 'connect' to the user
func main() {
	startHealthChecks(3 * time.Second) // checks who's dead and who's alive among the servers

	listener, err := net.Listen("tcp", ":8080")
	if err != nil {
		log.Fatalf("Failed to start TCP listener: %v", err)
	}
	defer listener.Close()

	fmt.Println("TCP Server listening on http://localhost:8080")

	for {
		connection, err := listener.Accept()
		if err != nil {
			log.Printf("Failed to accept connection: %v", err)
			continue
		}

		go handleConnection(connection)
	}
}

// maintain the individual connection every time it's requested
func handleConnection(connection net.Conn) {
	defer connection.Close()

	// connections are closed after the internal timout (30 seconds)
	connection.SetDeadline(time.Now().Add(internal.TransferTimeout))

	reader := bufio.NewReader(connection) // the "raw" HTTP request
	request, err := internal.ReadRequest(reader, internal.MaxBodySize)
	if err != nil {
		if errors.Is(err, internal.ErrBodyTooLarge) { // file is too big
			internal.WriteText(connection, "413 Content Too Large", "That file is too big")
			return
		}
		if !errors.Is(err, io.EOF) {
			log.Printf("Dropped a request that could not be parsed: %v", err)
		}
		return
	}

	switch {
	case request.Path == "/":
		serveIndex(connection, request.Method) // main HTML page
	case request.Path == "/random-image":
		proxyRequest(connection, request) // handed over to the backend server (select random image)
	case request.Method == "POST" && strings.HasPrefix(request.Path, "/upload/"):
		uploadImage(connection, request) // checked here, then handed over as well
	default:
		internal.WriteText(connection, "404 Not Found", "404 Not Found")
	}
}

// options available of specific services
func serveIndex(connection net.Conn, method string) {
	switch method {
	case "GET": // sends the HTML file across the connection
		html, err := os.ReadFile("index.html")
		if err != nil {
			internal.WriteText(connection, "500 Internal Server Error", "Could not read index.html")
			return
		}
		internal.WriteResponse(connection, "200 OK", "text/html", html)
	case "POST": // updates with the "Hello World" message, sent to JavaScript and formulated afterwards
		internal.WriteText(connection, "200 OK", "Hello World!")
	default:
		internal.WriteText(connection, "405 Method Not Allowed", "That's illegal...")
	}
}

// probes the whole pool once, then keeps probing it on every tick
func startHealthChecks(interval time.Duration) {
	for _, b := range pool {
		probe(b)
	}

	go func() {
		for range time.Tick(interval) {
			for _, b := range pool {
				go probe(b)
			}
		}
	}()
}

// checks if the backend is alive
func probe(b *backend) {
	connection, err := net.DialTimeout("tcp", b.address, dialTimeout)
	if err != nil { // backend is dead 😢
		if b.alive.Swap(false) {
			log.Printf("Backend %s went down: %v", b.address, err)
		}
		return
	}
	connection.Close()

	if !b.alive.Swap(true) { // backend is alive again 😄
		log.Printf("Backend %s is back up", b.address)
	}
}

// background checks the image file upload before it reaches to server
// to remove unnecessary work-around
func uploadImage(client net.Conn, request *internal.Request) {
	if _, err := internal.SafeImageName(strings.TrimPrefix(request.Path, "/upload/")); err != nil { // image contains invalid characters
		internal.WriteText(client, "400 Bad Request", err.Error())
		return
	}

	if len(request.Body) == 0 { // empty body
		internal.WriteText(client, "400 Bad Request", "Empty upload")
		return
	}

	if !internal.IsImage(request.Body) { // is not an image at all
		internal.WriteText(client, "415 Unsupported Media Type", "That file is not a PNG or a JPEG")
		return
	}

	proxyRequest(client, request)
}

// hands the request to the least busy backend
func proxyRequest(client net.Conn, request *internal.Request) {
	alive := candidates() // analyses the alive ones (function at the end of the file)

	// shuffles the healthy ones to prevent all the requests being send to only one backend
	rand.Shuffle(len(alive), func(i, j int) { alive[i], alive[j] = alive[j], alive[i] })
	sort.SliceStable(alive, func(i, j int) bool {
		return alive[i].inflight.Load() < alive[j].inflight.Load()
	})

	for _, b := range alive {
		if relayTo(b, client, request) {
			return
		}
	}

	internal.WriteText(client, "502 Bad Gateway", "No backend available")
}

// finally, forwards the request to the backend client
func relayTo(b *backend, client net.Conn, request *internal.Request) bool {
	backendConn, err := net.DialTimeout("tcp", b.address, dialTimeout)
	if err != nil { // checks again to make sure it didn't die mid way
		log.Printf("Backend %s is down: %v", b.address, err)
		b.alive.Store(false) // the next probe puts it back once it answers again
		return false
	}
	defer backendConn.Close()

	// makes sure the backend answers... otherwise it's dead
	backendConn.SetDeadline(time.Now().Add(backendTimeout))

	b.inflight.Add(1) // adds one when it receives a request
	defer b.inflight.Add(-1)

	// rebuilds the request by hand because the previous functions destroy it
	if err := internal.WriteRequest(backendConn, request.Method, request.Path, b.address, request.Body, contentTypeOf(request)...); err != nil {
		log.Printf("Failed to send the request to %s: %v", b.address, err)
		return false
	}

	log.Printf("%s -> %s", request.Path, b.address)

	// the backend answers with a full HTTP response, so it goes back verbatim
	written, err := io.Copy(client, backendConn)
	if err != nil {
		log.Printf("Failed to relay the answer from %s: %v", b.address, err)
		// retrying is only safe while the client has seen nothing
		return written > 0
	}

	return true
}

// check which backends are alive
func candidates() []*backend {
	alive := make([]*backend, 0, len(pool))
	for _, b := range pool {
		if b.alive.Load() {
			alive = append(alive, b)
		}
	}

	// tries the whole pool anyway, even if all our dead
	if len(alive) == 0 {
		alive = append(alive, pool...)
	}

	return alive
}

// used by relayTo, to prevent the backend from guessing the type
func contentTypeOf(request *internal.Request) []string {
	contentType := request.Header("Content-Type")
	if contentType == "" || len(request.Body) == 0 {
		return nil
	}

	return []string{"Content-Type: " + contentType}
}
