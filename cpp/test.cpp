#include <unistd.h>
#include <stdio.h>

#include "weebit.hpp"

void json_testing_handler(const char *const str, const size_t size) {
	printf("%lu%s", size, str);
}


int main(void) {
	static char buf[4096];
	ssize_t size;

	StreamParser *p = new StreamParser();
	if (p != NULL) {
		p->set_json_handler(json_testing_handler);

		do {
			size = read(STDIN_FILENO, buf, sizeof buf);
			ssize_t idx = 0;

			// the parser is HUNGRRRRY
			while (idx < size)
				p->feed(buf[idx++]);

		} while (size > 0);

		delete p;
	}

	return 0;
}
