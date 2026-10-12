// hlsdk-xash3d/dlls/sgtc_monsters.cpp
//
// StargateTC custom server monsters — see sgtc_monsters.h for the
// factory list and disassembly references. All behavior below is a clean-room
// re-implementation of the decoded think/touch shapes using stock SDK calls.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "player.h"
#include "weapons.h"
#include "soundent.h"
#include "skill.h"
#include "sgtc_monsters.h"

#ifndef CLIENT_DLL

#define SGTC_REPLIC_DETONATE_DELAY 15.0f
#define SGTC_LARVE_DETONATE_DELAY 15.0f
#define SGTC_REPLIC_SPEED 250.0f
#define SGTC_REPLIC_HOP 120.0f
#define SGTC_REPLICATEUR_SPEED 50.0f // decoded rodata float @ 0x1d9cc0
#define SGTC_TAC_RANGE 1024.0f
#define SGTC_TAC_REVEAL_RANGE 256.0f
#define SGTC_MINE_TRIP_RANGE 160.0f

// =============================================================
// CSGTCReplic — monster_replic (models/replic.mdl)
// Snark-hunt clone of CReplicMonster::HuntThink/SuperBounceTouch.
// =============================================================

TYPEDESCRIPTION CSGTCReplic::m_SaveData[] =
{
	DEFINE_FIELD( CSGTCReplic, m_flDie, FIELD_TIME ),
	DEFINE_FIELD( CSGTCReplic, m_flNextHunt, FIELD_TIME ),
	DEFINE_FIELD( CSGTCReplic, m_flNextSound, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CSGTCReplic, CGrenade );
LINK_ENTITY_TO_CLASS( monster_replic, CSGTCReplic );

void CSGTCReplic::Precache( void )
{
	PRECACHE_MODEL( "models/replic.mdl" );
	PRECACHE_SOUND( "squeek/sqk_hunt2.wav" );
	PRECACHE_SOUND( "squeek/sqk_deploy1.wav" );
}

void CSGTCReplic::Spawn( void )
{
	Precache();

	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/replic.mdl" );
	UTIL_SetSize( pev, Vector( -4, -4, 0 ), Vector( 4, 4, 8 ) );
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CSGTCReplic::SuperBounceTouch );
	SetThink( &CSGTCReplic::HuntThink );
	pev->nextthink = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time + 1E6;
	m_flNextSound = gpGlobals->time;

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	pev->health = gSkillData.snarkHealth;
	pev->gravity = 0.5;
	pev->friction = 0.5;
	pev->dmg = gSkillData.snarkDmgPop;

	m_flDie = gpGlobals->time + SGTC_REPLIC_DETONATE_DELAY;
	m_flFieldOfView = 0; // 180 degrees, like the snark

	pev->sequence = 0;
	ResetSequenceInfo();
}

void CSGTCReplic::HuntThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( gpGlobals->time >= m_flDie )
	{
		Detonate();
		return;
	}

	if ( m_flNextHunt <= gpGlobals->time )
	{
		m_hEnemy = BestVisibleEnemy();
		m_flNextHunt = gpGlobals->time + 0.5;
	}

	if ( m_hEnemy == NULL )
	{
		// Idle hop so the block pile looks alive.
		if ( pev->flags & FL_ONGROUND )
			pev->velocity = Vector( RANDOM_FLOAT( -80, 80 ), RANDOM_FLOAT( -80, 80 ), 150 );
		return;
	}

	Vector vecDelta = m_hEnemy->pev->origin - pev->origin;
	float flLen = vecDelta.Length();
	if ( flLen > 1.0f )
		vecDelta = vecDelta * ( 1.0f / flLen );

	pev->velocity.x = vecDelta.x * SGTC_REPLIC_SPEED;
	pev->velocity.y = vecDelta.y * SGTC_REPLIC_SPEED;
	if ( pev->flags & FL_ONGROUND )
		pev->velocity.z = SGTC_REPLIC_HOP;

	if ( m_flNextSound <= gpGlobals->time )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "squeek/sqk_hunt2.wav", 1.0, ATTN_NORM, 0, (int)RANDOM_FLOAT( 95, 110 ) );
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 256, 0.25 );
		m_flNextSound = gpGlobals->time + 1.0;
	}
}

void CSGTCReplic::SuperBounceTouch( CBaseEntity *pOther )
{
	if ( pOther->pev->takedamage && pOther != this )
	{
		pOther->TakeDamage( pev, pev, gSkillData.snarkDmgBite, DMG_SLASH );
		pev->velocity = pev->velocity * 0.5 + Vector( 0, 0, 100 );
	}

	if ( m_flNextSound <= gpGlobals->time )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "squeek/sqk_hunt2.wav", 1.0, ATTN_NORM, 0, (int)RANDOM_FLOAT( 95, 110 ) );
		m_flNextSound = gpGlobals->time + 0.5;
	}
}

void CSGTCReplic::Detonate( void )
{
	if ( m_hEnemy != NULL )
	{
		Vector vecDelta = m_hEnemy->pev->origin - pev->origin;
		if ( vecDelta.Length() < 128.0f )
			m_hEnemy->TakeDamage( pev, pev, pev->dmg, DMG_BLAST );
	}
	EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "squeek/sqk_deploy1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	SetTouch( NULL );
	SetThink( NULL );
	UTIL_Remove( this );
}

// =============================================================
// CSGTCLarve — monster_larve (models/w_larve.mdl)
// Snark-hunt clone of CLarveMonster::HuntThink/SuperBounceTouch.
// =============================================================

