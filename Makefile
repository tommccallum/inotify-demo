IDIR =include
CC=g++
CFLAGS=-I$(IDIR) -I/usr/include/openssl

ODIR=obj
LDIR=lib
SRC=src
LIBS=-lcrypto

_DEPS = Watcher.h WatchedEvent.h WatchableItem.h INotify.h File.h BoolOrError.h Base64.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = Watcher.o File.o WatchableItem.o Base64.o BoolOrError.o main.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: $(SRC)/%.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

watcher: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -rf $(ODIR) *~ $(INCDIR)/*~ watcher

all: watcher 