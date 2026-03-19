#pragma once
#include "Hit/KH_BVH.h"
#include "KH_Object.h"
#include "Hit/KH_LBVH.h"
#include "Editor/KH_Editor.h"
#include "Pipeline/KH_Shader.h"
#include "Utils/KH_DebugUtils.h"

enum class KH_BVH_BUILD_MODE;

struct KH_TriangleEncoded
{
	glm::vec4 P1, P2, P3;
	glm::vec4 N1, N2, N3;
	glm::ivec4 MaterialSlot;
};

struct KH_BRDFMaterialEncoded
{
	glm::vec4 Emissive;
	glm::vec4 BaseColor;
	glm::vec4 Param1;
	glm::vec4 Param2;
	glm::vec4 Param3;
};


struct KH_FlatBVHNodeEncoded
{
	glm::ivec4 Param1; //(Left, Right, )
	glm::ivec4 Param2; //(bIsLeaf, Offset, Size)
	glm::vec4 AABB_MinPos;
	glm::vec4 AABB_MaxPos;
};

struct KH_CameraParam {
    glm::vec4 AspectAndFovy; // .x = Aspect, .y = Fovy
    glm::vec4 Position;
    glm::vec4 Right;
    glm::vec4 Up;
    glm::vec4 Front;
};

template<typename BVHType>
class KH_Scene {
private:
    KH_SSBO<KH_TriangleEncoded> Triangle_SSBO;
    KH_SSBO<KH_BRDFMaterialEncoded> Material_SSBO;
    KH_SSBO<KH_FlatBVHNodeEncoded> LBVHNode_SSBO;
    KH_SSBO<uint32_t> SortedIndices_SSBO;
    KH_UBO<KH_CameraParam> CameraParam_UB0;

    void SetSSBOs();
    void SetRayTracingParam(KH_Shader& Shader);

public:
    template<typename... Args>
    KH_Scene(Args&&... args) : BVH(std::forward<Args>(args)...) {
        Triangle_SSBO.SetBindPoint(0);
        Material_SSBO.SetBindPoint(1);
        LBVHNode_SSBO.SetBindPoint(2);
        SortedIndices_SSBO.SetBindPoint(3);
        CameraParam_UB0.SetBindPoint(4);
    }

    ~KH_Scene() = default;

    void LoadObj(const std::string& path, float scale = 1.0, int MaterialSlot = KH_MATERIAL_UNDEFINED_SLOT);
    void AddTriangles(const KH_Triangle& Triangle);
    void BindAndBuild();
    void Render();
    void Clear();

    std::vector<KH_TriangleEncoded> EncodeTriangles();
    std::vector<KH_BRDFMaterialEncoded> EncodeBSDFMaterials();
    std::vector<KH_FlatBVHNodeEncoded> EncodeLBVHNodes();

    BVHType BVH;
    KH_AABB AABB;

    std::vector<KH_Model> Models;
    std::vector<KH_Triangle> Triangles;
    std::vector<KH_BRDFMaterial> Materials;
};

using KH_BVHScene = KH_Scene<KH_BVH>;
using KH_FlatBVHScene = KH_Scene<KH_FlatBVH>;
using KH_LBVHScene = KH_Scene<KH_LBVH>;

template <typename BVHType>
void KH_Scene<BVHType>::SetSSBOs()
{
    if constexpr (std::is_same_v<BVHType, KH_BVH>)
        return;

    std::vector<KH_TriangleEncoded> TriangleEncodeds = EncodeTriangles();
    std::vector<KH_BRDFMaterialEncoded> BSDFMaterialEncodeds = EncodeBSDFMaterials();
    std::vector<KH_FlatBVHNodeEncoded> LBVHNodeEncodeds = EncodeLBVHNodes();

    std::string DebugMessage = std::format("KH_TriangleEncoded size : {} Byte", sizeof(KH_TriangleEncoded));
    LOG_D(DebugMessage);
    DebugMessage = std::format("KH_BSDFMaterialEncoded size : {} Byte", sizeof(KH_BRDFMaterialEncoded));
    LOG_D(DebugMessage);
    DebugMessage = std::format("KH_LBVHNodeEcoded size : {} Byte", sizeof(KH_FlatBVHNodeEncoded));
    LOG_D(DebugMessage);

    Triangle_SSBO.SetData(TriangleEncodeds);
    Material_SSBO.SetData(BSDFMaterialEncodeds);
    LBVHNode_SSBO.SetData(LBVHNodeEncodeds);

    if constexpr (std::is_same_v<BVHType, KH_LBVH>)
        SortedIndices_SSBO.SetData(BVH.SortedIndices);

}