TYPEDESCRIPTION CSGTCLarve::m_SaveData[] =
{
	DEFINE_FIELD( CSGTCLarve, m_flDie, FIELD_TIME ),
	DEFINE_FIELD( CSGTCLarve, m_flNextHunt, FIELD_TIME ),
	DEFINE_FIELD( CSGTCLarve, m_flNextSound, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CSGTCLarve, CGrenade );
LINK_ENTITY_TO_CLASS( monster_larve, CSGTCLarve );

void CSGTCLarve::Precache( void )
{
	PRECACHE_MODEL( "models/w_larve.mdl" );
	PRECACHE_SOUND( "weapons/larve1.wav" );
	PRECACHE_SOUND( "weapons/larve2.wav" );
	PRECACHE_SOUND( "squeek/sqk_deploy1.wav" );
}

void CSGTCLarve::Spawn( void )
{
	Precache();

	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/w_larve.mdl" );
	UTIL_SetSize( pev, Vector( -8, -8, 0 ), Vector( 8, 8, 12 ) );
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CSGTCLarve::SuperBounceTouch );
	SetThink( &CSGTCLarve::HuntThink );
	pev->nextthink = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time + 1E6;
	m_flNextSound = gpGlobals->time;

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	pev->health = gSkillData.snarkHealth;
	pev->gravity = 0.5;
	pev->friction = 0.5;
	pev->dmg = gSkillData.snarkDmgPop;

	m_flDie = gpGlobals->time + SGTC_LARVE_DETONATE_DELAY;
	m_flFieldOfView = 0;

	pev->sequence = 0;
	ResetSequenceInfo();
}

void CSGTCLarve::HuntThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( gpGlobals->time >= m_flDie )
	{
		Detonate();
		return;
	}

	if ( m_flNextHunt <= gpGlobals->time )
	{
		m_hEnemy = BestVisibleEnemy();
		m_flNextHunt = gpGlobals->time + 0.5;
	}

	if ( m_hEnemy == NULL )
	{
		if ( pev->flags & FL_ONGROUND )
			pev->velocity = Vector( RANDOM_FLOAT( -60, 60 ), RANDOM_FLOAT( -60, 60 ), 130 );
		return;
	}

	Vector vecDelta = m_hEnemy->pev->origin - pev->origin;
	float flLen = vecDelta.Length();
	if ( flLen > 1.0f )
		vecDelta = vecDelta * ( 1.0f / flLen );

	pev->velocity.x = vecDelta.x * SGTC_REPLIC_SPEED;
	pev->velocity.y = vecDelta.y * SGTC_REPLIC_SPEED;
	if ( pev->flags & FL_ONGROUND )
		pev->velocity.z = SGTC_REPLIC_HOP;

	if ( m_flNextSound <= gpGlobals->time )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "weapons/larve1.wav", 1.0, ATTN_NORM, 0, (int)RANDOM_FLOAT( 95, 105 ) );
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 256, 0.25 );
		m_flNextSound = gpGlobals->time + 1.0;
	}
}

void CSGTCLarve::SuperBounceTouch( CBaseEntity *pOther )
{
	if ( pOther->pev->takedamage && pOther != this )
	{
		pOther->TakeDamage( pev, pev, gSkillData.snarkDmgBite, DMG_SLASH );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "weapons/larve2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
		pev->velocity = pev->velocity * 0.5 + Vector( 0, 0, 100 );
	}
}

void CSGTCLarve::Detonate( void )
{
	if ( m_hEnemy != NULL )
	{
		Vector vecDelta = m_hEnemy->pev->origin - pev->origin;
		if ( vecDelta.Length() < 128.0f )
			m_hEnemy->TakeDamage( pev, pev, pev->dmg, DMG_BLAST );
	}
	EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "weapons/larve1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	SetTouch( NULL );
	SetThink( NULL );
	UTIL_Remove( this );
}

// =============================================================
// CSGTCReplicateur — monster_replicateur + monster_replicator
// Headcrab-like hunter driven by ReplicateurThink (0x161be4).
// =============================================================

LINK_ENTITY_TO_CLASS( monster_replicateur, CSGTCReplicateur );
LINK_ENTITY_TO_CLASS( monster_replicator, CSGTCReplicateur );

void CSGTCReplicateur::Precache( void )
{
	PRECACHE_MODEL( "models/headcrab.mdl" );
	PRECACHE_SOUND( "headcrab/hc_alert1.wav" );
	PRECACHE_SOUND( "headcrab/hc_attack1.wav" );
	PRECACHE_SOUND( "headcrab/hc_pain1.wav" );
}

