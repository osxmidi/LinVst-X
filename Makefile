#!/usr/bin/make -f
# Makefile for LinVst #

CXX     = g++
WINECXX = wineg++ -Wl,--subsystem,windows 

CXX_FLAGS =

PREFIX  = /usr

BIN_DIR    = $(DESTDIR)$(PREFIX)/bin
VST_DIR = ./vst

BUILD_FLAGS  = -fPIC -O2 -DLVRT -DVST6432 -DEMBED -DEMBEDDRAG -DWAVES -DVESTIGE -DNEWTIME -DINOUTMEM -DCHUNKBUF -DTRACKTIONWM -DEMBEDRESIZE -DPCACHE $(CXX_FLAGS)

BUILD_FLAGS_WIN = -m64 -fPIC -O2 -DVST6432 -DEMBED -DEMBEDDRAG -DWAVES -DVESTIGE -DNEWTIME -DINOUTMEM -DCHUNKBUF -DTRACKTIONWM -DEMBEDRESIZE -DPCACHE -DXOFFSET -I/opt/wine-staging/include/wine/windows -I/opt/wine-stable/include/wine/windows -I/opt/wine-devel/include/wine/windows -I/usr/include/wine-development/windows -I/usr/include/wine-development/wine/windows -I/usr/include/wine/wine/windows
BUILD_FLAGS_WIN32 = -m32 -fPIC -O2 -DVST6432 -DEMBED -DEMBEDDRAG -DWAVES -DVESTIGE -DNEWTIME -DINOUTMEM -DCHUNKBUF -DVST32SERVER -DTRACKTIONWM -DEMBEDRESIZE -DPCACHE -DXOFFSET -I/opt/wine-staging/include/wine/windows -I/opt/wine-stable/include/wine/windows -I/opt/wine-devel/include/wine/windows -I/usr/include/wine-development/windows -I/usr/include/wine-development/wine/windows -I/usr/include/wine/wine/windows

LINK_FLAGS   = $(LDFLAGS)

LINK_PLUGIN = -shared -lpthread -ldl -lX11 $(LINK_FLAGS)
LINK_WINE = -L/opt/wine-stable/lib64/wine -L/opt/wine-devel/lib64/wine -L/opt/wine-staging/lib64/wine -L/opt/wine-stable/lib64/wine/x86_64-unix -L/opt/wine-devel/lib64/wine/x86_64-unix -L/opt/wine-staging/lib64/wine/x86_64-unix -L/usr/lib/x86_64-linux-gnu/wine-development -lpthread -lX11 -lshell32  -lole32 $(LINK_FLAGS)
LINK_WINE32 = -m32 -L/opt/wine-stable/lib/wine -L/opt/wine-devel/lib/wine -L/opt/wine-staging/lib/wine -L/opt/wine-stable/lib/wine/i386-unix -L/opt/wine-devel/lib/wine/i386-unix -L/opt/wine-staging/lib/wine/i386-unix -L/usr/lib/i386-linux-gnu/wine-development -lpthread -lX11 -lshell32 -lole32 $(LINK_FLAGS)

PATH_TO_FILE = /usr/include/bits

TARGETS     = linvstx.so lin-vst-server-x.exe lin-vst-server-x32.exe

PATH := $(PATH):/opt/wine-stable/bin:/opt/wine-devel/bin:/opt/wine-staging/bin

# --------------------------------------------------------------

all: $(TARGETS)

linvstx.so: linvst.unix.o remotepluginclient.unix.o paths.unix.o
	$(CXX) $^ $(LINK_PLUGIN) -o $@
	
lin-vst-server-x.exe: lin-vst-server.wine.o remotepluginserver.wine.o paths.wine.o
	$(WINECXX) $^ $(LINK_WINE) -o $@

ifneq ("$(wildcard $(PATH_TO_FILE))","")
lin-vst-server-x32.exe: lin-vst-server.wine32.o remotepluginserver.wine32.o paths.wine32.o
	$(WINECXX) $^ $(LINK_WINE32) -o $@
else
lin-vst-server-x32.exe:
endif

# --------------------------------------------------------------

linvst.unix.o: linvst.cpp
	$(CXX) $(BUILD_FLAGS) -c $^ -o $@
		
remotepluginclient.unix.o: remotepluginclient.cpp
	$(CXX) $(BUILD_FLAGS) -c $^ -o $@

paths.unix.o: paths.cpp
	$(CXX) $(BUILD_FLAGS) -c $^ -o $@


# --------------------------------------------------------------

lin-vst-server.wine.o: lin-vst-server.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN) -c $^ -o $@

remotepluginserver.wine.o: remotepluginserver.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN) -c $^ -o $@

paths.wine.o: paths.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN) -c $^ -o $@

lin-vst-server.wine32.o: lin-vst-server.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN32) -c $^ -o $@

remotepluginserver.wine32.o: remotepluginserver.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN32) -c $^ -o $@

paths.wine32.o: paths.cpp
	$(WINECXX) $(BUILD_FLAGS_WIN32) -c $^ -o $@



clean:
	rm -fR *.o *.exe *.so vst $(TARGETS)

install:
	install -d $(BIN_DIR)
	install -d $(VST_DIR)
	install -m 755 linvstx.so $(VST_DIR)
	install -m 755 lin-vst-server-x.exe lin-vst-server-x.exe.so $(BIN_DIR)
        ifneq ("$(wildcard $(PATH_TO_FILE))","")
	install -m 755 lin-vst-server-x32.exe lin-vst-server-x32.exe.so $(BIN_DIR)
        endif
