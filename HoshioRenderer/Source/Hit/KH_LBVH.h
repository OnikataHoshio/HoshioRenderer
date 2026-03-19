#pragma once

#include "KH_AABB.h"
#include "Pipeline/KH_Buffer.h"
#include "KH_BVH.h"

class KH_Triangle;

#define KH_LBVH_NULL_NODE -1

class KH_LBVHNode
{
public:
	int Left = KH_LBVH_NULL_NODE;
	int Right = KH_LBVH_NULL_NODE;
	KH_AABB AABB;
	glm::ivec2 Range = glm::ivec2(0);

	void Hit(std::vector<KH_BVHHitInfo>& HitInfos, std::vector<KH_LBVHNode>& LBVHNodes, uint32_t TriangleCount, int NodeID, KH_Ray& Ray);
};

class KH_LBVH : public KH_IBVH
{
public:
	KH_LBVH() = default;
	~KH_LBVH() override = default;

	KH_AABB AABB;
	int Root;

	std::vector<KH_LBVHNode> BVHNodes;
	std::vector<uint32_t> SortedIndices;

	void BindAndBuild(std::vector<KH_Triangle>& Triangles) override;

	void BindAndBuild(std::vector<KH_Triangle>& Triangles, KH_AABB AABB);

	bool IsLeafNode(int NodeID) const;

	int GetTriangleIndices(int NodeID) const;

	std::vector<KH_BVHHitInfo> Hit(KH_Ray& Ray) override;

private:
	void SortTriangleIndices();

	void FillDeltaBuffer();

	void InitLBVHNodes();

	void BuildBVH() override;

	int ComputeDelta(int i);

	bool IsLeftChild(int NodeID) const;

	bool IsRootNode(int NodeID) const;

	bool IsAllDataReady() const;

	bool CheckTriangles() const;

	bool CheckAABB() const;

	void FillModelMatrices(uint32_t TargetDepth) override;

	void FillModelMatrices_Inner(int LBVHNodeID, uint32_t CurrentDepth, uint32_t TargetDepth);

	uint32_t TriangleCount = 0;
	std::vector<uint64_t> Tri2Mortons;
	std::vector<int> AtomicTags;
	
	std::vector<int> DeltaBuffer;
};


class KH_GPU_LBVH
{
	
};
