// sgtc_player.cpp — StargateTC CBasePlayer extensions (ref/stc_i386.so).
// RadarScan @0x135592: MSG_ONE gmsgRadarBlip, BYTE type + 2× WRITE_COORD (×0.1).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "player.h"
#include "weapons.h"
#include "shake.h"
#include "sgtc_rules.h"
#include "sgtc_shield.h"

static const struct
{
	const char *pszClass;
	int iBlipType;
} s_radarClasses[] =
{
	{ "player",            2 },
	{ "item_objective",    6 },
	{ "game_nuke",         6 },
	{ "info_marker",       5 },
	{ "monster_mine",      1 },
	{ "weapon_c4",         5 },
	{ "monster_tac",       3 },
	{ "larve",             1 },
	{ "struct_goa_actual", 6 },
	{ "struct_tau_actual", 6 },
	{ "grenade",           1 },
	{ "monster_satchel",   4 },
};

static void SGTC_SendRadarBlip( CBasePlayer *pPlayer, int iType, int iDx, int iDy )
{
	if( !gmsgRadarBlip )
		return;
	MESSAGE_BEGIN( MSG_ONE, gmsgRadarBlip, NULL, pPlayer->edict() );
	WRITE_BYTE( iType );
	WRITE_COORD( (float)iDx * 0.1f );
	WRITE_COORD( (float)iDy * 0.1f );
	MESSAGE_END();
}

static BOOL SGTC_RadarTeamVisible( CBasePlayer *pViewer, CBaseEntity *pEnt )
{
	BOOL entAlive = FALSE;
	if( pEnt->IsPlayer() )
		entAlive = ((CBasePlayer *)pEnt)->IsAlive();
	else
	{
		CBaseMonster *pMon = pEnt->MyMonsterPointer();
		entAlive = ( pMon && pMon->IsAlive() );
	}
	int entTeam = pEnt->pev->team;
	int viewTeam = pViewer->pev->team;

	if( entAlive )
	{
		if( entTeam != viewTeam )
			return FALSE;
	}
	if( entTeam > 0 && viewTeam > 0 && entTeam != viewTeam )
		return FALSE;
	return TRUE;
}

static BOOL SGTC_RadarPassesFilters( CBasePlayer *pPlayer, CBaseEntity *pEnt, int iDx, int iDy )
{
	const int kHalf = 90;		// ref Endo_roundtoi(9*5*2) @0x135604
	const float kMaxDist = 929.0f;	// cmp 0x3a1 @0x13596c

	if( pEnt == pPlayer )
		return FALSE;
	if( abs( iDx ) <= kHalf && abs( iDy ) <= kHalf )
		return FALSE;

	Vector vecDelta( (float)iDx, (float)iDy, pEnt->pev->origin.z - pPlayer->pev->origin.z );
	if( vecDelta.Length() > kMaxDist )
		return FALSE;

	if( !SGTC_RadarTeamVisible( pPlayer, pEnt ) )
		return FALSE;

	// ref pev+0x148 / +0x14c: skip when entity has a future hide-until time (> now).
	if( pEnt->pev->effects & EF_NODRAW )
		return FALSE;
	if( pEnt->pev->dmgtime > gpGlobals->time )
		return FALSE;

	return TRUE;
}

// ref CBasePlayer::RadarScan @0x135592
void CBasePlayer::SGTC_RadarScan( void )
{
	if( gpGlobals->time < m_flRadarNext )
		return;

	if( !IsAlive() || pev->deadflag != DEAD_NO || IsObserver() )
	{
		m_flRadarNext = gpGlobals->time + 0.25f;
		return;
	}

	if( pev->team <= 0 )
	{
		m_flRadarNext = gpGlobals->time + 0.25f;
		return;
	}

	const int nClasses = (int)( sizeof( s_radarClasses ) / sizeof( s_radarClasses[0] ) );
	for( int i = 0; i < nClasses; i++ )
	{
		const char *pszClass = s_radarClasses[i].pszClass;
		const int iType = s_radarClasses[i].iBlipType;

		CBaseEntity *pEnt = NULL;
		while( ( pEnt = UTIL_FindEntityByClassname( pEnt, pszClass ) ) != NULL )
		{
			const int iDx = (int)( pEnt->pev->origin.x - pev->origin.x );
			const int iDy = (int)( pEnt->pev->origin.y - pev->origin.y );

			if( !SGTC_RadarPassesFilters( this, pEnt, iDx, iDy ) )
				continue;

			SGTC_SendRadarBlip( this, iType, iDx, iDy );
		}
	}

	m_flRadarNext = gpGlobals->time + 0.25f;
}