template <typename BVHType>
void KH_Scene<BVHType>::SetRayTracingParam(KH_Shader& Shader)
{
    if constexpr (std::is_same_v<BVHType, KH_BVH>)
        return;

    Shader.Use();

    const KH_Camera& Camera = KH_Editor::Instance().Camera;

    Triangle_SSBO.Bind();
    Material_SSBO.Bind();
    LBVHNode_SSBO.Bind();

    if constexpr (std::is_same_v<BVHType, KH_LBVH>)
        SortedIndices_SSBO.Bind();

    Shader.SetInt("TriangleCount", Triangles.size());
    Shader.SetInt("LBVHNodeCount", BVH.BVHNodes.size());
    Shader.SetInt("RootNodeID", BVH.Root);


    KH_CameraParam CameraParam;
    CameraParam.AspectAndFovy = glm::vec4(Camera.Aspect, Camera.Fovy, 0.0f, 0.0f);
    CameraParam.Position = glm::vec4(Camera.Position, 1.0f);
    CameraParam.Right = glm::vec4(Camera.Right, 1.0f);
    CameraParam.Up = glm::vec4(Camera.Up, 1.0f);
    CameraParam.Front = glm::vec4(Camera.Front, 1.0f);

    CameraParam_UB0.SetSingleData(CameraParam);
    CameraParam_UB0.Bind();

    //Shader.SetVec4("UCameraParam.AspectAndFovy", glm::vec4(Camera.Aspect, Camera.Fovy, 0.0f, 0.0f));
    //Shader.SetVec4("UCameraParam.Position", glm::vec4(Camera.Position, 1.0f));
    //Shader.SetVec4("UCameraParam.Right", glm::vec4(Camera.Right, 1.0f));
    //Shader.SetVec4("UCameraParam.Up", glm::vec4(Camera.Up, 1.0f));
    //Shader.SetVec4("UCameraParam.Front", glm::vec4(Camera.Front, 1.0f));
    
}

template <typename BVHType>
void KH_Scene<BVHType>::LoadObj(const std::string& path, float scale, int MaterialSlot)
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

            Triangles.emplace_back(scale * p[0], scale * p[1], scale * p[2], n[0], n[1], n[2], MaterialSlot);

            AABB.Merge(Triangles.back());

            index_offset += 3;
        }
    }
}

template <typename BVHType>
void KH_Scene<BVHType>::AddTriangles(const KH_Triangle& Triangle)
{
    Triangles.emplace_back(Triangle);
    AABB.Merge(Triangles.back());
}

template<typename BVHType>
void KH_Scene<BVHType>::BindAndBuild() {

    if constexpr (std::is_same_v<BVHType, KH_LBVH>) {
        BVH.BindAndBuild(Triangles, AABB);
    }
    else {
        BVH.BindAndBuild(Triangles);
    }

    SetSSBOs();
}

