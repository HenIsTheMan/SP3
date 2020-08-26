#include "SniperRifle.h"

SniperRifle::SniperRifle()
{

}

SniperRifle::~SniperRifle()
{

}

void SniperRifle::Init(void)
{
	currentAmmoRound = 20;
	maxAmmoRound = 20;
	currentTotalAmmo = 80;
	maxTotalAmmo = 80;
	ammoToReload = 0;
	canShoot = true;
	elapsedTime = 0.0;
	timeBetweenShots = 30.0;
	reloadTime=30.0;
	reloading=false;
}