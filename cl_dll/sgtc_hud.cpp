// sgtc_hud.cpp — SGTC HUD usermsg parsers (ref client.dll REA @0x10072e5f..0x10072c62).
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "sgtc_hud.h"

CHudSgtc g_SgtcHud;

// ref FUN_10076ba2 — Stamina byte → global DAT_100f71dc.
static int s_iSgtcStaminaByte;

int g_iSgtcVoteCount = -1;	// ref DAT_100babe4 (5 = vote window)
int g_iSgtcVoteSel = -1;	// ref DAT_100babe0 (0..4 mode highlight)
float g_flSgtcVoteAnim = 0;	// ref DAT_100f71e0 (mode cycle timer)

static void SGTC_DrawLocalizedHudLine( int x, int y, const char *token, int r, int g, int b )
{
	const char *text = CHudTextMessage::BufferedLocaliseTextString( token );
	gHUD.DrawHudString( x, y, ScreenWidth, text, r, g, b );
}

static void SGTC_VoteFormatCenter( int modeIndex )
{
	char buf[256];
	const char *fmt = gHUD.m_TextMessage.LookupString( "VoteMode", NULL );
	if( !fmt || !fmt[0] )
		fmt = "VoteMode %d";
	snprintf( buf, sizeof( buf ), fmt, modeIndex );
	CenterPrint( buf );
}

static void SGTC_InitPortraitSlots( CHudSgtc *hud );

int CHudSgtc::Init( void )
{
	m_iFlags = HUDELEM_ACTIVE;
	Reset();
	gHUD.AddHudElem( this );
	// ref FUN_10072e5f — SGTC view toggles (default 0).
	gEngfuncs.pfnRegisterVariable( "view_lowvis", "0", FCVAR_ARCHIVE );
	gEngfuncs.pfnRegisterVariable( "view_radar", "0", FCVAR_ARCHIVE );
	gEngfuncs.pfnRegisterVariable( "view_portraits", "0", FCVAR_ARCHIVE );
	gEngfuncs.pfnRegisterVariable( "view_seeownport", "0", FCVAR_ARCHIVE );
	SGTC_InitPortraitSlots( this );
	// ref FUN_10072e96 — reset vote globals unless mid-countdown at 5.
	if( g_iSgtcVoteCount != 5 )
		g_iSgtcVoteCount = -1;
	g_flSgtcVoteAnim = 0;
	g_iSgtcVoteSel = -1;
	return 1;
}

int CHudSgtc::VidInit( void )
{
	return 1;
}

static cvar_t *SGTC_Cvar( const char *name )
{
	return gEngfuncs.pfnGetCvarPointer( name );
}

static int SGTC_Sprite( const char *name, wrect_t *prc )
{
	int idx = gHUD.GetSpriteIndex( name );
	if( idx <= 0 )
		return 0;
	if( prc )
		*prc = gHUD.GetSpriteRect( idx );
	return idx;
}

static void SGTC_DrawSprite( const char *name, int x, int y )
{
	wrect_t rc;
	if( SGTC_Sprite( name, &rc ) )
		SPR_DrawAdditive( 0, x, y, &rc );
}

static void SGTC_DrawBarNeedle( const char *bar, const char *needle, int x, int y, float frac )
{
	wrect_t rcBar, rcNeedle;
	if( !SGTC_Sprite( bar, &rcBar ) )
		return;
	SPR_DrawAdditive( 0, x, y, &rcBar );
	if( frac < 0.0f )
		frac = 0.0f;
	if( frac > 1.0f )
		frac = 1.0f;
	if( SGTC_Sprite( needle, &rcNeedle ) )
	{
		int ny = y + (int)( ( rcBar.bottom - rcBar.top - rcNeedle.bottom ) * ( 1.0f - frac ) );
		SPR_DrawAdditive( 0, x + 4, ny, &rcNeedle );
	}
}

