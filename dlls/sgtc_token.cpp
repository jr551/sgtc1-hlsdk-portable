// sgtc_token.cpp — CToken / game_nuke (ref/stc_i386.so @0x194244 Spawn, @0x195df6 factory).
// FGD: studio models/bomb.mdl, countdown default 15.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "weapons.h"

class CToken : public CBaseDelay
{
public:
	void Spawn( void ) override;
	void Precache( void ) override;
	void KeyValue( KeyValueData *pkvd ) override;
	void Think( void ) override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;

	void Deconax( void );

private:
	int m_iCountdown;	// ref KeyValue "countdown" -> +0x19c
	int m_iBombState;	// ref +0x134 (0xc5 when idle)
	int m_iArmed;		// ref +0x138
	float m_flDetonateTime; // stored on pev path in ref Think (+0x160)
	BOOL m_fActivated;
};

LINK_ENTITY_TO_CLASS( game_nuke, CToken );

void CToken::Precache( void )
{
	PRECACHE_MODEL( "models/bomb.mdl" );
	PRECACHE_SOUND( "weapons/c4_beep.wav" );
}

void CToken::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "countdown" ) )
	{
		m_iCountdown = (int)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CToken::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/bomb.mdl" );

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 36 ) );

	if( m_iCountdown <= 0 )
		m_iCountdown = 15;

	m_iBombState = 0xc5;
	m_iArmed = 0;
	m_fActivated = FALSE;
	m_flDetonateTime = 0;

	pev->nextthink = gpGlobals->time + 0.1f;
}

void CToken::Deconax( void )
{
	m_iArmed = 0;
	m_fActivated = FALSE;
	m_flDetonateTime = 0;
	pev->effects &= ~EF_BRIGHTLIGHT;
}

void CToken::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !pActivator || !pActivator->IsPlayer() )
		return;

	if( ( pActivator->pev->origin - pev->origin ).Length() > 60.0f )
		return;

	if( !m_fActivated )
	{
		m_fActivated = TRUE;
		m_iArmed = 1;
		m_flDetonateTime = gpGlobals->time + (float)m_iCountdown;
		pev->nextthink = gpGlobals->time + 0.1f;
		EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/c4_beep.wav", 1.0f, ATTN_NORM );
		SUB_UseTargets( pActivator, USE_TOGGLE, 0 );
	}
}

void CToken::Think( void )
{
	if( m_fActivated && m_flDetonateTime > 0 && gpGlobals->time >= m_flDetonateTime )
	{
		const float flDamage = 500.0f;
		const float flRadius = 512.0f;

		RadiusDamage( pev->origin, pev, pev, flDamage, flRadius, CLASS_NONE, DMG_BLAST );

		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( (int)( flRadius / 40.0f ) );
		WRITE_BYTE( 15 );
		WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();

		SUB_UseTargets( this, USE_ON, 0 );
		UTIL_Remove( this );
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1f;
}
