#pragma once
#include "KH_AABB.h"
#include "Object/KH_Shape.h"

class KH_Shader;

enum class KH_BVH_SPLIT_MODE
{
	X_AXIS_SPLIT = 0,
	Y_AXIS_SPLIT = 1,
	Z_AXIS_SPLIT = 2
};


class KH_BVHNode
{
public:
	std::unique_ptr<KH_BVHNode> Left;
	std::unique_ptr<KH_BVHNode> Right;

	bool bIsLeaf = false;
	int Offset = 0;
	int Size = 0;

	KH_AABB AABB;

	void BuildNode(std::vector<KH_Triangle>& Triangles, int LeftIndex, int RightIndex, int Depth, int MaxNum, int MaxDepth);

private:
	static KH_BVH_SPLIT_MODE SelectSplitMode(KH_AABB& AABB);
};


class KH_BVH
{
public:
	std::unique_ptr<KH_BVHNode> Root;

	std::vector<KH_Triangle> Triangles;
	uint32_t MaxBVHDepth;
	uint32_t MaxBVHNodeTriangleNum;

	std::vector<glm::mat4> ModelMats;
	uint32_t MatCount = 0;

	unsigned int ModelMats_SSBO = 0;

	KH_BVH(uint32_t MaxDepth, uint32_t MaxNum);
	~KH_BVH() = default;

	void LoadObj(const std::string& path);

	void RenderAABB(KH_Shader Shader, glm::vec3 Color);

	void FillModelMatrices(uint32_t CurrentDepth, uint32_t TargetDepth);

private:
	void FillModelMatrices_Inner(KH_BVHNode* BVHNode, uint32_t CurrentDepth, uint32_t TargetDepth);

	void UpdateModelMatsSSBO();

};
