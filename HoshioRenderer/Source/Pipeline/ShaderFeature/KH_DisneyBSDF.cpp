#include "KH_DisneyBSDF.h"
#include "Editor/KH_Editor.h"

KH_DisneyBSDF::KH_DisneyBSDF()
    : KH_ShaderFeatureBase(KH_ShaderFeatureType::DisneyBSDF)
{
    Material_SSBO.SetBindPoint(4);
}

KH_DisneyBSDF::KH_DisneyBSDF(const KH_Shader& shader)
    : KH_ShaderFeatureBase(KH_ShaderFeatureType::DisneyBSDF, shader)
{
    Material_SSBO.SetBindPoint(4);
}

int KH_DisneyBSDF::AddMaterial(const KH_BSDFMaterial& material)
{
    Materials.push_back(material);
    return static_cast<int>(Materials.size()) - 1;
}

std::vector<KH_BSDFMaterial>& KH_DisneyBSDF::GetMaterials()
{
    return Materials;
}

const std::vector<KH_BSDFMaterial>& KH_DisneyBSDF::GetMaterials() const
{
    return Materials;
}

std::vector<KH_BSDFMaterialEncoded> KH_DisneyBSDF::EncodeMaterials() const
{
    std::vector<KH_BSDFMaterialEncoded> encoded(Materials.size());

    for (size_t i = 0; i < Materials.size(); ++i)
    {
        const KH_BSDFMaterial& mat = Materials[i];

        encoded[i].Emissive = glm::vec4(mat.Emissive, 1.0f);
        encoded[i].BaseColor = glm::vec4(mat.BaseColor, 1.0f);
        encoded[i].Param1 = glm::vec4(
            mat.Subsurface,
            mat.Metallic,
            mat.Specular,
            mat.SpecularTint);

        encoded[i].Param2 = glm::vec4(
            mat.Roughness,
            mat.Anisotropic,
            mat.Sheen,
            mat.SheenTint);

        encoded[i].Param3 = glm::vec4(
            mat.Clearcoat,
            mat.ClearcoatGloss,
            mat.IOR,
            mat.Transmission);
    }

    return encoded;
}

void KH_DisneyBSDF::UploadMaterialBuffer()
{
    std::vector<KH_BSDFMaterialEncoded> encoded = EncodeMaterials();

    if (encoded.empty())
    {
        encoded.resize(1);
    }

    Material_SSBO.SetData(encoded);
}

void KH_DisneyBSDF::BindBuffers()
{
    Material_SSBO.Bind();
}

void KH_DisneyBSDF::ClearMaterials()
{
    Materials.clear();
    UploadMaterialBuffer();
}

int KH_DisneyBSDF::GetMaterialCount() const
{
    return static_cast<int>(Materials.size());
}

bool KH_DisneyBSDF::DeleteMaterial(int materialID)
{
    if (materialID < 0 || materialID >= static_cast<int>(Materials.size()))
        return false;

    Materials.erase(Materials.begin() + materialID);
    return true;
}

void KH_DisneyBSDF::DrawControlPanel()
{
    KH_Editor& Editor = KH_Editor::Instance();

    static bool bEnableVNDF = false;
    static bool bEnableSkybox = false;


    ImGui::SeparatorText("DisneyBSDF");
    ImGui::Indent(20.0f);
    ImGui::Text("FrameCounter: %d", std::min(Editor.GetFrameCounter(), 2048u));

    bool bNeedReset = false;

    if (ImGui::Checkbox("EnableVNDF", &bEnableVNDF))
    {
        bNeedReset = true;
    }


    if (ImGui::Checkbox("EnableSkybox", &bEnableSkybox))
    {
        bNeedReset = true;
    }


    ImGui::Unindent(20.0f);

    if (bNeedReset)
    {
        Editor.RequestFrameReset();
    }

    SetEnableVNDF(bEnableVNDF);
    SetEnableSkybox(bEnableSkybox);
}

void KH_DisneyBSDF::ApplyUniforms()
{
    Shader.SetInt("uEnableVNDF", uEnableVNDF);
    Shader.SetInt("uEnableSkybox", uEnableSkybox);
}

void KH_DisneyBSDF::SetEnableVNDF(bool bEnable)
{
    uEnableVNDF = bEnable ? 1 : 0;
}

void KH_DisneyBSDF::SetEnableSkybox(bool bEnable)
{
    uEnableSkybox = bEnable ? 1 : 0;
}