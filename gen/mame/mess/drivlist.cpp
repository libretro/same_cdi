#include "emu.h"

#include "drivenum.h"

GAME_EXTERN(___empty);
GAME_EXTERN(cdi490a);
GAME_EXTERN(cdi910);
GAME_EXTERN(cdimono1);
GAME_EXTERN(cdimono2);

game_driver const *const driver_list::s_drivers_sorted[5] =
{
	&GAME_NAME(___empty),
	&GAME_NAME(cdi490a),
	&GAME_NAME(cdi910),
	&GAME_NAME(cdimono1),
	&GAME_NAME(cdimono2),
};

std::size_t const driver_list::s_driver_count = 5;
