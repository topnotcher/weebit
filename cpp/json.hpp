#include <tuple>
#include <cctype>
#include <cstddef>

#ifndef WEEBITCPPJSON_H
#define WEEBITCPPJSON_H

using std::size_t;
using std::isspace;

namespace weebit {

template <class T>
class JsonParser {
public:
	using JsonDocumentHandler = T;
	static const char JSON_START = '{';
	static const char JSON_END = '}';

	enum Status {
		DONE,
		CONTINUE,
		PARSE_ERROR,
		BUFFER_OVERFLOW,
		INVALID_STATE,
	};

	enum ParseFlags {
		allowInterDocWhiteSpace = 1,
		preserveWhiteSpace,
	};

private:
	enum {
		DOCUMENT,
		QUOTE,
	} state; 
	JsonDocumentHandler handler;
	size_t buf_size;
	size_t buf_bytes;
	char *buf;
	bool escaped_char;

	size_t level;

	template<unsigned flags=0>
	std::tuple<bool, Status> feed_document(const char d) {
		auto status = CONTINUE;
		auto consumed = true;

		if (d == JSON_START) {
			level++;

		// level is 0, but first character of doc was not {
		} else if (level == 0) {
			consumed = false;
			if (!isspace(d) || !(flags & allowInterDocWhiteSpace))
				status = PARSE_ERROR;

		} else if (d == JSON_END) {
			if (level > 0) {
				level--;

				if (level == 0) {
					status = DONE;
				}
			} else {
				// too many }. Nothing to do here!
				status = PARSE_ERROR;
				consumed = false;
			}

		} else if (d == '"') {
			state = QUOTE;

		// Stupid optimization: avoid buffering whitespace in the document.
		} else if (!(flags & preserveWhiteSpace) && isspace(d)) {
			consumed = false;
		}

		return std::make_tuple(consumed, status);
	}

	template<unsigned flags=0>
	std::tuple<bool, Status> feed_quoted(const char d) {
		if (!escaped_char) {
			if (d == '"')
				state = DOCUMENT;

			else if (d == '\\')
				escaped_char = true;

		} else {
			escaped_char = false;
		}

		return std::make_tuple(true, CONTINUE);
	}

	void reset() {
		level = 0;
		escaped_char = false;
		state = DOCUMENT;

		handler.flush();
	}

	inline bool consume(const char d) {
		return handler.consume(d);
	}

public:

	JsonParser(JsonDocumentHandler &h) {
		handler = h;
		reset();
	}

	~JsonParser() {}

	template<unsigned flags=0>
	Status feed(const char d) {
		auto status = CONTINUE;
		auto consumed = false;

		if (state == DOCUMENT) {
			std::tie(consumed, status) = feed_document<flags>(d);

		} else if (state == QUOTE) {
			std::tie(consumed, status) = feed_quoted<flags>(d);

		} else {
			status = INVALID_STATE;
		}

		if (consumed) {
			if (!handler.consume(d)) {
				status = BUFFER_OVERFLOW;

			} else if (status == DONE) {
				handler.done();
			}
		}

		// Could be either DONE or an error. In any case, state needs to be reset.
		if (status != CONTINUE) {
			reset();
		}

		return status;
	}
};

}
#endif
