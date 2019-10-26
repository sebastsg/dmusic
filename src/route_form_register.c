#include "route.h"
#include "user.h"
#include "session.h"
#include "network.h"

#include <string.h>

void route_form_register(struct http_data* data) {
	const char* name = http_data_string(data, "name");
	const char* password = http_data_string(data, "password");
	register_user(name, password);
}
