#pragma once

#include "KH_Common.h"

class KH_Ray
{
public:
	KH_Ray(glm::vec3 Start, glm::vec3 Direction);
	glm::vec3 Start = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 Direction = glm::vec3(1.0f, 0.0f, 0.0f);
};
