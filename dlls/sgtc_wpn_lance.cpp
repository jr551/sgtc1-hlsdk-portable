// sgtc_wpn_lance.cpp — StargateTC CLanceJaffa (split from sgtc_weapons.cpp; NEW file).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"
#include "sgtc_weapons.h"
#include "sgtc_lanceproj.h"

// ===========================================================================
// CLanceJaffa - "Lance Jaffa" staff weapon. Analogue: rpg.cpp (projectile).
// Events: events/lance.sc (bolt) / events/lance2.sc (melee bash).
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_lance, CLanceJaffa );

void CLanceJaffa::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_lancegoa.mdl" );
	m_iId = SGTC_WPN_LANCE;
	m_iDefaultAmmo = 22;
	FallInit();
}

void CLanceJaffa::Precache( void )
{
	PRECACHE_MODEL( "models/v_lancegoa.mdl" );
	PRECACHE_MODEL( "models/w_lancegoa.mdl" );
	PRECACHE_MODEL( "models/p_lancegoa.mdl" );
	PRECACHE_MODEL( "sprites/laserbeam.spr" );

	PRECACHE_SOUND( "weapons/tirlancegoa.wav" );
	PRECACHE_SOUND( "weapons/lancegoavide.wav" );
	PRECACHE_SOUND( "weapons/cbar_hit1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hit2.wav" );
	PRECACHE_SOUND( "weapons/hitbody1.wav" );
	PRECACHE_SOUND( "weapons/hitbody2.wav" );
	PRECACHE_SOUND( "weapons/hitbody3.wav" );
	PRECACHE_SOUND( "weapons/swing1.wav" );
	PRECACHE_SOUND( "weapons/swing2.wav" );
	PRECACHE_SOUND( "weapons/swing3.wav" );

	m_usFire = PRECACHE_EVENT( 1, "events/lance.sc" );
	m_usFire2 = PRECACHE_EVENT( 1, "events/lance2.sc" );
}

SGTC_GETITEM( CLanceJaffa, SGTC_AMMO_ENERGIE3, 22, WEAPON_NOCLIP, 3, 2, SGTC_WPN_LANCE, 5, 15 )
SGTC_ADD_TO_PLAYER( CLanceJaffa )

BOOL CLanceJaffa::Deploy( void )
{
	return SGTC_DEPLOY( 1, "models/v_lancegoa.mdl", "models/p_lancegoa.mdl", SGTC_DRAW, "egon" );
}

void CLanceJaffa::PrimaryAttack( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16;
	Vector vecDir = gpGlobals->v_forward;

#ifndef CLIENT_DLL
	// ref CLanceJaffa::PrimaryAttack @0x1005d6: CreateLanceProj( vecSrc,
	//   m_pPlayer->pev->velocity, tr.vecEndPos, m_pPlayer, this ).
	{
		TraceResult tr;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir * 8192.0f, dont_ignore_monsters,
			m_pPlayer->edict(), &tr );
		CLanceProj::CreateLanceProj( vecSrc, m_pPlayer->pev->velocity,
			tr.vecEndPos, m_pPlayer, this );
	}
#endif

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y,
		0, 0, 0, 0 );

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 1.0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CLanceJaffa::SecondaryAttack( void )	// ref @0x10ffd2
{
	// ref gate: m_pPlayer->m_flNextAttack <= time.
	if( m_pPlayer->m_flNextAttack > gpGlobals->time )
		return;

	// ref: if( Swing(1) || WTB <= m_flNextPrimaryAttack ) return;
	// else SwingAgain(); pev->nextthink = time + 0.1; SnagInvis().
	if( Swing( 1 ) || UTIL_WeaponTimeBase() <= m_flNextPrimaryAttack )
		return;

	SwingAgain();
	pev->nextthink = gpGlobals->time + 0.1f;
	m_pPlayer->SnagInvis();
}

void CLanceJaffa::SwingAgain( void )	// ref @0x10ffaa
{
	Swing( 0 );
}

