// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    chd.h

    MAME Compressed Hunks of Data file format, read side.

    The container format itself is documented in
    src/osd/libretro/libretro-common/formats/chd/FORMAT.md; the decoding
    is done by the rchd reader from libretro-common.  This build is a
    read-only consumer: creating, writing and verifying CHDs is not
    supported, and the writer entry points below return
    error::UNSUPPORTED_FORMAT so the (never-taken for CD media) diff-CHD
    path in romload fails cleanly instead of at link time.

***************************************************************************/
#ifndef MAME_LIB_UTIL_CHD_H
#define MAME_LIB_UTIL_CHD_H

#pragma once

#include "hashing.h"
#include "ioprocs.h"

#include "osdcore.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

struct rchd;


//**************************************************************************
//  CONSTANTS
//**************************************************************************

// codec and metadata tags are four-character big-endian constants
typedef uint32_t chd_codec_type;
typedef uint32_t chd_metadata_tag;

constexpr chd_codec_type CHD_MAKE_TAG(char a, char b, char c, char d)
{
	return (uint32_t(uint8_t(a)) << 24) | (uint32_t(uint8_t(b)) << 16) | (uint32_t(uint8_t(c)) << 8) | uint32_t(uint8_t(d));
}

// general codecs
constexpr chd_codec_type CHD_CODEC_NONE     = 0;
constexpr chd_codec_type CHD_CODEC_ZLIB     = CHD_MAKE_TAG('z','l','i','b');
constexpr chd_codec_type CHD_CODEC_ZSTD     = CHD_MAKE_TAG('z','s','t','d');
constexpr chd_codec_type CHD_CODEC_LZMA     = CHD_MAKE_TAG('l','z','m','a');
constexpr chd_codec_type CHD_CODEC_HUFFMAN  = CHD_MAKE_TAG('h','u','f','f');
constexpr chd_codec_type CHD_CODEC_FLAC     = CHD_MAKE_TAG('f','l','a','c');

// general codecs with CD frontend
constexpr chd_codec_type CHD_CODEC_CD_ZLIB  = CHD_MAKE_TAG('c','d','z','l');
constexpr chd_codec_type CHD_CODEC_CD_ZSTD  = CHD_MAKE_TAG('c','d','z','s');
constexpr chd_codec_type CHD_CODEC_CD_LZMA  = CHD_MAKE_TAG('c','d','l','z');
constexpr chd_codec_type CHD_CODEC_CD_FLAC  = CHD_MAKE_TAG('c','d','f','l');

// A/V codec
constexpr chd_codec_type CHD_CODEC_AVHUFF   = CHD_MAKE_TAG('a','v','h','u');

// metadata parameters
constexpr chd_metadata_tag CHDMETATAG_WILDCARD = 0;
constexpr uint32_t CHDMETAINDEX_APPEND = ~0;

// metadata flags
constexpr uint8_t CHD_MDFLAGS_CHECKSUM = 0x01;        // indicates data is checksummed

// standard hard disk metadata
constexpr chd_metadata_tag HARD_DISK_METADATA_TAG = CHD_MAKE_TAG('G','D','D','D');
extern const char *HARD_DISK_METADATA_FORMAT;

// hard disk identify information
constexpr chd_metadata_tag HARD_DISK_IDENT_METADATA_TAG = CHD_MAKE_TAG('I','D','N','T');

// hard disk key information
constexpr chd_metadata_tag HARD_DISK_KEY_METADATA_TAG = CHD_MAKE_TAG('K','E','Y',' ');

// pcmcia CIS information
constexpr chd_metadata_tag PCMCIA_CIS_METADATA_TAG = CHD_MAKE_TAG('C','I','S',' ');

// standard CD-ROM metadata
constexpr chd_metadata_tag CDROM_OLD_METADATA_TAG = CHD_MAKE_TAG('C','H','C','D');
constexpr chd_metadata_tag CDROM_TRACK_METADATA_TAG = CHD_MAKE_TAG('C','H','T','R');
extern const char *CDROM_TRACK_METADATA_FORMAT;
constexpr chd_metadata_tag CDROM_TRACK_METADATA2_TAG = CHD_MAKE_TAG('C','H','T','2');
extern const char *CDROM_TRACK_METADATA2_FORMAT;
constexpr chd_metadata_tag GDROM_OLD_METADATA_TAG = CHD_MAKE_TAG('C','H','G','T');
constexpr chd_metadata_tag GDROM_TRACK_METADATA_TAG = CHD_MAKE_TAG('C', 'H', 'G', 'D');
extern const char *GDROM_TRACK_METADATA_FORMAT;

// standard A/V metadata
constexpr chd_metadata_tag AV_METADATA_TAG = CHD_MAKE_TAG('A','V','A','V');
extern const char *AV_METADATA_FORMAT;

// A/V laserdisc frame metadata
constexpr chd_metadata_tag AV_LD_METADATA_TAG = CHD_MAKE_TAG('A','V','L','D');


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> chd_file