// ref FUN_10072502 — clear 33 portrait slot records @ Init.
static void SGTC_InitPortraitSlots( CHudSgtc *hud )
{
	for( int i = 0; i < SGTC_PORTRAIT_SLOTS; i++ )
	{
		hud->m_iPortraitSlotId[i] = 0;
		hud->m_flPortraitSlotHpPct[i] = 0;
		hud->m_iPortraitSlotTeam[i] = 0;
		hud->m_iPortraitSlotAux[i] = 0;
		hud->m_iPortraitSlotEnt[i] = 0;
		hud->m_iPortraitSlotHp[i][0] = 0;
		hud->m_iPortraitSlotHp[i][1] = 0;
		hud->m_iPortraitSlotHp[i][2] = 0;
		hud->m_szPortraitSlotName[i][0] = 0;
		hud->m_iPortraitSlotColor[i] = 0;
		hud->m_iPortraitSlotFlash[i] = 0;
		hud->m_iPortraitSlotGray[i] = 0x7d;
	}
}

// ref FUN_10072694 — upsert portrait column cell for active team.
static void SGTC_UpdatePortraitSlot( CHudSgtc *hud, int spr, float hpPct, int team, int aux, int ent,
	int hp0, int hp1, int hp2, const char *name, int colorId )
{
	if( !spr || !ent || !name || !name[0] )
		return;

	int hit = -1;
	for( int i = 0; i < SGTC_PORTRAIT_SLOTS; i++ )
	{
		if( hud->m_iPortraitSlotEnt[i] == ent && hud->m_iPortraitSlotId[i] != 0 )
		{
			hit = i;
			break;
		}
	}

	if( hit < 0 && team == hud->m_iPortraitActiveTeam )
	{
		for( int i = 0; i < SGTC_PORTRAIT_SLOTS; i++ )
		{
			if( hud->m_iPortraitSlotId[i] == 0 )
			{
				hit = i;
				break;
			}
		}
	}

	if( hit < 0 )
		return;

	int oldTeam = hud->m_iPortraitSlotTeam[hit];
	hud->m_iPortraitSlotId[hit] = spr;
	hud->m_flPortraitSlotHpPct[hit] = hpPct;
	hud->m_iPortraitSlotTeam[hit] = team;
	hud->m_iPortraitSlotAux[hit] = aux;
	hud->m_iPortraitSlotEnt[hit] = ent;
	hud->m_iPortraitSlotHp[hit][0] = hp0;
	hud->m_iPortraitSlotHp[hit][1] = hp1;
	hud->m_iPortraitSlotHp[hit][2] = hp2;
	strncpy( hud->m_szPortraitSlotName[hit], name, sizeof( hud->m_szPortraitSlotName[hit] ) - 1 );
	hud->m_iPortraitSlotColor[hit] = colorId;
	if( oldTeam != team )
	{
		for( int i = 0; i < SGTC_PORTRAIT_SLOTS; i++ )
		{
			if( hud->m_iPortraitSlotEnt[i] != 0 && hud->m_iPortraitSlotEnt[i] != hud->m_iPortraitEnt )
			{
				hud->m_iPortraitSlotId[i] = 0;
				hud->m_flPortraitSlotHpPct[i] = 0;
				hud->m_iPortraitSlotTeam[i] = 0;
				hud->m_iPortraitSlotAux[i] = 0;
				hud->m_iPortraitSlotEnt[i] = 0;
				hud->m_iPortraitSlotFlash[i] = 0;
				hud->m_iPortraitSlotGray[i] = 0x7d;
				hud->m_szPortraitSlotName[i][0] = 0;
			}
		}
	}
}

