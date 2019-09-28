#include "render.h"
#include "cache.h"
#include "group.h"

#include <stdlib.h>

void render_tags(struct render_buffer* buffer, const char* key, struct tag_data* tags, int num_tags) {
	struct render_buffer item_buffer;
	init_render_buffer(&item_buffer, 512);
	for (int i = 0; i < num_tags; i++) {
		append_buffer(&item_buffer, get_cached_file("html/tag_link.html", NULL));
		set_parameter(&item_buffer, "tag_link", tags[i].name);
		set_parameter(&item_buffer, "tag_name", tags[i].name);
	}
	set_parameter(buffer, key, item_buffer.data);
	free(item_buffer.data);
}
