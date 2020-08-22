#include "AssaultRifle.h"

AssaultRifle::AssaultRifle()
{

}

AssaultRifle::~AssaultRifle()
{

}

void AssaultRifle::Init(void)
{
	currentAmmoRound = 30;
	maxAmmoRound = 30;
	currentTotalAmmo = 120;
	maxTotalAmmo = 120;
	ammoToReload = 0;
	canShoot = true;
	elapsedTime = 0.0;
	timeBetweenShots = 350.0;
}