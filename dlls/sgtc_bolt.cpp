// sgtc_bolt.cpp — StargateTC CSGTCBolt (split from sgtc_weapons.cpp; NEW file).

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
// CSGTCBolt - local CGrenade-derived staff bolt.
// Spawned by CLanceJaffa / CBigLanceJaffa / CReetou. Map entities owned by
// the entities peer (lance_proj / big_lance_proj / reetou_proj) are NOT
// redeclared; this class keeps the bolt self-contained.
// ===========================================================================

void CSGTCBolt::Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->gravity = 0.0;

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CSGTCBolt::BoltTouch );
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 5.0;
}

void CSGTCBolt::Precache( void )
{
	PRECACHE_MODEL( "sprites/laserbeam.spr" );
}

CSGTCBolt *CSGTCBolt::CreateBolt( const Vector &vecOrigin, const Vector &vecAngles, CBaseEntity *pOwner, float flDamage, const char *pszModel )
{
	CSGTCBolt *pBolt = GetClassPtr( (CSGTCBolt *)NULL );

	pBolt->pev->origin = vecOrigin;
	pBolt->pev->angles = vecAngles;
	pBolt->pev->owner = pOwner ? pOwner->edict() : NULL;
	pBolt->m_flDamage = flDamage;

	pBolt->Spawn();
	SET_MODEL( ENT( pBolt->pev ), pszModel );

	UTIL_MakeVectors( vecAngles );
	pBolt->pev->velocity = gpGlobals->v_forward * 1200;
	pBolt->pev->angles = UTIL_VecToAngles( gpGlobals->v_forward );

	return pBolt;
}

void CSGTCBolt::BoltTouch( CBaseEntity *pOther )
{
	if( pOther->edict() == pev->owner )
		return;

	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );

	if( pOther->pev->takedamage )
	{
		pOther->TakeDamage( pev, pOwner ? pOwner->pev : pev, m_flDamage, DMG_BULLET );
	}

	UTIL_Remove( this );
}
