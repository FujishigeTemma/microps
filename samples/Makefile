APPS = tap.exe

ifeq ($(shell uname),Linux)
      APPS := $(APPS) pf_packet.exe
endif

ifeq ($(shell uname),Darwin)
      APPS := $(APPS) bpf.exe
endif

.SUFFIXES:
.SUFFIXES: .c

.PHONY: all clean

CFLAGS := $(CFLAGS) -g -W -Wall -Wno-unused-parameter

all: $(APPS)

$(APPS): %.exe : %.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(APPS)

.PHONY: init_tap
init_tap:
	mkdir -p /dev/net
	mknod /dev/net/tun c 10 200
	ip tuntap add mode tap user root name tap0
	ip addr add 192.0.2.1/24 dev tap0
	ip link set tap0 up
