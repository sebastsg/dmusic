#include "route.h"
#include "render.h"
#include "group.h"
#include "cache.h"

#include <string.h>
#include <stdlib.h>

void route_form_add_group_tag(struct route_result* result, struct http_data* data) {
	int group_id = atoi(http_data_string(data, "group"));
	const char* tag = http_data_string(data, "tag");
	if (group_id > 0 && strlen(tag) > 0) {
		create_group_tags(group_id, tag);
		struct render_buffer buffer;
		init_render_buffer(&buffer, 128);
		assign_buffer(&buffer, get_cached_file("html/tag_edit_row.html", NULL));
		set_parameter_int(&buffer, "group", group_id);
		set_parameter(&buffer, "tag", tag);
		set_parameter(&buffer, "tag", tag);
		set_route_result_html(result, buffer.data);
	}
}

void route_form_delete_group_tag(struct route_result* result, struct http_data* data) {
	int group_id = atoi(http_data_string(data, "group"));
	const char* tag = http_data_string(data, "tag");
	if (group_id > 0 && strlen(tag) > 0) {
		delete_group_tag(group_id, tag);
	}
}
