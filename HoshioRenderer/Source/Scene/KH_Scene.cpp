#include "KH_Scene.h"

void KH_Scene::CreateSSBOs()
{
	DestroySSBOs();

	glGenBuffers(1, &TriangleSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, TriangleSSBO);

	glBufferData(GL_SHADER_STORAGE_BUFFER,
		Triangles.size() * sizeof(KH_TriangleEncoded),
		Triangles.data(),
		GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	glGenBuffers(1, &MaterialSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, MaterialSSBO);

	glBufferData(GL_SHADER_STORAGE_BUFFER,
		Materials.size() * sizeof(KH_BSDFMaterial),
		Materials.data(),
		GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void KH_Scene::DestroySSBOs()
{
	if (TriangleSSBO)
		glDeleteBuffers(1, &TriangleSSBO);

	if (MaterialSSBO)
		glDeleteBuffers(1, &MaterialSSBO);
}

KH_Scene::KH_Scene(uint32_t MaxBVHDepth, uint32_t MaxLeafTriangles, KH_BVH_BUILD_MODE BuildMode)
	:BVH(MaxBVHDepth, MaxLeafTriangles)
{
	BVH.BuildMode = BuildMode;
}

KH_Scene::~KH_Scene()
{
	DestroySSBOs();
}

void KH_Scene::LoadObj(const std::string& path, uint32_t MaterialSlot)
{
	tinyobj::ObjReader reader;
	if (!reader.ParseFromFile(path)) {
		std::cerr << "TinyObjReader Error: " << reader.Error() << std::endl;
		return;
	}

	const auto& attrib = reader.GetAttrib();
	const auto& shapes = reader.GetShapes();


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

			Triangles.emplace_back(p[0], p[1], p[2], n[0], n[1], n[2], MaterialSlot);
			index_offset += 3;
		}
	}
}


void KH_Scene::BindAndBuild()
{
	BVH.BindAndBuild(Triangles);

	CreateSSBOs();
}

std::vector<KH_TriangleEncoded> KH_Scene::EncodeTriangles()
{
	const int nTriangles = Triangles.size();
	std::vector<KH_TriangleEncoded> TriangleEncodeds(nTriangles);

	for (int i = 0; i < nTriangles; i++)
	{
		KH_BSDFMaterial& Mat = Materials[Triangles[i].MaterialSlot];

		TriangleEncodeds[i].P1 = glm::vec4(Triangles[i].P1, 1.0);
		TriangleEncodeds[i].P2 = glm::vec4(Triangles[i].P2, 1.0);
		TriangleEncodeds[i].P3 = glm::vec4(Triangles[i].P3, 1.0);

		TriangleEncodeds[i].N1 = glm::vec4(Triangles[i].N1, 1.0);
		TriangleEncodeds[i].N2 = glm::vec4(Triangles[i].N2, 1.0);
		TriangleEncodeds[i].N3 = glm::vec4(Triangles[i].N3, 1.0);
 	}

	return TriangleEncodeds;
}

void KH_Scene::AddTriangles(KH_Triangle& Triangle)
{
	Triangles.emplace_back(Triangle);
}

void KH_Scene::Clear()
{
	Triangles.clear();
}

KH_ExampleScene::KH_ExampleScene()
{
	InitExampleScene1();
}

void KH_ExampleScene::InitExampleScene1()
{
	ExampleScene1.BVH.MaxBVHDepth = 11;
	ExampleScene1.BVH.MaxLeafTriangles = 1;

	ExampleScene1.BVH.BuildMode = KH_BVH_BUILD_MODE::SAH;

	KH_BSDFMaterial Material;
	Material.BaseColor = glm::vec3(1.0, 0.0, 0.0);

	ExampleScene1.Materials.push_back(Material);

	ExampleScene1.LoadObj("Assert/Models/cube.obj", 0);
	ExampleScene1.BindAndBuild();
}
