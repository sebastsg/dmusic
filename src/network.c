#include "network.h"

#include <fcntl.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cache.h"
#include "config.h"
#include "files.h"
#include "format.h"
#include "route.h"
#include "system.h"

#define DMUSIC_MAX_CLIENTS 1024
#define DMUSIC_MAX_BUFFER_SIZE 32768

struct network_state {
	struct sockaddr_in address;
	int listener;
	struct client_state clients[DMUSIC_MAX_CLIENTS];
};

struct network_state network;

static void clear_client_for_next_request(struct client_state* client) {
	free_route(&client->route);
	if (client->buffer) {
		client->buffer[0] = '\0';
	}
	client->size = 0;
	memset(&client->headers, 0, sizeof(struct http_headers));
	client->has_headers = false;
	time(&client->last_request);
	client->is_processing = false;
	client->write_begin = 0;
	client->write_end = 0;
	client->write_index = 0;
}

static int next_free_client_index() {
	for (int i = 0; i < DMUSIC_MAX_CLIENTS; i++) {
		if (network.clients[i].socket < 0) {
			return i;
		}
	}
	return -1;
}

static bool init_socket(int socket) {
	if (fcntl(socket, F_SETFL, O_NONBLOCK) == -1) {
		print_errno("Failed to set socket to non-blocking.");
		return false;
	}
	if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &(int){ 1 }, sizeof(int)) == -1) {
		print_errno("Failed to enable no-delay for socket.");
		return false;
	}
	return true;
}

static void free_socket(int socket, bool call_shutdown) {
	if (socket < 0) {
		return;
	}
	print_info_f("Closing connection to socket %i", socket);
	if (call_shutdown && shutdown(socket, SHUT_RDWR) == -1) {
		print_errno("Failed to shutdown socket.");
	}
	if (close(socket) == -1) {
		print_errno("Failed to close socket.");
	}
}

static void allow_listener_socket_to_reuse_address() {
	print_info("Allowing reuse of address and port.");
	if (setsockopt(network.listener, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) == -1) {
		print_errno("Failed to allow reuse of address.");
	}
	if (setsockopt(network.listener, SOL_SOCKET, SO_REUSEPORT, &(int){ 1 }, sizeof(int)) == -1) {
		print_errno("Failed to allow reuse of port.");
	}
}

static bool bind_listener_socket(bool allow_reuse) {
	if (allow_reuse) {
		allow_listener_socket_to_reuse_address();
	}
	if (bind(network.listener, (struct sockaddr*)&network.address, sizeof(network.address)) == -1) {
		print_errno_f("Failed to bind to port " A_CYAN "%i" A_RED ".", ntohs(network.address.sin_port));
		return false;
	}
	return true;
}

void initialize_network() {
	memset(network.clients, 0, sizeof(struct client_state) * DMUSIC_MAX_CLIENTS);
	for (int i = 0; i < DMUSIC_MAX_CLIENTS; i++) {
		network.clients[i].socket = -1;
		network.clients[i].route.client = &network.clients[i];
	}
	network.listener = socket(AF_INET, SOCK_STREAM, 0);
	if (network.listener == -1) {
		print_errno("Failed to create listener socket.");
		return;
	}
	if (!init_socket(network.listener)) {
		return;
	}
	network.address.sin_family = AF_INET;
	network.address.sin_addr.s_addr = INADDR_ANY;
	int port = atoi(get_property("server.port"));
	network.address.sin_port = htons(port);
	memset(network.address.sin_zero, 0, sizeof(network.address.sin_zero));
	if (bind_listener_socket(true)) {
		if (listen(network.listener, 16) == -1) {
			print_errno_f("Failed to listen to port %i.", port);
		}
	}
}

