#pragma once

#include "KH_Common.h"

class KH_Ray;

struct KH_AABBHitInfo
{
	float HitTime = std::numeric_limits<float>::max();
	bool bIsHit = false;
};

class KH_AABB
{
public:
	KH_AABB(glm::vec3 MinPos = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 MaxPos = glm::vec3(0.0f, 0.0f, 0.0f));

	KH_AABBHitInfo Hit(KH_Ray& Ray);

	bool CheckOverlap(KH_AABB& Other);

	glm::vec3 GetSize() const;

	glm::vec3 GetCenter() const;

	glm::vec3 GetScale() const;

	glm::mat4 GetModelMatrix() const;

	float GetSurfaceArea() const;

	static float ComputeSurfaceArea(glm::vec3 MinPos, glm::vec3 MaxPos);

	glm::vec3 MinPos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 MaxPos = glm::vec3(0.0f, 0.0f, 0.0f);

};

