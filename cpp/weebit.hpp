#include <cstddef>
#include <algorithm>

#include "json.hpp"

#ifndef WEEBITCPP_H
#define WEEBITCPP_H

using std::size_t;
using std::min;

namespace weebit {

template<class T>
class StreamParser {
private:
	using JsonHandlerType = T;
	using JsonParserType = JsonParser<JsonHandlerType>;
	
	enum {
		IDLE,
		WAIT_TYPE,
		JSON,
		MULTIPART,
	} state;

	const char OBJECT_START[3] = "{}";
	const char MULTIPART_START = '-';
	char buf[sizeof(OBJECT_START)];
	const size_t buf_size = sizeof(buf);

	size_t buf_bytes;

	JsonParserType json; 
	void reset(void) {
		state = StreamParser::IDLE;
		buf_bytes = 0;
	}

public:

	StreamParser(JsonHandlerType &json_handler) : json(json_handler) {
		reset();
	}

	~StreamParser() {}
	
	void feed(const char d) {
		const size_t hdr_size = sizeof(OBJECT_START) / sizeof(OBJECT_START[0])
								- sizeof(OBJECT_START[0]);

		switch (state) {
		/**
		* In the idle state, buffer chars until we hit {} (OBJECT_START).
		* After buffering the start, the next character should be either -
		* (MPF) or { (JSON). This gets determined in the WAIT_TYPE state.
		*/
		case StreamParser::IDLE:
			if (d == OBJECT_START[min(buf_bytes, hdr_size)] && buf_bytes < buf_size) {
				buf[buf_bytes++] = d; 
				if (buf_bytes == hdr_size)
					state = WAIT_TYPE;

			// The buffered sequence does not match OBJECT_START.
			} else {
				reset();
			}

			break;

		/**
		* We received {} already. This is the next character used to determine the
		* type of data that comes next.
		*/
		case WAIT_TYPE:
			// This character and data that follows are JSON.
			if (d == JsonParserType::JSON_START) {
				state = JSON;

				[[fallthrough]];

			// This begins a multipart form.
			} else if (d == MULTIPART_START) {
				state = StreamParser::MULTIPART;

				[[fallthrough]];

			// Unknown
			} else {
				reset();

				break;
			}

		/**
		* Feed characters to JSON as long as it keeps accepting them.  When it
		* no longer accepts them, reset the parser (JSON parser resets itself).
		*/
		case JSON:
			if (json.template feed<0u>(d) != JsonParserType::CONTINUE) {
				reset();
			}

			break;

		case MULTIPART: 
			reset();

			break;
		}
	}

	void feed(const char *str) {
		while (*str)
			feed(*str++);
	}

	void feed(const char *const str, const size_t size) {
		for (size_t i = 0; i < size; ++i)
			feed(str[i]);
	}
};

}

#endif
