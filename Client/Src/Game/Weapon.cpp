#include "Weapon.h"

Weapon::Weapon() : 
	primaryWeapon(NULL),
	secondaryWeapon(NULL),
	extraWeapon(NULL),
	currentSlot(0),
	currentAmmoRound(12),
	maxAmmoRound(12),
	currentTotalAmmo(60),
	maxTotalAmmo(60),
	ammoToReload(0),
	canShoot(true),
	elapsedTime(0.0),
	timeBetweenShots(1.0)
{
}

Weapon::~Weapon(){
	if(primaryWeapon){
		delete primaryWeapon;
		primaryWeapon = nullptr;
	}
	if(secondaryWeapon){
		delete secondaryWeapon;
		secondaryWeapon = nullptr;
	}
	if(extraWeapon){
		delete extraWeapon;
		extraWeapon = nullptr;
	}
}

void Weapon::Update(const double dt)
{
	if (canShoot)
		return;

	elapsedTime += dt;
	if (elapsedTime > timeBetweenShots)
	{
		canShoot = true;
		elapsedTime = 0.0;
	}
}

void Weapon::Reload()
{
	if (GetCurrentAmmoRound() == GetMaxAmmoRound() || GetCurrentTotalAmmo() == 0) // Do not need to reload
		return;

	// If the amount of ammo needed to reload to max is lesser than or equals to the extra ammo
	if (GetMaxAmmoRound() - GetCurrentAmmoRound() <= GetCurrentTotalAmmo())
	{
		// Num ammo to reload to max in the round
		SetAmmoToReload(GetMaxAmmoRound() - GetCurrentAmmoRound());
		// Reload to max ammo in the round
		SetCurrentAmmoRound(GetMaxAmmoRound());
	}
	else // If the amount of ammo needed to reload to max is more than the extra ammo
	{
		// Num ammo to reload is the extra ammo
		SetAmmoToReload(GetCurrentTotalAmmo());
		// Reload the remaining ammo in the round
		SetCurrentAmmoRound(GetCurrentAmmoRound() + GetAmmoToReload());
	}
	// Minus the total ammo in the gun
	SetCurrentTotalAmmo(GetCurrentTotalAmmo() - GetAmmoToReload());
}

void Weapon::SetTimeBetweenShots(double timeBetweenShots)
{
	this->timeBetweenShots = timeBetweenShots;
}

void Weapon::SetInventory(const int slot, Weapon* weapon)
{
	if (slot == 0)
		primaryWeapon = weapon;
	else if (slot == 1)
		secondaryWeapon = weapon;
	else if (slot == 2)
		extraWeapon = weapon;
}

void Weapon::SetCurrentSlot(const int slot)
{
	this->currentSlot = slot;
}

void Weapon::SetCurrentAmmoRound(int currentAmmoRound)
{
	this->currentAmmoRound = currentAmmoRound;
}

void Weapon::SetMaxAmmoRound(int maxAmmoRound)
{
	this->maxAmmoRound = maxAmmoRound;
}

void Weapon::SetCurrentTotalAmmo(int currentTotalAmmo)
{
	this->currentTotalAmmo = currentTotalAmmo;
}

void Weapon::SetMaxTotalAmmo(int maxTotalAmmo)
{
	this->maxTotalAmmo = maxTotalAmmo;
}

void Weapon::SetAmmoToReload(int ammoToReload)
{
	this->ammoToReload = ammoToReload;
}

void Weapon::SetCanShoot(bool canShoot)
{
	this->canShoot = canShoot;
}

double Weapon::GetTimeBetweenShots(void)
{
	return timeBetweenShots;
}

Weapon* Weapon::GetCurrentWeapon(void) const
{
	if (currentSlot == 0)
		return primaryWeapon;
	else if (currentSlot == 1)
		return secondaryWeapon;
	else if (currentSlot == 2)
		return extraWeapon;

	return NULL;
}

int Weapon::GetCurrentSlot(void) const
{
	return currentSlot;
}

int Weapon::GetCurrentAmmoRound(void)
{
	return currentAmmoRound;
}

int Weapon::GetMaxAmmoRound(void)
{
	return maxAmmoRound;
}

int Weapon::GetCurrentTotalAmmo(void)
{
	return currentTotalAmmo;
}

int Weapon::GetMaxTotalAmmo(void)
{
	return maxTotalAmmo;
}

int Weapon::GetAmmoToReload(void)
{
	return ammoToReload;
}

bool Weapon::GetCanShoot(void)
{
	return canShoot;
}