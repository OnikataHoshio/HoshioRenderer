#pragma once

#include "KH_Common.h"

struct KH_HitResult {
public:
	bool bIsHit = false;
	float Distance = std::numeric_limits<float>::max();
	glm::vec3 HitPoint = glm::vec3();
	glm::vec3 Normal = glm::vec3();
};

class KH_Ray
{
public:
	KH_Ray(glm::vec3 Start = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 Direction = glm::vec3(1.0f, 0.0f, 0.0f));
	glm::vec3 Start;
	glm::vec3 Direction;
};
