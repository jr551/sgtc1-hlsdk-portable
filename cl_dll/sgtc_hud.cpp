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
int g_iSgtcAnnouncePending = 0;	// ref DAT_100f1ce8 (Concuss byte 2 → scroller @0x10074366)

static void SGTC_DrawLocalizedHudLine( int x, int y, const char *token, int r, int g, int b )
{
	const char *text = CHudTextMessage::BufferedLocaliseTextString( token );
	gHUD.DrawHudString( x, y, ScreenWidth, text, r, g, b );
}

// ref @0x10076306 / @0x1007636f — DrawHudString x=0x14, y=ScreenHeight-0xdc, iMaxX=0x26c (ds:0x100ed4c4).
static void SGTC_DrawVotePromptLine( const char *token, int r, int g, int b )
{
	const char *text = CHudTextMessage::BufferedLocaliseTextString( token );
	gHUD.DrawHudString( 20, ScreenHeight - 220, 620, text, r, g, b );
}

// ref @0x100761dd — pulse m_iBlipPulse toward 0x9b using byte @+0x1685 as direction.
static void SGTC_TickBlipPulse( CHudSgtc *hud )
{
	if( hud->m_iBlipPulse < 0x9b && !hud->m_cBlipPulseDir )
		hud->m_iBlipPulse++;
	else if( hud->m_iBlipPulse > 0 && hud->m_cBlipPulseDir )
		hud->m_iBlipPulse--;

	if( hud->m_iBlipPulse <= 0 || hud->m_iBlipPulse >= 0x9b )
		hud->m_cBlipPulseDir = hud->m_cBlipPulseDir ? 0 : 1;
}

// ref @0x10074a20..0x10074e58 — ticket dock slide + sprite rect helper @0x10051150.
static void SGTC_TickTicketDock( CHudSgtc *hud, float flTime )
{
	if( !hud->m_iTicketModeFlag )
		return;

	int slideDir = 0;	// ref ebp-0xb8 (1 = shrink offset @0x10074d4e)
	hud->m_cTicketSlideDir = 0;

	// ref @0x10074ad7 — cascade subtracts (not else-if); may peel multiple tiers per frame.
	if( hud->m_iTicketSlideRemain > 0 )
	{
		int &rem = hud->m_iTicketSlideRemain;
		if( rem >= 0x600 )
			rem -= 0x600;
		if( rem >= 0x300 )
			rem -= 0x300;
		if( rem >= 0x180 )
			rem -= 0x180;
		if( rem >= 0xc0 )
			rem -= 0xc0;
		if( rem >= 0x60 )
			rem -= 0x60;
		if( rem >= 0x30 )
			rem -= 0x30;
		if( rem >= 0x18 )
		{
			slideDir = 2;
			rem -= 0x18;
		}
		if( rem >= 0xc )
		{
			slideDir = 1;
			rem -= 0xc;
		}
		if( rem >= 0x6 )
		{
			hud->m_bTicketDockLayout78 = 1;
			rem -= 0x6;
		}
		if( rem >= 0x3 )
			rem -= 0x3;
		if( rem >= 0x2 )
			rem -= 0x2;
		else if( rem > 0 )
			rem = 0;
	}

	// ref @0x10074c97 — skip frame/offset latch until m_flKeyTime reached.
	if( flTime < hud->m_flKeyTime )
		return;

	// ref @0x10074cb1 — dock frame/tint latch only when slideDir==2 this frame.
	if( slideDir == 2 )
	{
		hud->m_iTicketDockFrame++;
		if( hud->m_iTicketDockFrame > 10 )
			hud->m_iTicketDockFrame = 0;
	}

	if( slideDir == 1 )
	{
		if( hud->m_iTicketSlideOffset > 0x32 )
			hud->m_iTicketSlideOffset -= 2;
	}
	else if( hud->m_iTicketSlideOffset < 0x7d )
		hud->m_iTicketSlideOffset += 2;

	hud->m_cTicketSlideDir = (char)slideDir;
}

