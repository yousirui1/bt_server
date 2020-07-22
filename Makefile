VERSION = 1.1

# arm -> linux arm rk3188 
# x86 -> window x86 mingw32 dll
# x64 -> linux x64

TARGET_ARCH = x64
CROSS_COMPILE = #arm-
DEBUG = -g #-O2

TOP_DIR := $(shell pwd)
OBJ_DIR := $(TOP_DIR)/obj
outdir := $(TOP_DIR)/bin

AS = $(CROSS_COMPILE)as
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)ld
AR = $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy
QMAKE= qmake
CP = cp

CONFIG_COMPILER = gnu

outdir = ./bin

exeobj = bt_server

dllobj =

libobj = 

mainobj = main.o log.o tools.o socket.o cJSON.o opentracker.o trackerlogic.o scan_urlencoded_query.o ot_mutex.o ot_stats.o ot_vector.o ot_clean.o ot_udp.o ot_iovec.o ot_fullscrape.o ot_accesslist.o ot_http.o ot_livesync.o ot_rijndael.o

cppobj = torrent.o # torrent.o#client_test.o session_view.o torrent_view.o print.o #torrent.o

CFLAGS := -I. -I./include -I./libowfat -L./libowfat -L./lib

ifeq ($(TARGET_ARCH), arm)
CFLAGS += 
else ifeq ($(TARGET_ARCH), x86)
CFLAGS += -lmingw32 -lm -lws2_32 -lpthreadGC2 -lgdi32 
else ifeq ($(TARGET_ARCH), x64)
CFLAGS += -lpthread -lowfat  -lz
endif

export CC CXX TOP_DIR OBJ_DIR DEBUG P2V_DIR BT_DIR

all: $(exeobj)

CXXFLAGS = $(CFLAGS) -I ./libtorrent/include -L ./libtorrent/lib -ltorrent-rasterbar -lboost_system  -std=c++14

VPATH = .:./include:./lib:./src:

$(mainobj):%.o:%.c 
	$(CC) -Wall $(DEBUG) $(CFLAGS) $(DEFINES) -c $< -o $@ 
    
$(cppobj):%.o:%.cpp
	$(CXX) -Wall $(DEBUG) $(CXXFLAGS) $(DEFINES) -c $< -o $@

$(exeobj):$(mainobj) $(cppobj)
	$(CXX) $(DEBUG)  -o $(outdir)/$(exeobj) $(mainobj) $(cppobj) $(wildcard $(OBJ_DIR)/*.o)  $(CXXFLAGS) 
	rm -f *.o
	@echo "Version $(VERSION)"
	@echo "Build  $(TARGET_ARCH) program $(exeobj) OK"

test:
	$(CC) -Wall src/test.c src/log.c src/tools.c src/socket.c src/cJSON.c $(DEBUG) -I./include  -o ./bin/test  -lm
	@echo "Build  Test OK !!"

clean:
	rm -f *.o $(outdir)/$(exeobj) $(outdir)/*.dll $(outdir)/*.a $(OBJ_DIR)/*.o

