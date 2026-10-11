// sgtc_weapons_client.cpp — StargateTC client weapon prediction table (NEW).
//
// See sgtc_weapons_client.h for truth sources. Ammo registry names marked
// TBD need the server peer (EventualPlanarian sgtc_weapons.*) to confirm the
// AddAmmoNameToAmmoRegistry strings; slots/IDs/models/events are exact.

#include "hud.h"
#include "cl_util.h"
#include "sgtc_weapons_client.h"

// Ammo names: recovered where the .so keeps them adjacent to the link
// strings — Energie2 (zat), m16 ammo (m16), Larve (larve). Remainder TBD.
const SGTCClientWeaponInfo g_SGTCClientWeapons[] =
{
	// link, id, slot, pos, ammo1, maxAmmo1, maxClip, v_model, p_model, event
	{ "weapon_batsup", SGTC_WPN_BATSUP, 0, 2, NULL, -1, 0, "models/v_batsup.mdl", "models/p_batsup.mdl", "events/batsup.sc" },
	{ "weapon_coujaf", SGTC_WPN_COUJAF, 0, 0, NULL, -1, 0, "models/v_coujaf.mdl", "models/p_coujaf.mdl", "events/coujaf.sc" },
	{ "weapon_couteau", SGTC_WPN_COUTEAU, 0, 1, NULL, -1, 0, "models/v_couteau.mdl", "models/p_couteau.mdl", "events/couteau.sc" },
	{ "weapon_m92s", SGTC_WPN_M92S, 3, 3, "TBD_m92s", 60, 10, "models/v_m92s.mdl", "models/p_m92s.mdl", "events/eagle.sc" },
	{ "weapon_flashbang", SGTC_WPN_FLASHBANG, 5, 5, NULL, 10, 10, "models/v_grenade.mdl", "models/p_grenade.mdl", NULL },
	{ "weapon_gacgoa", SGTC_WPN_GACGOA, 4, 4, "TBD_gacgoa", 4, 10, "models/v_gacgoa.mdl", "models/p_gacgoa.mdl", NULL },
	{ "weapon_glstaff", SGTC_WPN_BIGLANCE, 5, 5, "TBD_biglance", 22, 20, "models/v_deathglider.mdl", "models/p_deathglider.mdl", "events/biglance.sc" },
	{ "weapon_goaregen", SGTC_WPN_GOAREGEN, 4, 4, "TBD_goaregen", 150, 0, "models/v_goaregen.mdl", "models/p_goaregen.mdl", NULL },
	{ "weapon_handgrenade", SGTC_WPN_HANDGRENADE, 6, 1, NULL, 3, 5, "models/v_grenade.mdl", "models/p_grenade.mdl", NULL },
	{ "weapon_invis", SGTC_WPN_INVIS, 3, 3, "TBD_invis", 10, -10, "models/v_invis.mdl", "models/p_invis.mdl", "events/invis.sc" },
	{ "weapon_lance", SGTC_WPN_LANCE, 2, 2, "TBD_lance", 22, 15, "models/v_lancegoa.mdl", "models/p_lancegoa.mdl", "events/lance.sc" },
	{ "weapon_larve", SGTC_WPN_LARVE, 1, 1, "Larve", 2, 10, "models/v_larve.mdl", "models/p_larve.mdl", "events/larve.sc" },
	{ "weapon_m16", SGTC_WPN_M16, 1, 1, "m16 ammo", 210, 15, "models/v_m16.mdl", "models/p_m16.mdl", "events/m16.sc" },
	{ "weapon_main", SGTC_WPN_MAIN, 1, 1, "TBD_main", 100, 25, "models/v_main.mdl", "models/p_main.mdl", "events/main.sc" },
	{ "weapon_medic", SGTC_WPN_MEDIC, 0, 0, "TBD_medic", 200, 1, "models/v_medic.mdl", "models/p_medic.mdl", NULL },
	{ "weapon_mine", SGTC_WPN_TRIPMINE, 3, 3, NULL, 2, -10, "models/v_mine.mdl", "models/p_mine.mdl", NULL },
	{ "weapon_mp5", SGTC_WPN_MP5, 2, 2, "TBD_mp5", 200, 15, "models/v_mp5.mdl", "models/p_mp5.mdl", "events/mp5.sc" },
	{ "weapon_mustardgrenade", SGTC_WPN_MUSTARDGRENADE, 5, 5, NULL, 1, 5, "models/v_mustardgrenade.mdl", "models/p_mustardgrenade.mdl", NULL },
	{ "weapon_narcogrenade", SGTC_WPN_NARCOGRENADE, 4, 4, NULL, 1, 5, "models/v_narcogrenade.mdl", "models/p_narcogrenade.mdl", NULL },
	{ "weapon_nervegrenade", SGTC_WPN_NERVEGRENADE, 3, 3, NULL, 1, 5, "models/v_nervegrenade.mdl", "models/p_nervegrenade.mdl", NULL },
	{ "weapon_p90", SGTC_WPN_P90, 0, 0, "TBD_p90", 250, 15, "models/v_p90.mdl", "models/p_p90.mdl", "events/p90.sc" },
	{ "weapon_psg1", SGTC_WPN_PSG1, 4, 4, "TBD_psg1", 50, 15, "models/v_psg1.mdl", "models/p_psg1.mdl", "events/psg1.sc" },
	{ "weapon_reetou", SGTC_WPN_REETOU, 4, 4, "TBD_reetou", 10, -10, "models/v_reetou.mdl", "models/p_reetou.mdl", "events/reetou.sc" },
	{ "weapon_smokegrenade", SGTC_WPN_SMOKEGRENADE, 2, 2, NULL, 1, 5, "models/v_smokegrenade.mdl", "models/p_smokegrenade.mdl", NULL },
	{ "weapon_spas", SGTC_WPN_SHOTGUN_STOCK, 3, 3, "TBD_spas", 125, 14, "models/v_spas.mdl", "models/p_spas.mdl", "events/spas.sc" },
	{ "weapon_tacgun", SGTC_WPN_TAC, 2, 2, "TBD_tac", 3, -10, "models/v_tac.mdl", "models/p_tac.mdl", "events/tac.sc" },
	{ "weapon_usas", SGTC_WPN_USAS, 5, 5, "TBD_usas", 125, 20, "models/v_usas.mdl", "models/p_usas.mdl", "events/usas.sc" },
	{ "weapon_zat", SGTC_WPN_ZAT, 2, 2, "Energie2", 100, 15, "models/v_zat.mdl", "models/p_zat.mdl", "events/zata.sc" },
	{ "weapon_zatarc", SGTC_WPN_ZATARC, 0, 0, "TBD_zatarc", 40, 15, "models/v_zar.mdl", "models/p_zar.mdl", "events/zatarc.sc" },
	{ "weapon_c4", SGTC_WPN_SATCHEL, 2, 2, NULL, 2, -10, "models/v_c4.mdl", "models/p_c4.mdl", NULL },
	{ "weapon_9mmAR", SGTC_WPN_9MMAR, 2, 2, "TBD_9mmAR", 200, 15, "models/v_9mmAR.mdl", "models/p_9mmAR.mdl", "events/mp5.sc" },
};

const int g_SGTCClientWeaponCount =
	sizeof( g_SGTCClientWeapons ) / sizeof( g_SGTCClientWeapons[0] );

void SGTC_ClientWeaponsInit( void )
{
	// MAX_WEAPONS is 32 (cdll_dll.h, shared game+client). SGTC uses IDs
	// 0-30 with 31 rows — fits; the table is advisory until the
	// integrator wires prediction entities into hl_weapons.cpp.
	if( g_SGTCClientWeaponCount > 31 )
		gEngfuncs.Con_Printf( "SGTC: weapon table overflow %d\n", g_SGTCClientWeaponCount );
	else
		gEngfuncs.Con_Printf( "SGTC: %d client weapons registered\n", g_SGTCClientWeaponCount );
}

const SGTCClientWeaponInfo *SGTC_ClientWeaponById( int iId )
{
	for( int i = 0; i < g_SGTCClientWeaponCount; i++ )
		if( g_SGTCClientWeapons[i].iId == iId )
			return &g_SGTCClientWeapons[i];
	return NULL;
}
