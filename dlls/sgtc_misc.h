// sgtc_misc.h — declarations for entities defined in sgtc_misc.cpp that other
// translation units need.
//
// CEntSpawn — factory `entity_spawn_round` (ref CEntSpawn @0x195bea Spawn /
// @0x195ba6 Event in ref/stc_i386.so). CStargateTeamDM::NewRound() walks every
// "entity_spawn_round" entity and invokes Event() on each one so the map's
// round-scoped spawn points re-create their named entity
// (ref NewRound @0x184f42: UTIL_FindEntityByClassname(...,"entity_spawn_round")
// -> CEntSpawn::Event(pEnt)).

#ifndef SGTC_MISC_H
#define SGTC_MISC_H

class CEntSpawn : public CBaseEntity
{
public:
	void Spawn( void );
	void Event( void );

	// ref +0x19c — STRING( pev->message ) resolved to a char* and cached in
	// Spawn; Event() re-creates this classname at the marker's origin/angles.
	const char *m_pSpawnClass;
};

#endif // SGTC_MISC_H
