#include "route.h"
#include "render.h"

#include <string.h>

void route_render(struct route_parameters* parameters) {
	set_route_result_html(parameters->result, render_resource(parameters->resource, parameters->session));
}
