#include <stdlib.h>

#ifndef WEEBIT_H
#define WEEBIT_H
struct _stream_parser;
typedef struct _stream_parser stream_parser;

extern void stream_parser_destroy(stream_parser *const);
extern void stream_parser_feed(stream_parser *const, const char d);
extern stream_parser *stream_parser_init(void);
extern void json_set_handler(stream_parser *const, void (*)(const char *const, const size_t));
#endif