void CSGTCReplicateur::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/headcrab.mdl" );
	UTIL_SetSize( pev, Vector( -12, -12, 0 ), Vector( 12, 12, 24 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_YELLOW;
	pev->health = gSkillData.headcrabHealth;
	// monster_replicator is the heavy variant of the same AI.
	if ( FClassnameIs( pev, "monster_replicator" ) )
		pev->health = gSkillData.headcrabHealth * 2.0f;
	pev->view_ofs = Vector( 0, 0, 20 );
	m_flFieldOfView = 0.5f;
	m_MonsterState = MONSTERSTATE_NONE;
	pev->takedamage = DAMAGE_AIM;

	pev->sequence = 0;
	pev->frame = 0;
	pev->framerate = 1.0;

	SetThink( &CSGTCReplicateur::ReplicateurThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

Vector CSGTCReplicateur::Center( void )
{
	// Decoded Center() adds +6 z over the origin.
	return pev->origin + Vector( 0, 0, 6 );
}

Vector CSGTCReplicateur::BodyTarget( const Vector &posSrc )
{
	return Center();
}

void CSGTCReplicateur::ReplicateurThink( void )
{
	// Decoded repath interval @ 0x1d9cc8.
	pev->nextthink = gpGlobals->time + 0.1;

	m_hEnemy = BestVisibleEnemy();

	CBaseEntity *pGoal = NULL;
	if ( m_hEnemy != NULL )
		pGoal = m_hEnemy;
	else
		pGoal = UTIL_FindEntityByClassname( NULL, "info_player_deathmatch" );

	if ( pGoal == NULL )
		return;

	Vector vecDelta = pGoal->pev->origin - pev->origin;
	vecDelta.z = 0;
	float flLen = vecDelta.Length();

	if ( flLen > 1.0f )
	{
		vecDelta = vecDelta * ( 1.0f / flLen );
		pev->velocity.x = vecDelta.x * SGTC_REPLICATEUR_SPEED;
		pev->velocity.y = vecDelta.y * SGTC_REPLICATEUR_SPEED;
		if ( ( pev->flags & FL_ONGROUND ) && m_hEnemy != NULL )
			pev->velocity.z = 180.0f; // hop at prey, headcrab-style
	}

	// Melee bite once in range.
	if ( m_hEnemy != NULL && flLen < 72.0f )
	{
		m_hEnemy->TakeDamage( pev, pev, gSkillData.headcrabDmgBite, DMG_SLASH );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "headcrab/hc_attack1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
		pev->nextthink = gpGlobals->time + 0.5; // bite recovery
	}
}

// =============================================================
// CSGTCImplanted — monster_implanted (CZombie alias)
// Zombie bruiser in a commander skin (0x1b7ffe).
// =============================================================

LINK_ENTITY_TO_CLASS( monster_implanted, CSGTCImplanted );

void CSGTCImplanted::Precache( void )
{
	PRECACHE_MODEL( "models/player/commander/commander.mdl" );
	PRECACHE_SOUND( "zombie/zo_attack1.wav" );
	PRECACHE_SOUND( "zombie/zo_alert10.wav" );
	PRECACHE_SOUND( "zombie/zo_pain1.wav" );
	PRECACHE_SOUND( "zombie/zo_idle1.wav" );
	PRECACHE_SOUND( "zombie/claw_strike1.wav" );
	PRECACHE_SOUND( "zombie/claw_miss1.wav" );
}

void CSGTCImplanted::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/player/commander/commander.mdl" );
	UTIL_SetSize( pev, Vector( -13, -13, 0 ), Vector( 13, 13, 72 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.zombieHealth;
	pev->view_ofs = Vector( 0, 0, 60 );
	m_flFieldOfView = 0.5f;
	m_MonsterState = MONSTERSTATE_NONE;
	pev->takedamage = DAMAGE_AIM;

	pev->sequence = 0;
	pev->frame = 0;
	pev->framerate = 1.0;

	SetThink( &CSGTCImplanted::ImplantedThink );
	pev->nextthink = gpGlobals->time + 0.5;
	AlertSound();
}

void CSGTCImplanted::PainSound( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "zombie/zo_pain1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
}

void CSGTCImplanted::AlertSound( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "zombie/zo_alert10.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
}

void CSGTCImplanted::ImplantedThink( void )
{
	pev->nextthink = gpGlobals->time + 0.5;

	m_hEnemy = BestVisibleEnemy();
	if ( m_hEnemy == NULL )
		return;

	Vector vecDelta = m_hEnemy->pev->origin - pev->origin;
	vecDelta.z = 0;
	float flLen = vecDelta.Length();

	if ( flLen < 80.0f )
	{
		// Slash, zombie-style.
		m_hEnemy->TakeDamage( pev, pev, gSkillData.zombieDmgOneSlash, DMG_SLASH );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "zombie/claw_strike1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
		pev->velocity.x = 0;
		pev->velocity.y = 0;
	}
	else
	{
		if ( flLen > 1.0f )
			vecDelta = vecDelta * ( 1.0f / flLen );
		pev->velocity.x = vecDelta.x * 120.0f;
		pev->velocity.y = vecDelta.y * 120.0f;
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "zombie/zo_attack1.wav", 0.8, ATTN_IDLE, 0, PITCH_NORM );
	}
}

// =============================================================
// CSGTCVortigaunt — monster_vortigaunt (CISlave alias)
// Slave-style zapper (same dual-factory alias pattern as stock,
// which links monster_vortigaunt to CISlave).
// =============================================================

TYPEDESCRIPTION CSGTCVortigaunt::m_SaveData[] =
{
	DEFINE_FIELD( CSGTCVortigaunt, m_flNextZap, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CSGTCVortigaunt, CBaseMonster );
// monster_vortigaunt factory stays registered to stock CISlave (islave.cpp)
// — the .so aliases the name to the islave class; CSGTCVortigaunt is kept
// for direct use but must not re-link the same map name.
//LINK_ENTITY_TO_CLASS( monster_vortigaunt, CSGTCVortigaunt );

void CSGTCVortigaunt::Precache( void )
{
	PRECACHE_MODEL( "models/islave.mdl" );
	PRECACHE_SOUND( "islave/slv_alert1.wav" );
	PRECACHE_SOUND( "islave/slv_attack1.wav" );
	PRECACHE_SOUND( "islave/slv_pain1.wav" );
}

void CSGTCVortigaunt::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/islave.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 64 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.slaveHealth;
	pev->view_ofs = Vector( 0, 0, 64 );
	m_flFieldOfView = 0.5f;
	m_MonsterState = MONSTERSTATE_NONE;
	pev->takedamage = DAMAGE_AIM;

	pev->sequence = 0;
	pev->frame = 0;
	pev->framerate = 1.0;

	m_flNextZap = gpGlobals->time;
	SetThink( &CSGTCVortigaunt::VortigauntThink );
	pev->nextthink = gpGlobals->time + 0.5;
}

void CSGTCVortigaunt::VortigauntThink( void )
{
	pev->nextthink = gpGlobals->time + 0.5;

	m_hEnemy = BestVisibleEnemy();
	if ( m_hEnemy == NULL )
		return;

	Vector vecDelta = m_hEnemy->pev->origin - pev->origin;
	float flLen = vecDelta.Length();

	if ( flLen < 512.0f && m_flNextZap <= gpGlobals->time )
	{
		// Ranged zap, slave-style.
		m_hEnemy->TakeDamage( pev, pev, gSkillData.slaveDmgZap, DMG_SHOCK );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "islave/slv_attack1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
		m_flNextZap = gpGlobals->time + 2.0;
	}
	else if ( flLen >= 80.0f )
	{
		vecDelta.z = 0;
		if ( flLen > 1.0f )
			vecDelta = vecDelta * ( 1.0f / flLen );
		pev->velocity.x = vecDelta.x * 120.0f;
		pev->velocity.y = vecDelta.y * 120.0f;
	}
	else
	{
		m_hEnemy->TakeDamage( pev, pev, gSkillData.slaveDmgClaw, DMG_SLASH );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "islave/slv_attack1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	}
}

// =============================================================
// CSGTCSkull — monster_skull (CRat, models/crane_rose.mdl)
// =============================================================

LINK_ENTITY_TO_CLASS( monster_skull, CSGTCSkull );

void CSGTCSkull::Precache( void )
{
	PRECACHE_MODEL( "models/crane_rose.mdl" );
	PRECACHE_SOUND( "misc/skull_attack1.wav" );
	PRECACHE_SOUND( "misc/skull_attack2.wav" );
	PRECACHE_SOUND( "misc/skull_idle1.wav" );
	PRECACHE_SOUND( "misc/skull_idle2.wav" );
	PRECACHE_SOUND( "misc/skull_idle3.wav" );
	PRECACHE_MODEL( "sprites/nargas.spr" );
}

void CSGTCSkull::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/crane_rose.mdl" );
	UTIL_SetSize( pev, Vector( -8, -8, 0 ), Vector( 8, 8, 16 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 8;
	pev->view_ofs = Vector( 0, 0, 6 );
	m_flFieldOfView = 0.5f;
	m_MonsterState = MONSTERSTATE_NONE;
	pev->takedamage = DAMAGE_AIM;

	pev->sequence = 0;
	pev->frame = 0;
	pev->framerate = 1.0;

	m_flNextSound = gpGlobals->time;
	SetTouch( &CSGTCSkull::SkullTouch );
	SetThink( &CSGTCSkull::ScuttleThink );
	pev->nextthink = gpGlobals->time + 0.2;
}

void CSGTCSkull::ScuttleThink( void )
{
	pev->nextthink = gpGlobals->time + 0.3;

	m_hEnemy = BestVisibleEnemy();

	if ( m_hEnemy != NULL )
	{
		// Flee the enemy, rat-style.
		Vector vecDelta = pev->origin - m_hEnemy->pev->origin;
		vecDelta.z = 0;
		float flLen = vecDelta.Length();
		if ( flLen > 1.0f )
			vecDelta = vecDelta * ( 1.0f / flLen );
		pev->velocity.x = vecDelta.x * 150.0f;
		pev->velocity.y = vecDelta.y * 150.0f;
		if ( m_flNextSound <= gpGlobals->time )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "misc/skull_attack1.wav", 1.0, ATTN_NORM, 0, (int)RANDOM_FLOAT( 95, 110 ) );
			m_flNextSound = gpGlobals->time + 1.0;
		}
	}
	else
	{
		// Wander.
		if ( pev->flags & FL_ONGROUND )
			pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), 60 );
		if ( m_flNextSound <= gpGlobals->time )
		{
			int iIdle = RANDOM_LONG( 0, 2 );
			const char *pszIdle = "misc/skull_idle1.wav";
			if ( iIdle == 1 )
				pszIdle = "misc/skull_idle2.wav";
			else if ( iIdle == 2 )
				pszIdle = "misc/skull_idle3.wav";
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pszIdle, 0.8, ATTN_IDLE, 0, PITCH_NORM );
			m_flNextSound = gpGlobals->time + RANDOM_FLOAT( 2.0, 4.0 );
		}
	}
}

