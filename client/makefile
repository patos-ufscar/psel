##
# Firewall Test client
#
# @file
# @version 0.1
BINARY=firewall_client
GO=go

.PHONY: all build clean run

all: build

build:
	$(GO) build -o $(BINARY) main.go

clean:
	rm -f $(BINARY)

run: build
	./$(BINARY) $(ARGS)


# end