// ref @0x133aca — toggle +0x925, MSG_ONE gmsgSubModel: 0,0,mode,iMaxAmmo1().
void CBasePlayer::SGTC_WeapDispSet( int mode )
{
	if( mode == 1 )
		m_bWeapSubModel = 1;
	else if( mode == 2 )
		m_bWeapSubModel = 0;

	if( !gmsgSubModel )
		return;

	int iMaxAmmo = 0;
	if( m_pActiveItem )
		iMaxAmmo = ((CBasePlayerWeapon *)m_pActiveItem)->iMaxAmmo1();

	MESSAGE_BEGIN( MSG_ONE, gmsgSubModel, NULL, pev );
	WRITE_BYTE( 0 );
	WRITE_BYTE( 0 );
	WRITE_BYTE( mode );
	WRITE_BYTE( iMaxAmmo );
	MESSAGE_END();
}

// ref @0x1336e8 — MSG_ONE gmsgStamina when m_iStamina (+0x43c) != m_iStaminaSent (+0xa24).
void CBasePlayer::SGTC_UpdateStamina( void )
{
	if( m_iStamina < 0 )
		m_iStamina = 0;

	if( m_iStamina == m_iStaminaSent )
		return;

	if( !gmsgStamina )
		return;

	MESSAGE_BEGIN( MSG_ONE, gmsgStamina, NULL, pev );
	WRITE_BYTE( m_iStamina );
	MESSAGE_END();
	m_iStaminaSent = m_iStamina;
}

// ref @0x1390b2 — shield charge, spawn effect_shield, follow owner.
void CBasePlayer::SGTC_ShieldThinker( void )
{
	if( m_flShieldLatch == 0.0f )
		m_flShieldLatch = gpGlobals->time - 1.0f;	// ref @0x1390e6

	if( gpGlobals->time >= m_flShieldThinkNext )
	{
		if( m_iShieldCharge > 0 && ( m_bShieldWall || m_bShieldSprint ) )
		{
			if( gpGlobals->frametime < 1.0f )
				m_iShieldCharge--;
		}
		else
		{
			m_bShieldSprint = 0;
			m_bShieldWall = 0;
		}

		if( m_iShieldCharge <= 9 && !m_bShieldWall && !m_bShieldSprint )
			m_iShieldCharge++;

		m_flShieldThinkNext = gpGlobals->time + 1.5f;	// ref rodata @0x1d74c4
	}

	if( !m_bShieldWall )
		return;

	if( !m_bShieldSpawned )
	{
		CBaseEntity *pShield = CBaseEntity::Create( "effect_shield", pev->origin, pev->v_angle, edict() );
		if( !pShield )
			return;

		pShield->pev->owner = edict();
		m_bShieldSpawned = 1;

		if( m_bShieldLarge )
		{
			pShield->pev->scale = 2.0f;
			pShield->pev->renderamt = 100.0f;
			pShield->pev->framerate = 1.0f;
		}
		return;
	}

	BOOL bFound = FALSE;
	CBaseEntity *pEnt = NULL;
	while( ( pEnt = UTIL_FindEntityByClassname( pEnt, "effect_shield" ) ) != NULL )
	{
		if( pEnt->pev->owner != edict() )
			continue;

		pEnt->pev->origin = pev->origin;
		pEnt->pev->origin.z -= 40.0f;	// ref @0x1d74dc
		if( m_flShieldLatch <= gpGlobals->time )
			pEnt->pev->renderamt = 130.0f;
		else
			pEnt->pev->renderamt = 100.0f;

		((CShield *)pEnt)->Think();
		bFound = TRUE;
	}

	m_bShieldSpawned = bFound;
}

