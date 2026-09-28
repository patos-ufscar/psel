# PATOS PSEL 2.0 - Load Balancer in GO

Welcome to my submission for the PATOS Selection Process (PSEL 2.0). 

## About the Project

This repository contains a custom **Load Balancer built from scratch in Golang**. 

In accordance with the challenge guidelines, this project avoids high-level abstractions, web frameworks, and standard HTTP libraries (such as `net/http`). Instead, it handles raw network sockets (`net.Conn`), manages manual HTTP request/header parsing, and manages traffic distribution using low-level Go primitives.

## Features & Implementation Strategy

- **Raw Socket Operations:** Built directly on top of Go's `net` package for raw TCP listeners and connections.
- **Custom Request Parser:** A hand-written HTTP reader, shared by the balancer and the backends, that parses the request line, the header block and the body with no external parsing libraries. Both tiers read a request through the same code, so a proxy and the server behind it can never disagree about where one request ends and the next begins.
- **Load Balancing Strategy:** Least Connections selection, filtered by backend health, with a random tie-break and fallback to the next candidate when a backend cannot be reached.
- **Active Health Checking:** A background routine probing every backend over raw TCP on a fixed interval, tracking each one as it goes down and comes back.
- **File Uploads:** Images arrive as the raw body of a request, are validated by their signature bytes rather than by anything the client claims about them, and are written under a name derived from their own contents, so two files never collide and the same file never duplicates.
- **Timeouts and Deadlines:** Dial timeouts and connection deadlines on both tiers, with the balancer deliberately giving up on a silent backend before its own client gives up on it.
- **No Dependencies:** Standard library only. `go.mod` requires nothing at all.

## Running It

All you need is Go. There are no dependencies to fetch.

```bash
git clone https://github.com/CalvinOgata/psel_patos_loadBalancer.git
cd psel_patos_loadBalancer
./run.sh
```

`run.sh` starts five backend servers on ports 8081 through 8085 and the load balancer on port 8080. Open http://localhost:8080 in a browser and you get a page with a button that asks for a random image and a file picker that sends a new one.

Three routes are served:

| Route | What it does |
| --- | --- |
| `GET /` | The page itself |
| `GET /random-image` | Proxied to a backend, which answers with the bytes of a random image and its file name in an `X-Image-Name` header |
| `POST /upload/<name>` | Takes the raw bytes of a PNG or JPEG as the request body and stores it under a name derived from its contents |

Uploading from the terminal instead of the page works the same way:

```bash
curl -X POST --data-binary @picture.png http://localhost:8080/upload/picture.png
```

Two things make the balancing visible. Replacing one of the five with a deliberately slow instance gives the balancer something to route around, and the pool only knows about ports 8081 through 8085, so the replacement has to reuse one of them:

```bash
fuser -k -n tcp 8082
go run ./server -port 8082 -delay 2s
```

Sending requests one at a time will not show much: with a single request in flight every backend looks equally idle, so the slow one still takes its turn. Send a burst and the least connections rule has something to work with, and the delayed instance starts being passed over:

```bash
seq 20 | xargs -P 20 -I{} curl -s -o /dev/null -w '%{time_total}\n' http://localhost:8080/random-image
```

Killing a backend shows the other half, the health check: it notices within three seconds, says so in the balancer's output, and requests carry on being served by whoever is left.

Ctrl+C stops the balancer, but the five backends were started as background jobs and keep running on their own, so they need stopping separately:

```bash
fuser -k -n tcp 8081 8082 8083 8084 8085
```

---

### Day 0: Getting Started

Laid the groundwork for the project today. I updated `README.md` with the overall project scope, set up `main.go` for the initial core logic, and built a basic `index.html` interface to make testing visual rather than terminal-bound. 

Most of my time was spent studying TCP/IP and HTTP fundamentals, specifically how HTTP is essentially structured text parsing over a network stream. I also spent time dissecting Go's standard libraries (`net`, `bufio`) to understand precisely how each function and connection operates under the hood.

### Day 1: System Architecture & Raw Backend Prototype

Took a step back today to refine the overall system architecture. I mapped out the core interaction between three primary components: the frontend interface, the load balancer, and the backend server. To keep the codebase clean and follow idiomatic Go project conventions, I reorganized the load balancer and backend entry points into separate subdirectories (`balancer` and `server`).

I also evaluated whether to run backend instances inside Docker containers or as standalone Go processes. I decided to start with native Go processes to maintain direct visibility into OS socket behavior before introducing containerization overhead. Finally, I wrote an initial prototype for the raw TCP backend server; it's still bare-bones, but it lays the foundation for image serving in the coming days.

### Day 2: Basic Proxy, Run Script & Working Image Delivery

Today was a tough one, but it got the core of the project working end to end. On the backend I tightened up the error handling, restricted the served files to PNG and JPEG, and added an `images` folder so the application finally has something real to hand out. I also reworked the frontend (`index.html`) so it stops announcing "Hello World" and actually displays the images it receives. To avoid juggling two terminals every time I wanted to test something, I wrote a small `run.sh` that starts the backend server and the load balancer together.

