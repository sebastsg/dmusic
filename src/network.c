#include "network.h"
#include "cache.h"
#include "config.h"
#include "files.h"
#include "format.h"
#include "route.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <unistd.h>

#define DMUSIC_MAX_CLIENTS 1024

struct network_state {
	struct sockaddr_in address;
	int listener;
	struct client_state clients[DMUSIC_MAX_CLIENTS];
};

struct network_state network;

static int next_free_client_index() {
	for (int i = 0; i < DMUSIC_MAX_CLIENTS; i++) {
		if (network.clients[i].socket < 0) {
			return i;
		}
	}
	return -1;
}

static bool init_socket(int socket) {
	int result = fcntl(socket, F_SETFL, O_NONBLOCK);
	if (result == -1) {
		fprintf(stderr, "Failed to set socket to non-blocking. Error: %s\n", strerror(errno));
		return false;
	}
	int option = 1;
	result = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(int));
	if (result == -1) {
		fprintf(stderr, "Failed to enable no-delay for socket. Error: %s\n", strerror(errno));
		return false;
	}
	return true;
}

static void free_socket(int socket, bool call_shutdown) {
	if (socket < 0) {
		return;
	}
	printf("Closing connection to socket %i\n", socket);
	if (call_shutdown && shutdown(socket, SHUT_RDWR) == -1) {
		fprintf(stderr, "Failed to shutdown socket. Error: %s\n", strerror(errno));
	}
	if (close(socket) == -1) {
		fprintf(stderr, "Failed to close socket. Error: %s\n", strerror(errno));
	}
}

void initialize_network() {
	int port = atoi(get_property("server.port"));
	for (int i = 0; i < DMUSIC_MAX_CLIENTS; i++) {
		network.clients[i].socket = -1;
		network.clients[i].buffer = NULL;
		network.clients[i].allocated = 0;
		network.clients[i].size = 0;
	}
	network.listener = socket(AF_INET, SOCK_STREAM, 0);
	if (network.listener == -1) {
		fprintf(stderr, "Failed to create listener socket. Error: %s\n", strerror(errno));
		return;
	}
	if (!init_socket(network.listener)) {
		return;
	}
	network.address.sin_family = AF_INET;
	network.address.sin_addr.s_addr = INADDR_ANY;
	network.address.sin_port = htons(port);
	memset(network.address.sin_zero, 0, sizeof(network.address.sin_zero));
	int result = bind(network.listener, (struct sockaddr*)&network.address, sizeof(network.address));
	if (result == -1) {
		fprintf(stderr, "Failed to bind listener socket to port %i. Error: %s\n", port, strerror(errno));
		return;
	}
	result = listen(network.listener, 16);
	if (result == -1) {
		fprintf(stderr, "Failed to listen to port %i. Error: %s\n", port, strerror(errno));
	}
}

void accept_client() {
	int client = next_free_client_index();
	if (client < 0) {
		return;
	}
	socklen_t addrsize = sizeof(network.address);
	network.clients[client].socket = accept(network.listener, (struct sockaddr*)&network.address, (socklen_t*)&addrsize);
	if (network.clients[client].socket < 0) {
		return;
	}
	init_socket(network.clients[client].socket);
	time(&network.clients[client].last_request);
}

void read_from_client(int socket, char** buffer, size_t* size, size_t* allocated, size_t max_size) {
	if (!*buffer) {
		*buffer = (char*)malloc(2048);
		(*buffer)[0] = '\0';
		*size = 0;
		*allocated = 2048;
	}
	size_t allocated_left = *allocated - *size - 1;
	size_t wanted_size = max_size > *size ? max_size - *size : allocated_left;
	ssize_t count = read(socket, *buffer + *size, wanted_size > allocated_left ? allocated_left : wanted_size);
	if (count < 0) {
		if (errno != EWOULDBLOCK && errno != EAGAIN) {
			fprintf(stderr, "Failed to read from client. Error: %s\n", strerror(errno));
		}
		return;
	}
	*size += count;
	(*buffer)[*size] = '\0';
	if (*size == *allocated - 1) {
		char* new_buffer = (char*)realloc(*buffer, (*allocated) * 4);
		if (new_buffer) {
			*buffer = new_buffer;
			*allocated *= 4;
		}
	}
}

void socket_write_all(struct client_state* client, const char* buffer, size_t size) {
	if (client->socket < 0 || !buffer) {
		return;
	}
	size_t written = 0;
	size = size > 0 ? size : strlen(buffer);
	while (size > written) {
		ssize_t count = send(client->socket, buffer + written, size - written, MSG_NOSIGNAL);
		if (count < 0) {
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				continue;
			}
			fprintf(stderr, "Failed to write to socket %i. Error: %s\n", client->socket, strerror(errno));
			free_socket(client->socket, false);
			client->socket = -1;
			return;
		}
		written += count;
	}
}

void poll_client(struct client_state* client) {
	if (client->has_headers) {
		if (!http_read_body(client)) {
			return;
		}
	} else {
		if (!http_read_headers(client)) {
			return;
		}
		printf("Socket %i: \"%s\"\n", client->socket, client->headers.resource);
		if (client->headers.content_length > 0) {
			return;
		}
	}
	struct http_headers response_headers;
	strcpy(response_headers.connection, client->headers.connection);

	struct route_result route;
	memset(&route, 0, sizeof(struct route_result));
	route.client = client;
	process_route(&route, client->headers.resource, client->buffer, client->size);
	response_headers.content_length = route.size;
	strcpy(response_headers.content_type, route.type);
	// todo: 1 write() call
	http_write_headers(client, &response_headers);
	socket_write_all(client, route.body, response_headers.content_length);
	free_route(&route);

	client->buffer[0] = '\0';
	client->size = 0;
	client->has_headers = false;
	time(&client->last_request);
	if (strcmp(client->headers.connection, "keep-alive")) {
		free_socket(client->socket, true);
		client->socket = -1;
	}
}

void poll_network() {
	accept_client();
	const time_t now = time(NULL);
	for (int i = 0; i < DMUSIC_MAX_CLIENTS; i++) {
		if (network.clients[i].socket >= 0) {
			poll_client(&network.clients[i]);
			if (now - network.clients[i].last_request > 30) {
				free_socket(network.clients[i].socket, true);
				network.clients[i].socket = -1;
				network.clients[i].buffer[0] = '\0';
				network.clients[i].size = 0;
				network.clients[i].has_headers = false;
			}
		}
	}
}

void free_network() {
	for (int i = 0; i < DMUSIC_MAX_CLIENTS; i++) {
		free_socket(network.clients[i].socket, true);
		free(network.clients[i].buffer);
		memset(&network.clients[i], 0, sizeof(struct client_state));
		network.clients[i].socket = -1;
	}
	free_socket(network.listener, false);
	network.listener = -1;
}
