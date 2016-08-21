PREFIX=/usr/local/

all: usbrelease

usbrelease: usbrelease.cpp
	g++ -std=c++11 ./usbrelease.cpp -lusb-1.0 -o usbrelease

install:
	install ./usbrelease ${PREFIX}/bin/usbrelease