void CSGTCSkull::SkullTouch( CBaseEntity *pOther )
{
	if ( pOther->pev->takedamage && pOther != this && m_hEnemy != NULL && pOther == (CBaseEntity *)m_hEnemy )
	{
		pOther->TakeDamage( pev, pev, 5.0f, DMG_SLASH );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "misc/skull_attack2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	}
}

// =============================================================
// CSGTCMine — monster_mine (models/w_mine.mdl)
// Tripmine-style proximity trap on a CGrenade base.
// =============================================================

TYPEDESCRIPTION CSGTCMine::m_SaveData[] =
{
	DEFINE_FIELD( CSGTCMine, m_flPowerUp, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CSGTCMine, CGrenade );
LINK_ENTITY_TO_CLASS( monster_mine, CSGTCMine );

void CSGTCMine::Precache( void )
{
	PRECACHE_MODEL( "models/w_mine.mdl" );
	PRECACHE_SOUND( "weapons/place.wav" );
	PRECACHE_SOUND( "weapons/c4_beep.wav" );
}

void CSGTCMine::Spawn( void )
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;
	SET_MODEL( ENT( pev ), "models/w_mine.mdl" );
	pev->frame = 0;
	pev->sequence = 0;
	UTIL_SetSize( pev, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ) );
	UTIL_SetOrigin( pev, pev->origin );

	if ( pev->spawnflags & 1 )
		m_flPowerUp = gpGlobals->time + 1.0;
	else
		m_flPowerUp = gpGlobals->time + 2.5;

	pev->takedamage = DAMAGE_YES;
	pev->health = 10;

	SetThink( &CSGTCMine::PowerupThink );
	pev->nextthink = gpGlobals->time + 0.2;
}

