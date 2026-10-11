// sgtc_wpn_batsup.cpp — StargateTC CBatonSupplice (split from sgtc_weapons.cpp; NEW file).

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
// CBatonSupplice - "Baton de supplice" melee. Analogue: crowbar.cpp.
// Events: events/batsup.sc (first swing) / events/batsup2.sc (follow-up).
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_batsup, CBatonSupplice );

void CBatonSupplice::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_batsup.mdl" );
	m_iId = SGTC_WPN_BATSUP;
	m_iClip = -1;
	FallInit();
}

void CBatonSupplice::Precache( void )
{
	PRECACHE_MODEL( "models/v_batsup.mdl" );
	PRECACHE_MODEL( "models/w_batsup.mdl" );
	PRECACHE_MODEL( "models/p_batsup.mdl" );

	PRECACHE_SOUND( "weapons/coujafcorps.wav" );
	PRECACHE_SOUND( "weapons/hitbody1.wav" );
	PRECACHE_SOUND( "weapons/hitbody2.wav" );
	PRECACHE_SOUND( "weapons/hitbody3.wav" );
	PRECACHE_SOUND( "weapons/swing1.wav" );
	PRECACHE_SOUND( "weapons/swing2.wav" );
	PRECACHE_SOUND( "weapons/swing3.wav" );

	m_usFire = PRECACHE_EVENT( 1, "events/batsup.sc" );
	m_usFire2 = PRECACHE_EVENT( 1, "events/batsup2.sc" );
}

SGTC_GETITEM( CBatonSupplice, NULL, -1, WEAPON_NOCLIP, 0, 2, SGTC_WPN_BATSUP, 0, 0 )
SGTC_ADD_TO_PLAYER( CBatonSupplice )

BOOL CBatonSupplice::Deploy( void )
{
	return DefaultDeploy( "models/v_batsup.mdl", "models/p_batsup.mdl", SGTC_DRAW, "crowbar" );
}

void CBatonSupplice::Holster( int skiplocal )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( SGTC_HOLSTER );
}

void CBatonSupplice::PrimaryAttack( void )
{
	if( !Swing( 1 ) )
	{
#ifndef CLIENT_DLL
		SetThink( &CBatonSupplice::SwingAgain );
		pev->nextthink = gpGlobals->time + 0.1;
#endif
	}
}

void CBatonSupplice::SwingAgain( void )
{
	Swing( 0 );
}

int CBatonSupplice::Swing( int fFirst )
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
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
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

			if( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/hitbody1.wav", 1, ATTN_NORM );
			}
		}
#endif

		if( fFirst )
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
	}

	if( !fDidHit )
	{
		PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire2,
			0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	return fDidHit;
}

SGTC_NO_SECONDARY( CBatonSupplice )
SGTC_NO_RELOAD( CBatonSupplice )
SGTC_WEAPON_IDLE( CBatonSupplice )
