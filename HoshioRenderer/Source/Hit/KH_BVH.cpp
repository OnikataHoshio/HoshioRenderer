#include "KH_BVH.h"
#include "KH_AABB.h"
#include "Editor/KH_Editor.h"
#include "Object/KH_Object.h"
#include "Pipeline/KH_Shader.h"

#include "Utils/KH_DebugUtils.h"

void KH_BVHNode::BuildNode(std::vector<KH_Triangle>& Triangles, uint32_t BeginIndex, uint32_t EndIndex, uint32_t Depth, uint32_t MaxNum, uint32_t MaxDepth)
{
	int count = EndIndex - BeginIndex;
	if (count <= 0) return;

	float MaxInf = std::numeric_limits<float>::max();
	AABB.MinPos = glm::vec3(MaxInf);
	AABB.MaxPos = glm::vec3(-MaxInf);

	for (int i = BeginIndex; i < EndIndex; i++) {
		AABB.MinPos = glm::min(AABB.MinPos, Triangles[i].GetAABB().MinPos - static_cast<float>(EPS));
		AABB.MaxPos = glm::max(AABB.MaxPos, Triangles[i].GetAABB().MaxPos + static_cast<float>(EPS));
	}

	if (count <= MaxNum || Depth >= MaxDepth) {

		this->bIsLeaf = true;
		this->Offset = BeginIndex;
		this->Size = count;
		this->Left = nullptr;
		this->Right = nullptr;
		return;
	}



	KH_BVH_SPLIT_MODE SplitMode = SelectSplitMode(AABB);
	switch (SplitMode) {
	case KH_BVH_SPLIT_MODE::X_AXIS_SPLIT:
		std::sort(Triangles.begin() + BeginIndex, Triangles.begin() + EndIndex, KH_Triangle::Cmpx);
		break;
	case KH_BVH_SPLIT_MODE::Y_AXIS_SPLIT:
		std::sort(Triangles.begin() + BeginIndex, Triangles.begin() + EndIndex, KH_Triangle::Cmpy);
		break;
	case KH_BVH_SPLIT_MODE::Z_AXIS_SPLIT:
		std::sort(Triangles.begin() + BeginIndex, Triangles.begin() + EndIndex, KH_Triangle::Cmpz);
		break;
	}

	int Mid = BeginIndex + count / 2;

	this->bIsLeaf = false;

	this->Left = std::make_unique<KH_BVHNode>();
	this->Left->BuildNode(Triangles, BeginIndex, Mid, Depth + 1, MaxNum, MaxDepth);

	this->Right = std::make_unique<KH_BVHNode>();
	this->Right->BuildNode(Triangles, Mid, EndIndex, Depth + 1, MaxNum, MaxDepth);
}

void KH_BVHNode::BuildNodeSAH(std::vector<KH_Triangle>& Triangles, uint32_t BeginIndex, uint32_t EndIndex,
	uint32_t Depth, uint32_t MaxNum, uint32_t MaxDepth)
{
	int count = EndIndex - BeginIndex;
	if (count <= 0) return;

	float MaxInf = std::numeric_limits<float>::max();
	AABB.MinPos = glm::vec3(MaxInf);
	AABB.MaxPos = glm::vec3(-MaxInf);

	for (int i = BeginIndex; i < EndIndex; i++) {
		AABB.MinPos = glm::min(AABB.MinPos, Triangles[i].GetAABB().MinPos - static_cast<float>(EPS));
		AABB.MaxPos = glm::max(AABB.MaxPos, Triangles[i].GetAABB().MaxPos + static_cast<float>(EPS));
	}

	if (count <= MaxNum || Depth >= MaxDepth) {
		this->bIsLeaf = true;
		this->Offset = BeginIndex;
		this->Size = count;
		this->Left = nullptr;
		this->Right = nullptr;
		return;
	}

	KH_BVHSplitInfo SplitInfo = SelectSplitModeSAH(Triangles, BeginIndex, EndIndex);
	switch (SplitInfo.SplitMode) {
	case KH_BVH_SPLIT_MODE::X_AXIS_SPLIT:
		std::sort(Triangles.begin() + BeginIndex, Triangles.begin() + EndIndex, KH_Triangle::Cmpx);
		break;
	case KH_BVH_SPLIT_MODE::Y_AXIS_SPLIT:
		std::sort(Triangles.begin() + BeginIndex, Triangles.begin() + EndIndex, KH_Triangle::Cmpy);
		break;
	case KH_BVH_SPLIT_MODE::Z_AXIS_SPLIT:
		std::sort(Triangles.begin() + BeginIndex, Triangles.begin() + EndIndex, KH_Triangle::Cmpz);
		break;
	}

	this->bIsLeaf = false;

	this->Left = std::make_unique<KH_BVHNode>();
	this->Left->BuildNode(Triangles, BeginIndex, SplitInfo.SplitIndex, Depth + 1, MaxNum, MaxDepth);

	this->Right = std::make_unique<KH_BVHNode>();
	this->Right->BuildNode(Triangles, SplitInfo.SplitIndex, EndIndex, Depth + 1, MaxNum, MaxDepth);
}

