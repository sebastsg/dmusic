#include "user.h"
#include "database.h"
#include "cache.h"
#include "render.h"
#include "system.h"

#include <openssl/sha.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DMUSIC_HASH_STRING_SIZE (SHA256_DIGEST_LENGTH * 2 + 1)
#define DMUSIC_SALT_SIZE        32
#define DMUSIC_SALT_STRING_SIZE (DMUSIC_SALT_SIZE * 2 + 1)

static void make_random_salt(char* salt) {
	*salt = '\0';
	for (int i = 0; i < DMUSIC_SALT_SIZE;) {
		unsigned char value = (unsigned char)(rand() % 256);
		i += sprintf(salt + i, "%02X", value);
	}
}

static void make_sha256_hash(const char* password, const char* salt, char* dest) {
	unsigned char binary_hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, password, strlen(password));
	if (salt) {
		SHA256_Update(&sha256, salt, strlen(salt));
	}
	SHA256_Final(binary_hash, &sha256);
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		dest += sprintf(dest, "%02X", binary_hash[i]);
	}
}

bool register_user(const char* name, const char* password) {
	if (strlen(name) < 1 || strlen(password) < 8) {
		return false;
	}
	const char* params[] = { name };
	PGresult* result = execute_sql("select \"name\" from \"user\" where \"name\" = $1", params, 1);
	bool exists = PQntuples(result) > 0;
	PQclear(result);
	if (exists) {
		print_info_f("User " A_CYAN "%s" A_YELLOW " is already registered.", name);
		return false;
	}
	char hash[DMUSIC_HASH_STRING_SIZE];
	char salt[DMUSIC_SALT_STRING_SIZE];
	make_random_salt(salt);
	make_sha256_hash(password, salt, hash);
	const char* args[] = { name, hash, salt };
	insert_row("user", false, 3, args);
	print_info_f("Registered new user " A_CYAN "%s" A_YELLOW ".", name);
	return true;
}

bool login_user(const char* name, const char* password) {
	if (strlen(name) < 1 || strlen(password) < 8) {
		return false;
	}
	const char* params[] = { name, "" };
	PGresult* result = execute_sql("select \"salt\" from \"user\" where \"name\" = $1", params, 1);
	char salt[DMUSIC_SALT_STRING_SIZE];
	if (PQntuples(result) == 0) {
		PQclear(result);
		print_info_f("User " A_CYAN "%s" A_YELLOW " does not exist.", name);
		return false;
	}
	strcpy(salt, PQgetvalue(result, 0, 0));
	PQclear(result);
	char hash[DMUSIC_HASH_STRING_SIZE];
	make_sha256_hash(password, salt, hash);
	params[1] = hash;
	result = execute_sql("select \"name\" from \"user\" where \"name\" = $1 and \"password_hash\" = $2", params, 2);
	bool exists = PQntuples(result) > 0;
	PQclear(result);
	if (exists) {
		print_info_f("User " A_CYAN "%s" A_YELLOW " has logged in.", name);
	} else {
		print_info_f("Failed login attempt for user " A_CYAN "%s" A_YELLOW ".", name);
	}
	return exists;
}

void render_profile(struct render_buffer* buffer) {
	assign_buffer(buffer, get_cached_file("html/profile.html", NULL));
}

void render_login(struct render_buffer* buffer) {
	assign_buffer(buffer, get_cached_file("html/login.html", NULL));
}

void render_register(struct render_buffer* buffer) {
	assign_buffer(buffer, get_cached_file("html/register.html", NULL));
}