// core file class
class chd_file
{
public:
	// error types
	enum class error
	{
		NO_INTERFACE = 1,
		NOT_OPEN,
		ALREADY_OPEN,
		INVALID_FILE,
		INVALID_DATA,
		REQUIRES_PARENT,
		FILE_NOT_WRITEABLE,
		CODEC_ERROR,
		INVALID_PARENT,
		HUNK_OUT_OF_RANGE,
		DECOMPRESSION_ERROR,
		COMPRESSION_ERROR,
		CANT_VERIFY,
		METADATA_NOT_FOUND,
		INVALID_METADATA_SIZE,
		UNSUPPORTED_VERSION,
		VERIFY_INCOMPLETE,
		INVALID_METADATA,
		INVALID_STATE,
		OPERATION_PENDING,
		UNSUPPORTED_FORMAT,
		UNKNOWN_COMPRESSION,
		WALKING_PARENT,
		COMPRESSING
	};

	// construction/destruction
	chd_file();
	virtual ~chd_file();

	// getters
	util::random_read &file();
	bool opened() const { return m_chd != nullptr; }
	uint32_t version() const;
	uint64_t logical_bytes() const;
	uint32_t hunk_bytes() const;
	uint32_t hunk_count() const;
	uint32_t unit_bytes() const;
	uint64_t unit_count() const;
	bool compressed() const;
	chd_codec_type compression(int index) const;
	chd_file *parent() const { return m_parent; }
	util::sha1_t sha1();
	util::sha1_t raw_sha1();
	util::sha1_t parent_sha1();

	// file create (unsupported in this read-only build)
	std::error_condition create(std::string_view filename, uint64_t logicalbytes, uint32_t hunkbytes, uint32_t unitbytes, chd_codec_type compression[4]);
	std::error_condition create(util::random_read_write::ptr &&file, uint64_t logicalbytes, uint32_t hunkbytes, uint32_t unitbytes, chd_codec_type compression[4]);
	std::error_condition create(std::string_view filename, uint64_t logicalbytes, uint32_t hunkbytes, chd_codec_type compression[4], chd_file &parent);
	std::error_condition create(util::random_read_write::ptr &&file, uint64_t logicalbytes, uint32_t hunkbytes, chd_codec_type compression[4], chd_file &parent);

	// file open
	std::error_condition open(std::string_view filename, bool writeable = false, chd_file *parent = nullptr);
	std::error_condition open(util::random_read_write::ptr &&file, bool writeable = false, chd_file *parent = nullptr);

	// file close
	void close();

	// read/write
	std::error_condition read_hunk(uint32_t hunknum, void *buffer);
	std::error_condition write_hunk(uint32_t hunknum, const void *buffer);
	std::error_condition read_units(uint64_t unitnum, void *buffer, uint32_t count = 1);
	std::error_condition write_units(uint64_t unitnum, const void *buffer, uint32_t count = 1);
	std::error_condition read_bytes(uint64_t offset, void *buffer, uint32_t bytes);
	std::error_condition write_bytes(uint64_t offset, const void *buffer, uint32_t bytes);

	// metadata management
	std::error_condition read_metadata(chd_metadata_tag searchtag, uint32_t searchindex, std::string &output);
	std::error_condition read_metadata(chd_metadata_tag searchtag, uint32_t searchindex, std::vector<uint8_t> &output);
	std::error_condition read_metadata(chd_metadata_tag searchtag, uint32_t searchindex, void *output, uint32_t outputlen, uint32_t &resultlen);
	std::error_condition read_metadata(chd_metadata_tag searchtag, uint32_t searchindex, std::vector<uint8_t> &output, chd_metadata_tag &resulttag, uint8_t &resultflags);
	std::error_condition write_metadata(chd_metadata_tag metatag, uint32_t metaindex, const void *inputbuf, uint32_t inputlen, uint8_t flags = CHD_MDFLAGS_CHECKSUM);
	std::error_condition write_metadata(chd_metadata_tag metatag, uint32_t metaindex, const std::string &input, uint8_t flags = CHD_MDFLAGS_CHECKSUM) { return write_metadata(metatag, metaindex, input.c_str(), input.length() + 1, flags); }
	std::error_condition write_metadata(chd_metadata_tag metatag, uint32_t metaindex, const std::vector<uint8_t> &input, uint8_t flags = CHD_MDFLAGS_CHECKSUM) { return write_metadata(metatag, metaindex, &input[0], input.size(), flags); }
	std::error_condition delete_metadata(chd_metadata_tag metatag, uint32_t metaindex);
	std::error_condition clone_all_metadata(chd_file &source);

private:
	// internal helpers
	std::error_condition open_common(chd_file *parent);
	std::error_condition pump_read(uint64_t offset, void *buffer, size_t bytes);
	std::error_condition feed_request(const struct rchd_request *req);
	const struct rchd_metadata *metadata_find(chd_metadata_tag searchtag, uint32_t searchindex) const;

	// state
	rchd *                       m_chd;      // decoder, non-null while open
	util::random_read_write::ptr m_file;     // handle to the open file
	chd_file *                   m_parent;   // pointer to parent file, or nullptr if none
};


// error category for CHD errors
std::error_category const &chd_category() noexcept;
inline std::error_condition make_error_condition(chd_file::error err) noexcept { return std::error_condition(int(err), chd_category()); }

namespace std {

template <> struct is_error_condition_enum<chd_file::error> : public std::true_type { };

} // namespace std

#endif // MAME_LIB_UTIL_CHD_H