int CLanceJaffa::Swing( int fFirst )	// ref @0x10f5d0 (crowbar-derived melee)
{
	// ref FindHullIntersection(vecSrc, tr, mins, maxs, edict) — local copy
	// (crowbar.cpp's is TU-local); projects the hull hit onto the BSP face.
	static const auto sgtcHullIntersect = []( const Vector &vecSrc, TraceResult &tr,
		const float *mins, const float *maxs, edict_t *pEntity )
	{
		TraceResult tmpTrace;
		Vector vecHullEnd = tr.vecEndPos;
		Vector vecEnd = vecSrc + ( vecHullEnd - vecSrc ) * 2.0f;
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace );
		if( tmpTrace.flFraction < 1.0f )
			tr = tmpTrace;
	};

	int fDidHit = FALSE;
	TraceResult tr;
	float flVol = 1.0f;
	int fHitWorld = TRUE;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	SendWeaponAnim( SGTC_FIRE2, 1, 0 );

	if( tr.flFraction >= 1.0f )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull,
			ENT( m_pPlayer->pev ), &tr );
		if( tr.flFraction < 1.0f )
		{
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if( !pHit || pHit->IsBSPModel() )
				sgtcHullIntersect( vecSrc, tr, (float *)VEC_DUCK_HULL_MIN,
					(float *)VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;
		}
	}
	if( fFirst )
	{
		PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire2,
			0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );
	}

	if( tr.flFraction >= 1.0f )
	{
		if( fFirst )
		{
			// miss
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
			m_trLastHit = tr;
		}
	}
	else
	{
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL
		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );


		if( pEntity )
		{
			ClearMultiDamage();
			// ref: talent-boosted damage 70 (0x46) when Goa'uld talent-4
			// active, else the skill-table crowbar damage.
			float flDamage = gSkillData.plrDmgCrowbar * 2;
			if( gpGlobals->time < m_pPlayer->m_flTalentCooldown &&
				m_pPlayer->pev->team == 2 &&
				m_pPlayer->m_iSgtcPortraitShown == 4 )
				flDamage = 70.0f;
			// ref: direct entity hit only when primary reuse expired or MP.
			if( ( m_flNextPrimaryAttack + 1.0f < UTIL_WeaponTimeBase() ) ||
				g_pGameRules->IsMultiplayer() )
			{
				pEntity->TraceAttack( m_pPlayer->pev, flDamage,
					gpGlobals->v_forward, &tr, DMG_CLUB );
			}
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

			// ref: body-hit sounds + return 1 on live targets.
			if( pEntity->Classify() != CLASS_NONE &&
				pEntity->Classify() != CLASS_MACHINE )
			{
				switch( RANDOM_LONG( 0, 2 ) )
				{
				case 0: EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON,
					"weapons/hitbody1.wav", 1, ATTN_NORM ); break;
				case 1: EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON,
					"weapons/hitbody2.wav", 1, ATTN_NORM ); break;
				case 2: EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON,
					"weapons/hitbody3.wav", 1, ATTN_NORM ); break;
				}
				m_pPlayer->m_iWeaponVolume = 128;
				if( !pEntity->IsAlive() )
					return TRUE;
				flVol = 0.1f;
				fHitWorld = FALSE;
			}
		}
#endif
		// ref: wall-hit texture sound + stored TraceResult.
		TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, BULLET_PLAYER_CROWBAR );
		if( g_pGameRules && g_pGameRules->IsMultiplayer() )
			flVol = 1.0f;
		switch( RANDOM_LONG( 0, 1 ) )
		{
		case 0:
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON,
				"weapons/cbar_hit1.wav", flVol, ATTN_NORM, 0,
				98 + RANDOM_LONG( 0, 3 ) );
			break;
		default:
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON,
				"weapons/cbar_hit2.wav", flVol, ATTN_NORM, 0,
				98 + RANDOM_LONG( 0, 3 ) );
			break;
		}
		m_trLastHit = tr;
	}
	m_pPlayer->m_iWeaponVolume = (int)( flVol * 512.0f );
	m_flNextPrimaryAttack = GetNextAttackDelay( 0.25 );
	if( gpGlobals->time < m_pPlayer->m_flTalentCooldown &&
		m_pPlayer->pev->team == 2 &&
		m_pPlayer->m_iSgtcPortraitShown == 4 )
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.15 );
	// ref Smack(): swing whiff sound would use m_trLastHit; the port plays
	// the event whiff above — keep event only.
	pev->nextthink = UTIL_WeaponTimeBase() + 0.2f;

	return fDidHit;
}

SGTC_NO_RELOAD( CLanceJaffa )
SGTC_WEAPON_IDLE( CLanceJaffa )
