#include <iostream>
#include <algorithm> 
#include <vector>
#include <omp.h>
#include <random>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#include "stb_image/stb_image_write.h"
#include "glm/glm.hpp"

#define EPS 1e-8
#define PI 3.1415926

const glm::vec3 RED(0.9f, 0.2f, 0.2f);
const glm::vec3 WHITE(1.0f, 1.0f, 1.0f);
const glm::vec3 GREEN(0.2f, 0.9f, 0.2f);
const glm::vec3 WARM_WHITE(1.0f, 1.0f, 0.9f);
const int SAMPLE_COUNT = 256;


std::uniform_real_distribution<> Distrubution(0.0, 1.0);
std::random_device RandomDevice;
std::mt19937 Gen(RandomDevice());


struct KH_Ray {
public:
	glm::vec3 OriginPoint = glm::vec3();
	glm::vec3 Direction = glm::vec3();
};

struct KH_Material {
public:
	bool IsEmissive = false;
	glm::vec3 Normal = glm::vec3();
	glm::vec3 Color = glm::vec3();
	double SpecularRate = 0.0f;
	double ReflectRoughness = 1.0f;

	double RefractRate = 0.0f;
	double Eta = 1.0f;
	double RefractRoughness = 0.0f;
};

struct KH_Intersect {
public:
	bool IsHit = false;
	double Distance = std::numeric_limits<double>::max();
	glm::vec3 HitPoint = glm::vec3();
	KH_Material Material;
};

class KH_IShape {
public:
	KH_IShape() = default;
	virtual KH_Intersect Intersect(KH_Ray& Ray) = 0;
};

class KH_Triangle : public KH_IShape {
private:
	bool CheckIsHit(glm::vec3 HitPoint) {

		const glm::vec3 V0V1 = V1 - V0;
		const glm::vec3 V0V2 = V2 - V0;
		const glm::vec3 V0P = HitPoint - V0;

		double Dot00 = glm::dot(V0V2, V0V2);
		double Dot01 = glm::dot(V0V2, V0V1);
		double Dot02 = glm::dot(V0V2, V0P);
		double Dot11 = glm::dot(V0V1, V0V1);
		double Dot12 = glm::dot(V0V1, V0P);

		double Denom = Dot00 * Dot11 - Dot01 * Dot01;

		if (std::abs(Denom) < EPS)
			return false;

		double InvDenom = 1.0 / Denom;

		double u = (Dot11 * Dot02 - Dot01 * Dot12) * InvDenom;
		double v = (Dot00 * Dot12 - Dot01 * Dot02) * InvDenom;

		if (u >= 0 && v >= 0 && u + v <= 1) {
			return true;
		}
		return false;
	}

public:
	KH_Triangle() = default;

	KH_Triangle(glm::vec3 V0, glm::vec3 V1, glm::vec3 V2, glm::vec3 Color) : 
		V0(V0), V1(V1), V2(V2)
	{
		const glm::vec3 V0V1 = V1 - V0;
		const glm::vec3 V0V2 = V2 - V0;

		Material.Normal = glm::normalize(glm::cross(V0V1, V0V2));
		Material.Color = Color;
	}

	glm::vec3 V0, V1, V2;
	KH_Material Material;

	// 通过 KH_IShape 继承
	KH_Intersect Intersect(KH_Ray& Ray) override
	{
		KH_Intersect Intersect;

		double Nom = glm::dot(Material.Normal, V0) - glm::dot(Ray.OriginPoint, Material.Normal);
		double Denom = glm::dot(Ray.Direction, Material.Normal);

		if (abs(Denom) < EPS) 
			return Intersect;

		double t = Nom / Denom;

		if (t < EPS)
			return Intersect;

		Intersect.HitPoint = Ray.OriginPoint + Ray.Direction * float(t);

		Intersect.IsHit = CheckIsHit(Intersect.HitPoint);

		if (Intersect.IsHit) {
			Intersect.Distance = t;
			Intersect.Material = Material;
		}
			
		return Intersect;
	}
};

class KH_Sphere : public KH_IShape{
public:
	KH_Sphere(glm::vec3 Center, double Radius, glm::vec3 Color) :
		Center(Center), Radius(Radius)
	{
		Material.Color = Color;
	};

