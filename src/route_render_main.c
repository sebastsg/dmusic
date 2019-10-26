#include "route.h"
#include "render.h"
#include "cache.h"

#include <string.h>

void route_render_main(struct route_parameters* parameters) {
	if (!parameters->session) {
		set_route_result_html(parameters->result, render_resource(parameters->resource, NULL));
		return;
	}
	if (strlen(parameters->resource) == 0) {
		parameters->resource = "playlists";
	}
	struct render_buffer buffer;
	init_render_buffer(&buffer, 4096);
	render_main(&buffer, parameters->resource, parameters->session);
	parameters->result->body = buffer.data;
	if (parameters->result->body) {
		strcpy(parameters->result->type, "text/html");
		parameters->result->size = strlen(parameters->result->body);
		parameters->result->freeable = true;
	}
}
