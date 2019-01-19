#include <unistd.h>
#include <stdio.h>

#include <string>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "weebit.hpp"

using std::string;


void json_testing_handler(const char *const str, const size_t) {
	rapidjson::Document d;
	rapidjson::StringStream json {str};

    d.ParseStream<rapidjson::kParseValidateEncodingFlag, rapidjson::UTF8<>>(json);

	if (!d.HasParseError()) {
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<decltype(buffer)> writer {buffer};
		d.Accept(writer);
		string str = buffer.GetString();

		printf("%lu%s", str.size(), str.c_str());
	} else {
		printf("0");
	}
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
