#include <tuple>
#include <cctype>
#include <cstddef>

#ifndef WEEBITCPPJSON_H
#define WEEBITCPPJSON_H

#ifdef __has_cpp_attribute
#  if __has_cpp_attribute(fallthrough)
#    define FALLTHROUGH [[fallthrough]]
#  endif
#endif
#ifndef FALLTHROUGH
#  define FALLTHROUGH
#endif

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
		removeWhiteSpace = 2,
	};

private:
	enum {
		IDLE,
		DOCUMENT,
		QUOTE,
		QUOTE_ESCAPE,
	} state; 
	JsonDocumentHandler handler;
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
		} else if ((flags & removeWhiteSpace) && isspace(d)) {
			consumed = false;
		}

		return std::make_tuple(consumed, status);
	}

	template<unsigned flags=0>
	std::tuple<bool, Status> feed_quoted(const char d) {
		if (d == '"')
			state = DOCUMENT;

		else if (d == '\\')
			state = QUOTE_ESCAPE;

		return std::make_tuple(true, CONTINUE);
	}

	template<unsigned flags=0>
	std::tuple<bool, Status> feed_quoted_escape(const char) {
		state = QUOTE;

		return std::make_tuple(true, CONTINUE);
	}

	template<unsigned flags=0>
	std::tuple<bool, Status> feed(const char d) {
		auto status = CONTINUE;
		auto consumed = false;

		switch (state) {
		case IDLE:
			reset();
			state = DOCUMENT;

			FALLTHROUGH;

		case DOCUMENT:
			std::tie(consumed, status) = feed_document<flags>(d);
			break;

		case QUOTE:
			std::tie(consumed, status) = feed_quoted<flags>(d);
			break;

		case QUOTE_ESCAPE:
			std::tie(consumed, status) = feed_quoted_escape<flags>(d);
			break;

		default:
			status = INVALID_STATE;
			break;
		}

		return std::make_tuple(consumed, status);
	}

public:

	template<unsigned flags=0>
	std::tuple<size_t, Status> feed(const char *const in, const size_t size) {
		auto status = CONTINUE;
		size_t total_consumed = 0;
		size_t idx = 0;

		auto start = in;

		while (idx < size && status == CONTINUE)  {
			total_consumed++;

			bool consume;
			std::tie(consume, status) = feed<flags>(in[idx++]);

			// Three cases:
			// - Status is DONE => we should consume all chars processed
			// - Status is CONTINUE, consume is false: hit whitespace or
			//   something that won't be consumed, but not a parse error. In
			//   this case, consume up to that char, but skip it.
			// - Status is CONTINUE and we're at the end of the buffer => consume it all.
			if ((status == DONE) || (status == CONTINUE && (!consume || idx == size))) {
				auto end = consume ? in + idx : in + idx - 1;

				if (!handler.consume(start, end - start)) {
					status = BUFFER_OVERFLOW;

				} else {
					start = in + idx;
				}
			}
		}

		if (status == DONE) {
			handler.done();
		}

		if (status != CONTINUE) {
			state = IDLE;
		}

		return std::make_tuple(total_consumed, status);
	}

	void reset() {
		level = 0;
		state = IDLE;

		handler.flush();
	}

	JsonParser(JsonDocumentHandler &h) : handler(h) {
		reset();
	}

	JsonParser(JsonDocumentHandler &&h) : JsonParser(h) {}

	~JsonParser() {}
};
}
#endif
