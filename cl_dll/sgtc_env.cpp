// sgtc_env.cpp — env particle parsers, ref client.dll @0x1006fc89/…
// READ sizes: FUN_10065073=byte, FUN_100650bd=short, FUN_10065129=long,
// FUN_10065280=float, FUN_10065213=string.
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "sgtc_env.h"

CClientEnv g_ClientEnv;

int CClientEnv::Init( void )
{
	m_iFlags = HUDELEM_ACTIVE;
	Reset();
	gHUD.AddHudElem( this );
	return 1;
}

int CClientEnv::VidInit( void )
{
	return 1;
}

int CClientEnv::Draw( float flTime )
{
	(void)flTime;
	// TEMP: ref spawns client-side env systems in Draw; kept as parsed state.
	return 0;
}

void CClientEnv::Reset( void )
{
	m_iEnvCount = 0;
	memset( m_Env, 0, sizeof( m_Env ) );
}

// ref FUN_1006fc67 — drain only.
int CClientEnv::MsgFunc_ClearEnv( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	for( int i = 0; i < iSize; i++ )
		READ_BYTE();
	return 1;
}

// ref FUN_1006fc89 — 19 WRITE_COORD + 7 BYTE + 1 SHORT (47 bytes, "EnvFull").
int CClientEnv::MsgFunc_EnvFull( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	if( m_iEnvCount >= SGTC_MAX_ENV_PARTICLES )
		return 1;
	CSgtcEnvParticle *e = &m_Env[m_iEnvCount++];
	memset( e, 0, sizeof( *e ) );
	e->active = 1;

	for( int i = 0; i < 3; i++ ) e->min[i] = READ_COORD();
	for( int i = 0; i < 3; i++ ) e->max[i] = READ_COORD();
	for( int i = 0; i < 3; i++ ) e->vel[i] = READ_COORD();
	for( int i = 0; i < 3; i++ ) e->acc[i] = READ_COORD();
	for( int i = 0; i < 3; i++ ) e->msec[i] = READ_COORD();
	for( int i = 0; i < 4; i++ ) e->color[i] = READ_BYTE();
	e->parms[0] = READ_COORD();
	e->parms[1] = READ_COORD();
	e->parms[2] = READ_COORD();
	e->radius  = READ_COORD();
	e->flags = READ_BYTE();
	e->type  = READ_BYTE();
	e->spr   = READ_SHORT();
	e->id    = READ_BYTE();
	e->state = 0;
	return 1;
}

// ref FUN_1007004a — 6 coords + byte + short (15 bytes REG "EnvRain").
int CClientEnv::MsgFunc_EnvRain( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	if( m_iEnvCount >= SGTC_MAX_ENV_PARTICLES )
		return 1;
	CSgtcEnvParticle *e = &m_Env[m_iEnvCount++];
	memset( e, 0, sizeof( *e ) );
	e->active = 1;
	e->type = 1; // rain
	for( int i = 0; i < 3; i++ ) e->min[i] = READ_COORD();
	for( int i = 0; i < 3; i++ ) e->max[i] = READ_COORD();
	e->parms[0] = READ_BYTE();
	e->spr = READ_SHORT();
	return 1;
}

// ref FUN_100701af — 15 coords + 6 bytes + short + id (45 bytes).
int CClientEnv::MsgFunc_EnvFount( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	if( m_iEnvCount >= SGTC_MAX_ENV_PARTICLES )
		return 1;
	CSgtcEnvParticle *e = &m_Env[m_iEnvCount++];
	memset( e, 0, sizeof( *e ) );
	e->active = 1;
	e->type = 2; // fountain
	for( int i = 0; i < 3; i++ ) e->min[i] = READ_COORD();
	for( int i = 0; i < 3; i++ ) e->max[i] = READ_COORD();
	for( int i = 0; i < 3; i++ ) e->vel[i] = READ_COORD();
	for( int i = 0; i < 3; i++ ) e->acc[i] = READ_COORD();
	for( int i = 0; i < 3; i++ ) e->msec[i] = READ_COORD();
	for( int i = 0; i < 6; i++ ) e->parms[i] = READ_BYTE();
	e->spr = READ_SHORT();
	e->id  = READ_BYTE();
	return 1;
}

// ref FUN_10070518 — 3 coords + 4 RGBA bytes (10 bytes "NewLens").
int CClientEnv::MsgFunc_NewLens( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	if( m_iEnvCount >= SGTC_MAX_ENV_PARTICLES )
		return 1;
	CSgtcEnvParticle *e = &m_Env[m_iEnvCount++];
	memset( e, 0, sizeof( *e ) );
	e->active = 1;
	e->type = 3; // lens flare
	for( int i = 0; i < 3; i++ ) e->min[i] = READ_COORD();
	for( int i = 0; i < 4; i++ ) e->color[i] = READ_BYTE();
	return 1;
}

// ref FUN_1007069f — 4 coords + string (var size "AddRing").
int CClientEnv::MsgFunc_AddRing( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	if( m_iEnvCount >= SGTC_MAX_ENV_PARTICLES )
		return 1;
	CSgtcEnvParticle *e = &m_Env[m_iEnvCount++];
	memset( e, 0, sizeof( *e ) );
	e->active = 1;
	e->type = 4; // ring
	for( int i = 0; i < 3; i++ ) e->min[i] = READ_COORD();
	e->parms[0] = READ_COORD();
	(void)READ_STRING();
	return 1;
}

// ref FUN_1006ffc2 — id byte + state byte, flips +0xb0 on matching envs.
int CClientEnv::MsgFunc_ChgPartSt( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	int id = READ_BYTE();
	int state = READ_BYTE();
	for( int i = 0; i < m_iEnvCount; i++ )
		if( m_Env[i].active && m_Env[i].id == id )
			m_Env[i].state = state;
	return 1;
}
