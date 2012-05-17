# COPYRIGHT_CHUNFENG
include config.mk
OS ?= LINUX32
CC ?= gcc
ifeq ($(OS),WIN32)
CFLAGS += -D_WIN32_
else
CFLAGS += 
endif
ifeq ($(CONFIG_DEBUG),y)
CFLAGS += -g
LDFLAGS += -g
else
CFLAGS += -s
LDFLAGS += -s -O2
endif
VER = $(shell cat VERSION)

#####################################################
GLIB_CFLAGS ?= $(shell pkg-config --cflags glib-2.0)
GLIB_LIBS ?= $(shell pkg-config --libs glib-2.0)

LIBSOUP_CFLAGS ?= $(shell pkg-config --cflags libsoup-2.4)
LIBSOUP_LIBS ?= $(shell pkg-config --libs libsoup-2.4)
#####################################################
ifeq ($(OS), WIN32)
TARGET = libgwebqq.dll
else
TARGET = libgwebqq.so
endif

OBJS = g_webqq.o

CFLAGS +=  -I./include -fPIC $(GLIB_CFLAGS) $(LIBSOUP_CFLAGS)
LDFLAGS += -Wl,-soname -Wl,$(TARGET) -shared -lm -lcrypt $(GLIB_LIBS) $(LIBSOUP_LIBS)

#####################################################

all:$(TARGET)

$(TARGET):$(TARGET).$(VER)
ifeq ($(OS),WIN32)
	mv $^ $@
else
	ln -sf $^ $@
endif

$(TARGET).$(VER):$(OBJS)
	$(CC) $(LDFLAGS) -o$@ $^

install:
	mkdir -p $(prefix)/lib
	mkdir -p $(prefix)/include/libchunfeng
	cp -dR ${TARGET} ${TARGET}.${VER} $(prefix)/lib
	rm -rf $(prefix)/include/libchunfeng/*
	cp -dR include/* $(prefix)/include/libchunfeng
	echo -n $(DEP_LIBS_FLAGS) > $(prefix)/lib/$(TARGET).deplibs
     
uninstall:
	rm -rf $(prefix)/lib/${TARGET} $(prefix)/lib/${TARGET}.$(VER)

clean:
	rm -rf test $(TARGET)  $(TARGET).${VER} *.o

.PHONY : all clean install uninstall

