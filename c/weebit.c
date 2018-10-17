#include <stdbool.h>
#include <stdlib.h>

#include "weebit.h"


/**
 * Initial state: PARSER_IDLE
 * PARSER_IDLE -> PARSER_RECV when {} is received
 * PARSER_RECV -> PARSER_RECV_JSON when { is received.
 * PARSER_RECV -> PARSER_RECV_MULTIPART when - is received.
 */
enum stream_parser_state {
	PARSER_IDLE,
	PARSER_RECV,
	PARSER_RECV_JSON,
	PARSER_RECV_MULTIPART,
};

static const size_t MAX_DOCUMENT_LENGTH = 65535;
static const char OBJECT_START[] = "{}";
static const char JSON_START = '{';
static const char MULTIPART_START = '-';

enum json_parser_state {
	JSON_STATE_DOCUMENT,
	JSON_STATE_QUOTE,
};

enum json_parser_status {
	JSON_PARSER_DONE,
	JSON_PARSER_CONTINUE,
	JSON_PARSER_PARSE_ERROR,
	JSON_PARSER_BUFFER_OVERFLOW,
};

typedef struct {
	enum json_parser_state state;
	size_t buf_size;
	char *buf;
	size_t buf_bytes;
	bool escaped_char;

	size_t level; 
	void (*handler)(const char *const, const size_t);
} json_parser;


typedef struct {
	int foo;
} multipart_parser;


struct _stream_parser {
	enum stream_parser_state state;	

	// maximum size of the input buffer.
	size_t buf_size;

	// input buffer
	char *buf;

	// bytes in the input buffer
	size_t buf_bytes;

	json_parser *json_parser;
	multipart_parser *multipart_parser;
};


static void stream_parser_reset(stream_parser *const);

static json_parser *json_parser_init(void);
static void json_parser_destroy(json_parser *const);
static multipart_parser *multipart_parser_init(void);
static void multipart_parser_destroy(multipart_parser *const);

static void json_start_document(json_parser *const, char *const, const size_t);
static enum json_parser_status json_feed(json_parser *const, const char);
static enum json_parser_status json_feed_quoted(json_parser *const, const char);
static enum json_parser_status json_feed_document(json_parser *const, const char);


static inline size_t min(const size_t a, const size_t b) {
	return (a < b) ? a : b;
}

static void stream_parser_reset(stream_parser *const p) {
	p->state = PARSER_IDLE;
	p->buf_bytes = 0;
}

void stream_parser_feed(stream_parser *const p, const char d) {
	if (p == NULL)
		return;

	const size_t hdr_size = sizeof(OBJECT_START) / sizeof(OBJECT_START[0]) - sizeof(OBJECT_START[0]);

	switch (p->state) {
	case PARSER_IDLE:
		if (d == OBJECT_START[min(p->buf_bytes, hdr_size)] && p->buf_bytes < p->buf_size) {
			p->buf[p->buf_bytes++] = d;

			if (p->buf_bytes == hdr_size)
				p->state = PARSER_RECV;

		} else {
			stream_parser_reset(p);
		}

		break;

	case PARSER_RECV:
		if (d == JSON_START) {
			p->state = PARSER_RECV_JSON;
			json_start_document(p->json_parser, p->buf + p->buf_bytes, p->buf_size - p->buf_bytes);
			stream_parser_feed(p, d); // recurse (rather than fall through)

		} else if (d == MULTIPART_START) {
			p->state = PARSER_RECV_MULTIPART;
			// multipart_start_parser(p->multipart_parser, p->buf + p->buf_bytes, p->buf_size - p->buf_bytes);
			// stream_parser_feed(p, d); // recurse (rather than fall through)

		} else {
			// invalid. Clear the buffer and current state.
			stream_parser_reset(p);
		}

		break;

	case PARSER_RECV_JSON:
		// feed bytes to the JSON parser until it returns true (complete
		// document or parser error)
		if (json_feed(p->json_parser, d) != JSON_PARSER_CONTINUE)
			stream_parser_reset(p);

		break;

	case PARSER_RECV_MULTIPART:
		/*
		if (multipart_feed_parser(p->json_parser, d))
			stream_parser_reset(p);
		*/
		// TODO
		stream_parser_reset(p);
		break;
	}
}

