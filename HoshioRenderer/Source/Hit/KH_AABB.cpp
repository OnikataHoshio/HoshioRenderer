#include "KH_AABB.h"

#include "KH_Ray.h"

KH_AABB::KH_AABB(glm::vec3 MinPos, glm::vec3 MaxPos)
	:MinPos(MinPos), MaxPos(MaxPos)
{

}

glm::vec3 KH_AABB::GetSize() const
{
	return MaxPos - MinPos;
}

glm::vec3 KH_AABB::GetCenter() const
{
	return (MinPos + MaxPos) / 2.0f;
}

glm::vec3 KH_AABB::GetScale() const
{
	return GetSize() / 2.0f;
}

glm::mat4 KH_AABB::GetModelMatrix() const
{
	glm::mat4 Model = glm::mat4(1.0f);

	glm::vec3 Position = GetCenter();
	glm::vec3 Scale = GetScale();

	Model = glm::translate(Model, Position);
	Model = glm::scale(Model, Scale);

	return Model;
}

float KH_AABB::GetSurfaceArea() const
{
	glm::vec3 Size = GetSize();
	float Area = Size.x * Size.y * 2;
	Area += Size.x * Size.z * 2;
	Area += Size.y * Size.z * 2;
	return Area;
}

float KH_AABB::ComputeSurfaceArea(glm::vec3 MinPos, glm::vec3 MaxPos)
{
	glm::vec3 Size = MaxPos - MinPos;
	float Area = Size.x * Size.y * 2;
	Area += Size.x * Size.z * 2;
	Area += Size.y * Size.z * 2;
	return Area;
}


KH_AABBHitInfo KH_AABB::Hit(KH_Ray& Ray)
{
	glm::vec3 SafeDir = Ray.Direction;
	SafeDir.x = (std::abs(SafeDir.x) < EPS) ? (SafeDir.x > 0 ? EPS : -EPS) : SafeDir.x;
	SafeDir.y = (std::abs(SafeDir.y) < EPS) ? (SafeDir.y > 0 ? EPS : -EPS) : SafeDir.y;
	SafeDir.z = (std::abs(SafeDir.z) < EPS) ? (SafeDir.z > 0 ? EPS : -EPS) : SafeDir.z;

	glm::vec3 invdir = 1.0f / SafeDir;

	glm::vec3 tMinSlab = (MinPos - Ray.Start) * invdir;
	glm::vec3 tMaxSlab = (MaxPos - Ray.Start) * invdir;

	glm::vec3 tMax = glm::max(tMinSlab, tMaxSlab);
	glm::vec3 tMin = glm::min(tMinSlab, tMaxSlab);

	float t0 = std::fmax(tMin.x, std::fmax(tMin.y, tMin.z));
	float t1 = std::fmin(tMax.x, std::fmin(tMax.y, tMax.z));


	KH_AABBHitInfo HitInfo;
	HitInfo.bIsHit = true;

	if (t0 > t1 || t1 < 0.0f)
	{
		HitInfo.bIsHit = false;
	}

	if (t0 < EPS)
	{
		HitInfo.HitTime = t1;
	}
	else
	{
		HitInfo.HitTime = t0;
	}

	return HitInfo;
}

bool KH_AABB::CheckOverlap(KH_AABB& Other)
{
	if (MinPos.x > Other.MaxPos.x || MaxPos.x < Other.MinPos.x) return false;
	if (MinPos.y > Other.MaxPos.y || MaxPos.y < Other.MinPos.y) return false;
	if (MinPos.z > Other.MaxPos.z || MaxPos.z < Other.MinPos.z) return false;
	return true;
}

