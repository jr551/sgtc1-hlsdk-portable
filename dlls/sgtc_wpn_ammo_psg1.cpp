// sgtc_wpn_ammo_psg1.cpp — StargateTC CPsg1AmmoClip (split from sgtc_weapons.cpp; NEW file).

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
// CPsg1AmmoClip - PSG1 ammunition pickup. Analogue: glock.cpp CGlockAmmo.
// ===========================================================================

LINK_ENTITY_TO_CLASS( ammo_psg1, CPsg1AmmoClip );

void CPsg1AmmoClip::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_psg1.mdl" );
	CBasePlayerAmmo::Spawn();
}

void CPsg1AmmoClip::Precache( void )
{
	PRECACHE_MODEL( "models/w_psg1.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );
}

BOOL CPsg1AmmoClip::AddAmmo( CBaseEntity *pOther )
{
	int bResult = ( pOther->GiveAmmo( 10, SGTC_AMMO_PSG1, 50 ) != -1 );

	if( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
	}

	return bResult;
}
