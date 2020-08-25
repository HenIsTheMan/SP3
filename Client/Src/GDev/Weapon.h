#pragma once

#include "Entity.h"

class Weapon
{
public:
	Weapon();
	~Weapon();

	void Update(const double dt);
	void Reload();

	// Setters
	void SetInventory(const int slot, Weapon* weapon);
	void SetCurrentSlot(const int slot);
	void SetCurrentAmmoRound(int currentAmmoRound);
	void SetMaxAmmoRound(int maxAmmoRound);
	void SetCurrentTotalAmmo(int currentTotalAmmo);
	void SetMaxTotalAmmo(int maxTotalAmmo);
	void SetAmmoToReload(int ammoToReload);
	void SetTimeBetweenShots(double timeBetweenShots);
	void SetElapsedTime(double elapsedTime);
	void SetReloadTime(double reloadTime);
	void SetCanShoot(bool canShoot);
	void SetReloading(bool reloading);

	// Getters
	Weapon* GetCurrentWeapon(void) const;
	int GetCurrentSlot(void) const;
	int GetCurrentAmmoRound(void);
	int GetMaxAmmoRound(void);
	int GetCurrentTotalAmmo(void);
	int GetMaxTotalAmmo(void);
	int GetAmmoToReload(void);
	double GetTimeBetweenShots(void);
	double GetElapsedTime(void);
	double GetReloadTime(void);
	bool GetCanShoot(void);
	bool GetReloading(void);

protected:
	double elapsedTime;
	double timeBetweenShots;
	double reloadTime;

	Weapon* primaryWeapon;
	Weapon* secondaryWeapon;
	Weapon* extraWeapon;
	int currentSlot;

	int currentAmmoRound; // Current ammo in the magazine
	int maxAmmoRound; // Max ammo in the magazine
	int currentTotalAmmo; // Current total ammo in the gun
	int maxTotalAmmo; // Max ammo in the gun
	int ammoToReload; // Num of ammo to reload to max ammo in the round
	bool canShoot; // Check whether player can shoot the weapon
	bool reloading; // Check whether player is still reloading the weapon
};