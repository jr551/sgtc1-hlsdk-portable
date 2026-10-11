// sgtc_wpn_coujaf.cpp — StargateTC CCoujaf (split from sgtc_weapons.cpp; NEW file).

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
// CCoujaf - "Coujaf" melee. Analogue: crowbar.cpp.
// Event: events/coujaf.sc.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_coujaf, CCoujaf );

void CCoujaf::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_coujaf.mdl" );
	m_iId = SGTC_WPN_COUJAF;
	m_iClip = -1;
	FallInit();
}

void CCoujaf::Precache( void )
{
	PRECACHE_MODEL( "models/v_coujaf.mdl" );
	PRECACHE_MODEL( "models/w_coujaf.mdl" );
	PRECACHE_MODEL( "models/p_coujaf.mdl" );
	PRECACHE_MODEL( "sprites/shockwave.spr" );

	PRECACHE_SOUND( "weapons/nightstrike.wav" );
	PRECACHE_SOUND( "weapons/coujafcorps.wav" );
	PRECACHE_SOUND( "weapons/hitbody1.wav" );
	PRECACHE_SOUND( "weapons/hitbody2.wav" );
	PRECACHE_SOUND( "weapons/hitbody3.wav" );
	PRECACHE_SOUND( "weapons/swing1.wav" );
	PRECACHE_SOUND( "weapons/swing2.wav" );
	PRECACHE_SOUND( "weapons/swing3.wav" );

	m_usFire = PRECACHE_EVENT( 1, "events/coujaf.sc" );
}

SGTC_GETITEM( CCoujaf, NULL, -1, WEAPON_NOCLIP, 0, 0, SGTC_WPN_COUJAF, 0, 0 )
SGTC_ADD_TO_PLAYER( CCoujaf )

BOOL CCoujaf::Deploy( void )
{
	return SGTC_DEPLOY( 2, "models/v_coujaf.mdl", "models/p_coujaf.mdl", SGTC_DRAW, "crowbar" );
}

void CCoujaf::PrimaryAttack( void )
{
	if( !Swing( 1 ) )
	{
#ifndef CLIENT_DLL
		SetThink( &CCoujaf::SwingAgain );
		pev->nextthink = gpGlobals->time + 0.1;
#endif
	}
}

void CCoujaf::SwingAgain( void )
{
	Swing( 0 );
}

int CCoujaf::Swing( int fFirst )
{
	int fDidHit = FALSE;
	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	if( fFirst )
	{
		PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire,
			0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );
	}

	if( tr.flFraction >= 1.0 )
	{
		if( fFirst )
		{
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.4 );
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if( pEntity )
		{
			ClearMultiDamage();
			pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgCrowbar,
				gpGlobals->v_forward, &tr, DMG_CLUB );
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
			EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/hitbody2.wav", 1, ATTN_NORM );
		}
#endif

		if( fFirst )
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.4 );
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	return fDidHit;
}

SGTC_NO_SECONDARY( CCoujaf )
SGTC_NO_RELOAD( CCoujaf )
SGTC_WEAPON_IDLE( CCoujaf )
