#include "KH_Texture.h"

#include "Scene/KH_Scene.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

#include "Utils/KH_DebugUtils.h"


namespace
{
    constexpr float kEpsilon = 1e-8f;

    float Luminance(float r, float g, float b)
    {
        return 0.2126f * r + 0.7152f * g + 0.0722f * b;
    }

    std::vector<float> BuildCDF(const std::vector<float>& pdf)
    {
        std::vector<float> cdf(pdf.size(), 0.0f);
        if (pdf.empty())
            return cdf;

        float accum = 0.0f;
        for (size_t i = 0; i < pdf.size(); ++i)
        {
            accum += pdf[i];
            cdf[i] = accum;
        }

        cdf.back() = 1.0f;
        return cdf;
    }

    int SampleCDFIndex(float xi, const std::vector<float>& cdf)
    {
        if (cdf.empty())
            return 0;

        xi = std::clamp(xi, 0.0f, 1.0f - 1e-7f);

        auto it = std::lower_bound(cdf.begin(), cdf.end(), xi);
        if (it == cdf.end())
            return static_cast<int>(cdf.size()) - 1;

        return static_cast<int>(std::distance(cdf.begin(), it));
    }

    float TexelCenter(int index, int resolution)
    {
        return (static_cast<float>(index) + 0.5f) / static_cast<float>(resolution);
    }

    std::vector<glm::vec4> ComputeHDRCache(const float* data, int width, int height, int nrComponents)
    {
        assert(data != nullptr);
        assert(width > 0 && height > 0);
        assert(nrComponents >= 3);

        const int nPixels = width * height;
        std::vector<glm::vec4> hdrCache(nPixels, glm::vec4(0.0f));

        std::vector<std::vector<float>> jointPdf(height, std::vector<float>(width, 0.0f));
        std::vector<float> marginalPdfV(height, 0.0f);

        float totalWeight = 0.0f;

        for (int v = 0; v < height; ++v)
        {
            const float theta = glm::pi<float>() * ((static_cast<float>(v) + 0.5f) / static_cast<float>(height));
            const float sinTheta = std::sin(theta);

            float rowSum = 0.0f;

            for (int u = 0; u < width; ++u)
            {
                const int offset = (v * width + u) * nrComponents;
                const float lum = Luminance(data[offset], data[offset + 1], data[offset + 2]);

                const float weight = lum * sinTheta;

                //const float weight = lum;
                jointPdf[v][u] = weight;
                rowSum += weight;
            }

            marginalPdfV[v] = rowSum;
            totalWeight += rowSum;
        }

        if (totalWeight <= kEpsilon)
        {
            float uniformPDF = 1.0f;

            for (int v = 0; v < height; ++v)
            {
                for (int u = 0; u < width; ++u)
                {

                    hdrCache[v * width + u] = glm::vec4(
                        TexelCenter(u, width),
                        TexelCenter(v, height),
                        uniformPDF,
                        uniformPDF
                    );
                }
            }
            return hdrCache;
        }

        
        for (float& p : marginalPdfV)
        {
            p /= totalWeight;
        }

        std::vector<std::vector<float>> conditionalPdfU(height, std::vector<float>(width, 0.0f));

        for (int v = 0; v < height; ++v)
        {
            const float rowWeight = std::accumulate(jointPdf[v].begin(), jointPdf[v].end(), 0.0f);

            if (rowWeight <= kEpsilon)
            {
                const float uniform = 1.0f / static_cast<float>(width);
                std::fill(conditionalPdfU[v].begin(), conditionalPdfU[v].end(), uniform);
            }
            else
            {
                for (int u = 0; u < width; ++u)
                {
                    conditionalPdfU[v][u] = jointPdf[v][u] / rowWeight;
                }
            }
        }

        const std::vector<float> cdfV = BuildCDF(marginalPdfV);

        std::vector<std::vector<float>> cdfU(height);
        for (int v = 0; v < height; ++v)
        {
            cdfU[v] = BuildCDF(conditionalPdfU[v]);
        }

        const float stepU = 1.0f / static_cast<float>(width);
        const float stepV = 1.0f / static_cast<float>(height);

        for (int y = 0; y < height; ++y)
        {
            const float xi1 = (static_cast<float>(y) + 0.5f) * stepV;
            const int sampledV = SampleCDFIndex(xi1, cdfV);
            const float sampleV = TexelCenter(sampledV, height);

            for (int x = 0; x < width; ++x)
            {
                const float xi2 = (static_cast<float>(x) + 0.5f) * stepU;

                const int sampledU = SampleCDFIndex(xi2, cdfU[sampledV]);
                const float sampleU = TexelCenter(sampledU, width);
                const float pdf_sampleUV = jointPdf[sampledV][sampledU] / totalWeight * nPixels;
                const float pdf_realUV = jointPdf[y][x] / totalWeight * nPixels;

                hdrCache[y * width + x] = glm::vec4(sampleU, sampleV, pdf_sampleUV, pdf_realUV);
            }
        }

        return hdrCache;
    }
}


