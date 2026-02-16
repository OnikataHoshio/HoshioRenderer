#pragma once

#include "Hit/KH_AABB.h"

struct KH_HitResult;
class KH_Ray;

struct KH_TriangleHitInfo
{
	bool bIsHit = false;
	float w1 = 0.0f, w2 = 0.0f, w3 = 0.0f;
};

class KH_IShape {
protected:
	KH_AABB AABB;
public:
	KH_IShape() = default;
	virtual ~KH_IShape() = default;
	virtual KH_HitResult Hit(KH_Ray& Ray) = 0;
	const KH_AABB& GetAABB() const;
};

struct KH_Triangle : public KH_IShape
{
public:
	glm::vec3 P1, P2, P3;
	glm::vec3 N1, N2, N3;
	glm::vec3 Center;
	glm::vec3 Normal;

	KH_Triangle();
	KH_Triangle(glm::vec3 P1, glm::vec3 P2, glm::vec3 P3);
	KH_Triangle(glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, glm::vec3 N1, glm::vec3 N2, glm::vec3 N3);

	KH_HitResult Hit(KH_Ray& Ray) override;

	static bool Cmpx(const KH_Triangle& t1, const KH_Triangle& t2);

	static bool Cmpy(const KH_Triangle& t1, const KH_Triangle& t2);

	static bool Cmpz(const KH_Triangle& t1, const KH_Triangle& t2);

private:
	KH_TriangleHitInfo CheckHitInfo(glm::vec3 HitPoint);
};