// ref @0x13a02d..0x13a0da — drain 10 stamina (+0x43c); latch always +0.5 @0x13a150.
static void SGTC_TickMoveLatchAndStaminaDrain( CBasePlayer *p )
{
	if( gpGlobals->time < p->m_flMoveLatch )
		return;

	if( !( p->pev->flags & FL_FROZEN ) && p->m_flMaxSpeedLatch != 0.0f
		&& p->pev->fuser2 == p->m_flMaxSpeedLatch )
	{
		const float move = p->pev->velocity.x + p->pev->velocity.y + p->pev->velocity.z;
		if( move > 0.0f )
		{
			p->m_iStamina -= 10;
			p->SGTC_UpdateStamina();
		}
	}

	p->m_flMoveLatch = gpGlobals->time + 0.5f;	// ref rodata @0x1d755c
}

// ref @0x13a15c..0x13a3a5 — pev+0x14c shield energy (armorvalue), pev+0x210 maxspeed.
void CBasePlayer::SGTC_TickShieldEnergyAndMaxspeed( void )
{
	static const float kShieldDrain = 5.0f;		// rodata @0x1d7558
	static const float kShieldCap = 255.0f;		// rodata @0x1d7540
	static const float kRunSpeed = 180.0f;		// rodata @0x1d7564
	static const float kLowStaminaSpeed = 10.0f;	// rodata @0x1d7554
	static const float kZatVelZ = -20.0f;		// rodata @0x1d7560
	static const float kLatchMoveMin = 0.125f;	// rodata @0x1d7528

	if( m_bZatTouchFlag && pev->velocity.z <= 0.0f )
		pev->velocity.z = kZatVelZ;

	if( m_bZatTouchFlag || m_bShieldTraceHit )
		return;

	if( m_bShieldSprint )
	{
		if( pev->armorvalue > 0.0f )
			pev->armorvalue -= kShieldDrain;
	}
	else
	{
		if( pev->armorvalue < kShieldCap )
		{
			if( pev->armorvalue > 0.0f && gpGlobals->time >= m_flGateTravel )
				pev->armorvalue += kShieldDrain;
		}
		else
		{
			pev->armorvalue = kShieldCap;
			pev->rendermode = 0;	// ref pev+0x148
			pev->renderfx = 0;	// ref pev+0x15c
		}
	}

	if( m_flMaxSpeedLatch > kLatchMoveMin )
		return;

	if( pev->flags & FL_ONGROUND )
	{
		if( pev->maxspeed != kRunSpeed )
			pev->maxspeed = kRunSpeed;
	}
	else
		pev->maxspeed = m_flMaxSpeedLatch;

	if( m_iStamina <= 19 )
		pev->maxspeed = kLowStaminaSpeed;
}

// ref @0x13a3a5 — when pev+0x14c < 0 and not sprinting, PointContents → armorvalue.
void CBasePlayer::SGTC_SyncShieldEnergyFromContents( void )
{
	if( pev->armorvalue >= 0.0f )
		return;
	if( m_bShieldSprint )
		return;

	pev->armorvalue = (float)UTIL_PointContents( pev->origin );
}

// ref @0x139f18 — when +0x444/+0x938 differ from +0x45c/+0x934, MSG_ONE Progress.
void CBasePlayer::SGTC_SendProgressIfDirty( void )
{
	if( m_iProgressSentPri == m_iNukeCharge && m_iNukeBonusSent == m_iNukeBonus )
		return;

	if( !gmsgProgress )
		return;

	MESSAGE_BEGIN( MSG_ONE, gmsgProgress, NULL, pev );
	WRITE_BYTE( m_iNukeCharge );
	WRITE_BYTE( m_iNukeBonus );
	for( int i = 0; i < 28; i++ )
		WRITE_BYTE( m_iProgressTeam[i] );
	WRITE_BYTE( 0 );
	MESSAGE_END();

	m_iProgressSentPri = m_iNukeCharge;
	m_iNukeBonusSent = m_iNukeBonus;
}

