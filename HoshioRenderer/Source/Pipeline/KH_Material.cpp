#include "KH_Material.h"

KH_DefaultMaterial::KH_DefaultMaterial()
	: KH_Singleton<KH_DefaultMaterial>()
{
	Initialize();
}

void KH_DefaultMaterial::Initialize()
{
	InitBaseMaterial1();
	InitBaseMaterial2();
}

void KH_DefaultMaterial::Deinitialize()
{

}

void KH_DefaultMaterial::InitBaseMaterial1()
{
    BaseMaterial1.bIsEmissive = true;
    BaseMaterial1.Color = glm::vec3(0.9f, 0.05f, 0.1f);
    BaseMaterial1.SpecularRate = 0.5f;
    BaseMaterial1.ReflectRoughness = 0.05f;
    BaseMaterial1.RefractRate = 0.8f;
    BaseMaterial1.Eta = 1.76f;
    BaseMaterial1.RefractRoughness = 0.1f;
}

void KH_DefaultMaterial::InitBaseMaterial2()
{
    BaseMaterial2.bIsEmissive = false;
    BaseMaterial2.Color = glm::vec3(0.2f, 0.22f, 0.25f);
    BaseMaterial2.SpecularRate = 0.9f;
    BaseMaterial2.ReflectRoughness = 0.35f;
    BaseMaterial2.RefractRate = 0.0f;
    BaseMaterial2.Eta = 1.0f;
    BaseMaterial2.RefractRoughness = 0.0f;
}

