#include "render.h"
#include "cache.h"
#include "database.h"
#include "session.h"
#include "session_track.h"

#include <stdlib.h>

void render_main(struct render_buffer* buffer, const char* resource) {
	struct session_track_data* tracks = NULL;
	int num_tracks = 0;
	load_session_tracks(&tracks, &num_tracks, get_session_username());
	char* content = render_resource(resource);
	assign_buffer(buffer, get_cached_file("html/main.html", NULL));
	set_parameter(buffer, "user", get_session_username());
	render_session_tracks_array(buffer, "tracks", tracks, num_tracks);
	set_parameter(buffer, "content", content);
	free(content);
	free(tracks);
}
