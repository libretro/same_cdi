/* Minimal libretro-common compatibility shim for the vendored rxml.c;
   see rxml/README. */
#ifndef LIBRETRO_COMMON_API_H__
#define LIBRETRO_COMMON_API_H__
#ifdef __cplusplus
#define RETRO_BEGIN_DECLS extern "C" {
#define RETRO_END_DECLS }
#else
#define RETRO_BEGIN_DECLS
#define RETRO_END_DECLS
#endif
#endif
