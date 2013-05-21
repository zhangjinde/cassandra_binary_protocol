OSTYPE = $(shell uname -s)
DEBUG = 0

# $(info OSTYPE = $(OSTYPE))

OBJDIR =          obj
BINDIR =          bin

CLIENT_OBJS =       $(OBJDIR)/client.o $(OBJDIR)/hexdump.o $(OBJDIR)/cql.o $(OBJDIR)/cql_parser.o
TEST_PARSER_OBJS =  $(OBJDIR)/test_parser.o $(OBJDIR)/cql_parser.o

PROGS =             $(BINDIR)/client $(BINDIR)/test_parser

all: $(OBJDIR) $(BINDIR) $(PROGS)

clean:
	-rm $(OBJDIR)/*.o $(PROGS)
	-rmdir $(OBJDIR)
	-rmdir $(BINDIR)

INCLUDES=.
INCLUDEFLAGS=$(foreach dir,$(INCLUDES),-I $(dir))

ifeq ($(DEBUG),1)
CFLAGS = -DDEBUG=1 -Wall -g
LINKFLAGS = -g
else
CFLAGS = -DDEBUG=0 -Wall -O2
LINKFLAGS = -O2
endif

ifeq ($(OSTYPE),Darwin)
  CC = clang
  CFLAGS += -DDARWIN
endif

ifeq ($(OSTYPE),Linux)
  CC = gcc
  CFLAGS += -DLINUX -Wno-multichar
endif

LIBFLAGS =

$(BINDIR)/client: $(CLIENT_OBJS)
	$(CC) $(LINKFLAGS) -o $@ $(CLIENT_OBJS)

$(BINDIR)/test_parser: $(TEST_PARSER_OBJS)
	$(CC) $(LINKFLAGS) -o $@ $(TEST_PARSER_OBJS)

$(OBJDIR):
	mkdir $(OBJDIR)

$(BINDIR):
	mkdir $(BINDIR)

$(OBJDIR)/%.o: %.c
	$(CC) -c $(INCLUDEFLAGS) $(CFLAGS) -o $@ $<

$(OBJDIR)/%.o: test/%.c
	$(CC) -c $(INCLUDEFLAGS) $(CFLAGS) -o $@ $<

$(OBJDIR)/cql_parser.o: cql.h cql_error.h cql_header.h cql_parser.h cql_client.h
$(OBJDIR)/test_parser.o: cql.h cql_error.h cql_header.h cql_parser.h cql_client.h
$(OBJDIR)/cql.o: cql.h cql_error.h cql_header.h cql_parser.h cql_client.h
$(OBJDIR)/hexdump.o: hexdump.h
$(OBJDIR)/client.o: cql.h cql_error.h cql_header.h cql_parser.h cql_client.h
