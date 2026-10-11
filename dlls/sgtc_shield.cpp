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

	pev->armorvalue = 130.0f;	// ref pev+0x14c @0x1d9e00
	pev->team = 5;			// ref pev+0x148

	m_flExpire = gpGlobals->time + 1.0f;	// ref +0x19c @0x1d9e04

	SetThink( &CShield::Think );
	pev->nextthink = gpGlobals->time + 0.1f;	// ref double 0.1 @0x1d9dd8
}

void CShield::Think( void )
{
	// ref @0x162234 — stepped armor from renderamt thresholds 100 / 130.
	if( pev->renderamt == 100.0f )
		pev->armorvalue = 130.0f;
	else if( pev->renderamt == 130.0f )
		pev->armorvalue = 80.0f;

	if( !pev->owner )
	{
		UTIL_Remove( this );
		return;
	}

	if( gpGlobals->time < m_flExpire )
	{
		pev->nextthink = gpGlobals->time + 0.1f;
		return;
	}

	BOOL bOwnerAlive = FALSE;
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		edict_t *pent = INDEXENT( i );
		if( !pent || pent->free )
			continue;
		if( pent == pev->owner )
		{
			bOwnerAlive = TRUE;
			break;
		}
	}

	if( !bOwnerAlive )
	{
		UTIL_Remove( this );
		return;
	}

	m_flExpire = gpGlobals->time + 1.4499999f;	// ref @0x1d9ddc
	pev->nextthink = gpGlobals->time + 0.1f;
}
