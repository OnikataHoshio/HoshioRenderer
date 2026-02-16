#include "KH_Shape.h"
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
    N1 = N2 = N3 = Normal;

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
    N1 = N2 = N3 = Normal;

    AABB.MinPos = glm::min(P1, glm::min(P2, P3));
    AABB.MaxPos = glm::max(P1, glm::max(P2, P3));
}

KH_Triangle::KH_Triangle(glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, glm::vec3 N1, glm::vec3 N2, glm::vec3 N3)
    : P1(P1), P2(P2), P3(P3), N1(N1), N2(N2), N3(N3)
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
    // MT算法
    KH_HitResult result;
    glm::vec3 edge1 = P2 - P1;
    glm::vec3 edge2 = P3 - P1;
    glm::vec3 pvec = glm::cross(Ray.Direction, edge2);
    float det = glm::dot(edge1, pvec);

    if (std::abs(det) < EPS) return result;

    float invDet = 1.0f / det;

    glm::vec3 tvec = Ray.Start - P1;
    float u = glm::dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f) return result;

    glm::vec3 qvec = glm::cross(tvec, edge1);
    float v = glm::dot(Ray.Direction, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f) return result;

    float t = glm::dot(edge2, qvec) * invDet;
    if (t < EPS) return result; 

    result.bIsHit = true;
    result.Distance = t;
    result.HitPoint = Ray.Start + t * Ray.Direction;

    float w1 = 1.0f - u - v;
    result.Normal = glm::normalize(w1 * N1 + u * N2 + v * N3);

    return result;
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

KH_TriangleHitInfo KH_Triangle::CheckHitInfo(glm::vec3 HitPoint)
{
    const glm::vec3 P1P2 = P2 - P1;
    const glm::vec3 P1P3 = P3 - P1;
    const glm::vec3 P1P = HitPoint - P1;

    float Dot00 = glm::dot(P1P3, P1P3);
    float Dot01 = glm::dot(P1P3, P1P2);
    float Dot02 = glm::dot(P1P3, P1P);
    float Dot11 = glm::dot(P1P2, P1P2);
    float Dot12 = glm::dot(P1P2, P1P);

    float Denom = Dot00 * Dot11 - Dot01 * Dot01;

    KH_TriangleHitInfo HitInfo;
    HitInfo.bIsHit = false;

    if (std::abs(Denom) < EPS)
        return HitInfo;

    float InvDenom = 1.0f / Denom;

    float u = (Dot11 * Dot02 - Dot01 * Dot12) * InvDenom;
    float v = (Dot00 * Dot12 - Dot01 * Dot02) * InvDenom;

    if (u >= 0 && v >= 0 && u + v <= 1) {
        HitInfo.bIsHit = true;
        HitInfo.w1 = 1 - u - v;
        HitInfo.w2 = u;
        HitInfo.w3 = v;
        return HitInfo;
    }
    return HitInfo;
}