// ref FUN_100772f2 — portrait name, avatar, HP bar (abbreviated).
static void SGTC_DrawPortraitCell( CHudSgtc *hud, int slot, int px, int py, char dim )
{
	if( !hud->m_iPortraitSlotId[slot] )
		return;

	const char *avatar = hud->m_szPortraitSlotName[slot];
	if( !avatar[0] )
		return;

	int r = 168, g = 211, b = 255;
	switch( hud->m_iPortraitSlotColor[slot] & 0x1f )
	{
	case 1: r = 255; g = 0; b = 0; break;
	case 2: r = 255; g = 130; b = 0; break;
	case 3: r = 255; g = 255; b = 0; break;
	case 4: r = 130; g = 255; b = 0; break;
	case 5: r = 0; g = 255; b = 0; break;
	default: break;
	}
	if( dim )
		r = g = b = 0;

	DrawConsoleString( px, py - 12, avatar );
	if( SGTC_Sprite( avatar, NULL ) )
	{
		wrect_t av;
		SGTC_Sprite( avatar, &av );
		SPR_Set( gHUD.GetSprite( gHUD.GetSpriteIndex( avatar ) ), r, g, b );
		SPR_DrawAdditive( 0, px + 8, py, &av );
	}

	float frac = hud->m_flPortraitSlotHpPct[slot] / 100.0f;
	if( frac < 0.0f )
		frac = 0.0f;
	if( frac > 1.0f )
		frac = 1.0f;
	if( frac > 0.0f && !dim )
	{
		int barX = px + 8;
		int barY = py + 56;
		FillRGBA( barX, barY, 64, 4, 30, 30, 30, 255 );
		FillRGBA( barX, barY, (int)( 64 * frac ), 4, 0, 130, 0, 255 );
	}
}

