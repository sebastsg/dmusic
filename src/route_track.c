#include "route.h"
#include "network.h"
#include "system.h"
#include "database.h"
#include "config.h"

void route_track(struct route_result* result, const char* resource) {
	print_info_f("Range: " A_CYAN "%s", result->client->headers.range);
	int album_release_id = 0;
	int disc_num = 0;
	int track_num = 0;
	int num = sscanf(resource, "%i/%i/%i", &album_release_id, &disc_num, &track_num);
	if (num != 3) {
		print_error_f("Invalid request: %s", resource);
		return;
	}
	char format[16];
	find_best_audio_format(format, album_release_id, false);
	route_file(result, server_track_path(format, album_release_id, disc_num, track_num));
}
