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

JSON_GLIB_CFLAGS ?= $(shell pkg-config --cflags json-glib-1.0)
JSON_GLIB_LIBS ?= $(shell pkg-config --libs json-glib-1.0)

SQLITE3_CFLAGS ?= $(shell pkg-config --cflags sqlite3)
SQLITE3_LIBS ?= $(shell pkg-config --libs sqlite3)
#####################################################
ifeq ($(OS), WIN32)
TARGET = libgwebqq.dll
else
TARGET = libgwebqq.so
endif

OBJS = gwq_util.o gwq_session.o gwq_login.o gwq_user.o gwq_poll.o gwq_recvmsg.o gwq_sendmsg.o

CFLAGS +=  -I./include -fPIC $(GLIB_CFLAGS) $(LIBSOUP_CFLAGS) $(JSON_GLIB_CFLAGS) $(SQLITE3_CFLAGS)
LDFLAGS += -Wl,-soname -Wl,$(TARGET) -shared -lm -lcrypt $(GLIB_LIBS) $(LIBSOUP_LIBS) $(JSON_GLIB_LIBS) $(SQLITE3_LIBS)
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

_INST_INC_DIR = $(DESTDIR)/usr/include/libgwebqq
_INST_LIB_DIR = $(DESTDIR)/usr/lib
install:
	rm -rf $(_INST_INC_DIR)
	mkdir -p $(_INST_INC_DIR)
	cp -dR include/* $(_INST_INC_DIR)
	mkdir -p $(_INST_LIB_DIR)
	cp -dR ${TARGET} ${TARGET}.${VER} $(_INST_LIB_DIR)
	echo -n $(DEP_LIBS_FLAGS) > $(_INST_LIB_DIR)/$(TARGET).deplibs
     
uninstall:
	rm -rf $(_INST_INC_DIR) \
		$(_INST_LIB_DIR)/${TARGET} \
		$(_INST_LIB_DIR)/${TARGET}.$(VER) \
		$(_INST_LIB_DIR)/$(TARGET).deplibs

clean:
	rm -rf test $(TARGET)  $(TARGET).${VER} *.o

.PHONY : all clean install uninstall

