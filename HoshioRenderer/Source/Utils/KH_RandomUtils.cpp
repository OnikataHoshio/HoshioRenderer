#include "KH_RandomUtils.h"

float KH_RandomUtils::RandomFloat()
{
	return Distribution(Gen);
}

glm::vec3 KH_RandomUtils::RandomVec3()
{
	glm::vec3 RandomVec3;
	do {
		RandomVec3 = 2.0f * glm::vec3(RandomFloat(), RandomFloat(), RandomFloat()) - 1.0f;
	} while (glm::dot(RandomVec3, RandomVec3) > 1.0f);
	return glm::normalize(RandomVec3);
}

glm::vec3 KH_RandomUtils::RandomDirection(glm::vec3& Normal)
{
	glm::vec3 Direction = glm::normalize(Normal + RandomVec3());
	return Direction;
}
