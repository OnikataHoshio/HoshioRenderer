#pragma once

#include "KH_Common.h"

struct KH_HitResult {
public:
	bool IsHit = false;
	float Distance = std::numeric_limits<float>::max();
	glm::vec3 HitPoint = glm::vec3();
};