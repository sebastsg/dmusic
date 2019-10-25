#include "route.h"
#include "user.h"
#include "session.h"
#include "network.h"

#include <string.h>

void route_form_login(struct route_result* result, struct http_data* data) {
	const char* name = http_data_string(data, "name");
	const char* password = http_data_string(data, "password");
	bool success = login_user(name, password);
	if (success) {
		const struct cached_session* session = create_session(name);
		strcpy(result->client->headers.session_cookie, session->id);
	}
}
