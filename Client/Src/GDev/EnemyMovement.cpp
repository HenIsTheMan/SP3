#include "EnemyMovement.h"

EnemyMovement::EnemyMovement(void):
	currentState(1)
{
}

glm::vec3 EnemyMovement::UpdateMovement(glm::vec3 Pos, glm::vec3 playerPos)
{
	float fDistToPlayer = glm::length(playerPos - Pos);
	glm::vec3 front;

	if (fDistToPlayer < 15.f)
	{
		currentState = 1;
	}
	else
	{
		currentState = 0;
	}

	switch (finiteState[currentState])
	{
	case (int)FSM::PATROL:
	{

		break;
	}

	case (int)FSM::ATTACK:
	{
		front = glm::normalize(glm::vec3(playerPos - Pos));
		return (front * Pos * 5.0f);
		break;
	}

	}
}

EnemyMovement::~EnemyMovement(void)
{
}
