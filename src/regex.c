#include "regex.h"
#include "system.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

// todo: store the compiled regex for reuse? especially considering if this is called in a loop.
// perhaps better to split into 2-3 functions, like how pcre works in the first place.
int regex_search(const char* pattern, const char** str) {
	int error = 0;
	PCRE2_SIZE error_offset = 0;
	pcre2_code* regex = pcre2_compile((PCRE2_SPTR8)pattern, PCRE2_ZERO_TERMINATED, 0, &error, &error_offset, NULL);
	if (!regex) {
		PCRE2_UCHAR error_str[128];
		pcre2_get_error_message(error, error_str, sizeof(error_str));
		print_error_f("Error compiling regex: \"%s\" at %i", error_str, (int)error_offset);
		return 0;
	}
	pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(regex, NULL);
	int rc = pcre2_match(regex, (PCRE2_SPTR)(*str), strlen(*str), 0, 0, match_data, NULL);
	int length = 0;
	if (rc >= 0) {
		PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(match_data);
		*str += ovector[0];
		length = ovector[1] - ovector[0];
	}
	pcre2_match_data_free(match_data);
	pcre2_code_free(regex);
	return length;
}