int CSGTCMine::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Shooting the mine sets it off.
	Detonate( NULL );
	return 1;
}

void CSGTCMine::PowerupThink( void )
{
	if ( gpGlobals->time >= m_flPowerUp )
	{
		pev->solid = SOLID_BBOX;
		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "weapons/place.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
		SetThink( &CSGTCMine::ActiveThink );
		pev->nextthink = gpGlobals->time + 0.3;
		return;
	}
	pev->nextthink = gpGlobals->time + 0.2;
}

void CSGTCMine::ActiveThink( void )
{
	pev->nextthink = gpGlobals->time + 0.3;

	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = UTIL_FindEntityByClassname( pEnt, "player" ) ) != NULL )
	{
		if ( pEnt->pev->takedamage == DAMAGE_NO || pEnt->pev->deadflag != DEAD_NO )
			continue;
		Vector vecDelta = pEnt->pev->origin - pev->origin;
		if ( vecDelta.Length() < SGTC_MINE_TRIP_RANGE )
		{
			Detonate( pEnt );
			return;
		}
	}
}

void CSGTCMine::Detonate( CBaseEntity *pVictim )
{
	if ( pVictim != NULL )
		pVictim->TakeDamage( pev, pev, gSkillData.plrDmgTripmine, DMG_BLAST );
	else
	{
		// Shot destruction: hurt whoever is closest.
		CBaseEntity *pEnt = NULL;
		while ( ( pEnt = UTIL_FindEntityInSphere( pEnt, pev->origin, SGTC_MINE_TRIP_RANGE ) ) != NULL )
		{
			if ( pEnt != this && pEnt->pev->takedamage != DAMAGE_NO )
				pEnt->TakeDamage( pev, pev, gSkillData.plrDmgTripmine, DMG_BLAST );
		}
	}
	EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "weapons/c4_beep.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	SetThink( NULL );
	UTIL_Remove( this );
}

// =============================================================
// CSGTCTac — monster_tac (models/w_tacb.mdl deployed)
// Static sentry with tac sound set + anti-invis sweep.
// =============================================================

TYPEDESCRIPTION CSGTCTac::m_SaveData[] =
{
	DEFINE_FIELD( CSGTCTac, m_flNextFire, FIELD_TIME ),
	DEFINE_FIELD( CSGTCTac, m_flNextSweep, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CSGTCTac, CBaseMonster );
LINK_ENTITY_TO_CLASS( monster_tac, CSGTCTac );

void CSGTCTac::Precache( void )
{
	PRECACHE_MODEL( "models/w_tac.mdl" );
	PRECACHE_MODEL( "models/w_tacb.mdl" );
	PRECACHE_MODEL( "models/w_tacs.mdl" );
	PRECACHE_MODEL( "models/w_tacp.mdl" );
	PRECACHE_SOUND( "weapons/tac.wav" );
	PRECACHE_SOUND( "weapons/tacs.wav" );
	PRECACHE_SOUND( "weapons/tacp.wav" );
	PRECACHE_SOUND( "weapons/tacact.wav" );
	PRECACHE_SOUND( "weapons/place.wav" );
	PRECACHE_SOUND( "weapons/invis.wav" );
	PRECACHE_MODEL( "sprites/gammawave.spr" );
	PRECACHE_EVENT( 1, "events/tac.sc" );
}

void CSGTCTac::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/w_tacb.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 48 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_AIM;
	pev->health = gSkillData.sentryHealth;
	pev->view_ofs = Vector( 0, 0, 32 );
	m_flFieldOfView = -1.0f; // turret sees all around
	m_MonsterState = MONSTERSTATE_NONE;
	m_bloodColor = DONT_BLEED;

	pev->sequence = 0;
	pev->frame = 0;
	pev->framerate = 1.0;

	m_flNextFire = gpGlobals->time;
	m_flNextSweep = gpGlobals->time;

	EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "weapons/place.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );

	SetThink( &CSGTCTac::TacThink );
	pev->nextthink = gpGlobals->time + 0.3;
}

void CSGTCTac::Smoker( void )
{
	// Firing puff, tac-style.
	EMIT_SOUND_DYN( ENT( pev ), CHAN_ITEM, "weapons/tacp.wav", 0.7, ATTN_NORM, 0, PITCH_NORM );
}

void CSGTCTac::TacThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	// Decloak sweep after CReetou::SearchInvisible: force dit
	// visible any cloaked player-scale target nearby so the tac
	// (and its team) can target reetou/invis users again.
	if ( m_flNextSweep <= gpGlobals->time )
	{
		CBaseEntity *pEnt = NULL;
		while ( ( pEnt = UTIL_FindEntityInSphere( pEnt, pev->origin, SGTC_TAC_REVEAL_RANGE ) ) != NULL )
		{
			if ( pEnt != this && pEnt->pev->renderamt < 200 && pEnt->pev->takedamage != DAMAGE_NO )
			{
				pEnt->pev->renderamt = 255;
				pEnt->pev->renderfx = kRenderNormal;
			}
		}
		m_flNextSweep = gpGlobals->time + 1.0;
	}

	m_hEnemy = BestVisibleEnemy();
	if ( m_hEnemy == NULL )
		return;

	Vector vecDelta = m_hEnemy->pev->origin - pev->origin;
	if ( vecDelta.Length() > SGTC_TAC_RANGE )
		return;

	if ( m_flNextFire <= gpGlobals->time )
	{
		m_hEnemy->TakeDamage( pev, pev, gSkillData.monDmgMP5, DMG_BULLET );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "weapons/tac.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
		Smoker();
		m_flNextFire = gpGlobals->time + 0.5;
	}
}

