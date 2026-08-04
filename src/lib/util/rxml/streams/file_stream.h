/* Minimal libretro-common compatibility shim for the vendored rxml.c:
   declarations for the file-path loader, which nothing in this tree
   calls; the stubs in rxml_filestream_stubs.c fail cleanly. */
#ifndef __LIBRETRO_SDK_FILE_STREAM_H
#define __LIBRETRO_SDK_FILE_STREAM_H
#include <stdint.h>
typedef struct RFILE RFILE;
enum
{
	RETRO_VFS_FILE_ACCESS_READ = (1 << 0)
};
enum
{
	RETRO_VFS_FILE_ACCESS_HINT_NONE = 0
};
#ifdef __cplusplus
extern "C" {
#endif
RFILE *filestream_open(const char *path, unsigned mode, unsigned hints);
int64_t filestream_get_size(RFILE *stream);
int64_t filestream_read(RFILE *stream, void *data, int64_t len);
int filestream_close(RFILE *stream);
#ifdef __cplusplus
}
#endif
#endif
