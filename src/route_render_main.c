#include "route.h"
#include "render.h"

#include <string.h>

void route_render_main(struct route_result* result, const char* resource) {
	if (strlen(resource) == 0) {
		resource = "playlists";
	}
	struct render_buffer buffer;
	init_render_buffer(&buffer, 4096);
	render_main(&buffer, resource);
	result->body = buffer.data;
	if (result->body) {
		strcpy(result->type, "text/html");
		result->size = strlen(result->body);
		result->freeable = true;
	}
}
