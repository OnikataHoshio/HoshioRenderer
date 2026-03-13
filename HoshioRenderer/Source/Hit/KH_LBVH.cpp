#include "KH_LBVH.h"
#include "Scene/KH_Object.h"
#include "Utils/KH_DebugUtils.h"
#include "Utils/KH_Algorithms.h"


void KH_LBVHNode::Hit(std::vector<KH_BVHHitInfo>& HitInfos, std::vector<KH_LBVHNode>& LBVHNodes, uint32_t TriangleCount, int NodeID,  KH_Ray& Ray)
{
	KH_AABBHitInfo AABBHit = AABB.Hit(Ray);
	if (!AABBHit.bIsHit) return;

	if (NodeID < TriangleCount)
	{
		KH_BVHHitInfo BVHHitInfo;
		BVHHitInfo.BeginIndex = Range.x;
		BVHHitInfo.EndIndex = Range.y;
		BVHHitInfo.HitTime = AABBHit.HitTime;
		BVHHitInfo.bIsHit = true;
		HitInfos.push_back(BVHHitInfo);
		return;
	}

	if (Left != KH_LBVH_NULL_NODE)  LBVHNodes[Left].Hit(HitInfos, LBVHNodes, TriangleCount, Left, Ray);
	if (Right != KH_LBVH_NULL_NODE) LBVHNodes[Right].Hit(HitInfos, LBVHNodes, TriangleCount, Right,  Ray);
}

void KH_LBVH::BindAndBuild(std::vector<KH_Triangle>& Triangles)
{
	this->Triangles = &Triangles;

	if (!IsAllDataReady())
		return;

	TriangleCount = Triangles.size();

	SortTriangleIndices();
	FillDeltaBuffer();
	InitLBVHNodes();
	BuildBVH();
}

void KH_LBVH::BindAndBuild(std::vector<KH_Triangle>& Triangles, KH_AABB AABB)
{
	this->Triangles = &Triangles;
	this->AABB = AABB;

	if (!IsAllDataReady())
		return;

	TriangleCount = Triangles.size();

	SortTriangleIndices();
	FillDeltaBuffer();
	InitLBVHNodes();
	BuildBVH();
}

std::vector<KH_BVHHitInfo> KH_LBVH::Hit(KH_Ray& Ray)
{
	std::vector<KH_BVHHitInfo> HitInfos;
	if (Root != KH_FLAT_BVH_NULL_NODE)
		BVHNodes[Root].Hit(HitInfos, BVHNodes, TriangleCount, Root, Ray);
	return HitInfos;
}

void KH_LBVH::SortTriangleIndices()
{
	const std::vector<KH_Triangle>& Tris = *Triangles;

	Tri2Mortons.resize(TriangleCount);

	glm::vec3 AABB_Size = AABB.GetSize();

	for (int i = 0; i < TriangleCount; i++)
	{
		glm::vec3 Positon = Tris[i].Center;
		Tri2Mortons[i] = KH_MortonCode::Morton3DFloat_IndexAugmentation((Positon - AABB.MinPos) / AABB_Size, i);
	}

	std::ranges::sort(Tri2Mortons);

	for (int i = 0; i < TriangleCount; ++i)
	{
		SortedIndices[i] = static_cast<uint32_t>(Tri2Mortons[i] & 0xFFFFFFFFU);
	}
}

int KH_LBVH::ComputeDelta(int i)
{
	uint64_t first = Tri2Mortons[i - 1];
	uint64_t second = Tri2Mortons[i];
	uint64_t diff = first ^ second;

	if (diff == 0) return 64;


#ifdef _MSC_VER
	unsigned long leading;
	_BitScanReverse64(&leading, diff);
	return 63 - (int)leading;
#else
	return __builtin_clzll(diff);
#endif
}



void KH_LBVH::FillDeltaBuffer()
{
	const int N = TriangleCount;
	DeltaBuffer.assign(N + 1, -1);

	DeltaBuffer[0] = -1;
	DeltaBuffer[N] = -1;

	for (int i = 1; i < N; i++)
	{
		DeltaBuffer[i] = ComputeDelta(i);
	}
}

