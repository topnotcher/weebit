#include <unistd.h>
#include <stdio.h>

#include "weebitc.h"

void json_testing_handler(const char *const str, const size_t size) {
	printf("%lu%s", size, str);
}


int main(void) {
	static char buf[4096];
	ssize_t size;

	stream_parser *p = stream_parser_init();
	if (p != NULL) {
		json_set_handler(p, json_testing_handler);

		do {
			size = read(STDIN_FILENO, buf, sizeof buf);
			ssize_t idx = 0;

			// the parser is HUNGRRRRY
			while (idx < size)
				stream_parser_feed(p, buf[idx++]);

		} while (size > 0);

		stream_parser_destroy(p);
	}

	return 0;
}