	KH_Intersect Intersect(KH_Ray& Ray) override
	{
		KH_Intersect Intersect;

		glm::vec3 OriginToCenter = Center - Ray.OriginPoint;
		float OC_LengthSquared = glm::dot(OriginToCenter, OriginToCenter);
		float ProjectionLength = glm::dot(OriginToCenter, glm::normalize(Ray.Direction));

		float DistanceToAxisSquared = OC_LengthSquared - ProjectionLength * ProjectionLength;

		float RadiusSquared = Radius * Radius;
		if (DistanceToAxisSquared > RadiusSquared)
			return Intersect;

		float ChordHalfLength = glm::sqrt(RadiusSquared - DistanceToAxisSquared);

		float t0 = ProjectionLength - ChordHalfLength; 
		float t1 = ProjectionLength + ChordHalfLength;

		float Distance = (t0 > EPS) ? t0 : t1; 
		
		if (Distance < EPS) 
			return Intersect;
		
		Intersect.Distance = Distance;
		Intersect.HitPoint = Ray.OriginPoint + Ray.Direction * Distance;
		Intersect.IsHit = true;
		Intersect.Material = Material;
		Intersect.Material.Normal = CalNormal(Intersect.HitPoint);
			
		return Intersect;
	}

	glm::vec3 CalNormal(glm::vec3 HitPoint) {
		return glm::normalize(HitPoint - Center);
	}

	glm::vec3 Center;
	double Radius;
	KH_Material Material;
};


double RandomFloat()
{
	return Distrubution(Gen);
}



glm::vec3 RandomVec3()
{
	glm::vec3 RandomVec3;
	do {
		RandomVec3 = 2.0f * glm::vec3(RandomFloat(), RandomFloat(), RandomFloat()) - 1.0f;
	} while (glm::dot(RandomVec3, RandomVec3) > 1.0f);
	return glm::normalize(RandomVec3);
}

glm::vec3 RandomDirection(glm::vec3& Normal)
{
	glm::vec3 Direction = glm::normalize(Normal + RandomVec3());
	return Direction;
}


unsigned int GetImageByteSize(const int Width, const int Height, const int Channel)
{
	return Width * Height * Channel;
}

void SaveImage() {
	const int Width = 800;
	const int Height = 800;
	const int Channel = 3;
	const unsigned int TotalBytes = GetImageByteSize(Width, Height, Channel);
	std::vector<unsigned char> PixelData(TotalBytes);
	unsigned int PixelIndex = 0;

	for (int w = 0; w < Width; w++)
	{
		for (int h = 0; h < Height; h++)
		{
			PixelData[PixelIndex++] = 255;
			PixelData[PixelIndex++] = (unsigned char)std::clamp(int(1.0f * h / Height * 255), 0, 255);
			PixelData[PixelIndex++] = (unsigned char)std::clamp(int(1.0f * w / Width * 255), 0, 255);
		}
	}
	
	stbi_write_png("output.png", Width, Height, Channel, PixelData.data(), Width * Channel);
}

void SaveImage(const char* FilePath, const int Width, const int Height, const int Channel, const void* Data)
{
	const int StrideBtyes = Width * Channel;
	stbi_write_png(FilePath, Width, Height, Channel, Data, StrideBtyes);
}

KH_Intersect CastRay(std::vector<std::unique_ptr<KH_IShape>>& Shapes,KH_Ray& Ray)
{
	KH_Intersect Intersect, temp;
	for (auto& PShape : Shapes) {
		temp = PShape->Intersect(Ray);
		if (temp.IsHit && temp.Distance < Intersect.Distance)
			Intersect = temp;
	}
	return Intersect;
}

