#pragma once
#include<Core.h>
class EnemyMovement
{
public:
	EnemyMovement(void);
	virtual glm::vec3 UpdateMovement(glm::vec3 pos, glm::vec3 playerPos);
	virtual ~EnemyMovement(void);

protected:
	enum struct FSM
	{
		PATROL,
		ATTACK,
		NUM_FSM
	};
	int finiteState[(int)FSM::NUM_FSM];

	int currentState;
};