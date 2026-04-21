#pragma once

#include "Pipeline/KH_ShaderFeature.h"
#include "Pipeline/KH_Buffer.h"

struct KH_BSDFMaterial
{
    glm::vec3 Emissive = glm::vec3(0.0f);
    glm::vec3 BaseColor = glm::vec3(1.0f);
    float Subsurface = 0.0f;
    float Metallic = 0.0f;
    float Specular = 0.0f;
    float SpecularTint = 0.0f;
    float Roughness = 0.0f;
    float Anisotropic = 0.0f;
    float Sheen = 0.0f;
    float SheenTint = 0.0f;
    float Clearcoat = 0.0f;
    float ClearcoatGloss = 0.0f;
    float IOR = 0.0f;
    float Transmission = 0.0f;
};

struct KH_BSDFMaterialEncoded
{
    glm::vec4 Emissive;
    glm::vec4 BaseColor;
    glm::vec4 Param1;
    glm::vec4 Param2;
    glm::vec4 Param3;
};

class KH_DisneyBSDF : public KH_ShaderFeatureBase
{
public:
    KH_DisneyBSDF();
    explicit KH_DisneyBSDF(const KH_Shader& shader);

    ~KH_DisneyBSDF() override = default;

    int AddMaterial(const KH_BSDFMaterial& material = KH_BSDFMaterial{});
    std::vector<KH_BSDFMaterial>& GetMaterials();
    const std::vector<KH_BSDFMaterial>& GetMaterials() const;

    void DrawControlPanel() override;
    void ApplyUniforms() override;

    void UploadMaterialBuffer() override;
    void BindBuffers() override;
    void ClearMaterials() override;
    int GetMaterialCount() const override;
    bool DeleteMaterial(int materialID) override;

private:
    std::vector<KH_BSDFMaterialEncoded> EncodeMaterials() const;

private:
    std::vector<KH_BSDFMaterial> Materials;
    KH_SSBO<KH_BSDFMaterialEncoded> Material_SSBO;

    int uEnableVNDF = 0;
    int uEnableSkybox = 0;

    void SetEnableVNDF(bool bEnable);
    void SetEnableSkybox(bool bEnable);
};