// sgtc_links.cpp — StargateTC entity registrations for stock SDK classes the
// reference build reuses. Verified by disasm (GetClassPtr thunk targets):
//   weapon_c4  → CSatchel    (stock satchel.cpp)
//   weapon_mine → CTripmine  (stock tripmine.cpp)
//   weapon_spas → CShotgun    (stock shotgun.cpp)

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"
// CSatchel / CShotgun / CTripmine are all declared in weapons.h.
#include "sgtc_weapons.h"

LINK_ENTITY_TO_CLASS( weapon_c4, CSatchel );
LINK_ENTITY_TO_CLASS( weapon_mine, CTripmine );
LINK_ENTITY_TO_CLASS( weapon_spas, CShotgun );
