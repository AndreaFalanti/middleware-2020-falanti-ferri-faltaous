CONTIKI_PROJECT = udp-client udp-server
all: $(CONTIKI_PROJECT)

MODULES += os/net/app-layer/mqtt 

CONTIKI=../..
CONTIKI_WITH_IPV4 = 1
#CFLAGS = -Wno-nonnull-compare -Wno-implicit-function-declaration 
include $(CONTIKI)/Makefile.include
