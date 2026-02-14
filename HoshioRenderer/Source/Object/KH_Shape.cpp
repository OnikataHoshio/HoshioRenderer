#include "KH_Shape.h"
#include "Hit/KH_HitResult.h"
#include "Hit/KH_Ray.h"

const KH_AABB& KH_IShape::GetAABB() const
{
    return AABB;
}

KH_Triangle::KH_Triangle()
{
    P1 = glm::vec3(0.0f, 0.0f, 0.0f);
    P2 = glm::vec3(0.0f, 0.0f, 0.0f);
    P3 = glm::vec3(0.0f, 0.0f, 0.0f);
    Center = glm::vec3(0.0f, 0.0f, 0.0f);
    Normal = glm::vec3(0.0f, -1.0f, 0.0f);

    AABB.MinPos = glm::min(P1, glm::min(P2, P3));
    AABB.MaxPos = glm::max(P1, glm::max(P2, P3));
}

KH_Triangle::KH_Triangle(glm::vec3 P1, glm::vec3 P2, glm::vec3 P3)
	: P1(P1), P2(P2), P3(P3)
{
    Center = (P1 + P2 + P3) / 3.0f;

	const glm::vec3 P1P2 = P2 - P1;
    const glm::vec3 P1P3 = P3 - P1;

    Normal = glm::normalize(glm::cross(P1P2, P1P3));

    AABB.MinPos = glm::min(P1, glm::min(P2, P3));
    AABB.MaxPos = glm::max(P1, glm::max(P2, P3));
}

KH_HitResult KH_Triangle::Hit(KH_Ray& Ray)
{
    KH_HitResult HitResult;

    float Nom = glm::dot(Normal, P1) - glm::dot(Ray.Start, Normal);
    float Denom = glm::dot(Ray.Direction, Normal);

    if (abs(Denom) < EPS)
        return HitResult;

    float t = Nom / Denom;

    if (t < EPS)
        return HitResult;

    HitResult.HitPoint = Ray.Start + Ray.Direction * float(t);

    HitResult.IsHit = CheckIsHit(HitResult.HitPoint);

    if (HitResult.IsHit) {
        HitResult.Distance = t;
    }

    return HitResult;
}

bool KH_Triangle::Cmpx(const KH_Triangle& t1, const KH_Triangle& t2)
{
	return t1.Center.x < t2.Center.x;
}

bool KH_Triangle::Cmpy(const KH_Triangle& t1, const KH_Triangle& t2)
{
	return t1.Center.y < t2.Center.y;
}

bool KH_Triangle::Cmpz(const KH_Triangle& t1, const KH_Triangle& t2)
{
	return t1.Center.z < t2.Center.z;
}

bool KH_Triangle::CheckIsHit(glm::vec3 HitPoint)
{
    const glm::vec3 P1P2 = P2 - P1;
    const glm::vec3 P1P3 = P3 - P1;
    const glm::vec3 P1P = HitPoint - P1;

    double Dot00 = glm::dot(P1P3, P1P3);
    double Dot01 = glm::dot(P1P3, P1P2);
    double Dot02 = glm::dot(P1P3, P1P);
    double Dot11 = glm::dot(P1P2, P1P2);
    double Dot12 = glm::dot(P1P2, P1P);

    double Denom = Dot00 * Dot11 - Dot01 * Dot01;

    if (std::abs(Denom) < EPS)
        return false;

    double InvDenom = 1.0 / Denom;

    double u = (Dot11 * Dot02 - Dot01 * Dot12) * InvDenom;
    double v = (Dot00 * Dot12 - Dot01 * Dot02) * InvDenom;

    if (u >= 0 && v >= 0 && u + v <= 1) {
        return true;
    }
    return false;
}
