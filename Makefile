######## Settings for D3 ###########
D3_release D3_debug: TARGET_ARCH := D3
D3_release D3_debug: VRMSDK_INSTALL_DIR = $(DIR)/D3/
D3_release D3_debug:    CROSS_COMPILE_PREFIX = arm-linux-gnueabihf-
D3_release D3_debug:    CROSS_BUILD_PATH=/usr/arm-linux-gnueabihf/vrmagic
D3_release D3_debug:    GST_INSTALL_PATH=$(CROSS_BUILD_PATH)
D3_release D3_debug:    CPP_WRAPPER_PATH = ../../../../wrappers/c++

D3_release D3_debug:     SDK_VERSION = 4.5.0
D3_release D3_debug:     LIBVRMUSBCAM2_VERSION = 3.5.0.0
D3_release D3_debug:     VM_LIB_VERSION = 1.0.1.0
D3_release D3_debug:     VRM_DIR = /opt/vrmagic/sdk-$(SDK_VERSION)

D3_release D3_debug:    INCPATH = -I/usr/arm-linux-gnueabihf/include \
	-I$(CROSS_BUILD_PATH)/usr/include \
	-I$(CROSS_BUILD_PATH)/usr/include/libvrmusbcam2-$(LIBVRMUSBCAM2_VERSION) \
	-I$(CROSS_BUILD_PATH)/usr/vm_lib-$(VM_LIB_VERSION)/include

D3_release D3_debug:    LIBPATH = -L/usr/arm-linux-gnueabihf/lib \
	-L$(CROSS_BUILD_PATH)/lib \
	-L$(CROSS_BUILD_PATH)/lib/arm-linux-gnueabihf \
	-L$(CROSS_BUILD_PATH)/usr/lib \
	-L$(CROSS_BUILD_PATH)/usr/lib/arm-linux-gnueabihf \
	-L$(CROSS_BUILD_PATH)/usr/lib/libvrmusbcam2-$(LIBVRMUSBCAM2_VERSION) \
	-L$(CROSS_BUILD_PATH)/usr/vm_lib-$(VM_LIB_VERSION)/lib

D3_release D3_debug:    LFLAGS = -Wl,--rpath-link=/usr/arm-linux-gnueabihf/lib:/usr/arm-linux-gnueabihf/vrmagic/lib/:/usr/arm-linux-gnueabihf/vrmagic/lib/arm-linux-gnueabihf:/usr/arm-linux-gnueabihf/vrmagic/usr/lib:/usr/arm-linux-gnueabihf/vrmagic/usr/lib/arm-linux-gnueabihf

D3_release D3_debug:    CFLAGS = -DD3_PLATFORM
D3_release D3_debug:    LIBS     = -lpthread -ludt -lzmq -lboost_system -lnanomsg
D3_release:             CXXFLAGS = -pipe --std=c++0x -O2 -march=armv7-a -mtune=cortex-a8 -mfpu=neon -ftree-vectorize -ffast-math -funsafe-math-optimizations -funroll-loops -fmessage-length=0 -I ../include  -I ../extern/libtiff/include -I ../extern/pugixml -I ../extern/catch -I ../extern/flatbuffers/include -fPIC -Wno-deprecated -Wall -W -Wno-unused $(CFLAGS) -DWITH_PURE_VIRTUAL
D3_debug:       CXXFLAGS = -pipe -g --std=c++0x -O0 -fmessage-length=0   -fPIC -Wno-deprecated -Wall -W -Wno-unused $(CFLAGS) -DWITH_PURE_VIRTUAL -I ../include  -I ../extern/libtiff/include -I ../extern/pugixml -I ../extern/catch -I ../extern/flatbuffers/include -DDEBUG



######## Settings for linux ###########

server client: INCPATH = -I /usr/include -I ../include
server client: LIBPATH = -L$/usr/lib/x86_64-linux-gnu/
server client: CXXFLAGS = -pipe -g --std=c++11 -O0 -fmessage-length=0   -fPIC -Wno-deprecated -w -Wno-unused $(CFLAGS) -DWITH_PURE_VIRTUAL -DNOD3
server client: LIBS     = -lpthread -ludt -lzmq -lboost_system -lnanomsg



######## General Settings ###########


LIBS     = -lpthread -ludt -lzmq -lboost_system -lnanomsg
CXX      = $(CROSS_COMPILE_PREFIX)g++
DEL_FILE = rm -f

######## Files to build ###########

OBJECTS_SERVER = server.o
OBJECTS_CLIENT = client.o

TARGET_SERVER = server
TARGET_CLIENT = client

######## Implicit rules ###########

.SUFFIXES: .o .cpp

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<



######## Build rules ###########

first: all

all: server client

server: Makefile $(TARGET_SERVER)
D3_release: Makefile $(TARGET_SERVER)
client: Makefile $(TARGET_CLIENT)

$(TARGET_SERVER):  $(OBJECTS_SERVER)
	$(CXX) $(LFLAGS) -o $(TARGET_SERVER) $(OBJECTS_SERVER) $(LIBPATH) $(LIBS)
$(TARGET_CLIENT):  $(OBJECTS_CLIENT)
	$(CXX) $(LFLAGS) -o $(TARGET_CLIENT) $(OBJECTS_CLIENT) $(LIBPATH) $(LIBS)

clean:
	-$(DEL_FILE) $(OBJECTS_SERVER)
	-$(DEL_FILE) $(OBJECTS_CLIENT)
	-$(DEL_FILE) *~ core *.core
	-$(DEL_FILE) $(TARGET_SERVER)
	-$(DEL_FILE) $(TARGET_CLIENT)
