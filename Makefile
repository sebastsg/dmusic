CC = gcc-8
CFLAGS = -fsanitize=address -fsanitize=undefined -g -O0 -std=c18 -D_DEFAULT_SOURCE -Wall -Wextra -Wno-parentheses -Wno-unused-parameter
LDFLAGS = -lasan -lubsan -lpq -lssl -lcrypto -lpthread -lpcre2-8
SRCDIR = src
OBJDIR = obj
OBJECTS = $(addprefix $(OBJDIR)/, \
 album.o\
 analyze.o\
 cache.o\
 config.o\
 database.o\
 files.o\
 format.o\
 generic.o\
 group.o\
 http.o\
 import.o\
 install.o\
 main.o\
 network.o\
 playlist.o\
 regex.o\
 render.o\
 render_main.o\
 render_tags.o\
 route.o\
 route_file.o\
 route_form.o\
 route_form_add_group.o\
 route_form_add_session_track.o\
 route_form_attach.o\
 route_form_download_remote.o\
 route_form_edit_group_tags.o\
 route_form_login.o\
 route_form_prepare.o\
 route_form_register.o\
 route_form_upload.o\
 route_image.o\
 route_render.o\
 route_render_main.o\
 route_track.o\
 search.o\
 session.o\
 session_track.o\
 stack.o\
 system.o\
 track.o\
 transcode.o\
 type.o\
 upload.o\
 user.o\
)

all: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o dmusic

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJECTS): | $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)

clean:
	rm dmusic $(OBJECTS)