int CHudSgtc::Draw( float flTime )
{
	if( m_iHideHud )
		return 0;

	// ref FUN_1007257b — expire portrait slots not matching active ent.
	for( int i = 0; i < SGTC_PORTRAIT_SLOTS; i++ )
	{
		if( m_iPortraitSlotEnt[i] != 0 && m_iPortraitSlotEnt[i] != m_iPortraitEnt )
		{
			m_iPortraitSlotId[i] = 0;
			m_flPortraitSlotHpPct[i] = 0;
			m_iPortraitSlotTeam[i] = 0;
			m_iPortraitSlotAux[i] = 0;
			m_iPortraitSlotEnt[i] = 0;
			m_iPortraitSlotFlash[i] = 0;
			m_iPortraitSlotGray[i] = 0x7d;
			m_szPortraitSlotName[i][0] = 0;
		}
	}

	const int panelH = 168;
	const int lx = 0;
	const int ly = ScreenHeight - panelH;
	const int rx = ScreenWidth - 200;

	SGTC_DrawSprite( "hud_left", lx, ly );
	SGTC_DrawSprite( "hud_right", rx, ly );

	float life = (float)gHUD.m_Health.m_iHealth;
	SGTC_DrawBarNeedle( "life_bar", "needle", lx + 24, ly + 28, life / 100.0f );
	SGTC_DrawBarNeedle( "armor_bar", "needle", lx + 48, ly + 28, m_flArmorBar / 100.0f );
	SGTC_DrawBarNeedle( "stamn_bar", "needle", lx + 72, ly + 28, (float)s_iSgtcStaminaByte / 100.0f );

	if( m_iHasKey )
		SGTC_DrawSprite( "bombkey", lx + 120, ly + 100 );

	cvar_t *cvRadar = SGTC_Cvar( "view_radar" );
	if( cvRadar && cvRadar->value > 0.0f )
	{
		int x = ScreenWidth - 220;
		int y = ScreenHeight - 220;
		if( gHUD.GetSpriteIndex( "radar" ) )
		{
			wrect_t rc = gHUD.GetSpriteRect( gHUD.GetSpriteIndex( "radar" ) );
			SPR_DrawAdditive( 0, x, y, &rc );
			if( gHUD.GetSpriteIndex( "blip" ) )
			{
				wrect_t bl = gHUD.GetSpriteRect( gHUD.GetSpriteIndex( "blip" ) );
				for( int i = 1; i < SGTC_RADAR_SLOTS; i++ )
				{
					if( !m_iRadarType[i] )
						continue;
					int bx = x + ( m_iRadarX[i] >> 3 );
					int by = y + ( m_iRadarY[i] >> 3 );
					SPR_DrawAdditive( 0, bx, by, &bl );
				}
				if( m_iRadarFocusX || m_iRadarFocusY )
				{
					int fx = x + ( m_iRadarFocusX >> 3 );
					int fy = y + ( m_iRadarFocusY >> 3 );
					SPR_DrawAdditive( 0, fx, fy, &bl );
				}
			}
		}
	}

	if( m_iTicketModeFlag && SGTC_Sprite( "ticketdock", NULL ) )
	{
		wrect_t dock;
		SGTC_Sprite( "ticketdock", &dock );
		int x = 10;
		int y = ScreenHeight - dock.bottom - 10;
		SPR_DrawAdditive( 0, x, y, &dock );
		gHUD.DrawHudNumber( x + 24, y + 20, DHN_DRAWZERO, m_iGtickets, 255, 255, 255 );
		gHUD.DrawHudNumber( x + 64, y + 20, DHN_DRAWZERO, m_iTtickets, 255, 255, 255 );
	}

	if( m_flRoundTime > 0.0f )
	{
		int remain = (int)( m_flRoundTime - ( flTime - m_flRoundStart ) );
		if( remain < 0 )
			remain = 0;
		gHUD.DrawHudNumber( ScreenWidth / 2 - 24, 8, DHN_3DIGITS | DHN_DRAWZERO, remain, 255, 200, 64 );
	}

	cvar_t *cvPort = SGTC_Cvar( "view_portraits" );
	if( cvPort && cvPort->value > 0.0f )
	{
		SGTC_DrawSprite( "port_left", lx + 8, ly - 104 );
		SGTC_DrawSprite( "port_mid", lx + 24, ly - 104 );
		SGTC_DrawSprite( "port_right", lx + 104, ly - 104 );
		int col = 0;
		for( int i = 0; i < SGTC_PORTRAIT_SLOTS; i++ )
		{
			if( !m_iPortraitSlotId[i] )
				continue;
			if( m_iPortraitActiveTeam && m_iPortraitSlotTeam[i] != m_iPortraitActiveTeam )
				continue;
			int py = ly - 96 - col * 72;
			if( py < 0 )
				break;
			char dim = ( m_iPortraitSlotEnt[i] != m_iPortraitEnt ) ? 1 : 0;
			SGTC_DrawPortraitCell( this, i, lx + 24, py, dim );
			col++;
		}
	}

	if( m_iBlipProgress > 0 && gHUD.GetSpriteIndex( "pbar" ) )
	{
		wrect_t bar = gHUD.GetSpriteRect( gHUD.GetSpriteIndex( "pbar" ) );
		int x = ScreenWidth / 2 - bar.right / 2;
		int y = ScreenHeight - 48;
		SPR_DrawAdditive( 0, x, y, &bar );
		if( gHUD.GetSpriteIndex( "pbare" ) )
		{
			wrect_t fill = gHUD.GetSpriteRect( gHUD.GetSpriteIndex( "pbare" ) );
			fill.right = (int)( fill.right * ( m_iBlipProgress / 100.0f ) );
			SPR_DrawAdditive( 0, x, y, &fill );
		}
	}

	if( m_fZatTouchActive && m_flZatTouchTime + 2.0f > flTime && gHUD.GetSpriteIndex( "zat_touch" ) )
	{
		wrect_t zt = gHUD.GetSpriteRect( gHUD.GetSpriteIndex( "zat_touch" ) );
		SPR_DrawAdditive( 0, ScreenWidth / 2 - zt.right / 2, ScreenHeight / 2 - zt.bottom / 2, &zt );
	}
	else if( m_fZatTouchActive && m_flZatTouchTime + 2.0f <= flTime )
		m_fZatTouchActive = 0;

	// ref @0x10076292 — vote prompts when portrait team filter active.
	if( m_iPortraitActiveTeam )
	{
		if( g_iSgtcVoteCount < 0 && g_iSgtcVoteSel < 0 && g_iSgtcVoteCount < 5 )
		{
			int px = m_iBlipProgress + 100;
			int py = m_iBlipProgress + 100;
			SGTC_DrawLocalizedHudLine( px, py, "#PLZVOTE", 255, 255, 255 );
		}
		if( flTime > m_flKeyTime )
		{
			int px = m_iBlipProgress + 100;
			int py = m_iBlipProgress + 100;
			SGTC_DrawLocalizedHudLine( px, py, "#CAMPWARN", 255, 255, 255 );
		}
	}

	return 1;
}

