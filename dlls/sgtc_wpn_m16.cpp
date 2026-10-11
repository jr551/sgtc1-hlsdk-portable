// sgtc_wpn_m16.cpp — StargateTC CM16 (split from sgtc_weapons.cpp; NEW file).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"
#include "sgtc_weapons.h"

// ===========================================================================
// CM16 - "M16" autorifle. Analogue: mp5.cpp.
// Event: events/m16.sc.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_m16, CM16 );

void CM16::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_m16.mdl" );
	m_iId = SGTC_WPN_M16;
	m_iDefaultAmmo = 210;
	FallInit();
}

void CM16::Precache( void )
{
	PRECACHE_MODEL( "models/v_m16.mdl" );
	PRECACHE_MODEL( "models/w_m16.mdl" );
	PRECACHE_MODEL( "models/p_m16.mdl" );
	PRECACHE_MODEL( "models/rshell.mdl" );
	PRECACHE_MODEL( "models/w_9mmARclip.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "items/clipinsert1.wav" );
	PRECACHE_SOUND( "items/cliprelease1.wav" );
	PRECACHE_SOUND( "weapons/M16-01.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );

	m_usFire = PRECACHE_EVENT( 1, "events/m16.sc" );
}

SGTC_GETITEM( CM16, SGTC_AMMO_M16, 210, 30, 2, 1, SGTC_WPN_M16, 0, 15 )
SGTC_ADD_TO_PLAYER( CM16 )

BOOL CM16::Deploy( void )
{
	return SGTC_DEPLOY( 2, "models/v_m16.mdl", "models/p_m16.mdl", SGTC_DRAW, "mp5" );
}

SGTC_GUN_PRIMARY( CM16, m_usFire, SGTC_BULLET_M16, VECTOR_CONE_5DEGREES, 1, 8192, 8, 2, 0.1 )
SGTC_NO_SECONDARY( CM16 )
SGTC_WEAPON_RELOAD( CM16, 30 )
SGTC_WEAPON_IDLE( CM16 )