The backend was by far the most time-consuming part. Adding new routes and reshaping them as the application kept evolving was challenging, and getting everything to line up (the manual HTTP parsing, the ports, and Go's syntax, which is fun and intuitive but has its quirks) was genuinely painful at times.

Two bugs stand out. The first: the button kept returning a 404 and I was convinced the backend was at fault, but the request never reached it. The balancer only knew the `/` route, so it was answering `/random-image` itself. Worse, its response builder hardcoded `HTTP/1.1 200 OK`, so that "404" arrived at the browser as a *successful* response and the page cheerfully rendered the error text as an image path. The second: the server created `./images` but read from `../images`: two different folders, depending on where the process was launched from. Both were good reminders that when you write HTTP by hand, nothing is going to catch these mistakes for you.

I also simplified the image flow. It used to take two round trips: one to ask for a random image's path, another to fetch that path. That only works because every backend currently reads the same folder; once each instance has its own filesystem, the balancer could route the two requests to different backends and ask instance B for a file only instance A has. Now `/random-image` returns the image bytes directly, with the file name riding along in an `X-Image-Name` header for the page to display: one click, one request, one backend. I added a guard against path traversal as well, since a raw socket server joins user input straight onto a file path with nothing in between.

The proxy foundation is laid, but it currently forwards everything to a single hardcoded backend at `localhost:8081`, and answers with a 502 when that one is down because it has nowhere else to turn. Turning that constant into a proper pool (with round-robin selection and health checks) is the job for the coming days.

### Day 3: Backend Pool, Health Checks & Least-Connections Balancing

The balancer finally has something to balance. Where there was one hardcoded address there are now five backends, with a health check quietly pinging each of them every three seconds; it only speaks up when one dies or comes back, which makes the terminal feel a little like listening for a heartbeat. Requests go to whichever healthy backend is least busy, and if one refuses the connection the balancer shrugs and tries the next. I also shut the door on asking for a specific image: you get what the random draw gives you.

Then I got curious about something small, and it turned into the lesson of the day. Requests here take about two milliseconds, so by the time the balancer looks, every backend reports zero open connections; they are always tied. Which means the tie-break, the part I had treated as a footnote, was quietly making every routing decision on its own. Breaking ties by lowest index sent forty requests in a row to the same backend. Forty. The other four just sat there, and nothing looked wrong at all. A random shuffle before the sort was enough to spread the same forty across all five.

Tomorrow, uploads, which means teaching both sides to read past the first line of a request for the very first time.

### Day 4: Uploads, a Parser of Its Own & the Timeouts I Never Wrote

I wanted today to be the last one. This project had been building up in my head for months, far longer than it has existed as a folder on disk, and I sat down this morning meaning to finish it rather than to improve it. That turned out to be a useful way to work: instead of adding what would be interesting, I kept asking what was still missing.

The first answer was the parser. Day 3 ended with a promise to teach both sides to read past the first line of a request, and the moment I started writing that, the parsing stopped being a `strings.Fields` call and became a set of rules: where the header block ends, how long a line may be, how many bytes of body to expect. Rules are the kind of thing two copies of a file quietly stop agreeing about. That is exactly the disagreement I had been avoiding since Day 2, when I chose to rebuild the request line by hand rather than forward raw bytes, so before writing the header parsing twice I moved it into a package both tiers import. The balancer and the backend now read a request through the same code, and the function that writes a request to a backend sits twenty lines below the one that reads it, which is the closest thing to a guarantee I can give myself that they will stay in step.

Then, finally, uploads. The body is the raw file, with the name in the path, which meant no `multipart/form-data` and therefore no hand-written boundary scanner: the single largest piece of tedium I have managed to avoid in this whole project. What surprised me is how little of the work was the upload and how much of it was refusing one. An extension is a claim, a `Content-Type` is a claim, so the first bytes get checked against the PNG and JPEG signatures instead. Names are narrowed to a small character set, which means nothing ever needs decoding and nothing can climb out of the folder. Each file is written under a name derived from its own contents, so two files cannot collide and uploading the same image twice overwrites it instead of breeding copies. And every write goes to a temporary file that is renamed into place, because a rename is atomic and a half written image is otherwise something a random draw can serve. That last detail made me finally filter the folder by extension, a loose end that had been sitting there since Day 2.

The real lesson came from going looking for trouble once I thought I was done. There was exactly one timeout in the entire project, on the health check, and none at all on the path a request actually travels. Every backend I had killed in four days was a local process, and a local process refuses connections instantly, so I had been testing only the polite kind of failure. A backend that accepts your connection and then says nothing at all would have held a request open forever. Four lines fixed it, and then a smaller mistake taught me something better: my first attempt gave the client and the backend the same thirty second deadline, so they expired in the same instant and the balancer had no time left to explain itself. The browser got an empty reply where a 502 belonged. A proxy's patience with a backend has to run out before its client's patience runs out with it, otherwise giving up is indistinguishable from crashing.

The last thing I did was delete code. Uploads had been going to all five backends, which is the right answer if each one owns its own filesystem, and no answer at all for five processes sharing one folder. I decided the shared store is the architecture rather than a shortcut, and took the replication out. Finishing, it turns out, looks a lot less like writing than I expected.

So this is the line I am drawing. Every commit after this one is documentation: a comment reworded, a small fix if something surfaces, nothing that changes how any of it works. The bulky work is done. After months of meaning to get here, the satisfying part turns out not to be the code that runs, it is that there is nothing left I am putting off.
