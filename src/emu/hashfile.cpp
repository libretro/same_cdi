// license:BSD-3-Clause
// copyright-holders:Wilbert Pol
/*********************************************************************

    hashfile.cpp

    Stub .hsi extrainfo lookup.  The real implementation opened
    hash/<sysname>.hsi databases and ran pugixml XPath queries keyed
    by the mounted image's CRC32/SHA1 to fetch a per-dump extrainfo
    string.  Its only caller is the get_default_card_software_hook
    that slot devices consult, and this core compiles no slot device
    with such an override (nor ships any .hsi database), so the
    lookup - pugixml's sole consumer in the tree - was unreachable.

*********************************************************************/

#include "emu.h"
#include "hashfile.h"


bool hashfile_extrainfo(const char *hash_path, const game_driver &driver, const util::hash_collection &hashes, std::string &result)
{
	result.clear();
	return false;
}


bool hashfile_extrainfo(device_image_interface &image, std::string &result)
{
	result.clear();
	return false;
}