// ref @0x1398ef — one-shot camp warn: HasKey(m_iInBombZone, 1).
void CBasePlayer::SGTC_TryBombKeyCampWarn( void )
{
	if( m_bBombKeyWarnSent )
		return;
	if( m_iBombKeyTimer > 0 )
		return;

	const float flNow = gpGlobals->time;
	if( flNow == CVAR_GET_FLOAT( "tourny" ) )
		return;
	if( flNow == CVAR_GET_FLOAT( "allowmodevote" ) )
		return;
	if( g_iSgtcMapType == 1 )
		return;
	if( !gmsgHasKey )
		return;

	MESSAGE_BEGIN( MSG_ONE, gmsgHasKey, NULL, pev );
	WRITE_BYTE( m_iInBombZone );
	WRITE_BYTE( 1 );
	MESSAGE_END();
	m_bBombKeyWarnSent = 1;
}

// ref @0x13a438 — clamp key count, HasKey on enter/leave bomb zone state.
void CBasePlayer::SGTC_SyncBombZoneHasKey( void )
{
	if( m_iInBombZone > 1 )
		m_iInBombZone = 1;

	BOOL bSend = FALSE;
	if( m_iInBombZone > 0 )
	{
		if( !m_bBombZoneKeyLatch )
			bSend = TRUE;
	}
	else if( m_bBombZoneKeyLatch == 1 )
		bSend = TRUE;

	if( bSend && gmsgHasKey )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgHasKey, NULL, pev );
		WRITE_BYTE( m_iInBombZone );
		WRITE_BYTE( 0 );
		MESSAGE_END();
	}

	m_bBombZoneKeyLatch = ( m_iInBombZone > 0 ) ? 1 : 0;
}

// ref ClientCommand @0xcb456 — menuselect arg <=4 arms bomb-key warn timer.
void CBasePlayer::SGTC_MenuselectBombKey( int sel )
{
	if( sel > 4 )
		return;
	m_iBombKeyTimer = sel + 1;
	m_bBombKeyWarnSent = 0;
}

// ref @0x136454 — toggle +0x4e9; on: pev+0x148 = kRenderTransAlpha (4).
void CBasePlayer::SGTC_ToggleVis( void )
{
	if( !m_bShieldSprint )
	{
		pev->rendermode = kRenderTransAlpha;
		m_bShieldSprint = 1;
	}
	else
		m_bShieldSprint = 0;
}

// ref @0x1336b8 — if shield sprint, snap pev+0x14c (armorvalue) to 220.
void CBasePlayer::SnagInvis( void )
{
	if( m_bShieldSprint )
		pev->armorvalue = 220.0f;	// rodata @0x1d6f9c
}

// ref @0x13720e — personal shield leg hit from enemy fire.
void CBasePlayer::SGTC_TryShieldTraceHit( entvars_t *pevAttacker, TraceResult *ptr )
{
	if( !pevAttacker || !ptr )
		return;
	if( ptr->iHitgroup != HITGROUP_RIGHTLEG )
		return;
	if( m_bSnagInvisLatch )
		return;
	if( m_bShieldWall )
		return;
	if( pevAttacker->team == pev->team )
		return;

	m_bShieldTraceHit = 1;
	m_flShieldTraceTime = gpGlobals->time;	// ref @0x13726a
}