// =============================================================
// CSGTCBodyPart — monster_arm/head/chest/leg/stomach 1-3
// Tier-scaled damage dummies (sk_monster_* cvars).
// =============================================================

LINK_ENTITY_TO_CLASS( monster_arm1, CSGTCBodyPart );
LINK_ENTITY_TO_CLASS( monster_arm2, CSGTCBodyPart );
LINK_ENTITY_TO_CLASS( monster_arm3, CSGTCBodyPart );
LINK_ENTITY_TO_CLASS( monster_head1, CSGTCBodyPart );
LINK_ENTITY_TO_CLASS( monster_head2, CSGTCBodyPart );
LINK_ENTITY_TO_CLASS( monster_head3, CSGTCBodyPart );
LINK_ENTITY_TO_CLASS( monster_chest1, CSGTCBodyPart );
LINK_ENTITY_TO_CLASS( monster_chest2, CSGTCBodyPart );
LINK_ENTITY_TO_CLASS( monster_chest3, CSGTCBodyPart );
LINK_ENTITY_TO_CLASS( monster_leg1, CSGTCBodyPart );
LINK_ENTITY_TO_CLASS( monster_leg2, CSGTCBodyPart );
LINK_ENTITY_TO_CLASS( monster_leg3, CSGTCBodyPart );
LINK_ENTITY_TO_CLASS( monster_stomach1, CSGTCBodyPart );
LINK_ENTITY_TO_CLASS( monster_stomach2, CSGTCBodyPart );
LINK_ENTITY_TO_CLASS( monster_stomach3, CSGTCBodyPart );

void CSGTCBodyPart::Precache( void )
{
	PRECACHE_MODEL( "models/skelete.mdl" );
}

