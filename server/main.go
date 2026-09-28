// the code for each one of the servers

package main

import (
	"bufio"
	"errors"
	"flag"
	"fmt"
	"io"
	"log"
	"math/rand"
	"net"
	"os"
	"path/filepath"
	"strings"
	"time"

	"github.com/CalvinOgata/psel_patos_loadBalancer/internal" // the hand-written HTTP parser, shared with the balancer
)

var (
	// informs the main image folder
	imagesDir = flag.String("images", "images", "Folder holding the images to serve")
	// holds the answer back on purpose, so the balancer has a slow backend to route around
	delay = flag.Duration("delay", 0, "Artificial delay before answering")
)

// makes sure the connection is up for the LD to send stuff
func main() {
	port := flag.String("port", "8081", "Port to listen on")
	flag.Parse()

	if err := os.MkdirAll(*imagesDir, 0755); err != nil {
		log.Fatalf("Failed to create images directory: %v", err)
	}

	listener, err := net.Listen("tcp", ":"+*port)
	if err != nil {
		log.Fatalf("Failed to start server: %v", err)
	}
	defer listener.Close()

	fmt.Printf("Server running on http://localhost:%s\n", *port)

	for {
		connection, err := listener.Accept()
		if err != nil {
			continue
		}
		go handleConnection(connection)
	}
}

// handles the connection and distributes tasks based on the input
func handleConnection(connection net.Conn) {
	defer connection.Close()

	// kills the connection after a while if there's no response
	connection.SetDeadline(time.Now().Add(internal.TransferTimeout))

	reader := bufio.NewReader(connection)
	request, err := internal.ReadRequest(reader, internal.MaxBodySize) // verifies again if the content isn't too large
	if err != nil {
		if errors.Is(err, internal.ErrBodyTooLarge) {
			internal.WriteText(connection, "413 Content Too Large", "That file is too big")
			return
		}
		// a connection that closes without saying anything is what a health
		// probe looks like from here, so it is not worth a line of log
		if !errors.Is(err, io.EOF) {
			log.Printf("Dropped a request that could not be parsed: %v", err)
		}
		return
	}

	switch {
	case request.Method == "GET" && request.Path == "/random-image":
		serveImage(connection)
	case request.Method == "POST" && strings.HasPrefix(request.Path, "/upload/"):
		storeImage(connection, request)
	default:
		internal.WriteText(connection, "404 Not Found", "Non-existing path")
	}
}

// serves a random image from the 'images' folder
func serveImage(connection net.Conn) {
	// the connection stays open while it sleeps, which is what makes this
	// instance look busy to the balancer
	time.Sleep(*delay)

	name, err := pickRandomImage()
	if err != nil {
		internal.WriteText(connection, "404 Not Found", err.Error())
		return
	}

	body, err := os.ReadFile(filepath.Join(*imagesDir, name))
	if err != nil {
		internal.WriteText(connection, "404 Not Found", "Image not Found")
		return
	}

	internal.WriteResponse(connection, "200 OK", contentTypeFor(name), body, "X-Image-Name: "+name)
}

// stores an uploaded image, after making sure it really is one
func storeImage(connection net.Conn, request *internal.Request) {
	name, err := internal.SafeImageName(strings.TrimPrefix(request.Path, "/upload/"))
	if err != nil {
		internal.WriteText(connection, "400 Bad Request", err.Error())
		return
	}

	if len(request.Body) == 0 {
		internal.WriteText(connection, "400 Bad Request", "Empty upload")
		return
	}

	if !internal.IsImage(request.Body) {
		internal.WriteText(connection, "415 Unsupported Media Type", "That file is not a PNG or a JPEG")
		return
	}

	stored := internal.StoredName(name, request.Body)
	if err := writeImageFile(stored, request.Body); err != nil {
		log.Printf("Failed to store %s: %v", stored, err)
		internal.WriteText(connection, "500 Internal Server Error", "Could not store the image")
		return
	}

	log.Printf("Stored %s (%d bytes)", stored, len(request.Body))
	internal.WriteResponse(connection, "201 Created", "text/plain",
		[]byte("Stored as "+stored), "X-Image-Name: "+stored)
}

// writes to a temporary file and renames it into place. A rename inside one folder
// is atomic, so a request drawing a random image never catches a half-written file
func writeImageFile(name string, body []byte) error {
	temporary, err := os.CreateTemp(*imagesDir, ".upload-*")
	if err != nil {
		return err
	}
	defer os.Remove(temporary.Name()) // does nothing once the rename below lands

	if _, err := temporary.Write(body); err != nil {
		temporary.Close()
		return err
	}

	// CreateTemp opens at 0600, which would leave uploads less readable than the
	// images that shipped with the project
	if err := temporary.Chmod(0644); err != nil {
		temporary.Close()
		return err
	}
	if err := temporary.Close(); err != nil {
		return err
	}

	return os.Rename(temporary.Name(), filepath.Join(*imagesDir, name))
}

// picks a random image from the folder
func pickRandomImage() (string, error) {
	entries, err := os.ReadDir(*imagesDir)
	if err != nil {
		return "", fmt.Errorf("could not read the images folder")
	}

	var files []string
	for _, entry := range entries {
		// skips folders, and the temporary an upload leaves behind mid-write
		if !entry.IsDir() && internal.HasImageExtension(entry.Name()) {
			files = append(files, entry.Name())
		}
	}

	if len(files) == 0 {
		return "", fmt.Errorf("no images on the folder")
	}

	return files[rand.Intn(len(files))], nil
}

// matches the specific content type
func contentTypeFor(name string) string {
	switch strings.ToLower(filepath.Ext(name)) {
	case ".png":
		return "image/png"
	case ".jpg", ".jpeg":
		return "image/jpeg"
	default:
		return "application/octet-stream"
	}
}