// ref @0x13e995 — expire +0xa78 after ~2s from +0xa68.
void CBasePlayer::SGTC_TickShieldTraceLatch( void )
{
	static const float kShieldTraceHold = 2.0f;	// rodata @0x1d2e20

	if( !m_bShieldTraceHit )
		return;
	if( gpGlobals->time - m_flShieldTraceTime < kShieldTraceHold )
		return;

	m_bShieldTraceHit = 0;
	pev->renderfx = 0;	// ref pev+0x15c
	pev->armorvalue = 0.0f;	// rodata @0x1d2e24
	pev->rendercolor = Vector( 255, 255, 255 );	// rodata @0x1d2e28
}

// ref @0x13378a — zat stun from zat_proj / zatboom on players.
void CBasePlayer::Zat( entvars_t *pevInflictor )
{
	m_bZatLatch = 1;

	if( m_bSnagInvisLatch || m_bShieldWall )
		return;
	if( pevInflictor && pevInflictor->team == pev->team )
		return;

	float flDmg;
	if( m_bZatTouchFlag )
	{
		if( pevInflictor && pevInflictor->frags > pev->frags )
			flDmg = 2000.0f;	// rodata @0x1d6fa8
		else
			flDmg = 80.0f;	// rodata @0x1d6fa4
	}
	else
		flDmg = 10.0f;	// rodata @0x1d6fac

	TakeDamage( pevInflictor, pevInflictor, flDmg, DMG_SHOCK );

	if( !m_bZatTouchFlag && gmsgZatTouch )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgZatTouch, NULL, pev );
		MESSAGE_END();
	}

	m_bZatTouchFlag = 1;

	pev->armorvalue = 2.0f;	// rodata @0x1d6fb0
	pev->rendercolor.x = 25.0f;	// @0x1d6fb4
	pev->rendercolor.y = 10.0f;	// @0x1d6fac (reuse)
	pev->rendercolor.z = 255.0f;	// @0x1d6fb8
	m_bShieldSprint = 0;
	SnagInvis();
	pev->maxspeed = 50.0f;	// rodata @0x1d6fbc

	m_iStamina -= 60;
	if( m_iStamina < 0 )
		m_iStamina = 0;
	SGTC_UpdateStamina();

	UTIL_ScreenFade( this, g_vecZero, 0.0f, 100.0f, 0, FFADE_IN );	// ref @0x1339f9
}

// ref helper @0x1abc1c — Manhattan 3d distance as int.
static int SGTC_Endo_distance3d( const Vector &a, const Vector &b )
{
	return (int)fabs( a.x - b.x ) + (int)fabs( a.y - b.y ) +
		(int)fabs( a.z - b.z );
}

// ref @0x139c13 — team spawn info proximity + camp latch (+0x4a4/+0x4a8/+0x4b4).
void CBasePlayer::SGTC_TickSpawnCampProximity( void )
{
	static const float kProxExtendSec = 1.0f;	// rodata @0x1d7548
	static const float kCampTouchSec = 0.3f;	// rodata @0x1d754c
	static const int kNearSpawnDist = 0xc7;
	static const int kCampDist = 0xc8;

	const char *pszSpawnClass = NULL;
	if( pev->team == 1 )
		pszSpawnClass = "info_player_goauld";
	else if( pev->team == 2 )
		pszSpawnClass = "info_player_torri";
	else
		return;

	CBaseEntity *pSpawn = NULL;
	while( ( pSpawn = UTIL_FindEntityByClassname( pSpawn, pszSpawnClass ) ) != NULL )
	{
		if( SGTC_Endo_distance3d( pev->origin, pSpawn->pev->origin ) <= kNearSpawnDist )
		{
			m_flSpawnProxDeadline = gpGlobals->time + kProxExtendSec;
			break;
		}
	}

	const int iDistRef = SGTC_Endo_distance3d( pev->origin, m_vecSpawnCampRef );
	BOOL bCampTick = FALSE;
	if( iDistRef > kCampDist )
		bCampTick = TRUE;
	else if( gpGlobals->time >= m_flSpawnProxDeadline )
		bCampTick = TRUE;

	if( bCampTick )
	{
		m_vecSpawnCampRef = pev->origin;
		m_flSpawnCampTouch = gpGlobals->time + kCampTouchSec;
	}
}

