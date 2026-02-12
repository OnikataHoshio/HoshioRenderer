#pragma once

#include "KH_Common.h"

class KH_Ray;

class KH_AABB
{
public:
	KH_AABB(glm::vec3 MinPos = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 MaxPos = glm::vec3(0.0f, 0.0f, 0.0f));

	float Hit(KH_Ray& Ray);

	bool CheckOverlap(KH_AABB& Other);

	glm::vec3 GetSize() const;

	glm::vec3 GetCenter() const;

	glm::vec3 GetScale() const;

	glm::mat4 GetModelMatrix() const;

	glm::vec3 MinPos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 MaxPos = glm::vec3(0.0f, 0.0f, 0.0f);

};

