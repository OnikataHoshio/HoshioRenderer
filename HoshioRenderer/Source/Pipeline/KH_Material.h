#pragma once

#include "KH_Common.h"
struct KH_Material {
public:
	bool IsEmissive = false;
	glm::vec3 Color = glm::vec3();
	float SpecularRate = 0.0f;
	float ReflectRoughness = 1.0f;

	float RefractRate = 0.0f;
	float Eta = 1.0f;
	float RefractRoughness = 0.0f;
};