#pragma once

#include "KH_Common.h"

struct KH_BaseMaterial {
public:
	bool bIsEmissive = false;
	glm::vec3 Color = glm::vec3();
	float SpecularRate = 0.0f;
	float ReflectRoughness = 1.0f;

	float RefractRate = 0.0f;
	float Eta = 1.0f;
	float RefractRoughness = 0.0f;
};

class KH_DefaultMaterial : public KH_Singleton<KH_DefaultMaterial>
{
	friend class KH_Singleton<KH_DefaultMaterial>;

private:
	KH_DefaultMaterial();
	virtual ~KH_DefaultMaterial() = default;

	void Initialize();
	void Deinitialize();

	void InitBaseMaterial1();
	void InitBaseMaterial2();

public:
	KH_BaseMaterial BaseMaterial1;
	KH_BaseMaterial BaseMaterial2;
};


