/* rxml_load_document() takes a file path and reads it through
   libretro-common's file streams.  This tree only ever parses from
   memory (util::xml::file feeds rxml_load_document_string_opts), so
   the stream layer is stubbed to fail cleanly rather than vendored. */
#include "streams/file_stream.h"
#include <stddef.h>

RFILE *filestream_open(const char *path, unsigned mode, unsigned hints)
{
	(void)path;
	(void)mode;
	(void)hints;
	return NULL;
}

int64_t filestream_get_size(RFILE *stream)
{
	(void)stream;
	return -1;
}

int64_t filestream_read(RFILE *stream, void *data, int64_t len)
{
	(void)stream;
	(void)data;
	(void)len;
	return -1;
}

int filestream_close(RFILE *stream)
{
	(void)stream;
	return 0;
}
