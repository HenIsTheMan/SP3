#include "EnemyMovement.h"

EnemyMovement::EnemyMovement(void):
	currentState(0)
{
}

void EnemyMovement::UpdateMovement(glm::vec3 Pos, glm::vec3 playerPos)
{
	float fDistToPlayer = glm::length(playerPos - Pos);
	if (fDistToPlayer > 15.f)
	{
		currentState = 0;
	}
	else
	{
		currentState = 1;
	}

	switch (finiteState[currentState])
	{
	case (int)FSM::PATROL:
	{

		break;
	}

	case (int)FSM::ATTACK:
	{

		break;
	}

	}
}

EnemyMovement::~EnemyMovement(void)
{
}
