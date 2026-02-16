#pragma once

#include "KH_Common.h"


class KH_RandomUtils : public KH_Singleton<KH_RandomUtils>
{
	friend class KH_Singleton<KH_RandomUtils>;

private:
	std::random_device RandomDevice;
	std::mt19937 Gen{ RandomDevice() };
	std::uniform_real_distribution<float> Distribution{ 0.0, 1.0 };

public:
	float RandomFloat();

	glm::vec3 RandomVec3();

	glm::vec3 RandomDirection(glm::vec3& Normal);
};