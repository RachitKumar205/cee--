CC      = gcc
CFLAGS  = -Wall -I. $(shell pg_config --includedir | sed 's/^/-I/')
LDFLAGS = -L$(shell pg_config --libdir) -lpq

EXAMPLE_SRC = examples/hello.c
TRANSPILED  = examples/hello.transpiled.c
BINARY      = examples/hello.out

.PHONY: all transpile compile clean

all: transpile compile

transpile: $(EXAMPLE_SRC) transpile.py
	python transpile.py $(EXAMPLE_SRC) $(TRANSPILED)

compile: $(TRANSPILED) runtime/db_runtime.c
	$(CC) $(CFLAGS) -o $(BINARY) $(TRANSPILED) runtime/db_runtime.c $(LDFLAGS)

clean:
	rm -f $(TRANSPILED) examples/hello.transpiled.e.c $(BINARY)
