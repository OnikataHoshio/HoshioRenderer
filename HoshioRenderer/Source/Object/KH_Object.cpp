#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "KH_Object.h"
#include "Pipeline/KH_Shader.h"
#include "Editor/KH_Camera.h"

KH_Camera* KH_Object::Camera = nullptr;

glm::mat4 KH_Object::GetModelMatrix()
{
    glm::mat4 Model = glm::mat4(1.0f);

    Model = glm::translate(Model, Position);

    Model = glm::rotate(Model, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    Model = glm::rotate(Model, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    Model = glm::rotate(Model, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    Model = glm::scale(Model, Scale);

    return Model;
}

void KH_Object::SetCamera(KH_Camera* Camera)
{
    KH_Object::Camera = Camera;
}

bool KH_Model::LoadOBJ(const std::string& path)
{
    Clear();

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(path))
    {
        std::cerr << reader.Error() << std::endl;
        return false;
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();

    //TODO Optimize
    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            glm::vec3 Vertex = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            Vertices.push_back(Vertex);
            Indices.push_back(static_cast<uint32_t>(Indices.size()));
        }
    }

    return true;
}

void KH_Model::UpdateBuffer()
{
    DeleteBuffer(); 

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(glm::vec3), Vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Indices.size() * sizeof(uint32_t), Indices.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO); 
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void KH_Model::Render(KH_Shader& Shader)
{
    if (!bHasUpdate)
    {
        UpdateBuffer();
        bHasUpdate = true;
    }

    Shader.Use();
    Shader.SetMat4("model", GetModelMatrix());
    Shader.SetMat4("view", Camera->GetViewMatrix());
    Shader.SetMat4("projection", Camera->GetProjMatrix());

    glBindVertexArray(this->VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(Indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void KH_Model::Clear()
{
	Vertices.clear();
	Indices.clear();

    DeleteBuffer();
}

void KH_Model::DeleteBuffer()
{
    if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    if (VBO) { glDeleteBuffers(1, &VBO); VBO = 0; }
    if (EBO) { glDeleteBuffers(1, &EBO); EBO = 0; }
}

void KH_Model::SetVertices(std::vector<glm::vec3>& Vertices)
{
    this->Vertices = Vertices;
}

void KH_Model::SetIndices(std::vector<uint32_t>& Indices)
{
    this->Indices = Indices;
}


KH_DefaultModels& KH_DefaultModels::Get()
{
    static KH_DefaultModels DefaultModels;
    return DefaultModels;
}

KH_DefaultModels::KH_DefaultModels()
{
    InitCube();
}

void KH_DefaultModels::InitCube()
{
   std::vector<glm::vec3> Vertices = {
        // 前面 (Z+)
        {-1.0f, -1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f},
        // 后面 (Z-)
        {-1.0f, -1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f},
        // 左面 (X-)
        {-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f, -1.0f},
        // 右面 (X+)
        { 1.0f, -1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f},
        // 上面 (Y+)
        {-1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f, -1.0f},
        // 下面 (Y-)
        {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f,  1.0f}, {-1.0f, -1.0f,  1.0f}
    };

    std::vector<uint32_t> Indices = {
        0,  1,  2,  2,  3,  0,   // 前
	    4,  5,  6,  6,  7,  4,   // 后
	    8,  9,  10, 10, 11, 8,   // 左
	    12, 13, 14, 14, 15, 12,  // 右
	    16, 17, 18, 18, 19, 16,  // 上
	    20, 21, 22, 22, 23, 20   // 下
    };

    Cube.SetVertices(Vertices);
    Cube.SetIndices(Indices);

}
