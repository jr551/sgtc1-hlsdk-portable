// sgtc_shield.cpp — effect_shield / CShield (ref/stc_i386.so @0x162388 Spawn, @0x16221a Think).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "sgtc_shield.h"

// ref rodata @0x1d9de0 / hull constants @0x1d9df4..0x1d9e04 / think @0x1d9dc4..
static const char *const kShieldModel = "models/shield.mdl";

LINK_ENTITY_TO_CLASS( effect_shield, CShield );

void CShield::Precache( void )
{
	PRECACHE_MODEL( kShieldModel );
}

void CShield::Spawn( void )
{
	Precache();

	pev->movetype = MOVETYPE_NONE;	// ref pev+0x108 = 0
	pev->solid = SOLID_NOT;		// ref pev+0x10c = 0

	SET_MODEL( ENT( pev ), kShieldModel );

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pev->renderamt = 100.0f;	// ref pev+0x160
	pev->angles = pev->v_angle;
	pev->origin = pev->origin - Vector( 0, 0, 40.0f );	// ref fsub 40 @0x1d9dfc

	pev->renderamt = 130.0f;	// ref pev+0x14c @0x1d9e00
	pev->rendermode = 5;		// ref pev+0x148 @0x1d9dfc (kRenderTransAdd in engine enum order)
	pev->view_ofs = g_vecZero;	// ref pev+0x174

	m_flExpire = gpGlobals->time + 1.0f;	// ref +0x19c @0x1d9e04

	SetThink( &CShield::Think );
	pev->nextthink = gpGlobals->time + 0.1f;	// ref double 0.1 @0x1d9dd8
	Think();	// ref: immediate virtual call via vslot +0xc4
}

void CShield::Think( void )
{
	// ref @0x16221a — health-driven renderamt thresholds:
	// pev->health <= 100 -> renderamt 130; pev->health <= 80 -> renderamt 200.
	if( pev->health <= 100.0f )
		pev->renderamt = 130.0f;
	if( pev->health <= 80.0f )
		pev->renderamt = 200.0f;

	if( !pev->owner )
		UTIL_Remove( this );

	if( m_flExpire < gpGlobals->time )
	{
		// ref: walk the player list; remove when no player owns this shield.
		BOOL bOwnerAlive = FALSE;
		for( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *plr = UTIL_PlayerByIndex( i );
			if( !plr )
				continue;
			if( plr->edict() == pev->owner )
				bOwnerAlive = TRUE;
		}
		if( !bOwnerAlive )
			UTIL_Remove( this );
		m_flExpire = gpGlobals->time + 1.0f;
	}
	pev->nextthink = gpGlobals->time + 0.1f;
}
