#pragma once
#include "Hit/KH_BVH.h"
#include "KH_Object.h"

enum class KH_BVH_BUILD_MODE;

struct KH_TriangleEncoded
{
	glm::vec4 P1, P2, P3;
	glm::vec4 N1, N2, N3;
};

class KH_Scene
{
private:
	void CreateSSBOs();
	void DestroySSBOs();

	GLuint TriangleSSBO = 0;
	GLuint MaterialSSBO = 0;

public:
	KH_Scene() = default;
	KH_Scene(uint32_t MaxBVHDepth, uint32_t MaxLeafTrianglesm, KH_BVH_BUILD_MODE BuildMode);
	~KH_Scene();

	std::vector<KH_Triangle> Triangles;
	std::vector<KH_BSDFMaterial> Materials;
	KH_BVH BVH;

	void LoadObj(const std::string& path, uint32_t MaterialSlot);

	void AddTriangles(KH_Triangle& Triangle);

	void BindAndBuild();

	std::vector<KH_TriangleEncoded> EncodeTriangles();

	void Render();

	void Clear();
};


class KH_ExampleScene : public KH_Singleton<KH_ExampleScene>
{
	friend class KH_Singleton<KH_ExampleScene>;

private:
	KH_ExampleScene();
	virtual ~KH_ExampleScene() override = default;

	void InitExampleScene1();
public:
	KH_Scene ExampleScene1;

};