void KH_BVHNode::Hit(std::vector<KH_BVHHitInfo>& HitInfos, KH_Ray& Ray)
{
	KH_AABBHitInfo AABBHit = AABB.Hit(Ray);
	if (!AABBHit.bIsHit) return;

	if (bIsLeaf)
	{
		KH_BVHHitInfo BVHHitInfo;
		BVHHitInfo.BeginIndex = Offset;
		BVHHitInfo.EndIndex = Offset + Size;
		BVHHitInfo.HitTime = AABBHit.HitTime; 
		BVHHitInfo.bIsHit = true;
		HitInfos.push_back(BVHHitInfo);
		return;
	}

	if (Left)  Left->Hit(HitInfos, Ray);
	if (Right) Right->Hit(HitInfos, Ray);
}

KH_BVH_SPLIT_MODE KH_BVHNode::SelectSplitMode(KH_AABB& AABB)
{
	glm::vec3 Size = AABB.GetSize();
	int axis = 0; 
	if (Size.y > Size[axis]) axis = 1; 
	if (Size.z > Size[axis]) axis = 2; 

	return static_cast<KH_BVH_SPLIT_MODE>(axis);
}

KH_BVHSplitInfo KH_BVHNode::SelectSplitModeSAH(std::vector<KH_Triangle>& Triangles, int BeginIndex,
	int EndIndex)
{
	int count = EndIndex - BeginIndex;
	float MaxInf = std::numeric_limits<float>::max();

	KH_BVHSplitInfo BVHSplitInfo;
	BVHSplitInfo.Cost = std::numeric_limits<float>::max();

	for (int axis = 0; axis < 3; axis++)
	{
		switch (axis) {
		case 0:
			std::sort(Triangles.begin() + BeginIndex, Triangles.begin() + EndIndex, KH_Triangle::Cmpx);
			break;
		case 1:
			std::sort(Triangles.begin() + BeginIndex, Triangles.begin() + EndIndex, KH_Triangle::Cmpy);
			break;
		case 2:
			std::sort(Triangles.begin() + BeginIndex, Triangles.begin() + EndIndex, KH_Triangle::Cmpz);
			break;
		default:
			break;
		}

		std::vector<glm::vec3> LeftMin(count, glm::vec3(MaxInf));
		std::vector<glm::vec3> LeftMax(count, glm::vec3(-MaxInf));
		std::vector<glm::vec3> RightMin(count, glm::vec3(MaxInf));
		std::vector<glm::vec3> RightMax(count, glm::vec3(-MaxInf));

		LeftMin[0] = Triangles[BeginIndex].GetAABB().MinPos;
		LeftMax[0] = Triangles[BeginIndex].GetAABB().MaxPos;
		for (int i = BeginIndex + 1; i < EndIndex; i++) {
			int index = i - BeginIndex;
			LeftMin[index] = glm::min(LeftMin[index - 1], Triangles[i].GetAABB().MinPos);
			LeftMax[index] = glm::max(LeftMax[index - 1], Triangles[i].GetAABB().MaxPos);
		}

		RightMin[count - 1] = Triangles[EndIndex - 1].GetAABB().MinPos;
		RightMax[count - 1] = Triangles[EndIndex - 1].GetAABB().MaxPos;
		for (int i = EndIndex - 2; i >= BeginIndex; i--) {
			int index = i - BeginIndex;
			RightMin[index] = glm::min(RightMin[index + 1], Triangles[i].GetAABB().MinPos);
			RightMax[index] = glm::max(RightMax[index + 1], Triangles[i].GetAABB().MaxPos);
		}


		for (int i = BeginIndex + 1; i < EndIndex - 1; i++)
		{
			int LeftCost = KH_AABB::ComputeSurfaceArea(LeftMin[i], LeftMax[i]) * (i - BeginIndex);
			int RightCost = KH_AABB::ComputeSurfaceArea(RightMin[i], RightMax[i]) * (EndIndex - i);
			int TotalCost = LeftCost + RightCost;

			if (TotalCost < BVHSplitInfo.Cost)
			{
				BVHSplitInfo.Cost = TotalCost;
				BVHSplitInfo.SplitIndex = i;
				BVHSplitInfo.SplitMode = static_cast<KH_BVH_SPLIT_MODE>(axis);
			}
		}
	}

	return BVHSplitInfo;
}

