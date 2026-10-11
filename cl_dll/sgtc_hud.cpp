// sgtc_hud.cpp — SGTC HUD usermsg parsers (ref client.dll REA @0x10072e5f..0x10072c62).
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "sgtc_hud.h"

CHudSgtc g_SgtcHud;

int CHudSgtc::Init( void )
{
	m_iFlags = HUDELEM_ACTIVE;
	Reset();
	gHUD.AddHudElem( this );
	return 1;
}

int CHudSgtc::VidInit( void )
{
	return 1;
}

int CHudSgtc::Draw( float flTime )
{
	(void)flTime;
	// Full hud.txt layout draw is future work; state is kept for parity with ref.
	return 0;
}

void CHudSgtc::Reset( void )
{
	m_iGtickets = 0;
	m_iTtickets = 0;
	m_iTicketMode = 0;
	m_iHudInfoMode = 0;
	m_iStamina = 0;
	m_iSubModel = 0;
	m_iHasKey = 0;
	m_iRadioTeam = 0;
	m_iRadioMsg = 0;
	m_flArmorBar = 0;
	m_iArmorAux = 0;
	m_iArmorOverlay = 0;
	m_iArmorState = 0;
	m_iArmorParam = 0;
	m_iBlipCmd = 0;
	memset( m_iBlipSlots, 0, sizeof( m_iBlipSlots ) );
	m_iBlipTail = 0;
	m_iBlipProgress = 0;
	m_fZatTouchActive = 0;
	m_flZatTouchTime = 0;
	m_iHideHud = 0;
}

// ref FUN_100772b4 — ZatTouch, REG size 0 on server.
int CHudSgtc::MsgFunc_ZatTouch( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_fZatTouchActive = 1;
	m_flZatTouchTime = gHUD.m_flTime;
	return 1;
}

// ref FUN_10076cf7 — READ_BYTE -> +0x5c
int CHudSgtc::MsgFunc_Gtickets( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iGtickets = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;
	return 1;
}

int CHudSgtc::MsgFunc_Ttickets( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iTtickets = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;
	return 1;
}

int CHudSgtc::MsgFunc_TicketMode( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iTicketMode = READ_BYTE();
	return 1;
}

// ref FUN_10076ba2
int CHudSgtc::MsgFunc_HudInfoMode( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iHudInfoMode = READ_BYTE();
	return 1;
}

int CHudSgtc::MsgFunc_Stamina( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iStamina = READ_BYTE();
	return 1;
}

int CHudSgtc::MsgFunc_SubModel( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iSubModel = READ_BYTE();
	return 1;
}

int CHudSgtc::MsgFunc_HasKey( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iHasKey = READ_BYTE();
	(void)READ_BYTE(); // second byte in REG size 2
	return 1;
}

int CHudSgtc::MsgFunc_Radio( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iRadioTeam = READ_BYTE();
	m_iRadioMsg = READ_BYTE();
	return 1;
}

// ref FUN_10076a85 — 4-byte subcommand stream (server REG "Armor", 4).
int CHudSgtc::MsgFunc_Armor( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	int a = READ_BYTE();
	int b = READ_BYTE();
	int sub = READ_BYTE();
	int d = READ_BYTE();

	if( sub == 0 )
	{
		m_flArmorBar = (float)a;
		m_iArmorAux = b;
	}
	if( sub == 1 )
		m_iArmorOverlay = 1;
	else if( sub == 2 )
		m_iArmorOverlay = 0;

	if( sub == 3 && m_iArmorState == 0 )
	{
		m_iArmorState = 1;
		m_iRadioTeam = 0; // ref clears DAT_100f71d4
	}
	else if( sub == 4 && m_iArmorState == 2 )
	{
		m_iArmorState = 3;
		m_iRadioTeam = 0;
	}
	if( sub == 5 && m_iArmorState == 0 )
	{
		m_iArmorState = 1;
		m_iRadioTeam = 1;
	}
	if( sub == 1 )
		m_iArmorParam = d;
	return 1;
}

// ref FUN_10076c5f — radar blip batch (28 slots + cmd/progress bytes).
int CHudSgtc::MsgFunc_Blip( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iBlipCmd = READ_BYTE();
	m_iBlipTail = READ_BYTE();
	for( int i = 0; i < SGTC_BLIP_SLOTS; i++ )
		m_iBlipSlots[i] = READ_BYTE();
	int tail2 = READ_BYTE();
	(void)tail2;

	if( m_iBlipCmd == 0 )
		m_iBlipProgress = 0;
	if( m_iBlipCmd == 100 )
		m_iBlipProgress = 100;
	m_iFlags |= HUD_ACTIVE;
	return 1;
}

int CHudSgtc::MsgFunc_Progress( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	for( int i = 0; i < iSize; i++ )
	{
		int b = READ_BYTE();
		if( b == 0 )
			m_iBlipProgress = 0;
		else if( b == 100 )
			m_iBlipProgress = 100;
	}
	return 1;
}

int CHudSgtc::MsgFunc_HideHud( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	if( iSize > 0 )
		m_iHideHud = READ_BYTE();
	else
		m_iHideHud = 1;
	return 1;
}

static int SGTC_DrainMsg( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	for( int i = 0; i < iSize; i++ )
		READ_BYTE();
	return 1;
}

int CHudSgtc::MsgFunc_AddPortrait( const char *pszName, int iSize, void *pbuf )
{
	return SGTC_DrainMsg( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_Reset( const char *pszName, int iSize, void *pbuf )
{
	Reset();
	return SGTC_DrainMsg( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_AboutMe( const char *pszName, int iSize, void *pbuf )
{
	return SGTC_DrainMsg( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_TogglePor( const char *pszName, int iSize, void *pbuf )
{
	return SGTC_DrainMsg( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_SetRndTim( const char *pszName, int iSize, void *pbuf )
{
	return SGTC_DrainMsg( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_NewRnd( const char *pszName, int iSize, void *pbuf )
{
	Reset();
	return 1;
}

int CHudSgtc::MsgFunc_ClearEnv( const char *pszName, int iSize, void *pbuf )
{
	return SGTC_DrainMsg( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_EnvFull( const char *pszName, int iSize, void *pbuf )
{
	return SGTC_DrainMsg( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_EnvRain( const char *pszName, int iSize, void *pbuf )
{
	return SGTC_DrainMsg( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_EnvFount( const char *pszName, int iSize, void *pbuf )
{
	return SGTC_DrainMsg( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_NewLens( const char *pszName, int iSize, void *pbuf )
{
	return SGTC_DrainMsg( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_AddRing( const char *pszName, int iSize, void *pbuf )
{
	return SGTC_DrainMsg( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_ChgPartSt( const char *pszName, int iSize, void *pbuf )
{
	return SGTC_DrainMsg( pszName, iSize, pbuf );
}