void CHudSgtc::Reset( void )
{
	m_iGtickets = 0;
	m_iTtickets = 0;
	m_iTicketMode = 0;
	m_iHudInfoMode = 0;
	m_iStamina = 0;
	s_iSgtcStaminaByte = 0;
	m_iSubModel = 0;
	m_iHasKey = 0;
	m_iRadioTeam = 0;
	m_iRadioMsg = 0;
	m_iArmorTeamTint = 0;
	m_iTacDispActive = 0;
	m_flArmorBar = 0;
	m_iArmorAux = 0;
	m_iArmorOverlay = 0;
	m_iArmorState = 0;
	m_iArmorParam = 0;
	m_iBlipProgress = 0;
	m_iRadarFocusX = 0;
	m_iRadarFocusY = 0;
	memset( m_iRadarType, 0, sizeof( m_iRadarType ) );
	memset( m_iRadarX, 0, sizeof( m_iRadarX ) );
	memset( m_iRadarY, 0, sizeof( m_iRadarY ) );
	memset( m_iRadarAlpha, 0, sizeof( m_iRadarAlpha ) );
	m_fZatTouchActive = 0;
	m_flZatTouchTime = 0;
	m_iHideHud = 0;
	m_iPortraitSprite = 0;
	m_iPortraitEnt = 0;
	m_flPortraitTime = 0;
	m_iPortraitX = 0;
	memset( m_iPortraitHp, 0, sizeof( m_iPortraitHp ) );
	m_szPortraitName[0] = 0;
	m_iPortraitId = 0;
	m_iPortraitActiveTeam = 0;
	m_iAboutMeSelf = 0;
	m_iAboutMePlayer = 0;
	m_iKeyMode = 0;
	m_flKeyTime = 0;
	m_flRoundTime = 0;
	m_flRoundStart = 0;
	m_iTicketModeFlag = 0;
	m_iHudInfoByte = 0;
	m_iProgress = 0;
	m_iProgressIcon = 0;
	memset( m_iProgressTeam, 0, sizeof( m_iProgressTeam ) );
	m_iProgressEnd = 0;
	SGTC_InitPortraitSlots( this );
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

// ref FUN_10076d24 — Ttickets → +0x60
int CHudSgtc::MsgFunc_Ttickets( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iTtickets = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;
	return 1;
}

// ref FUN_10076cf7 — Gtickets → +0x5c
int CHudSgtc::MsgFunc_Gtickets( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iGtickets = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;
	return 1;
}

// ref FUN_10076bfb — TicketMode → +0x58 flag
int CHudSgtc::MsgFunc_TicketMode( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	int b = READ_BYTE();
	m_iTicketModeFlag = ( b >= 1 ) ? 1 : 0;
	m_iFlags |= HUD_ACTIVE;
	return 1;
}

// ref FUN_10076bce — HudInfoMode → +0x54
int CHudSgtc::MsgFunc_HudInfoMode( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iHudInfoByte = READ_BYTE();
	return 1;
}

int CHudSgtc::MsgFunc_Stamina( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	s_iSgtcStaminaByte = READ_BYTE();
	m_iStamina = s_iSgtcStaminaByte;
	return 1;
}

// ref FUN_10076d51 — SubModel global DAT_100f6fb8
int CHudSgtc::MsgFunc_SubModel( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iSubModel = READ_BYTE();
	return 1;
}

// ref FUN_10076d7d — HasKey: keymode + vote timer (offsets +0x34/+0x30).
int CHudSgtc::MsgFunc_HasKey( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	int mode = READ_BYTE();
	if( mode < 2 )
		m_iKeyMode = mode;
	else if( mode == 2 )
		m_flKeyTime = gHUD.m_flTime + 5.0f;
	else
		m_flKeyTime = 0.0f;

	int vote = READ_BYTE();
	if( vote == 1 )
	{
		g_iSgtcVoteCount = -1;
		g_iSgtcVoteSel = -1;
		m_iHasKey = 0;
		g_flSgtcVoteAnim = 0;
	}
	else if( vote == 2 )
	{
		if( g_iSgtcVoteCount >= 0 )
			SGTC_VoteFormatCenter( g_iSgtcVoteCount );
		else
		{
			g_iSgtcVoteCount = 5;
			g_iSgtcVoteSel = -1;
			m_iHasKey = 0;
			g_flSgtcVoteAnim = 0;
		}
	}
	return 1;
}

// ref FUN_10076c3d @0x10076c3d — BEGIN_READ only; radio text uses Titles.txt elsewhere.
int CHudSgtc::MsgFunc_Radio( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
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
		m_iArmorTeamTint = 0; // ref DAT_100f71d4
	}
	else if( sub == 4 && m_iArmorState == 2 )
	{
		m_iArmorState = 3;
		m_iArmorTeamTint = 0;
		m_iTacDispActive = 0; // ref DAT_100f70d0
	}
	if( sub == 5 && m_iArmorState == 0 )
	{
		m_iArmorState = 1;
		m_iArmorTeamTint = 1;
	}
	if( sub == 10 )
		m_iTacDispActive = 1;
	if( sub == 1 )
		m_iArmorParam = d;
	return 1;
}