glm::vec3 PathTracing(std::vector<std::unique_ptr<KH_IShape>>& Shapes, KH_Ray& Ray, unsigned int Depth)
{
	if (Depth > 8)
		return glm::vec3(0.0f);

	KH_Intersect Intersect = CastRay(Shapes, Ray);

	if (!Intersect.IsHit)
		return glm::vec3(0.0f);

	if (Intersect.Material.IsEmissive)
		return Intersect.Material.Color;

	double P = 0.8;

	if (RandomFloat() > 0.8)
		return glm::vec3(0.0f);
	

	KH_Ray RandomRay;
	RandomRay.OriginPoint = Intersect.HitPoint;
	RandomRay.Direction = RandomDirection(Intersect.Material.Normal);
	float cosine = std::max(glm::dot(Intersect.Material.Normal, RandomRay.Direction), 0.0f);

	glm::vec3 Color;

	double RandomValue = RandomFloat();

	if (RandomValue < Intersect.Material.SpecularRate)
	{

		glm::vec3 ReflectDirection = glm::reflect(Ray.Direction, Intersect.Material.Normal);
		RandomRay.Direction = glm::mix( ReflectDirection, RandomRay.Direction, Intersect.Material.ReflectRoughness);
		Color = PathTracing(Shapes, RandomRay, ++Depth) * cosine;
	}
	else if (RandomValue < Intersect.Material.RefractRate &&
		RandomValue >= Intersect.Material.SpecularRate)
	{
		float CosTheta = glm::dot(Ray.Direction, Intersect.Material.Normal);
		glm::vec3 Normal = Intersect.Material.Normal;
		float Eta = Intersect.Material.Eta; 

		float Ratio;
		if (CosTheta > 0) { // 从内部射向外部
			Normal = -Normal;
			Ratio = Eta; // n_inside / n_outside
		}
		else { // 从外部射向内部
			Ratio = 1.0f / Eta; // n_outside / n_inside
		}

		glm::vec3 RefractDir = glm::refract(Ray.Direction, Normal, Ratio);

		if (glm::length(RefractDir) < EPS) { // 发生全反射
			RandomRay.Direction = glm::reflect(Ray.Direction, Normal);
		}
		else {
			RandomRay.Direction = RefractDir;
		}

		// 关键：防止自交，向法线反方向微调起点
		RandomRay.OriginPoint = Intersect.HitPoint - Normal * float(EPS * 100.0f);
		Color = PathTracing(Shapes, RandomRay, Depth + 1);
	}
	else {
		glm::vec3 MaterialColor = Intersect.Material.Color;
		glm::vec3 LightColor = PathTracing(Shapes, RandomRay, ++Depth) * cosine;
		Color = MaterialColor * LightColor;
	}

	return Color/ float(P) ;
	
}

void Render(std::vector<std::unique_ptr<KH_IShape>>& Shapes)
{
	const int Width = 800;
	const int Height = 800;
	const int Channel = 3;
	const unsigned int TotalBytes = GetImageByteSize(Width, Height, Channel);
	std::vector<unsigned char> PixelData(TotalBytes);

	const glm::vec3 EyePos = glm::vec3(0.0f, 0.0f, 5.0f);
	const double ScreenZ = 1.0;

	auto ToByte = [](float c) -> unsigned char { 
		return (unsigned char)std::clamp(int(c * 255.0f), 0, 255); 
		};


	omp_set_num_threads(50); 
	#pragma omp parallel for
	for (int j = 0; j < Height; j++)
	{
		for (int i = 0; i < Width; i++)
		{
			KH_Ray Ray;
			Ray.OriginPoint = EyePos;

			double x = ((i + 0.5) / Width) * 2.0 - 1.0;
			double y = ((j + 0.5) / Height) * 2.0 - 1.0;

			x += (RandomFloat() - 0.5f) / double(Width);
			y += (RandomFloat() - 0.5f) / double(Height);

			y = -y;

			glm::vec3 CanvasPos = glm::vec3(x, y, ScreenZ);
			glm::vec3 Direction = CanvasPos - EyePos;

			Ray.Direction = glm::normalize(Direction);
			
			glm::vec3 Color = glm::vec3(0.0f);

			KH_Intersect Intersect = CastRay(Shapes, Ray);

			for (int k = 0; k < SAMPLE_COUNT; k++)
			{
				Color += PathTracing(Shapes, Ray, 0);
			}

			Color = float(2 * PI / SAMPLE_COUNT) * Color;


			int pixelOffset = (j * Width + i) * Channel; 
			PixelData[pixelOffset + 0] = ToByte(Color.r); 
			PixelData[pixelOffset + 1] = ToByte(Color.g); 
			PixelData[pixelOffset + 2] = ToByte(Color.b);
		}
	}

	SaveImage("output.png", Width, Height, Channel, PixelData.data());
}