void KH_LBVH::InitLBVHNodes()
{
	const std::vector<KH_Triangle>& Tris = *Triangles;
	const int N = TriangleCount;
	BVHNodes.resize(2 * N - 1);

	for (int i = 0; i < N; i++)
	{
		BVHNodes[i].Range = glm::i64vec2(Tri2Mortons[i], Tri2Mortons[i]);
		BVHNodes[i].AABB.Update(Tris[SortedIndices[i]]);
	}

	AtomicTags.assign( N - 1, -1);
}


bool KH_LBVH::IsLeafChild(int NodeID) const
{
	glm::i64vec2 Range = BVHNodes[NodeID].Range;
	return DeltaBuffer[Range.x] < DeltaBuffer[Range.y + 1];
}


bool KH_LBVH::IsRootNode(int NodeID) const
{
	const int N = TriangleCount;
	glm::i64vec2 Range = BVHNodes[NodeID].Range;
	return Range.x == BVHNodes[0].Range.x && Range.y == BVHNodes[N - 1].Range.y;
}

void KH_LBVH::BuildBVH()
{
	const std::vector<KH_Triangle>& Tris = *Triangles;
	const int N = TriangleCount;

	for (int i = 0; i < N; i++)
	{
		int NodeID = i;
		int ParentLocalID;
		int ParentGlobalID;
		while (true)
		{
			if (IsLeafChild(NodeID))
			{
				ParentLocalID = BVHNodes[NodeID].Range.y;
				AtomicTags[ParentLocalID] += 1;
				ParentGlobalID = ParentLocalID + N;
				BVHNodes[ParentGlobalID].Range.x = BVHNodes[NodeID].Range.x;
				BVHNodes[ParentGlobalID].Left = NodeID;
			}
			else
			{
				ParentLocalID = BVHNodes[NodeID].Range.x - 1;
				AtomicTags[ParentLocalID] += 1;
				ParentGlobalID = ParentLocalID + N;
				BVHNodes[ParentGlobalID].Range.y = BVHNodes[NodeID].Range.y;
				BVHNodes[ParentGlobalID].Right = NodeID;
			}

			BVHNodes[ParentGlobalID].AABB.Merge(BVHNodes[NodeID].AABB);

			if (AtomicTags[ParentLocalID] <= 0)
				break;

			if (IsRootNode(ParentGlobalID))
			{
				Root = ParentGlobalID;
				break;
			}

			NodeID = ParentGlobalID;
		}
	}
}

bool KH_LBVH::IsAllDataReady() const
{
	return CheckTriangles() && CheckAABB();
}

bool KH_LBVH::CheckTriangles() const
{
	if (Triangles == nullptr)
	{
		std::string DebugMessage = std::format("KH_LBVH::CheckTriangles: Triangles array pointer has not been set!");
		LOG_E(DebugMessage);
		return false;
	}

	if (Triangles->empty())
	{
		std::string DebugMessage = std::format("KH_LBVH::CheckTriangles: Triangles array is empty!");
		LOG_E(DebugMessage);
		return false;
	}

	return true;

}

bool KH_LBVH::CheckAABB() const
{
	if (AABB.IsInvalid())
	{
		std::string DebugMessage = std::format("KH_LBVH::CheckAABB: AABB is invalid!");
		LOG_E(DebugMessage);
		return false;
	}
	return true;
}

void KH_LBVH::FillModelMatrices(uint32_t TargetDepth)
{
	ModelMats.clear();
	MatCount = 0;

	FillModelMatrices_Inner(Root, 0, TargetDepth);

	UpdateModelMatsSSBO();
}

void KH_LBVH::FillModelMatrices_Inner(int LBVHNodeID, uint32_t CurrentDepth, uint32_t TargetDepth)
{
	if (LBVHNodeID == KH_LBVH_NULL_NODE)
		return;

	if (CurrentDepth == TargetDepth || LBVHNodeID < Triangles->size())
	{
		//TODO : Do some optimization
		ModelMats.push_back(BVHNodes[LBVHNodeID].AABB.GetModelMatrix());
		MatCount += 1;
		return;
	}

	FillModelMatrices_Inner(BVHNodes[LBVHNodeID].Left, CurrentDepth + 1, TargetDepth);
	FillModelMatrices_Inner(BVHNodes[LBVHNodeID].Right, CurrentDepth + 1, TargetDepth);
}