KH_BVH::KH_BVH(uint32_t MaxBVHDepth, uint32_t MaxLeafTriangles)
	:MaxBVHDepth(MaxBVHDepth), MaxLeafTriangles(MaxLeafTriangles)
{
	Root = std::make_unique<KH_BVHNode>();
}

void KH_BVH::LoadObj(const std::string& path)
{
	tinyobj::ObjReader reader;
	if (!reader.ParseFromFile(path)) {
		std::cerr << "TinyObjReader Error: " << reader.Error() << std::endl;
		return;
	}

	const auto& attrib = reader.GetAttrib();
	const auto& shapes = reader.GetShapes();

	Triangles.clear();

	size_t totalIndices = 0;
	for (const auto& shape : shapes) totalIndices += shape.mesh.indices.size();
	Triangles.reserve(totalIndices / 3);

	std::vector<glm::vec3> vertexNormals(attrib.vertices.size() / 3, glm::vec3(0.0f));

	for (const auto& shape : shapes) {
		size_t index_offset = 0;
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			tinyobj::index_t idx0 = shape.mesh.indices[index_offset + 0];
			tinyobj::index_t idx1 = shape.mesh.indices[index_offset + 1];
			tinyobj::index_t idx2 = shape.mesh.indices[index_offset + 2];

			glm::vec3 p0(attrib.vertices[3 * idx0.vertex_index + 0], attrib.vertices[3 * idx0.vertex_index + 1], attrib.vertices[3 * idx0.vertex_index + 2]);
			glm::vec3 p1(attrib.vertices[3 * idx1.vertex_index + 0], attrib.vertices[3 * idx1.vertex_index + 1], attrib.vertices[3 * idx1.vertex_index + 2]);
			glm::vec3 p2(attrib.vertices[3 * idx2.vertex_index + 0], attrib.vertices[3 * idx2.vertex_index + 1], attrib.vertices[3 * idx2.vertex_index + 2]);

			glm::vec3 faceNormal = glm::cross(p1 - p0, p2 - p0);

			vertexNormals[idx0.vertex_index] += faceNormal;
			vertexNormals[idx1.vertex_index] += faceNormal;
			vertexNormals[idx2.vertex_index] += faceNormal;

			index_offset += 3;
		}
	}

	for (const auto& shape : shapes) {
		size_t index_offset = 0;
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			tinyobj::index_t idx[3];
			glm::vec3 p[3], n[3];

			for (size_t v = 0; v < 3; v++) {
				idx[v] = shape.mesh.indices[index_offset + v];
				p[v] = { attrib.vertices[3 * idx[v].vertex_index + 0], attrib.vertices[3 * idx[v].vertex_index + 1], attrib.vertices[3 * idx[v].vertex_index + 2] };

				n[v] = glm::normalize(vertexNormals[idx[v].vertex_index]);
			}

			Triangles.emplace_back( p[0], p[1], p[2], n[0], n[1], n[2]);
			index_offset += 3;
		}
	}

	BuildBVH();
	FillModelMatrices(MaxBVHDepth);

	std::string DebugMessage = std::format("Model Range : [({},{},{}),({},{},{})]", 
		Root->AABB.MinPos.x, Root->AABB.MinPos.y, Root->AABB.MinPos.z,
		Root->AABB.MaxPos.x, Root->AABB.MaxPos.y, Root->AABB.MaxPos.z);
	LOG_D(DebugMessage);

}