stream_parser *stream_parser_init(void) {
	char *buf = malloc(MAX_DOCUMENT_LENGTH);
	stream_parser *p = malloc(sizeof *p);
	json_parser *j = json_parser_init();
	multipart_parser *m = multipart_parser_init();

	if (p == NULL || j == NULL || m == NULL || buf == NULL)  {
		json_parser_destroy(j);
		multipart_parser_destroy(m);
		free(p);
		free(buf);

		p = NULL;
	} else {
		p->buf = buf;
		p->buf_size = MAX_DOCUMENT_LENGTH;

		p->json_parser = j;
		p->multipart_parser = m;

		stream_parser_reset(p);
	}

	return p;
}

void stream_parser_destroy(stream_parser *const p) {
	if (p != NULL) {
		json_parser_destroy(p->json_parser);
		p->json_parser = NULL;

		multipart_parser_destroy(p->multipart_parser);
		p->multipart_parser = NULL;

		free(p->buf);	
		p->buf = NULL;

		free(p);
	}
}

json_parser *json_parser_init(void) {
	json_parser *j = malloc(sizeof *j);

	if (j != NULL) {
		j->handler = NULL;
		json_start_document(j, NULL, 0);
	}

	return j;
}

void json_parser_destroy(json_parser *const j) {
	free(j);
}

multipart_parser *multipart_parser_init(void) {
	multipart_parser *m = malloc(sizeof *m);

	if (m != NULL) {
		// TODO
	}

	return m;
}

void multipart_parser_destroy(multipart_parser *const m) {
	free(m);
}

static void json_start_document(json_parser *const j, char *const buf, const size_t buf_size) {
	if (j != NULL) {
		j->state = JSON_STATE_DOCUMENT;
		j->buf = buf;
		j->buf_size = buf_size;
		j->buf_bytes = 0;
		j->level = 0;
		j->escaped_char = false;
	}
}

static enum json_parser_status json_feed(json_parser *const j, const char d) {
	enum json_parser_status status = JSON_PARSER_CONTINUE;

	if (j->buf_bytes >= j->buf_size) {
		status = JSON_PARSER_BUFFER_OVERFLOW;

	} else {
		j->buf[j->buf_bytes++] = d;

		if (j->state == JSON_STATE_DOCUMENT) {
			status =json_feed_document(j, d);

		} else if (j->state == JSON_STATE_QUOTE) {
			status = json_feed_quoted(j, d);
		}
	}

	return status;
}

static enum json_parser_status json_feed_document(json_parser *const j, const char d) {
	enum json_parser_status status = JSON_PARSER_CONTINUE;

	if (d == '{') {
		j->level++;

	} else if (d == '}') {
		if (j->level > 0) {
			j->level--;

			if (j->level == 0 && j->buf_bytes < j->buf_size - 1) {

				if (j->handler) {
					j->buf[j->buf_bytes] = '\0';
					j->handler(j->buf, j->buf_bytes);
				}

				// done receiving a document
				status = JSON_PARSER_DONE;
			}

		} else {

			// too many }. Nothing to do here!
			status = JSON_PARSER_PARSE_ERROR;
		}

	} else if (d == '"') {
		j->state = JSON_STATE_QUOTE;
	}

	return status;
}

static enum json_parser_status json_feed_quoted(json_parser *const j, const char d) {
	/* Return to the document state if we reach the closing quote.
	 * - And the closing quote was not escaped by \.
	 * - The closing quote is escaped by \ if and only if:
	 *     - The character before the quote was \; and
	 *     - The character before the \ was not \\
	 */
	if (!j->escaped_char) {
		if (d == '"')
			j->state = JSON_STATE_DOCUMENT;

		else if (d == '\\')
			j->escaped_char = true;

	} else {
		j->escaped_char = false;
	}

	return JSON_PARSER_CONTINUE;
}

void json_set_handler(stream_parser *const p, void (*handler)(const char *const, const size_t)) {
	if (p != NULL && p->json_parser != NULL)
		p->json_parser->handler = handler;
}