void CSGTCBodyPart::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/skelete.mdl" );
	UTIL_SetSize( pev, Vector( -13, -13, 0 ), Vector( 13, 13, 72 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->view_ofs = Vector( 0, 0, 60 );
	m_flFieldOfView = 0.5f;
	m_MonsterState = MONSTERSTATE_NONE;
	pev->takedamage = DAMAGE_AIM;

	// Tier from the classname digit, scaled by the matching
	// sk_monster_<part><tier> damage-adjuster cvar.
	int iTier = 1;
	if ( FClassnameIs( pev, "monster_arm1" ) ) iTier = 1;
	else if ( FClassnameIs( pev, "monster_arm2" ) ) iTier = 2;
	else if ( FClassnameIs( pev, "monster_arm3" ) ) iTier = 3;
	else if ( FClassnameIs( pev, "monster_head1" ) ) iTier = 1;
	else if ( FClassnameIs( pev, "monster_head2" ) ) iTier = 2;
	else if ( FClassnameIs( pev, "monster_head3" ) ) iTier = 3;
	else if ( FClassnameIs( pev, "monster_chest1" ) ) iTier = 1;
	else if ( FClassnameIs( pev, "monster_chest2" ) ) iTier = 2;
	else if ( FClassnameIs( pev, "monster_chest3" ) ) iTier = 3;
	else if ( FClassnameIs( pev, "monster_leg1" ) ) iTier = 1;
	else if ( FClassnameIs( pev, "monster_leg2" ) ) iTier = 2;
	else if ( FClassnameIs( pev, "monster_leg3" ) ) iTier = 3;
	else if ( FClassnameIs( pev, "monster_stomach1" ) ) iTier = 1;
	else if ( FClassnameIs( pev, "monster_stomach2" ) ) iTier = 2;
	else if ( FClassnameIs( pev, "monster_stomach3" ) ) iTier = 3;

	float flAdjust = 1.0f;
	if ( FClassnameIs( pev, "monster_arm1" ) ) flAdjust = GetSkillCvar( "sk_monster_arm1" );
	else if ( FClassnameIs( pev, "monster_arm2" ) ) flAdjust = GetSkillCvar( "sk_monster_arm2" );
	else if ( FClassnameIs( pev, "monster_arm3" ) ) flAdjust = GetSkillCvar( "sk_monster_arm3" );
	else if ( FClassnameIs( pev, "monster_head1" ) ) flAdjust = GetSkillCvar( "sk_monster_head1" );
	else if ( FClassnameIs( pev, "monster_head2" ) ) flAdjust = GetSkillCvar( "sk_monster_head2" );
	else if ( FClassnameIs( pev, "monster_head3" ) ) flAdjust = GetSkillCvar( "sk_monster_head3" );
	else if ( FClassnameIs( pev, "monster_chest1" ) ) flAdjust = GetSkillCvar( "sk_monster_chest1" );
	else if ( FClassnameIs( pev, "monster_chest2" ) ) flAdjust = GetSkillCvar( "sk_monster_chest2" );
	else if ( FClassnameIs( pev, "monster_chest3" ) ) flAdjust = GetSkillCvar( "sk_monster_chest3" );
	else if ( FClassnameIs( pev, "monster_leg1" ) ) flAdjust = GetSkillCvar( "sk_monster_leg1" );
	else if ( FClassnameIs( pev, "monster_leg2" ) ) flAdjust = GetSkillCvar( "sk_monster_leg2" );
	else if ( FClassnameIs( pev, "monster_leg3" ) ) flAdjust = GetSkillCvar( "sk_monster_leg3" );
	else if ( FClassnameIs( pev, "monster_stomach1" ) ) flAdjust = GetSkillCvar( "sk_monster_stomach1" );
	else if ( FClassnameIs( pev, "monster_stomach2" ) ) flAdjust = GetSkillCvar( "sk_monster_stomach2" );
	else if ( FClassnameIs( pev, "monster_stomach3" ) ) flAdjust = GetSkillCvar( "sk_monster_stomach3" );
	if ( flAdjust <= 0.0f )
		flAdjust = 1.0f;

	pev->health = 50.0f * (float)iTier * flAdjust;
	pev->max_health = pev->health;

	pev->sequence = 0;
	pev->frame = 0;
	pev->framerate = 1.0;

	SetThink( &CSGTCBodyPart::IdleThink );
	pev->nextthink = gpGlobals->time + 1.0;
}

void CSGTCBodyPart::IdleThink( void )
{
	// Passive target dummy: hold still, stay targetable.
	pev->nextthink = gpGlobals->time + 1.0;
	pev->velocity.x = 0;
	pev->velocity.y = 0;
}

// =============================================================
// CSGTCZatBoom — monster_zatboom (2.0, models/w_zat.mdl)
// =============================================================

TYPEDESCRIPTION CSGTCZatBoom::m_SaveData[] =
{
	DEFINE_FIELD( CSGTCZatBoom, m_flBlastTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CSGTCZatBoom, CBaseMonster );
LINK_ENTITY_TO_CLASS( monster_zatboom, CSGTCZatBoom );

void CSGTCZatBoom::Precache( void )
{
	PRECACHE_MODEL( "models/w_zat.mdl" );
	PRECACHE_SOUND( "weapons/zatoverload.wav" );
	PRECACHE_SOUND( "weapons/zatblow.wav" );
	PRECACHE_SOUND( "weapons/zatimpact.wav" );
	PRECACHE_SOUND( "items/weapondrop1.wav" );
	PRECACHE_MODEL( "sprites/zatimpact.spr" );
	PRECACHE_MODEL( "sprites/gammawave.spr" );
}

void CSGTCZatBoom::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/w_zat.mdl" );
	UTIL_SetSize( pev, Vector( -8, -8, 0 ), Vector( 8, 8, 16 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_AIM;
	pev->health = 20;
	m_flFieldOfView = 0;
	m_MonsterState = MONSTERSTATE_NONE;
	m_bloodColor = DONT_BLEED;

	m_flBlastTime = gpGlobals->time + 2.0;

	EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "weapons/zatoverload.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );

	SetThink( &CSGTCZatBoom::ChargeThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

int CSGTCZatBoom::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Shooting the cell hurries the overload.
	if ( m_flBlastTime > gpGlobals->time + 0.2f )
		m_flBlastTime = gpGlobals->time + 0.2f;
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CSGTCZatBoom::ChargeThink( void )
{
	if ( gpGlobals->time >= m_flBlastTime )
	{
		Discharge();
		return;
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

void CSGTCZatBoom::Discharge( void )
{
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = UTIL_FindEntityInSphere( pEnt, pev->origin, 192.0f ) ) != NULL )
	{
		if ( pEnt == this || pEnt->pev->takedamage == DAMAGE_NO )
			continue;
		if( pEnt->IsPlayer() )
		{
			CBasePlayer *pHit = (CBasePlayer *)pEnt;
			if( pHit->IsAlive() )
				pHit->Zat( pev );
		}
		else
			pEnt->TakeDamage( pev, pev, 80.0f, DMG_SHOCK );
	}
	EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "weapons/zatblow.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	SetThink( NULL );
	UTIL_Remove( this );
}

// =============================================================
// CSGTCTrapBox — monster_trapbox (2.0, models/trapbox.mdl)
// =============================================================

LINK_ENTITY_TO_CLASS( monster_trapbox, CSGTCTrapBox );

void CSGTCTrapBox::Precache( void )
{
	PRECACHE_MODEL( "models/trapbox.mdl" );
	PRECACHE_SOUND( "weapons/place.wav" );
	PRECACHE_SOUND( "weapons/trapbox.wav" );
	PRECACHE_MODEL( "sprites/plasma.spr" );
}

void CSGTCTrapBox::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/trapbox.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 32 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->movetype = MOVETYPE_STEP;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_AIM;
	pev->health = 50;
	m_flFieldOfView = 0;
	m_MonsterState = MONSTERSTATE_NONE;
	m_bloodColor = DONT_BLEED;

	EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "weapons/place.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );

	SetThink( &CSGTCTrapBox::WatchThink );
	pev->nextthink = gpGlobals->time + 0.5;
}

int CSGTCTrapBox::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// ref CTrapBox::TakeDamage @0x15ee94: DMG_CRUSH absorb; 0x1000000 remove;
	// team(attacker)==team(owner) filter; else base. Killed() runs Trigger().
	if( bitsDamageType == DMG_CRUSH )
		return 0;
	if( bitsDamageType == 0x1000000 )
	{
		UTIL_Remove( this );
		return 0;
	}
	if( pevAttacker && pev->owner )
	{
		CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
		if( pOwner && pevAttacker->team == pOwner->pev->team )
			return 0;
	}
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CSGTCTrapBox::Killed( entvars_t *pevAttacker, int iGib )
{
	Trigger();
	CBaseMonster::Killed( pevAttacker, iGib );
}

void CSGTCTrapBox::WatchThink( void )
{
	pev->nextthink = gpGlobals->time + 0.5;

	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = UTIL_FindEntityInSphere( pEnt, pev->origin, 96.0f ) ) != NULL )
	{
		if ( pEnt->pev->takedamage != DAMAGE_NO && pEnt->pev->deadflag == DEAD_NO && pEnt->IsPlayer() )
		{
			Trigger();
			return;
		}
	}
}

void CSGTCTrapBox::Trigger( void )
{
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = UTIL_FindEntityInSphere( pEnt, pev->origin, 128.0f ) ) != NULL )
	{
		if ( pEnt != this && pEnt->pev->takedamage != DAMAGE_NO )
			pEnt->TakeDamage( pev, pev, 60.0f, DMG_BLAST );
	}
	EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "weapons/trapbox.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	SetThink( NULL );
	SetTouch( NULL );
	UTIL_Remove( this );
}