template <typename BVHType>
void KH_Scene<BVHType>::Render()
{
    if constexpr (std::is_same_v<BVHType, KH_BVH>)
        return;


    //KH_Shader& Shader = KH_ExampleShaders::Instance().RayTracingShader1_3;
    KH_Framebuffer& Framebuffer = KH_Editor::Instance().GetCanvasFramebuffer();

    Framebuffer.Bind();
    
    if constexpr (std::is_same_v<BVHType, KH_FlatBVH>)
        SetRayTracingParam(KH_ExampleShaders::Instance().RayTracingShader1_3);
    else if constexpr (std::is_same_v<BVHType, KH_LBVH>)
        SetRayTracingParam(KH_ExampleShaders::Instance().RayTracingShader2_0);

    glBindVertexArray(KH_DefaultModels::Instance().Plane.VAO);
    glDrawElements(KH_DefaultModels::Instance().Plane.GetDrawMode(), static_cast<GLsizei>(KH_DefaultModels::Instance().Plane.GetIndicesSize()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    Triangle_SSBO.Unbind();
    Material_SSBO.Unbind();
    LBVHNode_SSBO.Unbind();
    SortedIndices_SSBO.Unbind();

    Framebuffer.Unbind();
}

template <typename BVHType>
void KH_Scene<BVHType>::Clear()
{
    Triangles.clear();
}

template <typename BVHType>
std::vector<KH_TriangleEncoded> KH_Scene<BVHType>::EncodeTriangles()
{
    if constexpr (std::is_same_v<BVHType, KH_BVH>)
        return std::vector<KH_TriangleEncoded>();

    const int nTriangles = Triangles.size();
    std::vector<KH_TriangleEncoded> TriangleEncodeds(nTriangles);

    for (int i = 0; i < nTriangles; i++)
    {
        TriangleEncodeds[i].P1 = glm::vec4(Triangles[i].P1, 1.0);
        TriangleEncodeds[i].P2 = glm::vec4(Triangles[i].P2, 1.0);
        TriangleEncodeds[i].P3 = glm::vec4(Triangles[i].P3, 1.0);

        TriangleEncodeds[i].N1 = glm::vec4(Triangles[i].N1, 1.0);
        TriangleEncodeds[i].N2 = glm::vec4(Triangles[i].N2, 1.0);
        TriangleEncodeds[i].N3 = glm::vec4(Triangles[i].N3, 1.0);

        TriangleEncodeds[i].MaterialSlot = glm::ivec4(Triangles[i].MaterialSlot, 0.0, 0.0, 0.0);
    }

    return TriangleEncodeds;
}

template <typename BVHType>
std::vector<KH_BRDFMaterialEncoded> KH_Scene<BVHType>::EncodeBSDFMaterials()
{
    if constexpr (std::is_same_v<BVHType, KH_BVH>)
        return std::vector<KH_BRDFMaterialEncoded>();

    const int nMaterials = Materials.size();
    std::vector<KH_BRDFMaterialEncoded> BSDFMaterialEncodeds(nMaterials);

    for (int i = 0; i < nMaterials; i++)
    {
        KH_BRDFMaterial& Mat = Materials[i];

        BSDFMaterialEncodeds[i].Emissive = glm::vec4(Mat.Emissive, 1.0);
        BSDFMaterialEncodeds[i].BaseColor = glm::vec4(Mat.BaseColor, 1.0);
        BSDFMaterialEncodeds[i].Param1 = glm::vec4(Mat.Subsurface, Mat.Metallic, Mat.Specular, Mat.SpecularTint);
        BSDFMaterialEncodeds[i].Param2 = glm::vec4(Mat.Roughness, Mat.Anisotropic, Mat.Sheen, Mat.SheenTint);
        BSDFMaterialEncodeds[i].Param3 = glm::vec4(Mat.Clearcoat, Mat.ClearcoatGloss, Mat.IOR, Mat.Transmission);
    }

    return BSDFMaterialEncodeds;
}

template <typename BVHType>
std::vector<KH_FlatBVHNodeEncoded> KH_Scene<BVHType>::EncodeLBVHNodes()
{
    if constexpr (std::is_same_v<BVHType, KH_BVH>)
        return std::vector<KH_FlatBVHNodeEncoded>();

    const int nLBVHNodes = BVH.BVHNodes.size();
    std::vector<KH_FlatBVHNodeEncoded> LBVHNodeEncoded(nLBVHNodes);

    for (int i = 0; i < nLBVHNodes; i++)
    {
        auto& Node = BVH.BVHNodes[i];

        LBVHNodeEncoded[i].Param1 = glm::ivec4(Node.Left, Node.Right, 0.0, 0.0);
        if constexpr (std::is_same_v<BVHType, KH_FlatBVH>)
        {
            LBVHNodeEncoded[i].Param2 = glm::ivec4(Node.bIsLeaf ? 1 : 0, Node.Offset, Node.Size, 0);
        }
    	else if constexpr (std::is_same_v<BVHType, KH_LBVH>)
        {
            bool bIsLeaf = BVH.IsLeafNode(i);
            LBVHNodeEncoded[i].Param2 = glm::ivec4(bIsLeaf ? 1 : 0, i, 1, 0);
        }
        LBVHNodeEncoded[i].AABB_MinPos = glm::vec4(Node.AABB.MinPos, 1.0);
        LBVHNodeEncoded[i].AABB_MaxPos = glm::vec4(Node.AABB.MaxPos, 1.0);
    }
    return LBVHNodeEncoded;
}


template<typename SceneType>
class KH_ExampleScenes : public KH_Singleton<KH_ExampleScenes<SceneType>>
{
    friend class KH_Singleton<KH_ExampleScenes<SceneType>>;

private:
    KH_ExampleScenes() {
        InitBunny();
        InitDebugBox();
        InitSingleTriangle();
    }
    ~KH_ExampleScenes() override = default;

    void InitSingleTriangle();
    void InitDebugBox();
    void InitBunny();

public:
    SceneType Bunny;
    SceneType SingleTriangle;
    SceneType DebugBox;
};


using KH_FlatBVHExampleScenes = KH_ExampleScenes<KH_FlatBVHScene>;
using KH_LBVHExampleScenes = KH_ExampleScenes<KH_LBVHScene>;

template <typename SceneType>
void KH_ExampleScenes<SceneType>::InitSingleTriangle()
{
    SingleTriangle.BVH.MaxBVHDepth = 11;
    SingleTriangle.BVH.MaxLeafTriangles = 1;
    SingleTriangle.BVH.BuildMode = KH_BVH_BUILD_MODE::SAH;

    KH_BRDFMaterial Material;
    Material.BaseColor = glm::vec3(1.0, 0.0, 0.0);

    SingleTriangle.Materials.push_back(Material);

    KH_Triangle Triangle(
        glm::vec3(-0.5, -0.5, 0.0),
        glm::vec3(0.5, -0.5, 0.0),
        glm::vec3(0.0, 0.5, 0.0),
        0
    );

    SingleTriangle.AddTriangles(Triangle);
    SingleTriangle.BindAndBuild();
}

template <typename SceneType>
void KH_ExampleScenes<SceneType>::InitBunny()
{
    Bunny.BVH.MaxBVHDepth = 5;
    Bunny.BVH.MaxLeafTriangles = 1;

    Bunny.BVH.BuildMode = KH_BVH_BUILD_MODE::SAH;

    KH_BRDFMaterial Material1;
    Material1.BaseColor = glm::vec3(1.0, 0.0, 0.0);

    KH_BRDFMaterial Material2;
    Material2.BaseColor = glm::vec3(0.0, 1.0, 0.0);

    glm::vec3 v0(-1.0f, 0.15f, -1.0f);
    glm::vec3 v1(1.0f, 0.15f, -1.0f);
    glm::vec3 v2(1.0f, 0.15f, 1.0f);
    glm::vec3 v3(-1.0f, 0.15f, 1.0f);

    KH_Triangle Triangle1(v0, v1, v2, 1);
    KH_Triangle Triangle2(v0, v2, v3, 1);

    Bunny.Materials.push_back(Material1);
    Bunny.Materials.push_back(Material2);

    Bunny.LoadObj("Assert/Models/bunny.obj", 5.0, 0);
    Bunny.AddTriangles(Triangle1);
    Bunny.AddTriangles(Triangle2);
    Bunny.BindAndBuild();
}

template <typename SceneType>
void KH_ExampleScenes<SceneType>::InitDebugBox()
{
    DebugBox.BVH.MaxBVHDepth = 5;
    DebugBox.BVH.MaxLeafTriangles = 1;

    glm::vec3 Colors[6] = {
        glm::vec3(1, 0, 0), 
        glm::vec3(0, 1, 0), 
        glm::vec3(0, 0, 1), 
        glm::vec3(1, 1, 0), 
        glm::vec3(1, 0, 1), 
        glm::vec3(0, 1, 1)  
    };

    for (int i = 0; i < 6; ++i) {
        KH_BRDFMaterial Mat;
        Mat.BaseColor = glm::vec4(Colors[i], 1.0f);
        DebugBox.Materials.push_back(Mat);
    }

    glm::vec3 v[8] = {
        {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f}, {0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, // Front 0,1,2,3
        {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}  // Back  4,5,6,7
    };

    auto AddQuad = [&](int a, int b, int c, int d, int matID) {
        DebugBox.AddTriangles(KH_Triangle(v[a], v[b], v[c], matID));
        DebugBox.AddTriangles(KH_Triangle(v[a], v[c], v[d], matID));
        };

    AddQuad(0, 1, 2, 3, 0); // Front
    AddQuad(5, 4, 7, 6, 1); // Back
    AddQuad(4, 0, 3, 7, 2); // Left
    AddQuad(1, 5, 6, 2, 3); // Right
    AddQuad(3, 2, 6, 7, 4); // Top
    AddQuad(4, 5, 1, 0, 5); // Bottom

    DebugBox.BindAndBuild();
}

