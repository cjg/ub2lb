SOURCES=parthenope-install.c
CFLAGS+=-Wall -W -Werror

OBJECTS := $(patsubst %.c,%.o,$(filter %.c, $(SOURCES)))

all: parthenope-install

parthenope-install: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o parthenope-install
	
%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $<
	
clean:
	-rm -rf parthenope-install $(OBJECTS)
