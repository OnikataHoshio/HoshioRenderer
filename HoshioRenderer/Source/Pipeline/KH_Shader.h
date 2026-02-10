#pragma once

#include "KH_Common.h"

class KH_Shader {
public:
    unsigned int ID;

    KH_Shader(const char* vertexPath, const char* fragmentPath);

    void Use() const;

    void SetInt(const std::string& name, int value) const;
    void SetUint(const std::string& name, uint32_t value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetMat4(const std::string& name, const glm::mat4& mat) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;

private:
    static void CheckCompileErrors(unsigned int shader, std::string type);
};