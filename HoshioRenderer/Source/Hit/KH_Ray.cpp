#include "KH_Ray.h"

KH_Ray::KH_Ray(glm::vec3 Start, glm::vec3 Direction)
	:Start(Start)
{
	this->Direction = glm::normalize(Direction);
}