// =============================================================
// CSGTCShieldEmit — monster_shieldemit (2.0, models/shgen.mdl)
// =============================================================

LINK_ENTITY_TO_CLASS( monster_shieldemit, CSGTCShieldEmit );

void CSGTCShieldEmit::Precache( void )
{
	PRECACHE_MODEL( "models/shgen.mdl" );
	PRECACHE_MODEL( "models/shield.mdl" );
	PRECACHE_MODEL( "models/bigshield.mdl" );
}

void CSGTCShieldEmit::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/shgen.mdl" );
	UTIL_SetSize( pev, Vector( -24, -24, 0 ), Vector( 24, 24, 64 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_AIM;
	pev->health = gSkillData.turretHealth;
	pev->max_health = pev->health;
	m_flFieldOfView = 0;
	m_MonsterState = MONSTERSTATE_NONE;
	m_bloodColor = DONT_BLEED;

	SetThink( &CSGTCShieldEmit::EmitThink );
	pev->nextthink = gpGlobals->time + 1.0;
}

void CSGTCShieldEmit::EmitThink( void )	// ref CSEmit::Think @0x160872
{
	// Lazily create the effect_bigshield child once (ref this+0x3f8).
	if( !m_pSgtcEffect )
	{
		m_pSgtcEffect = CBaseEntity::Create( "effect_bigshield",
			pev->origin, Vector( 0, 0, 0 ), edict() );
		EMIT_SOUND( ENT( pev ), CHAN_BODY,
			"forcefield/forcefield_loading.wav", 1.0f, 0.8f );
	}
	if( m_pSgtcEffect )
	{
		// Pass 1 (players): same-team, 3D dist <= 220, z-window
		// (-200..+100) -> stamp shield timestamp (ref ent+0xa2c).
		CBaseEntity *pScan = NULL;
		while( ( pScan = UTIL_FindEntityByClassname( pScan, "player" ) ) != NULL )
		{
			float dz = pScan->pev->origin.z - pev->origin.z;
			BOOL bOut = ( dz > 200.0f || -dz > 100.0f ) ? TRUE : FALSE;
			Vector vThem( pScan->pev->origin );
			Vector vUs( pev->origin );
			int dist = (int)( vUs - vThem ).Length();
			if( dist < 0xdc && pScan->pev->team == pev->team && !bOut )
				pScan->m_flSgtcShieldTime = gpGlobals->time + 0.1f;
		}
		// Pass 2 (all ents in 250 sphere): z-window only -> stamp
		// suppress timer (ref ent+0x128) + back-pointer (ref ent+0x12c).
		pScan = NULL;
		while( ( pScan = UTIL_FindEntityInSphere( pScan, pev->origin, 250.0f ) ) != NULL )
		{
			float dz = pScan->pev->origin.z - pev->origin.z;
			if( dz > 200.0f || -dz > 100.0f )
				continue;
			pScan->m_flSgtcSuppressTime = gpGlobals->time + 0.1f;
			pScan->m_pSgtcSuppressor = this;
		}
		// State machine (ref this+0x104): 1 -> child frame 80; 2 -> unload + remove both.
		if( m_iSgtcMode == 1 )
		{
			m_pSgtcEffect->pev->frame = 80.0f;
			m_iSgtcMode = 0;
		}
		if( m_iSgtcMode == 2 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_BODY,
				"forcefield/forcefield_unloading.wav", 1.0f, 0.8f );
			UTIL_Remove( m_pSgtcEffect );
			UTIL_Remove( this );
			m_iSgtcMode = 0;
		}
		// FX heartbeat (ref this+0x3f4): TE_DLIGHT-ish pulse every 0.2 s.
		if( m_flSgtcNextFx < gpGlobals->time )
		{
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_DLIGHT );
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_BYTE( 40 );
				WRITE_BYTE( 255 );
				WRITE_BYTE( 232 );
				WRITE_BYTE( 70 );
				WRITE_BYTE( 5 );
				WRITE_BYTE( 5 );
			MESSAGE_END();
			m_flSgtcNextFx = gpGlobals->time + 0.2f;
		}
	}
	pev->nextthink = gpGlobals->time + 0.005f;	// ref bottom: time + 0.005
}

// =============================================================
// CSGTCLockDown — monster_lockdown (2.0, models/lockpatch.mdl)
// =============================================================

LINK_ENTITY_TO_CLASS( monster_lockdown, CSGTCLockDown );

void CSGTCLockDown::Precache( void )
{
	PRECACHE_MODEL( "models/lockpatch.mdl" );
	PRECACHE_SOUND( "weapons/place.wav" );
}

void CSGTCLockDown::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/lockpatch.mdl" );
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_NO;
}

void CSGTCLockDown::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( pev->solid == SOLID_BBOX )
	{
		pev->solid = SOLID_NOT;
		pev->effects |= EF_NODRAW;
	}
	else
	{
		pev->solid = SOLID_BBOX;
		pev->effects &= ~EF_NODRAW;
	}
	EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "weapons/place.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	UTIL_SetOrigin( pev, pev->origin ); // relink
}

#endif // CLIENT_DLL