int main()
{
	std::vector<std::unique_ptr<KH_IShape>> Shapes;

	float L = -1.0f, R = 1.0f; 
	float B = -1.0f, T = 1.0f; 
	float N = 1.0f, F = -3.0f; 

	//Shapes.push_back(std::make_unique<KH_Triangle>(
	//	glm::vec3(-0.5f, -0.5f, -0.5f),
	//	glm::vec3(0.0f, -0.5f, 0.5f),
	//	glm::vec3(0.5f, -0.5f, -0.5f),
	//	RED));

	auto Sphere1 = std::make_unique<KH_Sphere>(
		glm::vec3(-0.6, -0.8, 0.6), 0.2, 
		WHITE);
	Sphere1->Material.SpecularRate = 0.3;
	Sphere1->Material.ReflectRoughness = 0.1;

	auto Sphere2 = std::make_unique<KH_Sphere>(
		glm::vec3(-0.1, -0.7, 0.2), 0.3,
		WHITE);
	Sphere2->Material.RefractRate = 0.95;
	Sphere2->Material.Eta = 1.5;
	Sphere2->Material.RefractRoughness = 0.0;

	auto Sphere3 = std::make_unique<KH_Sphere>(
		glm::vec3(0.5, -0.6, -0.5), 0.4,
		WHITE);
	Sphere3->Material.SpecularRate = 0.3;

	Shapes.push_back(std::move(Sphere1));
	Shapes.push_back(std::move(Sphere2));
	Shapes.push_back(std::move(Sphere3));


	// --- 1. 地面 (Bottom) - 法线朝上 (+Y) ---
	Shapes.push_back(std::make_unique<KH_Triangle>(
		glm::vec3(L, B, N), 
		glm::vec3(R, B, F), 
		glm::vec3(L, B, F), 
		WHITE));
	Shapes.push_back(std::make_unique<KH_Triangle>(
		glm::vec3(L, B, N), 
		glm::vec3(R, B, N), 
		glm::vec3(R, B, F), 
		WHITE));

	//// --- 2. 天花板 (Top) - 法线朝下 (-Y) ---
	Shapes.push_back(std::make_unique<KH_Triangle>(
		glm::vec3(L, T, N), 
		glm::vec3(L, T, F), 
		glm::vec3(R, T, F), 
		WHITE));
	Shapes.push_back(std::make_unique<KH_Triangle>(
		glm::vec3(L, T, N), 
		glm::vec3(R, T, F), 
		glm::vec3(R, T, N), WHITE));

	// --- 3. 后壁 (Back Wall) - 法线朝前 (+Z) ---
	Shapes.push_back(std::make_unique<KH_Triangle>(
		glm::vec3(L, B, F), 
		glm::vec3(R, B, F), 
		glm::vec3(R, T, F), 
		WHITE));
	Shapes.push_back(std::make_unique<KH_Triangle>(
		glm::vec3(L, B, F), 
		glm::vec3(R, T, F), 
		glm::vec3(L, T, F),
		WHITE));

	// --- 4. 左墙 (Left Wall) - 法线朝右 (+X) ---
	Shapes.push_back(std::make_unique<KH_Triangle>(
		glm::vec3(L, B, F), 
		glm::vec3(L, T, N), 
		glm::vec3(L, B, N), 
		RED));
	Shapes.push_back(std::make_unique<KH_Triangle>(
		glm::vec3(L, B, F), 
		glm::vec3(L, T, F), 
		glm::vec3(L, T, N), 
		RED));

	// --- 5. 右墙 (Right Wall) - 法线朝左 (-X) ---
	Shapes.push_back(std::make_unique<KH_Triangle>(
		glm::vec3(R, B, N), 
		glm::vec3(R, T, F), 
		glm::vec3(R, B, F), 
		GREEN));
	Shapes.push_back(std::make_unique<KH_Triangle>(
		glm::vec3(R, B, N), 
		glm::vec3(R, T, N), 
		glm::vec3(R, T, F), 
		GREEN));

	float lightY = T - 0.01f;
	auto l1 = std::make_unique<KH_Triangle>(
		glm::vec3(-0.5, lightY, 0.0), 
		glm::vec3(0.5, lightY, -1.0),  
		glm::vec3(0.5, lightY, 0.0), 
		WARM_WHITE * 2.0f);

	auto l2 = std::make_unique<KH_Triangle>(
		glm::vec3(-0.5, lightY,  0.0),
		glm::vec3(-0.5, lightY, -1.0), 
		glm::vec3(0.5, lightY, -1.0),  
		WARM_WHITE * 2.0f);
	l1->Material.IsEmissive = true;
	l2->Material.IsEmissive = true;
	Shapes.push_back(std::move(l1));
	Shapes.push_back(std::move(l2));



	Render(Shapes);

	return 0;
}