KH_TextureResource::~KH_TextureResource()
{
    if (ID != 0)
    {
        glDeleteTextures(1, &ID);
        ID = 0;
    }
}

KH_Texture::KH_Texture(std::shared_ptr<KH_TextureResource> resource)
    : Resource(std::move(resource))
{
}

bool KH_Texture::IsValid() const
{
    return Resource != nullptr && Resource->ID != 0;
}

unsigned int KH_Texture::GetID() const
{
    return Resource ? Resource->ID : 0;
}

KH_TEXTURE_TYPE KH_Texture::GetType() const
{
    return Resource ? Resource->Type : KH_TEXTURE_TYPE::DEFAULT;
}

const std::string& KH_Texture::GetFileName() const
{
    static std::string Empty;
    return Resource ? Resource->FileName : Empty;
}

//void KH_Texture::Bind(KH_Shader& Shader, const std::string& Name, uint32_t Unit) const
//{
//    if (!IsValid())
//        return;
//
//    glActiveTexture(GL_TEXTURE0 + Unit);
//    glBindTexture(GL_TEXTURE_2D, Resource->ID);
//    Shader.SetInt(Name, static_cast<int>(Unit));
//}

void KH_Texture::Bind(uint32_t Unit) const
{
    if (!IsValid())
        return;

    glActiveTexture(GL_TEXTURE0 + Unit);
    glBindTexture(GL_TEXTURE_2D, Resource->ID);
}

void KH_Texture::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

KH_Texture KH_TextureManager::LoadTexture(const std::string& filePath,
    bool generateMipmap,
    KH_TEXTURE_TYPE type,
    bool flipY)
{
    const std::string key = NormalizePath(filePath);

    auto it = TextureCache.find(key);
    if (it != TextureCache.end())
    {
        if (auto shared = it->second.lock())
        {
            return KH_Texture(shared);
        }
    }

    auto resource = CreateTextureResource(key, generateMipmap, type, flipY);
    if (!resource || resource->ID == 0)
    {
        return KH_Texture();
    }

    TextureCache[key] = resource;
    return KH_Texture(resource);
}

KH_Texture KH_TextureManager::CreateHDRCache(const std::string& filePath, bool generateMipmap, bool flipY)
{
    const std::string normalizedPath = NormalizePath(filePath);
    std::string key = normalizedPath + "-HDRCache";

    auto it = TextureCache.find(key);
    if (it != TextureCache.end())
    {
        if (auto shared = it->second.lock())
        {
            return KH_Texture(shared);
        }
    }

    auto resource = CreateHDRCacheTextureResource(normalizedPath, generateMipmap, flipY);

    if (!resource || resource->ID == 0)
    {
        return KH_Texture();
    }

    TextureCache[key] = resource;
    return KH_Texture(resource);
}

void KH_TextureManager::GarbageCollect()
{
    for (auto it = TextureCache.begin(); it != TextureCache.end();)
    {
        if (it->second.expired())
            it = TextureCache.erase(it);
        else
            ++it;
    }
}

void KH_TextureManager::Clear()
{
    TextureCache.clear();
}

std::string KH_TextureManager::NormalizePath(const std::string& path) const
{
    try
    {
        std::filesystem::path p(path);
        p = std::filesystem::weakly_canonical(p);
        return p.generic_string();
    }
    catch (...)
    {
        return std::filesystem::path(path).generic_string();
    }
}

std::shared_ptr<KH_TextureResource> KH_TextureManager::CreateTextureResource(const std::string& normalizedPath,
    bool generateMipmap,
    KH_TEXTURE_TYPE type,
    bool flipY)
{
    auto resource = std::make_shared<KH_TextureResource>();
    resource->FileName = normalizedPath;

    glGenTextures(1, &resource->ID);

    if (resource->ID == 0)
    {
        LOG_E(std::format("glGenTextures failed for path: {}", normalizedPath));
        return nullptr;
    }

    std::string extension;
    auto pos = normalizedPath.find_last_of('.');
    if (pos != std::string::npos)
        extension = normalizedPath.substr(pos + 1);

    if (extension == "hdr" || extension == "HDR")
    {
        resource->Type = KH_TEXTURE_TYPE::HDR;
        LoadHDRTexture(*resource, normalizedPath.c_str(), generateMipmap, flipY);
    }
    else
    {
        resource->Type = type;
        LoadStandardTexture(*resource, normalizedPath.c_str(), generateMipmap, flipY);
    }

    if (resource->ID == 0)
        return nullptr;

    return resource;
}