void KH_BVH::RenderAABB(KH_Shader Shader, glm::vec3 Color)
{
	Shader.Use();
	Shader.SetMat4("view", KH_Editor::Instance().Camera.GetViewMatrix());
	Shader.SetMat4("projection", KH_Editor::Instance().Camera.GetProjMatrix());
	Shader.SetVec3("Color", Color);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glDisable(GL_CULL_FACE);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ModelMats_SSBO);

	glBindVertexArray(KH_DefaultModels::Get().EmptyCube.VAO);
	glDrawElementsInstanced(
		KH_DefaultModels::Get().EmptyCube.GetDrawMode(),
		static_cast<GLsizei>(KH_DefaultModels::Get().Cube.GetIndicesSize()), 
		GL_UNSIGNED_INT,
		0,
		static_cast<GLsizei>(MatCount) 
	);
	glBindVertexArray(0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
}

std::vector<KH_BVHHitInfo> KH_BVH::Hit(KH_Ray& Ray)
{
	std::vector<KH_BVHHitInfo> HitInfos;
	if (Root != nullptr)
		Root->Hit(HitInfos, Ray);
	return HitInfos;
}

void KH_BVH::FillModelMatrices(uint32_t TargetDepth)
{
	ModelMats.clear();
	MatCount = 0;

	FillModelMatrices_Inner(Root.get(), 0, TargetDepth);

	UpdateModelMatsSSBO();
}

void KH_BVH::FillModelMatrices_Inner(KH_BVHNode* BVHNode, uint32_t CurrentDepth, uint32_t TargetDepth)
{
	if (BVHNode == nullptr)
		return;

	if (CurrentDepth == TargetDepth || BVHNode->bIsLeaf)
	{
		//TODO : Do some optimization
		ModelMats.push_back(BVHNode->AABB.GetModelMatrix());
		MatCount += 1;
		return;
	}

	FillModelMatrices_Inner(BVHNode->Left.get(), CurrentDepth + 1, TargetDepth);
	FillModelMatrices_Inner(BVHNode->Right.get(), CurrentDepth + 1, TargetDepth);
}

void KH_BVH::UpdateModelMatsSSBO()
{
	if (ModelMats_SSBO != 0)
	{
		glDeleteBuffers(1, &ModelMats_SSBO);
	}

	glGenBuffers(1, &ModelMats_SSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ModelMats_SSBO);

	glBufferData(GL_SHADER_STORAGE_BUFFER,
		ModelMats.size() * sizeof(glm::mat4),
		ModelMats.data(),
		GL_STATIC_DRAW);

	// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ModelMats_SSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void KH_BVH::BuildBVH()
{
	switch (BuildMode)
	{
	case KH_BVH_BUILD_MODE::Base:
		this->Root->BuildNode(Triangles, 0, Triangles.size(), 0, MaxLeafTriangles, MaxBVHDepth);
		break;
	case KH_BVH_BUILD_MODE::SAH:
		this->Root->BuildNodeSAH(Triangles, 0, Triangles.size(), 0, MaxLeafTriangles, MaxBVHDepth);
		break;
	}
}
