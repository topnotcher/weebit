#include <unistd.h>

#include <cstdio>
#include <string>
#include <vector>
#include <iostream>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "json.hpp"
#include "weebit.hpp"


struct JsonHandler {
	static const size_t min_size = 8192;
	static const size_t max_size = 1024 * 1024;
	std::vector<char> vec;

	JsonHandler() {
		vec.reserve(min_size);
	}
	~JsonHandler() { }

	bool consume(const char *const str, const size_t size) {
		bool consumed = true;
		for (size_t idx = 0; idx < size && consumed; ++idx)
			consumed = consume(str[idx]);

		return consumed;
	}

	/**
	 * consume JSON
	 */
	bool consume(const char d) {
		if (vec.size() < max_size) {
			vec.push_back(d);
			return true;

		} else {
			return false;
		}
	}

	/**
	 * Consumed JSON is a complete document.
	 */
	void done() {
		rapidjson::Document d;
		d.Parse<rapidjson::kParseValidateEncodingFlag, rapidjson::UTF8<>>(vec.data(), vec.size());

		if (!d.HasParseError()) {
			std::cout << vec.size();
			std::cout.write(vec.data(), vec.size());

		} else {
			std::cout << 0;
		}
	}

	/**
	 * Flush any consumed data.
	 */
	void flush() {
		vec.clear();
	}
};

struct Stream {
	size_t stream_id;
	using JsonHandlerType = JsonHandler;
	using StreamParserType = weebit::StreamParser<JsonHandlerType>;

	StreamParserType parser;

	Stream(const size_t stream_id, JsonHandlerType &json)
		: stream_id(stream_id), parser(json) {}

	void feed(const char *const buf, size_t size) {
		parser.feed(buf, size);
	}
};

struct ParserTester {
	static const size_t max_streams = 10;
	Stream *streams[max_streams];
	Stream::JsonHandlerType json;

	ParserTester() {
		for (size_t i = 0; i < max_streams; ++i) {
			streams[i] = nullptr;
		}
	}
	~ParserTester() { }

	Stream *alloc_stream() {
		for (size_t i = 0; i < max_streams; ++i) {
			if (streams[i] == nullptr) {
				// Each stream shares a JSON handler. (for demo purposes only)
				streams[i] = new Stream(i, json);
				return streams[i];
			}
		}

		return nullptr;
	}
};

void stream_file(const int fd, Stream *const s) {
	char buf[4096];
	ssize_t size;
	do {
		size = read(fd, buf, sizeof buf);
		if (size > 0)
			s->feed(buf, size);

	} while (size > 0);
}

int main(void) {
	ParserTester t;
	Stream *s = t.alloc_stream();
	if (s != nullptr) {
		stream_file(STDIN_FILENO, s);
	} else {
		std::cout << "Failed to allocate stream!" << std::endl;
	}

	return 0;
}
