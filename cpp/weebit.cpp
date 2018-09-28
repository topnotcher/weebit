#include "weebit.hpp"

template <typename T>
static inline T min(const T a, const T b) {
	return (a < b) ? a : b;
}

StreamParser::StreamParser() {
	this->buf = new char[StreamParser::MAX_DOCUMENT_LENGTH];
	this->buf_size = MAX_DOCUMENT_LENGTH;
	this->reset();
}

StreamParser::~StreamParser() {
	delete this->buf;
	this->buf = nullptr;
	this->buf_size = 0;
}

void StreamParser::feed(const char d) {
	const size_t hdr_size = sizeof(StreamParser::OBJECT_START) / sizeof(StreamParser::OBJECT_START[0])
							- sizeof(StreamParser::OBJECT_START[0]);

	switch (this->state) {
	case StreamParser::IDLE:
		if (d == StreamParser::OBJECT_START[min(this->buf_bytes, hdr_size)] && this->buf_bytes < this->buf_size) {
			this->buf[this->buf_bytes++] = d;

			if (this->buf_bytes == hdr_size)
				this->state = StreamParser::WAIT_TYPE;

		} else {
			this->reset();
		}

		break;

	case StreamParser::WAIT_TYPE:
		if (d == StreamParser::JSON_START) {
			this->state = StreamParser::JSON;
			this->json.start(this->buf + this->buf_bytes, this->buf_size - this->buf_bytes);
			this->feed(d);

		} else if (d == StreamParser::MULTIPART) {
			/* this->state = StreamParser::MULTIPART;
			this->feed(d); */
			this->reset();

		} else {
			this->reset();
		}

		break;

	case StreamParser::JSON:
		if (this->json.feed(d) != JsonParser::CONTINUE) {
			this->reset();
		}

		break;

	case StreamParser::MULTIPART: 
		this->reset();

		break;
	}
}

void StreamParser::reset(void) {
	this->state = StreamParser::IDLE;
	this->buf_bytes = 0;
}

JsonParser::JsonParser() {
	this->handler = nullptr;
	this->buf = nullptr;
	this->buf_size = 0;
	this->level = 0;
}

JsonParser::~JsonParser() {

}

JsonParser::status JsonParser::feed(const char d) {
	auto status = JsonParser::CONTINUE;

	if (this->buf_bytes >= this->buf_size) {
		status = JsonParser::BUFFER_OVERFLOW;

	} else if (this->state == JsonParser::IDLE) {
		status = JsonParser::INVALID_STATE;

	} else {
		this->buf[this->buf_bytes++] = d;

		if (this->state == JsonParser::DOCUMENT) {
			status = this->feed_document(d);

		} else if (this->state == JsonParser::QUOTE) {
			status = this->feed_quoted(d);

		} else {
			status = JsonParser::INVALID_STATE;
		}
	}

	if (status != JsonParser::CONTINUE) {
		this->state = JsonParser::IDLE;
	}

	return status;
}

JsonParser::status JsonParser::feed_document(const char d) {
	auto status = JsonParser::CONTINUE;

	if (d == '{') {
		this->level++;

	} else if (d == '}') {
		if (this->level > 0) {
			this->level--;

			if (this->level == 0) {

				if (this->handler != nullptr) {
					this->buf[this->buf_bytes] = '\0';
					this->handler(this->buf, this->buf_bytes);
				}

				status = JsonParser::DONE;
			}

		} else {

			// too many }. Nothing to do here!
			status = JsonParser::PARSE_ERROR;
		}

	} else if (d == '"') {
		this->state = JsonParser::QUOTE;
	}

	return status;
}

JsonParser::status JsonParser::feed_quoted(const char d) {
	if (d == '"' && this->buf_bytes >= 2 && this->buf[this->buf_bytes - 2] != '\\')
		this->state = JsonParser::DOCUMENT;

	return JsonParser::CONTINUE;
}

void JsonParser::start(char *const buf, const size_t buf_size) {
	this->buf = buf;
	this->buf_bytes = 0;
	this->level = 0;

	if (this->buf != nullptr && buf_size > 0) {
		// reserve space for a NUL terminator
		this->buf_size = buf_size - 1;
		this->state = JsonParser::DOCUMENT;

	} else {
		this->buf = nullptr;
		this->buf_size = 0;
		this->state = JsonParser::IDLE;
	}
}