void accept_client() {
	int client = next_free_client_index();
	if (client < 0) {
		return;
	}
	socklen_t addrsize = sizeof(network.address);
	network.clients[client].socket = accept(network.listener, (struct sockaddr*)&network.address, (socklen_t*)&addrsize);
	if (network.clients[client].socket == -1) {
		if (errno != EWOULDBLOCK && errno != EAGAIN) {
			print_errno("Failed to accept client.");
			if (errno == EINVAL) {
				print_error("The listener socket is probably invalid. Restart the program in a minute.");
				raise(SIGINT);
				exit(0);
			}
		}
		return;
	}
	init_socket(network.clients[client].socket);
	clear_client_for_next_request(&network.clients[client]);
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
			print_errno("Failed to read from client.");
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

size_t socket_write_all(struct client_state* client, const char* buffer, size_t size) {
	if (client->socket < 0 || !buffer) {
		return 0;
	}
	size_t written = 0;
	size = size > 0 ? size : strlen(buffer);
	while (size > written) {
		ssize_t count = send(client->socket, buffer + written, size - written, MSG_NOSIGNAL);
		if (count < 0) {
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				return written;
			}
			print_errno_f("Failed to write to socket %i.", client->socket);
			free_socket(client->socket, false);
			client->socket = -1;
			return written;
		}
		written += count;
	}
	return written;
}

void finish_client_request(struct client_state* client) {
	clear_client_for_next_request(client);
	if (strcmp(client->headers.connection, "keep-alive")) {
		free_socket(client->socket, true);
		client->socket = -1;
	}
}

void init_client_response(struct client_state* client) {
	struct http_headers response_headers;
	memset(&response_headers, 0, sizeof(struct http_headers));
	strcpy(response_headers.connection, client->headers.connection);
	strcpy(client->headers.status, "200");
	process_route(&client->route, client->headers.resource, client->buffer, client->size);
	response_headers.content_length = client->route.size;
	strcpy(response_headers.content_type, client->route.type);
	strcpy(response_headers.session_cookie, client->headers.session_cookie);
	strcpy(response_headers.status, client->headers.status);
	strcpy(response_headers.content_range, client->headers.content_range);
	http_write_headers(client, &response_headers);
	client->write_index = 0;
	client->write_begin = 0;
	client->write_end = response_headers.content_length;
	if (client->write_end - client->write_begin == 0) {
		finish_client_request(client);
	}
}

void read_client_request(struct client_state* client) {
	if (client->is_processing) {
		return;
	}
	if (client->has_headers) {
		client->is_processing = http_read_body(client);
	} else if (http_read_headers(client)) {
		print_info_f("Socket %i: " A_CYAN "%s", client->socket, client->headers.resource);
		if (strcmp(client->headers.host, get_property("server.host")) != 0) {
			print_error_f("Ignoring request for host \"%s\"", client->headers.host);
			client->last_request = 0;
			client->is_processing = false;
		} else {
			client->is_processing = (client->headers.content_length == 0);
		}
	} else {
		client->is_processing = false;
	}
	if (client->is_processing) {
		init_client_response(client);
	}
}

void process_client_request(struct client_state* client) {
	size_t write_size = client->write_end - client->write_index;
	if (write_size > DMUSIC_MAX_BUFFER_SIZE) {
		write_size = DMUSIC_MAX_BUFFER_SIZE;
	}
	size_t written = socket_write_all(client, client->route.body + client->write_index, write_size);
	if (written > 0) {
		client->write_index += written;
		if (client->write_index >= client->write_end) {
			finish_client_request(client);
		}
	}
}

void poll_client(struct client_state* client) {
	read_client_request(client);
	if (client->is_processing) {
		process_client_request(client);
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
			}
		}
	}
}

void free_network() {
	for (int i = 0; i < DMUSIC_MAX_CLIENTS; i++) {
		clear_client_for_next_request(&network.clients[i]);
		free_socket(network.clients[i].socket, true);
		free(network.clients[i].buffer);
		memset(&network.clients[i], 0, sizeof(struct client_state));
		network.clients[i].socket = -1;
	}
	free_socket(network.listener, false);
	network.listener = -1;
}
