all:

# The core files
objs-y    := src/httpd_main.c src/httpd_parse.c src/httpd_sess.c src/httpd_txrx.c src/httpd_uri.c util/src/ctrl_sock.c
cflags-y  := -Iinclude -Iutil/include
-include $(objs-y:.c=.d)

# Files from an example
ifneq ($(EXAMPLE),)
  include $(EXAMPLE)/recipe.mk
  targets-y := $(exec-y)
else
  targets-y := libflick.a
endif

# Manage ports
PORT ?= unix
cflags-y += -Iport/$(PORT)

# The rules
all: $(targets-y)

$(exec-y): $(objs-y:.c=.o)
	$(CC) -g -o $@ $^

libflick.a: $(objs-y:.c=.o)
	rm -f libflick.a
	$(AR) cru $@ $^

%.o: %.c
	$(CC) $(cflags-y) -Wall -MMD -g -c $< -o $@

clean:
	rm -f $(objs-y:.c=.o) $(objs-y:.c=.d) $(targets-y)
