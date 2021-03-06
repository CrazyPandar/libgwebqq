# COPYRIGHT_CHUNFENG
include config.mk
OS ?= LINUX32
VER = $(shell cat VERSION)
CC ?= gcc
CFLAGS += -Wall
LDFLAGS += 
ifeq ($(DEBUG),y)
CFLAGS += -g
LDFLAGS += -g
else
CFLAGS += -s
LDFLAGS += -s -O2
endif

#####################################################
GLIB_CFLAGS ?= $(shell pkg-config --cflags glib-2.0)
GLIB_LIBS ?= $(shell pkg-config --libs glib-2.0)

#GTK_CFLAGS ?= $(shell pkg-config --cflags gtk+-3.0)
#GTK_LIBS ?= $(shell pkg-config --libs gtk+-3.0)

LIBSOUP_CFLAGS ?= $(shell pkg-config --cflags libsoup-2.4)
LIBSOUP_LIBS ?= $(shell pkg-config --libs libsoup-2.4)

JSON_GLIB_CFLAGS ?= $(shell pkg-config --cflags json-glib-1.0)
JSON_GLIB_LIBS ?= $(shell pkg-config --libs json-glib-1.0)
###########################################################
TARGET = test
OBJS = test.o

CFLAGS += -I./include $(GLIB_CFLAGS) $(LIBSOUP_CFLAGS) $(JSON_GLIB_CFLAGS)
LDFLAGS += -lm -lcrypt $(GLIB_LIBS) $(LIBSOUP_LIBS) $(JSON_GLIB_LIBS) -L. -lgwebqq
###########################################################

all:$(TARGET)

$(TARGET):$(TARGET).$(VER)
ifeq ($(OS),WIN32)
	mv $^ $@
else
	ln -sf $^ $@
endif


$(TARGET).$(VER):$(OBJS)
	$(CC) $^ $(LDFLAGS) -o$@ 

	
install:

uninstall:
	
clean:
	rm -rf $(TARGET)  $(TARGET).${VER} *.o

	
.PHONY : all clean install uninstall