// ref FUN_10076c5f — Progress batch (icon + 28 team bytes + tail).
int CHudSgtc::MsgFunc_Progress( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iProgress = READ_BYTE();
	m_iProgressIcon = READ_BYTE();
	for( int i = 0; i < SGTC_BLIP_SLOTS; i++ )
		m_iProgressTeam[i] = READ_BYTE();
	m_iProgressEnd = READ_BYTE();

	if( m_iProgress == 0 )
		m_iBlipProgress = 0;
	if( m_iProgress == 100 )
		m_iBlipProgress = 100;
	m_iFlags |= HUD_ACTIVE;
	return 1;
}

// ref FUN_100770dd — Blip: type byte + 2 coords, ring slots 1..255.
int CHudSgtc::MsgFunc_Blip( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	int type = READ_BYTE();
	int rx = (int)READ_COORD();
	int ry = (int)READ_COORD();

	if( type == 0x32 )
	{
		m_iRadarFocusX = rx;
		m_iRadarFocusY = ry;
	}
	else if( type == 0x3d )
	{
		m_iRadarFocusX = 0;
		m_iRadarFocusY = 0;
	}
	else
	{
		for( int slot = 1; slot < SGTC_RADAR_SLOTS; slot++ )
		{
			if( m_iRadarType[slot] == 0 )
			{
				m_iRadarType[slot] = type;
				m_iRadarX[slot] = rx;
				m_iRadarY[slot] = ry;
				m_iRadarAlpha[slot] = 255;
				break;
			}
		}
	}
	m_iFlags |= HUD_ACTIVE;
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

#include "sgtc_env.h"

static int SGTC_DrainMsg( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	for( int i = 0; i < iSize; i++ )
		READ_BYTE();
	return 1;
}

// ref @0x100768d4 — short,long,short,short,short,short,short,short,string,byte
int CHudSgtc::MsgFunc_AddPortrait( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	int spr   = READ_SHORT();	// iVar1
	int t     = READ_LONG();	// iVar2 → +0x28
	int i3    = READ_SHORT();	// DAT_100f70c8
	int i4    = READ_SHORT();
	int x     = READ_SHORT();	// iVar5 → compared w/ this+0x40
	int hp0   = READ_SHORT();	// +0x44
	int hp1   = READ_SHORT();	// +0x48
	int hp2   = READ_SHORT();	// +0x4c
	const char *name = READ_STRING();
	int id    = READ_BYTE();

	if( m_iPortraitX == x )
	{
		m_iPortraitActiveTeam = i3;
		m_flPortraitTime = (float)t;
		m_iPortraitId = id;
		m_iPortraitHp[0] = hp0;
		m_iPortraitHp[1] = hp1;
		m_iPortraitHp[2] = hp2;
		if( spr )
		{
			m_iPortraitSprite = spr;
			m_iPortraitEnt = x;
		}
		strncpy( m_szPortraitName, name, sizeof( m_szPortraitName ) - 1 );
	}
	SGTC_UpdatePortraitSlot( this, spr, (float)t, i3, i4, x, hp0, hp1, hp2, name, id );
	m_iFlags |= HUD_ACTIVE;
	return 1;
}

// ref @0x10076eb9 — short + byte; byte==0 resets portrait arrays, else SetPortrait(iVar1)
int CHudSgtc::MsgFunc_Reset( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	int arg = READ_SHORT();
	int mode = READ_BYTE();
	if( mode == 0 )
		Reset();
	else
		m_iPortraitEnt = arg;
	g_iSgtcVoteCount = -1;
	g_iSgtcVoteSel = -1;
	return 1;
}

// ref @0x10076a20 — byte 0 → short self @+0x20, short player @+0x40
int CHudSgtc::MsgFunc_AboutMe( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	if( READ_BYTE() == 0 )
	{
		m_iAboutMeSelf = READ_SHORT();
		m_iAboutMePlayer = READ_SHORT();
		m_iPortraitX = m_iAboutMePlayer; // ref +0x1564 copy
	}
	return 1;
}

// ref @0x10077088 — no payload; toggles view_portraits cvar.
int CHudSgtc::MsgFunc_TogglePor( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	cvar_t *cv = gEngfuncs.pfnGetCvarPointer( "view_portraits" );
	if( cv )
		gEngfuncs.Cvar_SetValue( "view_portraits", cv->value == 0.0f ? 1.0f : 0.0f );
	return 1;
}

// ref @0x100771f3 — short round-time + byte state.
int CHudSgtc::MsgFunc_SetRndTim( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_flRoundTime = (float)READ_SHORT();
	m_flRoundStart = gHUD.m_flTime;
	int state = READ_BYTE();
	if( state )
		m_iTicketMode = state; // ref calls this vtable+0x1c
	return 1;
}

// ref @0x1007724b — clears zat flag + portrait arrays + 256-slot loop.
int CHudSgtc::MsgFunc_NewRnd( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_fZatTouchActive = 0;
	Reset();
	return 1;
}

int CHudSgtc::MsgFunc_ClearEnv( const char *pszName, int iSize, void *pbuf )
{
	return g_ClientEnv.MsgFunc_ClearEnv( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_EnvFull( const char *pszName, int iSize, void *pbuf )
{
	return g_ClientEnv.MsgFunc_EnvFull( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_EnvRain( const char *pszName, int iSize, void *pbuf )
{
	return g_ClientEnv.MsgFunc_EnvRain( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_EnvFount( const char *pszName, int iSize, void *pbuf )
{
	return g_ClientEnv.MsgFunc_EnvFount( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_NewLens( const char *pszName, int iSize, void *pbuf )
{
	return g_ClientEnv.MsgFunc_NewLens( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_AddRing( const char *pszName, int iSize, void *pbuf )
{
	return g_ClientEnv.MsgFunc_AddRing( pszName, iSize, pbuf );
}

int CHudSgtc::MsgFunc_ChgPartSt( const char *pszName, int iSize, void *pbuf )
{
	return g_ClientEnv.MsgFunc_ChgPartSt( pszName, iSize, pbuf );
}