std::shared_ptr<KH_TextureResource> KH_TextureManager::CreateHDRCacheTextureResource(
    const std::string& normalizedPath, bool generateMipmap, bool flipY)
{
    auto resource = std::make_shared<KH_TextureResource>();
    resource->FileName = normalizedPath;

    glGenTextures(1, &resource->ID);

    if (resource->ID == 0)
    {
        LOG_E(std::format("glGenTextures failed for path: {}", normalizedPath));
        return nullptr;
    }

    std::string extension;
    auto pos = normalizedPath.find_last_of('.');
    if (pos != std::string::npos)
        extension = normalizedPath.substr(pos + 1);


    if (extension != "hdr")
    {
        LOG_E(std::format("The image used to generate HDR cache does not have 'hdr' extentsion : {}", normalizedPath));
        return nullptr;
    }

    resource->Type = KH_TEXTURE_TYPE::HDR;

    CreateHDRCacheTexture(*resource, normalizedPath.c_str(), generateMipmap, flipY);

    if (resource->ID == 0)
        return nullptr;

    return resource;
    
}


void KH_TextureManager::LoadStandardTexture(KH_TextureResource& resource,
    const char* path,
    bool generateMipmap,
    bool flipY) const
{
    stbi_set_flip_vertically_on_load(flipY);

    int width = 0;
    int height = 0;
    int nrComponents = 0;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

    if (!data)
    {
        LOG_E(std::format("Texture failed to load at path: {}", path));
        if (resource.ID != 0)
        {
            glDeleteTextures(1, &resource.ID);
            resource.ID = 0;
        }
        return;
    }

    GLenum dataFormat = GL_RGB;
    GLenum internalFormat = GL_RGB8;

    if (nrComponents == 1)
    {
        dataFormat = GL_RED;
        internalFormat = GL_R8;
    }
    else if (nrComponents == 3)
    {
        dataFormat = GL_RGB;
        internalFormat = GL_RGB8;
    }
    else if (nrComponents == 4)
    {
        dataFormat = GL_RGBA;
        internalFormat = GL_RGBA8;
    }

    resource.Width = width;
    resource.Height = height;
    resource.Channels = nrComponents;


    glBindTexture(GL_TEXTURE_2D, resource.ID);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (generateMipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
}

void KH_TextureManager::LoadHDRTexture(KH_TextureResource& resource,
    const char* path,
    bool generateMipmap,
    bool flipY) const
{
    stbi_set_flip_vertically_on_load(flipY);

    int width = 0;
    int height = 0;
    int nrComponents = 0;
    float* data = stbi_loadf(path, &width, &height, &nrComponents, 0);

    if (!data)
    {
        LOG_E(std::format("HDR Texture failed to load at path: {}", path));
        if (resource.ID != 0)
        {
            glDeleteTextures(1, &resource.ID);
            resource.ID = 0;
        }
        return;
    }

    resource.Width = width;
    resource.Height = height;
    resource.Channels = nrComponents;

    glBindTexture(GL_TEXTURE_2D, resource.ID);

    GLenum dataFormat = (nrComponents == 4) ? GL_RGBA : GL_RGB;
    GLenum internalFormat = (nrComponents == 4) ? GL_RGBA32F : GL_RGB32F;

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_FLOAT, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (generateMipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
}

void KH_TextureManager::CreateHDRCacheTexture(KH_TextureResource& resource, const char* path, bool generateMipmap, bool flipY) const
{
    stbi_set_flip_vertically_on_load(flipY);

    int width = 0;
    int height = 0;
    int nrComponents = 0;
    float* data = stbi_loadf(path, &width, &height, &nrComponents, 0);

    if (!data)
    {
        LOG_E(std::format("HDR Texture failed to load at path: {}", path));
        if (resource.ID != 0)
        {
            glDeleteTextures(1, &resource.ID);
            resource.ID = 0;
        }
        return;
    }

    resource.Width = width;
    resource.Height = height;
    resource.Channels = nrComponents;

    std::vector<glm::vec4> hdrCache = ComputeHDRCache(data, width, height, nrComponents);

    glBindTexture(GL_TEXTURE_2D, resource.ID);

    GLenum dataFormat = GL_RGBA;
    GLenum internalFormat = GL_RGBA32F;

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_FLOAT, hdrCache.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (generateMipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(data);
}

KH_ExampleTextures::KH_ExampleTextures()
{
    InitTextures();
}

void KH_ExampleTextures::InitTextures()
{
    SkyboxHDR = KH_TextureManager::Instance().LoadTexture(
        "Assert/Images/HDR/qwantani_dusk_2_puresky_4k.hdr",
        false,
        KH_TEXTURE_TYPE::HDR,
        true
    );

    SkyboxHDRCache = KH_TextureManager::Instance().CreateHDRCache(
        "Assert/Images/HDR/qwantani_dusk_2_puresky_4k.hdr",
        false,
        true
    );
}
