// sgtc_wpn_psg1.cpp — StargateTC CPsg1 (split from sgtc_weapons.cpp; NEW file).

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
// CPsg1 - "PSG1" sniper. Analogue: python.cpp (slow, high-damage hitscan).
// Event: events/psg1.sc.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_psg1, CPsg1 );

void CPsg1::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_psg1.mdl" );
	m_iId = SGTC_WPN_PSG1;
	m_iDefaultAmmo = 50;
	FallInit();
}

void CPsg1::Precache( void )
{
	PRECACHE_MODEL( "models/v_psg1.mdl" );
	PRECACHE_MODEL( "models/w_psg1.mdl" );
	PRECACHE_MODEL( "models/p_psg1.mdl" );
	PRECACHE_MODEL( "models/shell.mdl" );

	PRECACHE_SOUND( "weapons/psg1_reload.wav" );
	PRECACHE_SOUND( "weapons/psg1_fire.wav" );
	PRECACHE_SOUND( "weapons/psg1_empty.wav" );

	m_usFire = PRECACHE_EVENT( 1, "events/psg1.sc" );
}

SGTC_GETITEM( CPsg1, SGTC_AMMO_PSG1, 50, 10, 2, 4, SGTC_WPN_PSG1, 0, 15 )
SGTC_ADD_TO_PLAYER( CPsg1 )

BOOL CPsg1::Deploy( void )
{
	return DefaultDeploy( "models/v_psg1.mdl", "models/p_psg1.mdl", SGTC_DRAW, "bow" );
}

SGTC_GUN_PRIMARY( CPsg1, m_usFire, SGTC_BULLET_PSG1, VECTOR_CONE_1DEGREES, 1, 8192, 40, 0, 1.0 )
SGTC_NO_SECONDARY( CPsg1 )
SGTC_WEAPON_RELOAD( CPsg1, 10 )
SGTC_WEAPON_IDLE( CPsg1 )

// Reference maps weapon_spas to stock CShotgun (shotgun.cpp); stock
// registration stands, so CSGTCSpas is removed.