static void SGTC_DrawTicketDock( CHudSgtc *hud, float flTime )
{
	if( !hud->m_iTicketModeFlag )
		return;

	const int iDock = gHUD.GetSpriteIndex( "ticketdock" );
	if( !iDock )
		return;
	wrect_t dock = gHUD.GetSpriteRect( iDock );

	// ref CHudSgtc::Draw @0x10074a4c / @0x10074e8b — fixed anchor; SPR_Set tints (not position).
	const int x = ScreenWidth - 0x4e;
	const int y = ScreenHeight - 0x8e;

	int tintR, tintG, tintB;
	if( hud->m_cTicketSlideDir == 1 )	// ref @0x10074d4e
		tintR = tintG = tintB = hud->m_iTicketSlideOffset;
	else if( hud->m_iTicketDockFrame > 5 )	// ref @0x10074ccd
	{
		tintR = 0xf3;
		tintG = 0xb7;
		tintB = 0;
	}
	else
	{
		tintR = 0xff;
		tintG = 0xd5;
		tintB = 0x53;
	}
	if( hud->m_bTicketDockLayout78 )	// ref @0x10074df8
	{
		tintR = 0x78;
		tintG = 0x78;
		tintB = 0x32;
	}
	if( flTime < hud->m_flKeyTime )	// ref @0x10074e25
	{
		tintG = 0;
		tintB = 0;
	}

	SPR_Set( gHUD.GetSprite( iDock ), tintR, tintG, tintB );
	SPR_DrawAdditive( 0, x, y, &dock );

	// ref @0x10074f0e — G/T counts when m_iArmorOverlay==0 (+0x1684).
	if( hud->m_iArmorOverlay != 0 )
		return;

	const int numY = ScreenHeight - 0x25;
	if( hud->m_iGtickets > 0 )
		gHUD.DrawHudNumber( 0x46, numY, DHN_DRAWZERO, hud->m_iGtickets, 0xa8, 0xd3, 0xff );
	if( hud->m_iTtickets > 0 )
		gHUD.DrawHudNumber( 0x82, numY, DHN_DRAWZERO, hud->m_iTtickets, 0xa8, 0xd3, 0xff );
}

// ref @0x1004b45a / TeamNames VGUI @0x10057bed — fill DAT_100e1a34[0..0x1b] from resource bytes.
static void SGTC_InitProgressSlotX( CHudSgtc *hud )
{
	for( int i = 0; i < SGTC_BLIP_SLOTS; i++ )
		hud->m_iProgressSlotX[i] = ( i < 14 ) ? 0x10 : 0x12;
}

// ref FUN_1002a9ca @0x1002abe2: slots 0..13 use DAT_100e1a34[i]; slots 14..27 use
// (&DAT_100e1a6c)[i] which aliases DAT_100e1aa4 + (i-14)*4 (progress team ints).
static int SGTC_ProgressSlotStep( const CHudSgtc *hud, int slot )
{
	if( slot < 14 )
		return hud->m_iProgressSlotX[slot];
	return hud->m_iProgressTeam[slot - 14];
}

// ref VGUI panel update @0x1002abe2 — per-slot team progress chip state (no widgets on Xash).
static void SGTC_TickProgressTeamUi( CHudSgtc *hud )
{
	if( hud->m_iProgress <= 0 )
		return;

	// ref FUN_1002a9ca entry: DAT_100e1b90 recomputed every VGUI/progress frame.
	hud->m_iProgressIconCursor = 0;

	for( int i = 0; i < SGTC_BLIP_SLOTS; i++ )
	{
		int &st = hud->m_iProgressTeamUiState[i];

		if( st == 1 )
			st = 0;

		const int team = hud->m_iProgressTeam[i];

		if( team == 0 )
		{
			if( st < 2 )
				st = 2;
		}
		else if( team == 1 )
		{
			if( st == 2 )
				st = 0;
		}
		else if( team == 2 )
		{
			hud->m_iProgressIconCursor += SGTC_ProgressSlotStep( hud, i );
			st = 1;
			hud->m_iProgressTeam[i] = 1;	// ref @0x1002acb7
		}
	}
}

static const char *SGTC_ProgressIconSprite( int icon )
{
	if( icon == 1 )
		return "star_g";
	if( icon == 2 )
		return "star_p";
	return "star_s";
}

static void SGTC_DrawProgressTeamUi( CHudSgtc *hud )
{
	if( hud->m_iHideHud || hud->m_iProgress <= 0 )
		return;

	const char *sprName = SGTC_ProgressIconSprite( hud->m_iProgressIcon );
	const int idx = gHUD.GetSpriteIndex( sprName );
	if( !idx )
		return;

	wrect_t rc = gHUD.GetSpriteRect( idx );
	const int anchorX = ScreenWidth / 2 - hud->m_iProgressIconCursor;
	const int rowY0 = ScreenHeight - 0xb0;
	const int rowY1 = rowY0 + 0xe;

	int xWalk = 0;
	for( int i = 0; i < SGTC_BLIP_SLOTS; i++ )
	{
		if( hud->m_iProgressTeamUiState[i] != 1 )
		{
			xWalk += SGTC_ProgressSlotStep( hud, i );
			continue;
		}

		const int y = ( i < 14 ) ? rowY0 : rowY1;
		const int x = anchorX + xWalk;
		xWalk += SGTC_ProgressSlotStep( hud, i );
		SPR_Set( gHUD.GetSprite( idx ), 255, 255, 255 );
		SPR_DrawHoles( 0, x, y, &rc );
	}
}

