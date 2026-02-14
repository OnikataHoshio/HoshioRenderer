#include "KH_BVH.h"
#include "KH_AABB.h"
#include "Editor/KH_Editor.h"
#include "Object/KH_Object.h"
#include "Pipeline/KH_Shader.h"

void KH_BVHNode::BuildNode(std::vector<KH_Triangle>& Triangles, int LeftIndex, int RightIndex, int Depth, int MaxNum, int MaxDepth)
{
	int count = RightIndex - LeftIndex;
	if (count <= 0) return;

	float MaxInf = std::numeric_limits<float>::infinity();
	AABB.MinPos = glm::vec3(MaxInf);
	AABB.MaxPos = glm::vec3(-MaxInf);

	for (int i = LeftIndex; i < RightIndex; i++) {
		AABB.MinPos = glm::min(AABB.MinPos, Triangles[i].GetAABB().MinPos);
		AABB.MaxPos = glm::max(AABB.MaxPos, Triangles[i].GetAABB().MaxPos);
	}

	if (count <= MaxNum || Depth >= MaxDepth) {
		this->bIsLeaf = true;
		this->Offset = LeftIndex;
		this->Size = count;
		this->Left = nullptr;
		this->Right = nullptr;
		return;
	}

	KH_BVH_SPLIT_MODE SplitMode = SelectSplitMode(AABB);
	switch (SplitMode) {
	case KH_BVH_SPLIT_MODE::X_AXIS_SPLIT:
		std::sort(Triangles.begin() + LeftIndex, Triangles.begin() + RightIndex, KH_Triangle::Cmpx);
		break;
	case KH_BVH_SPLIT_MODE::Y_AXIS_SPLIT:
		std::sort(Triangles.begin() + LeftIndex, Triangles.begin() + RightIndex, KH_Triangle::Cmpy);
		break;
	case KH_BVH_SPLIT_MODE::Z_AXIS_SPLIT:
		std::sort(Triangles.begin() + LeftIndex, Triangles.begin() + RightIndex, KH_Triangle::Cmpz);
		break;
	}

	int Mid = LeftIndex + count / 2; // 中位数划分

	this->bIsLeaf = false;

	this->Left = std::make_unique<KH_BVHNode>();
	this->Left->BuildNode(Triangles, LeftIndex, Mid, Depth + 1, MaxNum, MaxDepth);

	this->Right = std::make_unique<KH_BVHNode>();
	this->Right->BuildNode(Triangles, Mid, RightIndex, Depth + 1, MaxNum, MaxDepth);
}


KH_BVH_SPLIT_MODE KH_BVHNode::SelectSplitMode(KH_AABB& AABB)
{
	glm::vec3 Size = AABB.GetSize();
	int axis = 0; 
	if (Size.y > Size[axis]) axis = 1; 
	if (Size.z > Size[axis]) axis = 2; 

	return static_cast<KH_BVH_SPLIT_MODE>(axis);
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

	for (const auto& shape : shapes) {
		size_t index_offset = 0;

		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			int fv = shape.mesh.num_face_vertices[f];

			if (fv == 3) {
				glm::vec3 p[3];
				for (size_t v = 0; v < 3; v++) {
					tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
					p[v] = {
						attrib.vertices[3 * idx.vertex_index + 0],
						attrib.vertices[3 * idx.vertex_index + 1],
						attrib.vertices[3 * idx.vertex_index + 2]
					};
				}
				Triangles.emplace_back(p[0], p[1], p[2]);
			}
			index_offset += fv;
		}
	}

	BuildBVH();
	FillModelMatrices(MaxBVHDepth);
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
	this->Root->BuildNode(Triangles, 0, Triangles.size(), 0, MaxLeafTriangles, MaxBVHDepth);
}
