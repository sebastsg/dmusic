CC = gcc-8
CFLAGS = -fsanitize=address -fsanitize=undefined -g -O0 -std=c18 -D_DEFAULT_SOURCE -Wall -Wextra -Wno-parentheses -Wno-unused-parameter
LDFLAGS = -lasan -lubsan -lpq -lssl -lcrypto -lpthread -lpcre2-8
SRCDIR = src
OBJDIR = obj
OBJECTS = $(addprefix $(OBJDIR)/, cache.o config.o data.o database.o files.o format.o install.o main.o prepare.o render.o)

all: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o dmusic

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJECTS): | $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

clean:
	rm dmusic $(OBJECTS)