// ref @0x100750c5 — ramp m_iBlipProgress toward m_iProgress while 0 < progress < 100.
static void SGTC_TickBlipBarProgress( CHudSgtc *hud, float flTime )
{
	if( hud->m_iProgress <= 0 || hud->m_iProgress >= 100 )
		return;

	// ref @0x100750db: x87 `test ah,0x41` after comparing flTime to the latch => strict flTime > latch.
	if( flTime > hud->m_flBlipProgressLatch )
	{
		hud->m_iBlipProgress++;
		hud->m_flBlipProgressLatch = flTime + 0.15f; // ref fadd QWORD @0x10075115 (ds:0x10094a78 = 0.15)
	}

	const int iFloor = hud->m_iProgress - 7;
	if( hud->m_iBlipProgress < iFloor )
		hud->m_iBlipProgress = hud->m_iProgress;
}

// ref @0x10076464 — vote cast lines: x=ScreenWidth/3, iMaxX=ScreenWidth/3+0x12c.
static void SGTC_DrawVoteCastHudLine( int y, const char *token, int r, int g, int b )
{
	const char *text = CHudTextMessage::BufferedLocaliseTextString( token );
	const int x = ScreenWidth / 3;
	gHUD.DrawHudString( x, y, x + 300, text, r, g, b );
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

// ref @0x100612d2 — votecastmenu client command (cycles vote mode index).
static void SGTC_VoteCastMenu_f( void )
{
	if( g_iSgtcVoteCount >= 5 )
		return;
	const float now = gHUD.m_flTime;
	if( now <= g_flSgtcVoteAnim )
	{
		g_iSgtcVoteSel = 0;
		g_flSgtcVoteAnim = now + 5.0f;
	}
	else
	{
		g_flSgtcVoteAnim = now + 0.5f;
		g_iSgtcVoteSel++;
		if( g_iSgtcVoteSel > 4 )
			g_iSgtcVoteSel = 0;
	}
}

static void SGTC_DrawVoteCastMenu( float flTime )
{
	if( g_iSgtcVoteCount < 0 || g_iSgtcVoteCount >= 5 )
		return;
	// ref @0x10076374 — flTime >= g_flSgtcVoteAnim → confirm @0x10076623 (sel>=0).
	if( flTime >= g_flSgtcVoteAnim )
	{
		if( g_iSgtcVoteSel >= 0 )
		{
			SGTC_VoteFormatCenter( g_iSgtcVoteSel );
			gEngfuncs.pfnPlaySoundByName( "buttons/rt_button.wav", 1.0f );
			g_iSgtcVoteCount = g_iSgtcVoteSel;
			g_iSgtcVoteSel = -1;
		}
		return;
	}

	static const char *modeTok[] = {
		"1. #MODE0", "2. #MODE1", "3. #MODE2", "4. #MODE3", "5. #MODE4"
	};
	// ref @0x10076464 — y = ScreenHeight/2 + (-0x32..+0x1e), x = ScreenWidth/3.
	const int midY = ScreenHeight / 2;
	SGTC_DrawVoteCastHudLine( midY - 0x32, "#SELPREF", 100, 100, 255 );
	const int modeY[] = { midY - 0x1e, midY - 0xf, midY, midY + 0xf, midY + 0x1e };
	for( int i = 0; i < 5; i++ )
	{
		int r = 255, g = 255, b = 255;
		if( i == g_iSgtcVoteSel )
			r = 0;	// ref @0x1007646f — selected line r=0, g/b=0xff
		SGTC_DrawVoteCastHudLine( modeY[i], modeTok[i], r, g, b );
	}
}

static void SGTC_InitPortraitSlots( CHudSgtc *hud );

// ref PTR_DAT_100babe8[32] — announce / mode tokens (Concuss byte 2 indexes this).
static const char *SGTC_AnnounceTokenById( int id )
{
	static const char *const kTable[] = {
		NULL, "#KEY_COLLECTED", "#KEY_DROPPED", "#BOMB_ARMING", "#BOMB_ARMING_INTERRUPTED",
		"#BOMB_ARMED", "#CTRL_CAP_ENEMY", "#CTRL_CAP_ALLY", "#STRUCT_ATTACKED",
		"#ENEMY_CAP_KILLED", "#ALLY_CAP_KILLED", "#TOURN_START", "#TOURN_ABORTED", "#TOURN_AWARD",
		"#CTP_STND_TAU", "#CTP_STND_GOU",
		"#MODE0", "#MODE1", "#MODE2", "#MODE3", "#MODE4",
		"radio/son00.wav", "radio/son01.wav", "radio/son02.wav", "radio/son03.wav",
		"radio/son04.wav", "radio/son05.wav", "radio/son06.wav", "radio/son07.wav",
		"radio/son08.wav", "radio/son09.wav", "radio/son10.wav",
	};
	if( id < 0 || id >= (int)( sizeof( kTable ) / sizeof( kTable[0] ) ) )
		return NULL;
	return kTable[id];
}

static void SGTC_InitAnnounceLines( CHudSgtc *hud )
{
	for( int i = 0; i < SGTC_ANNOUNCE_LINES; i++ )
	{
		hud->m_szAnnounceTok[i] = NULL;
		hud->m_flAnnounceLatch[i] = 0.0f;
		hud->m_iAnnounceScroll[i] = 0;
	}
}

// ref @0x10074366 — shift queue + latch new token on DAT_100f1ce8.
static void SGTC_PushAnnounceLine( CHudSgtc *hud, float flTime, int tokenId )
{
	const char *tok = SGTC_AnnounceTokenById( tokenId );
	for( int i = 0; i < SGTC_ANNOUNCE_LINES - 1; i++ )
	{
		hud->m_szAnnounceTok[i] = hud->m_szAnnounceTok[i + 1];
		hud->m_flAnnounceLatch[i] = hud->m_flAnnounceLatch[i + 1];
		hud->m_iAnnounceScroll[i] = hud->m_iAnnounceScroll[i + 1];
	}
	const int last = SGTC_ANNOUNCE_LINES - 1;
	hud->m_szAnnounceTok[last] = tok;
	hud->m_flAnnounceLatch[last] = flTime;
	hud->m_iAnnounceScroll[last] = 0;
	gEngfuncs.pfnPlaySoundByName( "buttons/log.wav", 1.0f );
}

// ref @0x100744d4..0x10074757 — fade + DrawHudString + FillRGBA frame.
static void SGTC_DrawAnnounceScroller( CHudSgtc *hud, float flTime )
{
	if( g_iSgtcAnnouncePending > 0 )
	{
		SGTC_PushAnnounceLine( hud, flTime, g_iSgtcAnnouncePending );
		g_iSgtcAnnouncePending = 0;
	}

	const int lineH = 0xc;
	const int lineGap = 3;
	const int lineCount = SGTC_ANNOUNCE_LINES;
	const int maxW = 0x12c;
	int baseX = ScreenWidth / 2 - ( ( lineH + lineGap ) * lineCount ) * 2;
	int baseY = ScreenHeight / 5 - ( lineH * ( lineCount + lineGap ) ) / 2;
	if( baseY < 0x69 )
		baseY = 0x69;

	int minScroll = 100;
	for( int i = 0; i < lineCount; i++ )
	{
		int fade = 0;
		if( hud->m_iAnnounceScroll[i] == 0 && hud->m_flAnnounceLatch[i] != 0.0f
			&& flTime > hud->m_flAnnounceLatch[i] + 6.0f )
			fade = 1;
		if( hud->m_iAnnounceScroll[i] > 0 && hud->m_flAnnounceLatch[i] != 0.0f
			&& flTime > hud->m_flAnnounceLatch[i] + 0.08f )
			fade = 1;

		if( fade && hud->m_iAnnounceScroll[i] < 100 && hud->m_flAnnounceLatch[i] != 0.0f )
		{
			hud->m_iAnnounceScroll[i]++;
			hud->m_flAnnounceLatch[i] = flTime;
		}

		if( hud->m_iAnnounceScroll[i] < minScroll && hud->m_flAnnounceLatch[i] != 0.0f )
			minScroll = hud->m_iAnnounceScroll[i];

		if( hud->m_iAnnounceScroll[i] > 99 )
		{
			hud->m_iAnnounceScroll[i] = 0;
			hud->m_flAnnounceLatch[i] = 0.0f;
			hud->m_szAnnounceTok[i] = NULL;
		}

		if( hud->m_flAnnounceLatch[i] == 0.0f || !hud->m_szAnnounceTok[i] )
			continue;

		const int scroll = hud->m_iAnnounceScroll[i];
		const int r = 100 - scroll;
		const int g = 100 - scroll;
		const int b = ( 100 - scroll ) * 2;
		const char *text = CHudTextMessage::BufferedLocaliseTextString( hud->m_szAnnounceTok[i] );
		const int y = baseY + i * ( lineH + lineGap );
		gHUD.DrawHudString( baseX + 4, y, baseX + 4 + maxW, text, r, g, b );
	}

	const int frameA = 100 - minScroll;
	const int frameH = ( lineH + lineGap ) * lineCount + 8;
	FillRGBA( baseX, baseY - 4, 2, frameH, 0x32, 0x32, 0xff, frameA );
	FillRGBA( baseX + maxW + 6, baseY - 4, 2, frameH, 0x32, 0x32, 0xff, frameA );
	FillRGBA( baseX + 1, baseY - 4, maxW + 6, 2, 0x32, 0x32, 0xff, frameA );
	FillRGBA( baseX + 1, baseY - 4 + frameH, maxW + 6, 2, 0x32, 0x32, 0xff, frameA );
}

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
	gEngfuncs.pfnAddCommand( "votecastmenu", SGTC_VoteCastMenu_f );
	SGTC_InitPortraitSlots( this );
	SGTC_InitProgressSlotX( this );
	SGTC_InitAnnounceLines( this );
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

void CHudSgtc::Think( void )
{
	(void)0;
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

// ref CHudSgtc::Draw @0x100738a6 — tac overlay tri-strip → sprite layers (tacdisp + tac_*).
static void SGTC_TickArmorTacOverlay( CHudSgtc *hud, float flTime )
{
	if( hud->m_iArmorState == 1 )
	{
		hud->m_flArmorTacPhase = flTime;
		hud->m_iArmorState = 2;
		return;
	}
	if( hud->m_iArmorState == 3 || ( hud->m_iArmorState == 2 && hud->m_iTacDispActive ) )
	{
		hud->m_flArmorTacPhase = flTime;
		hud->m_iArmorState = 0;
	}
}

static void SGTC_DrawArmorTacOverlay( CHudSgtc *hud, float flTime, int lx, int ly )
{
	if( !hud->m_iArmorOverlay && !hud->m_iTacDispActive && hud->m_iArmorState == 0 )
		return;

	SGTC_TickArmorTacOverlay( hud, flTime );

	const int tx = lx + 36;
	const int ty = ly - 44;

	wrect_t base;
	if( SGTC_Sprite( "tacdisp", &base ) )
		SPR_DrawAdditive( 0, tx, ty, &base );

	float armorPts = hud->m_flArmorBar;
	if( hud->m_iArmorParam > 0 )
		armorPts = (float)hud->m_iArmorParam;
	if( armorPts <= 2000.0f )
	{
		if( armorPts <= 100.0f )
			armorPts *= 100.0f;
	}

	const char *layer = "tac_green";
	if( armorPts > 5000.0f )
		layer = "tac_red";
	else if( armorPts > 2000.0f )
		layer = "tac_yellow";

	int r = 0, g = 0, b = 255;
	if( hud->m_iArmorTeamTint )
	{
		g = 255;
		b = 0;
	}

	wrect_t tint;
	if( !SGTC_Sprite( layer, &tint ) )
		return;

	SPR_Set( gHUD.GetSprite( gHUD.GetSpriteIndex( layer ) ), r, g, b );
	SPR_DrawAdditive( 0, tx, ty, &tint );
}

// ref CHudSgtc::Draw @0x10073f07 — radar ring slots 1..255, alpha @+0x1158 fades by 5.
static void SGTC_DrawRadarBlips( CHudSgtc *hud, int baseX, int baseY )
{
	int blIdx = gHUD.GetSpriteIndex( "blip" );
	if( !blIdx )
		return;
	wrect_t bl = gHUD.GetSpriteRect( blIdx );

	for( int slot = 1; slot < SGTC_RADAR_SLOTS; slot++ )
	{
		if( hud->m_iRadarType[slot] <= 0 )
			continue;

		int alpha = hud->m_iRadarAlpha[slot];
		if( alpha <= 0 )
		{
			hud->m_iRadarType[slot] = 0;
			continue;
		}

		int r = alpha, g = alpha, b = alpha;
		if( hud->m_iPortraitActiveTeam == 2 )
			b = 0;

		int bx = baseX + ( hud->m_iRadarX[slot] >> 3 );
		int by = baseY + ( hud->m_iRadarY[slot] >> 3 );
		SPR_Set( gHUD.GetSprite( blIdx ), r, g, b );
		SPR_DrawAdditive( 0, bx, by, &bl );

		alpha -= 5;
		hud->m_iRadarAlpha[slot] = alpha;
		if( alpha < 1 )
			hud->m_iRadarType[slot] = 0;
	}
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

// ref FUN_1007257b — drop slots whose team != m_iPortraitActiveTeam (DAT_0x100f70c8).
static void SGTC_ExpirePortraitSlots( CHudSgtc *hud )
{
	for( int i = 0; i < SGTC_PORTRAIT_SLOTS; i++ )
	{
		if( hud->m_iPortraitSlotTeam[i] == hud->m_iPortraitActiveTeam )
			continue;

		hud->m_iPortraitSlotId[i] = 0;
		hud->m_iPortraitSlotEnt[i] = 0;
		hud->m_iPortraitSlotFlash[i] = 0;
		hud->m_iPortraitSlotGray[i] = 0x7d;
		hud->m_iPortraitSlotTeam[i] = 0;
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

// ref FUN_10072923 — portrait sprite id (1..10) → hud sprite name.
static const char *SGTC_PortraitAvatarSprite( int id )
{
	static const char *const kTable[] =
	{
		"oneill", "carter", "jackson", "tealc", "commander",
		"systemlord", "anubis", "goauld3", "snakegoauld", "horus",
	};
	if( id < 1 || id > 10 )
		return "inconnu";
	return kTable[id - 1];
}

// ref FUN_100772f2 — packed color id decode @0x100776c8..0x1007777e (threshold chain).
struct SGTC_PortraitColorFlags
{
	char c0, b8, bc, c4, c8, cc;
};

static void SGTC_DecodePortraitColorId( int colorId, SGTC_PortraitColorFlags *f )
{
	int ac = colorId;
	memset( f, 0, sizeof( *f ) );
	if( ac >= 0xc0 ) { f->cc = 2; ac -= 0xc0; }
	if( ac >= 0x60 ) { f->cc = 1; ac -= 0x60; }
	if( ac >= 0x30 ) { f->c8 = 1; ac -= 0x30; }
	if( ac >= 0x18 ) { f->c4 = 2; ac -= 0x18; }
	if( ac >= 0xc ) { f->c4 = 1; ac -= 0xc; }
	if( ac >= 0x6 ) { f->c0 = 1; ac -= 0x6; }
	if( ac >= 0x3 ) { f->bc = 1; ac -= 0x3; }
	if( ac >= 0x2 ) { f->b8 = 1; ac -= 0x2; }
}

// ref jump table @0x10077bd3 (index colorId & 0x1f, max 0x12).
static void SGTC_PortraitTextRgb( int colorId, int &r, int &g, int &b )
{
	static const unsigned char kTbl[19][3] =
	{
		{ 0xa8, 0xd3, 0xff }, { 0xff, 0x00, 0x00 }, { 0xff, 0x82, 0x00 },
		{ 0xff, 0xff, 0x00 }, { 0x82, 0xff, 0x00 }, { 0x00, 0xff, 0x00 },
		{ 0x00, 0xff, 0x82 }, { 0x00, 0xff, 0xff }, { 0x00, 0x82, 0xff },
		{ 0x00, 0x00, 0xff }, { 0x82, 0x00, 0xff }, { 0xff, 0x00, 0xff },
		{ 0xff, 0x00, 0x82 }, { 0x78, 0xaf, 0x50 }, { 0xa0, 0x78, 0x82 },
		{ 0xff, 0xff, 0xff }, { 0xff, 0x80, 0xff }, { 0x80, 0x80, 0x40 },
		{ 0x80, 0x80, 0x80 },
	};
	int idx = colorId & 0x1f;
	if( idx > 18 )
		idx = 0;
	r = kTbl[idx][0];
	g = kTbl[idx][1];
	b = kTbl[idx][2];
	if( r + g + b == 0 )	// ref @0x10077604
	{
		r = 0xa8;
		g = 0xd3;
		b = 0xff;
	}
}

// ref FUN_100772f2 — name @+0x48, py-0x10; avatar tint + flash @0x10077792; HP @0x10077a99.
static void SGTC_DrawPortraitCell( CHudSgtc *hud, int slot, int px, int py, float flTime, char dimName )
{
	const int sprId = hud->m_iPortraitSlotId[slot];
	const int ent = hud->m_iPortraitSlotEnt[slot];
	const char *label = hud->m_szPortraitSlotName[slot];
	const int colorId = hud->m_iPortraitSlotColor[slot];

	if( !sprId || !ent || !label || !label[0] )
	{
		const char *err = CHudTextMessage::BufferedLocaliseTextString( "! ERROR !" );
		gHUD.DrawHudString( px + 0x48, py + 0xc, ScreenWidth, err, 0, 0, 255 );
		return;
	}

	int r, g, b;
	SGTC_PortraitTextRgb( colorId, r, g, b );
	if( dimName )
		r = g = b = 0;

	const char *text = CHudTextMessage::BufferedLocaliseTextString( label );
	gHUD.DrawHudString( px + 0x48, py - 0x10, ScreenWidth, text, r, g, b );

	SGTC_PortraitColorFlags flags;
	SGTC_DecodePortraitColorId( colorId, &flags );

	int sr = r, sg = g, sb = b;
	if( flags.c4 == 2 )
	{
		if( hud->m_iPortraitSlotFlash[slot] > 5 )
		{
			sr = 0xf3;
			sg = 0xb7;
			sb = 0;
		}
		else
		{
			sr = 0xff;
			sg = 0xd5;
			sb = 0x53;
		}
		hud->m_iPortraitSlotFlash[slot]++;
		if( hud->m_iPortraitSlotFlash[slot] > 10 )
			hud->m_iPortraitSlotFlash[slot] = 0;
	}
	else if( flags.c4 == 1 )
	{
		int gray = hud->m_iPortraitSlotGray[slot];
		if( gray > 0x32 )
		{
			gray -= 2;
			hud->m_iPortraitSlotGray[slot] = gray;
		}
		sr = sg = sb = gray;
	}
	else if( hud->m_iPortraitSlotGray[slot] < 0x7d )
	{
		int gray = hud->m_iPortraitSlotGray[slot] + 2;
		hud->m_iPortraitSlotGray[slot] = gray;
		sr = sg = sb = gray;
	}

	if( flags.c0 )
	{
		sr = 0x78;
		sg = 0x78;
		sb = 0x32;
	}

	const char *avatar = SGTC_PortraitAvatarSprite( sprId );
	if( SGTC_Sprite( avatar, NULL ) )
	{
		wrect_t av;
		SGTC_Sprite( avatar, &av );
		SPR_Set( gHUD.GetSprite( gHUD.GetSpriteIndex( avatar ) ), sr, sg, sb );
		SPR_DrawAdditive( 0, px + 9, py + 0x10, &av );
	}

	float frac = hud->m_flPortraitSlotHpPct[slot] / 100.0f;
	if( frac < 0.0f )
		frac = 0.0f;
	if( frac > 1.0f )
		frac = 1.0f;
	if( frac <= 0.0f || dimName )
		return;

	const int barW = 0x40;
	const int barX = px + 0x42;
	const int barY = py + 0x44;
	FillRGBA( barX, barY, barW, 4, 0x1e, 0x1e, 0x1e, 255 );
	FillRGBA( barX, barY, (int)( barW * frac ), 4, 0, 0x82, 0, 255 );

	if( flags.cc == 1 )
		FillRGBA( barX, py + 0x40, barW, 2, 255, 0, 0, 255 );
	else if( flags.cc == 2 )
		FillRGBA( barX, py + 0x40, barW, 2, 0, 0x82, 0, 255 );
}

int CHudSgtc::Draw( float flTime )
{
	if( m_iHideHud )
		return 0;

	const int panelH = 168;
	const int lx = 0;
	const int ly = ScreenHeight - panelH;
	const int rx = ScreenWidth - 200;

	SGTC_DrawSprite( "hud_left", lx, ly );
	SGTC_DrawSprite( "hud_right", rx, ly );

	float life = (float)gHUD.m_Health.m_iHealth;
	SGTC_DrawBarNeedle( "life_bar", "needle", lx + 24, ly + 28, life / 100.0f );
	SGTC_DrawBarNeedle( "armor_bar", "needle", lx + 48, ly + 28, m_flArmorBar / 100.0f );
	SGTC_DrawArmorTacOverlay( this, flTime, lx, ly );
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
			SGTC_DrawRadarBlips( this, x, y );
			if( gHUD.GetSpriteIndex( "blip" ) )
			{
				wrect_t bl = gHUD.GetSpriteRect( gHUD.GetSpriteIndex( "blip" ) );
				if( m_iRadarFocusX || m_iRadarFocusY )
				{
					int fx = x + ( m_iRadarFocusX >> 3 );
					int fy = y + ( m_iRadarFocusY >> 3 );
					SPR_Set( gHUD.GetSprite( gHUD.GetSpriteIndex( "blip" ) ), 255, 255, 255 );
					SPR_DrawAdditive( 0, fx, fy, &bl );
				}
			}
		}
	}

	SGTC_DrawAnnounceScroller( this, flTime );
	SGTC_TickTicketDock( this, flTime );
	SGTC_DrawTicketDock( this, flTime );
	SGTC_TickProgressTeamUi( this );
	SGTC_DrawProgressTeamUi( this );

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
		const int portX = ScreenWidth - 0xc6;	// ref @0x10073acd
		const int portY = ScreenHeight - 0xa1;	// ref @0x10073adc
		SGTC_DrawSprite( "port_left", portX, portY );
		SGTC_DrawSprite( "port_mid", portX + 16, portY );
		SGTC_DrawSprite( "port_right", portX + 96, portY );

		SGTC_ExpirePortraitSlots( this );

		cvar_t *cvSeeOwn = SGTC_Cvar( "view_seeownport" );
		int col = 0;
		for( int i = 0; i < SGTC_PORTRAIT_SLOTS; i++ )
		{
			if( !m_iPortraitSlotEnt[i] )
				continue;
			if( !m_szPortraitSlotName[i][0] )
				continue;
			if( m_iPortraitSlotId[i] <= 0 )
				continue;
			if( m_iPortraitSlotTeam[i] <= 0 )
				continue;
			if( m_iPortraitSlotTeam[i] != m_iPortraitActiveTeam )
				continue;
			if( m_iPortraitSlotEnt[i] == m_iPortraitEnt && cvSeeOwn && cvSeeOwn->value != 0.0f )
				continue;

			col++;
			const int px = ScreenWidth - ( col * 0x50 + 0x10 );	// ref @0x10073c70
			// ref @0x10073c84 — gray cell backdrop before FUN_100772f2.
			const char *graySpr = SGTC_PortraitAvatarSprite( m_iPortraitSlotId[i] );
			if( SGTC_Sprite( graySpr, NULL ) )
			{
				wrect_t gr;
				SGTC_Sprite( graySpr, &gr );
				const int g = m_iPortraitSlotGray[i] & 0xff;
				SPR_Set( gHUD.GetSprite( gHUD.GetSpriteIndex( graySpr ) ), g, g, g );
				SPR_DrawAdditive( 0, px, portY, &gr );
			}
			SGTC_DrawPortraitCell( this, i, px, portY, flTime, 0 );
			if( px <= 0 )
				break;
		}
	}

	SGTC_TickBlipPulse( this );
	SGTC_TickBlipBarProgress( this, flTime );

	// ref pbar block @0x10075156: pbare is the gray container, pbar is the top-drained fill.
	if( m_iBlipProgress > 0 && m_iBlipProgress < 0x65 )
	{
		const int bgIdx = gHUD.GetSpriteIndex( "pbare" );
		if( bgIdx > 0 )
		{
			// ref @0x1007517c: bgX = ScreenWidth/2 - 0x47, bgY = ScreenHeight - ScreenHeight/4 - 7.
			wrect_t bg = gHUD.GetSpriteRect( bgIdx );
			const int bgX = ScreenWidth / 2 - 0x47;
			const int bgY = ScreenHeight - ScreenHeight / 4 - 7;
			SPR_Set( gHUD.GetSprite( bgIdx ), 120, 120, 120 ); // ref @0x100751ef: push 0x78,0x78,0x78
			SPR_DrawHoles( 0, bgX, bgY, &bg ); // ref @0x1007520b: call ds:0x100ed728 (SPR_DrawHoles)
		}

		const int fIdx = gHUD.GetSpriteIndex( "pbar" );
		if( fIdx > 0 )
		{
			// ref @0x10075222: fillX = ScreenWidth/2 - 0x40, fillY = ScreenHeight - ScreenHeight/4.
			wrect_t fr = gHUD.GetSpriteRect( fIdx );
			const int fillX = ScreenWidth / 2 - 0x40;
			const int fillY = ScreenHeight - ScreenHeight / 4;

			int p = m_iBlipProgress;
			if( p < 0 )
				p = 0;
			const double rem = ( 100.0 - p ) / 100.0; // ref fsubr/fdiv ds:0x100a1d30 (100.0)

			// ref @0x10075296: rect.top -= (int)(rect.top * rem); left/right/bottom untouched.
			fr.top = fr.top - (int)( fr.top * rem );

			// ref @0x100752db: SPR_Set( h, (int)(180*rem), (int)(180-180*rem), 0 ).
			const int r = (int)( 180.0f * rem );               // 180.0f @ds:0x1009eff8
			const int g = (int)( 180.0f - 180.0f * rem );
			SPR_Set( gHUD.GetSprite( fIdx ), r, g, 0 );
			SPR_DrawHoles( 0, fillX, fillY, &fr ); // ref @0x10075315: call ds:0x100ed728 (SPR_DrawHoles)
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
			const int c = m_iBlipPulse + 100;
			SGTC_DrawVotePromptLine( "#PLZVOTE", c, c, 255 );
		}
		if( flTime >= m_flKeyTime )
		{
			const int c = m_iBlipPulse + 100;
			SGTC_DrawVotePromptLine( "#CAMPWARN", 255, c, c );
		}
	}

	SGTC_DrawVoteCastMenu( flTime );

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
	m_flArmorTacPhase = 0;
	m_iBlipPulse = 0;
	m_iBlipProgress = 0;
	m_flBlipProgressLatch = 0;
	m_cBlipPulseDir = 0;
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
	m_iTicketDockFrame = 0;
	m_iTicketSlideOffset = 0;
	m_iTicketSlideRemain = 0;
	m_bTicketDockLayout78 = 0;
	m_cTicketSlideDir = 0;
	m_iProgress = 0;
	m_iProgressIcon = 0;
	memset( m_iProgressTeam, 0, sizeof( m_iProgressTeam ) );
	m_iProgressEnd = 0;
	memset( m_iProgressTeamUiState, 0, sizeof( m_iProgressTeamUiState ) );
	m_iProgressIconCursor = 0;
	SGTC_InitProgressSlotX( this );
	SGTC_InitAnnounceLines( this );
	SGTC_InitPortraitSlots( this );
	g_iSgtcAnnouncePending = 0;
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
	if( m_iTicketModeFlag )
	{
		m_iTicketDockFrame = 0;
		m_iTicketSlideOffset = 0;
		m_iTicketSlideRemain = 0xc00;	// ref DAT_100f6fb0 initial @0x10074ad7
		m_bTicketDockLayout78 = 0;
	}
	m_iFlags |= HUD_ACTIVE;
	return 1;
}

// ref FUN_10076bce — HudInfoMode → +0x54
int CHudSgtc::MsgFunc_HudInfoMode( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	m_iHudInfoMode = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;
	return 1;
}

// ref FUN_100592cf (Concuss, 2 bytes) — byte0 concussion when byte2==0; byte1 → DAT_100f1ce8.
int CHudSgtc::MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	(void)pszName;
	BEGIN_READ( pbuf, iSize );
	const int concuss = READ_BYTE();
	g_iSgtcAnnouncePending = READ_BYTE();
	if( g_iSgtcAnnouncePending == 0 )
	{
		char one = (char)concuss;
		return gHUD.MsgFunc_Concuss( pszName, 1, &one );
	}
	m_iFlags |= HUD_ACTIVE;
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
	{
		m_iBlipProgress = 0;
		memset( m_iProgressTeamUiState, 0, sizeof( m_iProgressTeamUiState ) );
		m_iProgressIconCursor = 0;
	}
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
