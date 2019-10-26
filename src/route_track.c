#include "route.h"
#include "network.h"
#include "system.h"
#include "database.h"
#include "config.h"

void route_track(struct route_parameters* parameters) {
	if (!parameters->session) {
		print_error("Session is needed to stream track.");
		return;
	}
	print_info_f("Range: " A_CYAN "%s", parameters->result->client->headers.range);
	int album_release_id = 0;
	int disc_num = 0;
	int track_num = 0;
	int num = sscanf(parameters->resource, "%i/%i/%i", &album_release_id, &disc_num, &track_num);
	if (num != 3) {
		print_error_f("Invalid request: %s", parameters->resource);
		return;
	}
	char format[16];
	find_best_audio_format(format, album_release_id, false);
	route_file(parameters->result, server_track_path(format, album_release_id, disc_num, track_num));
}
