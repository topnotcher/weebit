#include <cstddef>

#ifndef WEEBITCPP_H
#define WEEBITCPP_H

using std::size_t;

class JsonParser {
	public:
	typedef void (*handler_callback)(const char *const, const size_t);

	enum status {
		DONE,
		CONTINUE,
		PARSE_ERROR,
		BUFFER_OVERFLOW,
		INVALID_STATE,
	};

	private:
	enum {
		IDLE,
		DOCUMENT,
		QUOTE,
	} state;

	handler_callback handler;
	size_t buf_size;
	size_t buf_bytes;
	char *buf;

	size_t level;

	JsonParser::status feed_document(const char);
	JsonParser::status feed_quoted(const char);

	public:

	JsonParser();
	~JsonParser();

	void start(char *const, const size_t);
	JsonParser::status feed(const char);

	void set_handler(handler_callback handler) {
		this->handler = handler;
	};
};

class StreamParser {

	private:
	
	enum {
		IDLE,
		WAIT_TYPE,
		JSON,
		MULTIPART,
	} state;

	const size_t MAX_DOCUMENT_LENGTH = 65535;
	const char OBJECT_START[3] = "{}";
	const char JSON_START = '{';
	const char MULTIPART_START = '-';

	size_t buf_bytes;
	size_t buf_size;
	char *buf;

	JsonParser json;

	void reset(void);

	public:

	StreamParser();
	~StreamParser();
	
	void set_json_handler(JsonParser::handler_callback handler) {
		this->json.set_handler(handler);
	}

	void feed(const char);
};

#endif
