############### configuration #################
PROJ_NAME = libgwebqq
TARGETS = libgwebqq test
CFLAGS += -Wall -Wno-unused-label -Wno-unused-function
LDFLAGS += 
#VER =
#RELEASE_PATH = 
export CFLAGS LDFLAGS
###############################################

VER ?= $(shell cat VERSION)
RELEASE_PATH ?= ..
all install clean:
	@for i in `echo $(TARGETS)`; do \
		$(MAKE) -f $$i.makefile $@ || exit 1; \
	done

release:clean
	rm -f $(RELEASE_PATH)/$(PROJ_NAME)-$(VER).tar.gz
	tar -zcf  $(RELEASE_PATH)/$(PROJ_NAME)-$(VER).tar.gz * --exclude=.svn
distclean:clean
	echo ""> config.mk

.PHONY:all install clean release distclean