// ref @0x139b26 — holster clears SubModel disp (+0x925) via WeapDispSet(2) when +0xcd4 empty.
static void SGTC_TickWeapSubModelDisp( CBasePlayer *p )
{
	if( !p->m_bWeapSubModel )
		return;
	if( p->m_pActiveItem )
		return;
	p->SGTC_WeapDispSet( 2 );
}

// ref @0x139b5d — periodic cloak refresh while +0x448 (0.2s latch @+0xa90).
void CBasePlayer::SGTC_TickCloakPulse( void )
{
	static const float kCloakPulseSec = 0.2f;	// rodata @0x1d754a
	static const float kCloakHealthGate = 100.0f;	// rodata @0x1d753c — pev+0x160

	if( !m_bSgtcCloakTick )
		return;
	if( pev->health != kCloakHealthGate )
		return;
	if( gpGlobals->time < pev->animtime )
		return;
	if( gpGlobals->time < m_flCloakTickNext )
		return;

	pev->effects |= EF_NODRAW;
	if( m_bSnagInvisLatch )
		SnagInvis();

	m_flCloakTickNext = gpGlobals->time + kCloakPulseSec;
}

// ref @0x139814 — arm 0.2s window (rodata 0x1d7530), expire +0xa3e, copy to +0x4e8.
void CBasePlayer::SGTC_TickLanceStunLatch( void )
{
	static const float kLanceStunSec = 0.2f;

	if( !m_bLanceStunLatch && m_bLanceStunFlag )
		m_flLanceStunEnd = gpGlobals->time + kLanceStunSec;

	if( m_bLanceStunFlag && gpGlobals->time >= m_flLanceStunEnd )
		m_bLanceStunFlag = 0;

	m_bLanceStunLatch = m_bLanceStunFlag;
}

// ref @0x1398ac — while +0x463, copy rodata (0,0,-10) into pev+0x174 (ref view_ofs).
void CBasePlayer::SGTC_TickNukeViewSnap( void )
{
	static const float kViewSnapZ = -10.0f;	// rodata @0x1d7548 (via ebx-0x2d4d8)

	if( !m_bNukeViewSnap )
		return;

	pev->view_ofs = Vector( 0.0f, 0.0f, kViewSnapZ );
}

// ref @0x1399cf — hurtinvert preset PE colors; when +0x4c0 sum == 0, copy recallall RGB.
void CBasePlayer::SGTC_TickHurtInvertPeFx( void )
{
	static const float kZero = 0.0f;		// rodata @0x1d752c (ebx-0x2d4e8)
	static const float kAmt255 = 255.0f;	// rodata @0x1d7540 (ebx-0x2d4d4)
	static const float kTwo = 2.0f;		// rodata @0x1d7544 (ebx-0x2d4d0)
	static const float kOne = 1.0f;		// rodata @0x1d7548 (ebx-0x2d4cc)

	if( m_bHurtInvertFx )
	{
		pev->rendermode = kRenderTransAdd;	// ref pev+0x148 = 5
		pev->renderamt = kAmt255;
		pev->rendercolor.x = kTwo;
		pev->rendercolor.y = kZero;
		pev->rendercolor.z = kZero;
		pev->renderfx = kRenderFxGlowShell;	// ref pev+0x15c = 0x13
		pev->renderamt = kOne;
		pev->rendercolor.x = kAmt255;
		pev->rendercolor.y = kAmt255;
		pev->rendercolor.z = kTwo;
	}

	const float flSum = m_vecHurtInvertRgb.x + m_vecHurtInvertRgb.y + m_vecHurtInvertRgb.z;
	if( flSum != kZero )
		return;

	pev->renderfx = kRenderFxGlowShell;
	pev->renderamt = kOne;
	pev->rendercolor = m_vecHurtInvertRgb;
}

