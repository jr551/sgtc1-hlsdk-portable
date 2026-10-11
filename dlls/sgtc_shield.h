// sgtc_shield.h — personal shield wall entity (ref CShield @0x162388 / effect_shield).

#ifndef SGTC_SHIELD_H
#define SGTC_SHIELD_H

class CShield : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT Think( void );

	float m_flExpire;	// ref +0x19c — think gate vs gpGlobals->time
};

#endif // SGTC_SHIELD_H
