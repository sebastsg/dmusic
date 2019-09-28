#include "route.h"
#include "render.h"

#include <string.h>

void route_render(struct route_result* result, const char* resource) {
	result->body = render_resource(resource);
	if (result->body) {
		strcpy(result->type, "text/html");
		result->size = strlen(result->body);
		result->freeable = true;
	}
}
