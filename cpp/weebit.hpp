#include <cstddef>
#include <algorithm>
#include <tuple>

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

	void feed(const char *const in, const size_t size) {
		size_t consumed = 0;
		const char *rptr = in; 

		while (consumed < size) {
			size_t consumed_by_state = 0;
			consumed_by_state = consume_with_state(rptr, size - consumed);
			rptr += consumed_by_state;
			consumed += consumed_by_state;
		}
	}

	size_t consume_with_state(const char *const in, const size_t size) {
		const size_t hdr_size = sizeof(OBJECT_START) / sizeof(OBJECT_START[0])
								- sizeof(OBJECT_START[0]);

		size_t consumed = 0;

		switch (state) {

		/**
		* In the idle state, buffer chars until we hit {} (OBJECT_START).
		* After buffering the start, the next character should be either -
		* (MPF) or { (JSON). This gets determined in the WAIT_TYPE state.
		*/
		case StreamParser::IDLE:
			consumed = 1;
			if (*in == OBJECT_START[min(buf_bytes, hdr_size)] && buf_bytes < buf_size) {
				buf[buf_bytes++] = *in; 
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
			if (*in == JsonParserType::JSON_START) {
				state = JSON;

				[[fallthrough]];

			// This begins a multipart form.
			} else if (*in == MULTIPART_START) {
				state = StreamParser::MULTIPART;

				[[fallthrough]];

			// Unknown
			} else {
				consumed = 1;
				reset();

				break;
			}

		/**
		* Feed characters to JSON as long as it keeps accepting them.  When it
		* no longer accepts them, reset the parser (JSON parser resets itself).
		*/
		case JSON:
			typename JsonParserType::Status status;
			std::tie(consumed, status) = json.template feed<0u>(in, size);

			if (status != JsonParserType::CONTINUE)
				reset();

			break;

		case MULTIPART: 
			consumed = 1;
			reset();

			break;

		default:
			consumed = 1;
			break;
		}

		return consumed;
	}

	// void feed(const char *str) {
	// 	while (*str)
	// 		feed(*str++);
	// }
};

}

#endif