// ref ClientCommand @0xc94c1 hurtinvert, @0xc9301 recallall.
BOOL SGTC_HandleClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	if( FStrEq( pcmd, "hurtinvert" ) )
	{
		if( !pPlayer->m_bShieldLarge )
			return TRUE;
		pPlayer->m_bHurtInvertFx ^= 1;
		return TRUE;
	}

	if( FStrEq( pcmd, "recallall" ) )
	{
		if( CMD_ARGC() <= 2 )
			return TRUE;

		pPlayer->m_vecHurtInvertRgb.x = (float)atoi( CMD_ARGV( 1 ) );
		pPlayer->m_vecHurtInvertRgb.y = (float)atoi( CMD_ARGV( 2 ) );
		pPlayer->m_vecHurtInvertRgb.z = (float)atoi( CMD_ARGV( 3 ) );
		return TRUE;
	}

	return FALSE;
}

// ref @0x139db0 — while pev+0x160 (health marker) == 1, latch +0x3fc and sustain zat visuals.
void CBasePlayer::SGTC_TickZatTouchMaintain( void )
{
	static const float kHealthGate = 1.0f;	// rodata @0x1d7548
	static const float kLatch3fc = 0.125f;	// rodata @0x1d7528
	static const float kZatArmor = 1.0f;	// rodata @0x1d7548 (reuse)

	if( pev->health != kHealthGate )
		return;

	m_flSgtcLatch3fc = kLatch3fc;
	m_iStamina = 0;

	if( !m_bZatTouchFlag )
		return;

	pev->renderfx = kRenderFxGlowShell;	// ref pev+0x15c = 0x13
	pev->armorvalue = kZatArmor;
	pev->rendercolor.x = 25.0f;	// rodata @0x1d7550
	pev->rendercolor.y = 10.0f;	// rodata @0x1d7554
	pev->rendercolor.z = 255.0f;	// rodata @0x1d7540
}

// ref @0x139e51 — when armor latch expired and not sprinting past gate travel, restore PE shield colors.
void CBasePlayer::SGTC_TickClearZatShieldPe( void )
{
	static const float kShieldCap = 255.0f;	// rodata @0x1d7540

	if( pev->armorvalue <= gpGlobals->time )
		return;
	if( m_bShieldSprint )
		return;
	if( m_flGateTravel == gpGlobals->time )
		return;

	pev->armorvalue = kShieldCap;
	pev->renderfx = kRenderFxNone;
	pev->rendermode = kRenderNormal;
}

// ref @0x139edb — drop nuke arm counter when +0x42c charge window ends.
void CBasePlayer::SGTC_TickNukeChargeExpiry( void )
{
	if( m_flNukeChargeTime > gpGlobals->time )
		return;
	m_iNukeCharge = 0;
}

// ref CBasePlayer::SgtcThink @0x13963a (full body ~7 KB; tail @0x13a404).
void CBasePlayer::SGTC_SgtcThink( void )
{
	if( !IsAlive() )
		return;

	// ref @0x1397b3 — nuke bonus display counter floor.
	if( m_iNukeBonus <= 5 )
		m_iNukeBonus = 6;

	// ref @0x1397d4 — on ladder, drop personal shield deploy flag (+0x3e0).
	if( IsOnLadder() && m_bShieldWall )
		m_bShieldWall = 0;

	SGTC_TickLanceStunLatch();
	SGTC_TickNukeViewSnap();
	SGTC_TryBombKeyCampWarn();
	SGTC_TickHurtInvertPeFx();
	SGTC_TickWeapSubModelDisp( this );
	SGTC_TickCloakPulse();
	SGTC_TickSpawnCampProximity();
	SGTC_TickZatTouchMaintain();
	SGTC_TickClearZatShieldPe();
	SGTC_TickNukeChargeExpiry();
	SGTC_SendProgressIfDirty();
	SGTC_TickMoveLatchAndStaminaDrain( this );
	SGTC_TickShieldEnergyAndMaxspeed();
	SGTC_SyncShieldEnergyFromContents();
	SGTC_ShieldThinker();
	SGTC_RadarScan();
	SGTC_SyncBombZoneHasKey();
}
