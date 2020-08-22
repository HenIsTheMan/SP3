#include "Pistol.h"

Pistol::Pistol()
{

}

Pistol::~Pistol()
{

}

void Pistol::Init(void)
{
	currentAmmoRound = 12;
	maxAmmoRound = 12;
	currentTotalAmmo = 60;
	maxTotalAmmo = 60;
	ammoToReload = 0;
	canShoot = true;
	elapsedTime = 0.0;
	timeBetweenShots = 1000.0;
}