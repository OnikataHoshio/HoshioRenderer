#version 460 core
out vec4 FragColor;

in vec3 CanvasPos; 

struct Primitive{
    vec4 P1, P2, P3;
    vec4 N1, N2, N3;
    ivec2 PrimitiveType;
    ivec2 MaterialSlot;
};

struct EncodedBSDFMaterial{
	vec4 Emissive;  
	vec4 BaseColor;
    vec4 Param1;
    vec4 Param2;
    vec4 Param3;
};

struct BSDFMaterial{
	vec3 Emissive; 
	vec3 BaseColor;
	float Subsurface;
	float Metallic;
	float Specular;
	float SpecularTint;
	float Roughness;
	float Anisotropic;
	float Sheen;
	float SheenTint;
	float Clearcoat;
	float ClearcoatGloss;
    float IOR;
    float Transmission;
};

struct LBVHNode{
    ivec4 Param1;
    ivec4 Param2;
    vec4 AABB_MinPos;
    vec4 AABB_MaxPos;
};

layout(std430, binding = 0) buffer PrimitiveSSBO { Primitive Primitives[]; };
layout(std430, binding = 1) buffer Morton3DBuffer { uvec2 SortedMorton3D[]; };
layout(std430, binding = 2) buffer LBVHNodeBuffer { LBVHNode LBVHNodes[]; };
layout(std430, binding = 3) buffer AuxiliaryBuffer { ivec4 Root; };
layout(std430, binding = 4) buffer EncodedBSDFMaterialSSBO{ EncodedBSDFMaterial Materials[]; };

layout(std140, binding = 5) uniform CameraBlock {
    vec4 AspectAndFovy; // x: Aspect, y: Fovy
    vec4 Position;
    vec4 Right;
    vec4 Up;
    vec4 Front;
} UCameraParam;

uniform sampler2D uLastFrame;
uniform sampler2D uSkybox;
uniform sampler2D uHDRCache;
uniform int uLBVHNodeCount;
uniform uint uFrameCounter;
uniform uvec2 uResolution; 

uniform int uEnableSobol;
uniform int uEnableSkybox;


const vec3 SkyColor = vec3(1.0, 1.0, 1.0);

struct Ray
{
	vec3 Start;
	vec3 Direction;
};

struct HitResult {
	bool bIsHit;
    bool bIsInside;
	float Distance;
	vec3 HitPoint;
	vec3 GeoNormal;
    vec3 ShadeNormal;
    int MaterialSlot;
};

#define INF             1e30
#define EPS             1e-8
#define PI              3.14159265358979323846

uint rngState;

void InitRNG()
{
    uvec2 fc = uvec2(gl_FragCoord.xy);
    rngState = uint(fc.x) * 1973u + uint(fc.y) * 9277u + uint(uFrameCounter) * 26699u + 1u;
}

uint wang_hash(uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

float rand()
{
    rngState = wang_hash(rngState);
    return float(rngState) / 4294967296.0;
}


BSDFMaterial DecodeBSDFMaterial(int MaterialSlotID)
{
    BSDFMaterial Material;
    EncodedBSDFMaterial EncodedMaterial = Materials[MaterialSlotID];

    Material.BaseColor = EncodedMaterial.BaseColor.xyz;
    Material.Emissive = EncodedMaterial.Emissive.xyz;
    Material.Subsurface = EncodedMaterial.Param1.x;
    Material.Metallic = EncodedMaterial.Param1.y;
    Material.Specular = EncodedMaterial.Param1.z;
    Material.SpecularTint = EncodedMaterial.Param1.w;
    Material.Roughness = EncodedMaterial.Param2.x;
    Material.Anisotropic = EncodedMaterial.Param2.y;
    Material.Sheen = EncodedMaterial.Param2.z;
    Material.SheenTint = EncodedMaterial.Param2.w;
    Material.Clearcoat = EncodedMaterial.Param3.x;
    Material.ClearcoatGloss = EncodedMaterial.Param3.y;
    Material.IOR = EncodedMaterial.Param3.z;
    Material.Transmission = EncodedMaterial.Param3.w;

    return Material;
}

vec3 SampleHemisphere(float xi_1, float xi_2) {
    float z = xi_1;
    float r = max(0, sqrt(1.0 - z*z));
    float phi = 2.0 * PI * xi_2;
    return vec3(r * cos(phi), r * sin(phi), z);
}


vec3 ToNormalHemisphere(vec3 v, vec3 N) {
    N = normalize(N);
    vec3 helper = vec3(1, 0, 0);
    if(abs(N.x)>0.999) helper = vec3(0, 0, 1);
    vec3 tangent = normalize(cross(N, helper));
    vec3 bitangent = normalize(cross(N, tangent));
    return normalize(v.x * tangent + v.y * bitangent + v.z * N);
}

vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv /= vec2(2.0 * PI, PI);
    uv += 0.5;
    return uv;
}

vec3 SampleSkybox(vec3 v) {
    vec2 uv = SampleSphericalMap(normalize(v));
    vec3 color = texture(uSkybox, uv).rgb;
    return color;
}

vec3 SampleCosineHemisphere(float xi_1, float xi_2, vec3 N) {
    float r = sqrt(xi_1);
    float theta = xi_2 * 2.0 * PI;
    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(max(0.0, 1.0 - xi_1));

    vec3 L = ToNormalHemisphere(vec3(x, y, z), N);
    return L;
}

vec3 SampleGTR2(float xi_1, float xi_2, vec3 N, float alpha){
    alpha = max(alpha, 1e-3);
    float phi = 2.0 * PI * xi_1;
    float cosTheta = sqrt((1.0 - xi_2)/(1.0 + (alpha*alpha - 1.0) * xi_2));
    float sinTheta = sqrt(max(0, 1.0 - cosTheta*cosTheta));

    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    float z = cosTheta;

    vec3 H = vec3(x, y, z);
    H = ToNormalHemisphere(H, N);

    return H;
}

vec3 SampleGTR1(float xi_1, float xi_2, vec3 N, float alpha){
    alpha = max(alpha, 1e-3);
    float alpha2 = alpha * alpha;
    float phi = 2.0 * PI * xi_1;
    float cosTheta = sqrt((1.0 - pow(alpha2, 1- xi_2))/(1 - alpha2));
    float sinTheta = sqrt(max(0, 1.0 - cosTheta*cosTheta));

    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    float z = cosTheta;

    vec3 H = vec3(x, y, z);
    H = ToNormalHemisphere(H, N);

    return H;
}

vec3 GetRayDirection(vec2 uv)
{
    float scale = tan(radians(UCameraParam.AspectAndFovy.y) * 0.5);

    vec2 pixelSize = 2.0 / vec2(uResolution);

    uv += pixelSize * (rand() - 0.5);

    vec3 DirWorldSpace = (uv.x * UCameraParam.AspectAndFovy.x * scale) * UCameraParam.Right.xyz +
        (uv.y * scale) * UCameraParam.Up.xyz +
        UCameraParam.Front.xyz;

    return normalize(DirWorldSpace);
}

HitResult HitTriangle(int Primitive_index, Ray ray)
{
    HitResult hit_result;

    hit_result.bIsHit = false;
    hit_result.bIsInside = false;
    hit_result.Distance = INF;
    hit_result.MaterialSlot = -1;

    Primitive Primitive = Primitives[Primitive_index];

    vec3 p1 = Primitive.P1.xyz;
    vec3 p2 = Primitive.P2.xyz;
    vec3 p3 = Primitive.P3.xyz;

    vec3 edge1 = p2 - p1;
    vec3 edge2 = p3 - p1;

    vec3 Ng = normalize(cross(edge1, edge2));

    vec3 pvec = cross(ray.Direction, edge2);
    float det = dot(edge1, pvec);

    if (abs(det) < EPS) return hit_result;

    float invDet = 1.0 / det;

    vec3 tvec = ray.Start - p1;
    float u = dot(tvec, pvec) * invDet;
    if (u < 0.0 || u > 1.0) return hit_result;

    vec3 qvec = cross(tvec, edge1);
    float v = dot(ray.Direction, qvec) * invDet;
    if (v < 0.0 || u + v > 1.0) return hit_result;

    float t = dot(edge2, qvec) * invDet;
    if (t < EPS) return hit_result;

    hit_result.bIsHit = true;
    hit_result.Distance = t;
    hit_result.HitPoint = ray.Start + t * ray.Direction;
    hit_result.MaterialSlot = Primitive.MaterialSlot.x;

    float w1 = 1.0 - u - v;
    vec3 Ns = normalize(w1 * Primitive.N1.xyz + u * Primitive.N2.xyz + v * Primitive.N3.xyz);

    if (dot(Ng, ray.Direction) > 0.0){
        hit_result.bIsInside = true;
        Ng = -Ng;
    }

    if (dot(Ns, Ng) < 0.0)
        Ns = -Ns;

    hit_result.GeoNormal = Ng;
    hit_result.ShadeNormal = Ns;

    return hit_result;
}

HitResult Hit(Ray ray, int l, int r) 
{
    HitResult hit_result;
    hit_result.bIsHit = false;    
    hit_result.bIsInside = false;
    hit_result.Distance = INF;
    hit_result.MaterialSlot = -1; 

	for (int i = l; i <= r; i++)
	{
		HitResult temp = HitTriangle(int(SortedMorton3D[i].y), ray);
		if (temp.bIsHit && temp.Distance < hit_result.Distance)
			hit_result = temp;
	}
    return hit_result;
}

float HitAABB(int node_index, Ray ray)
{
    vec3 AABB_MinPos = vec3(LBVHNodes[node_index].AABB_MinPos);
    vec3 AABB_MaxPos = vec3(LBVHNodes[node_index].AABB_MaxPos);

    vec3 invDir = 1.0 / ray.Direction;
    
    vec3 t0s = (AABB_MinPos - ray.Start) * invDir;
    vec3 t1s = (AABB_MaxPos - ray.Start) * invDir;
    
    vec3 tmin = min(t0s, t1s);
    vec3 tmax = max(t0s, t1s);
    
    float t_start = max(tmin.x, max(tmin.y, tmin.z));
    float t_end = min(tmax.x, min(tmax.y, tmax.z));
    
    if (t_start <= t_end && t_end > 0.0)
    {
        return t_start > 0.0 ? t_start : t_end; 
    }

    return -1.0;
}

HitResult HitBVH(Ray ray)
{
    HitResult hit_result;
    hit_result.bIsHit = false;
    hit_result.bIsInside = false;
    hit_result.Distance = INF;
    hit_result.MaterialSlot = -1; 

    int stack[256];
    int top = 0;

    stack[top++] = Root.x;

    while(top > 0)
    {
        int cur_node_idx = stack[--top];
        
        if(cur_node_idx < 0 || cur_node_idx >= uLBVHNodeCount) 
            continue;

        int left = LBVHNodes[cur_node_idx].Param1.x;
        int right = LBVHNodes[cur_node_idx].Param1.y;
        int isLeaf = LBVHNodes[cur_node_idx].Param1.z;
        int front = LBVHNodes[cur_node_idx].Param2.x;
        int back = LBVHNodes[cur_node_idx].Param2.y;

        if(isLeaf == 1)
        {
            HitResult temp = Hit(ray, front , back); 
            if (temp.bIsHit && temp.Distance < hit_result.Distance)
			    hit_result = temp;
            continue;
        }
        
        float t_left = -INF;
        float t_right = -INF;

        if(left >= 0 && left < uLBVHNodeCount)
            t_left = HitAABB(left, ray);
        if(right >= 0 && right < uLBVHNodeCount)
            t_right = HitAABB(right, ray);

        if(t_left > 0 && t_right > 0)
        {
            if(t_left > t_right)
            {
                stack[top++] = left;
                stack[top++] = right;
            }else{
                stack[top++] = right;
                stack[top++] = left;
            }
        }
        else if (t_left > 0.0) {
            stack[top++] = left;
        } else if (t_right > 0.0) { 
            stack[top++] = right;
        }
    }
    
    return hit_result;
}

float sqr(float x) { return x*x; }

float SchlickFresnel(float u)
{
    float m = clamp(1-u, 0, 1);
    float m2 = m*m;
    return m2*m2*m; // pow(m,5)
}

bool SameHemisphere(vec3 a, vec3 b, vec3 N)
{
    return dot(a, N) * dot(b, N) > 0.0;
}

float AbsDot(vec3 a, vec3 b)
{
    return abs(dot(a, b));
}

float FrDielectric(float cosThetaI, float eta)
{
    cosThetaI = clamp(cosThetaI, -1.0, 1.0);

    float sin2ThetaI = max(0.0, 1.0 - cosThetaI * cosThetaI);
    float sin2ThetaT = eta * eta * sin2ThetaI;

    if (sin2ThetaT >= 1.0)
        return 1.0;

    float cosThetaT = sqrt(max(0.0, 1.0 - sin2ThetaT));
    float absCosI = abs(cosThetaI);

    float Rs = (absCosI - eta * cosThetaT) / max(absCosI + eta * cosThetaT, 1e-6);
    float Rp = (eta * absCosI - cosThetaT) / max(eta * absCosI + cosThetaT, 1e-6);

    return 0.5 * (Rs * Rs + Rp * Rp);
}

vec3 HalfVectorReflection(vec3 wi, vec3 wo)
{
    vec3 h = wi + wo;
    float len2 = dot(h, h);
    if (len2 <= 1e-12)
        return vec3(0.0);
    return h * inversesqrt(len2);
}

vec3 HalfVectorTransmission(vec3 wi, vec3 wo, float eta_wi2wo)
{
    vec3 h =  wi * eta_wi2wo + wo;
    float len2 = dot(h, h);
    if (len2 <= 1e-12)
        return vec3(0.0);
    return h * inversesqrt(len2);
}

float GTR1(float NdotH, float a)
{
    a = max(a, 1e-3);
    if (a >= 1) return 1/PI;
    float a2 = a*a;
    float t = 1 + (a2-1)*NdotH*NdotH;
    return (a2-1) / (PI*log(a2)*t);
}

float GTR2(float NdotH, float a)
{
    a = max(a, 1e-3);
    float a2 = a*a;
    float t = 1 + (a2-1)*NdotH*NdotH;
    return a2 / (PI * t*t);
}

float GTR2_aniso(float NdotH, float HdotX, float HdotY, float ax, float ay)
{
    return 1 / (PI * ax*ay * sqr( sqr(HdotX/ax) + sqr(HdotY/ay) + NdotH*NdotH ));
}

float smithG_GGX(float NdotV, float alphaG)
{
    float a = alphaG*alphaG;
    float b = NdotV*NdotV;
    return 1 / (NdotV + sqrt(a + b - a*b));
}

float smithG_GGX_aniso(float NdotV, float VdotX, float VdotY, float ax, float ay)
{
    return 1 / (NdotV + sqrt( sqr(VdotX*ax) + sqr(VdotY*ay) + sqr(NdotV) ));
}


float smithG1_GGX(float NdotV, float alpha)
{
    NdotV = max(NdotV, 1e-6);
    alpha = max(alpha, 1e-3);

    float a2 = alpha * alpha;
    float n2 = NdotV * NdotV;
    return (2.0 * NdotV) / (NdotV + sqrt(a2 + (1.0 - a2) * n2));
}

float smithG_GGX_Glass(float NdotL, float NdotV, float alpha)
{
    return smithG1_GGX(NdotL, alpha) * smithG1_GGX(NdotV, alpha);
}

vec3 mon2lin(vec3 x)
{
    return pow(x, vec3(2.2));
}

vec3 EvalBRDF(vec3 L, vec3 V, vec3 N, int MaterialSlotID)
{
    BSDFMaterial Mat = DecodeBSDFMaterial(MaterialSlotID);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    if (NdotL <= 0.0 || NdotV <= 0.0) return vec3(0.0);

    vec3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);
    float LdotH = max(dot(L, H), 0.0);

    vec3 Cdlin = Mat.BaseColor;
    float Cdlum = 0.3 * Cdlin[0] + 0.6 * Cdlin[1] + 0.1 * Cdlin[2];

    vec3 Ctint = Cdlum > 0.0 ? Cdlin / Cdlum : vec3(1.0);
    vec3 Cspec0 = mix(Mat.Specular * 0.08 * mix(vec3(1.0), Ctint, Mat.SpecularTint), Cdlin, Mat.Metallic);
    vec3 Csheen = mix(vec3(1.0), Ctint, Mat.SheenTint);

    float FL = SchlickFresnel(NdotL), FV = SchlickFresnel(NdotV);
    float Fd90 = 0.5 + 2.0 * LdotH * LdotH * Mat.Roughness;
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

    float Fss90 = LdotH * LdotH * Mat.Roughness;
    float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1.0 / max(NdotL + NdotV, 1e-6) - 0.5) + 0.5);

    vec3 diffuse = (1.0 / PI) * mix(Fd, ss, Mat.Subsurface) * Cdlin;

    float roughness = max(Mat.Roughness, 0.02);
    float alpha = roughness * roughness;

    float Ds = GTR2(NdotH, alpha);
    float FH = SchlickFresnel(LdotH);
    vec3 Fs = mix(Cspec0, vec3(1.0), FH);
    float Gs = smithG_GGX(NdotL, roughness) * smithG_GGX(NdotV, roughness);

    vec3 specular = Gs * Fs * Ds;

    vec3 Fsheen = FH * Mat.Sheen * Csheen;
    diffuse += Fsheen;

    float Dr = GTR1(NdotH, mix(0.1, 0.001, Mat.ClearcoatGloss));
    float Fr = mix(0.04, 1.0, FH);
    float Gr = smithG_GGX(NdotL, 0.25) * smithG_GGX(NdotV, 0.25);

    return diffuse * (1.0 - Mat.Metallic)
         + specular
         + 0.25 * Mat.Clearcoat * Gr * Fr * Dr; 
}



vec3 EvalGlassReflection(vec3 wi, vec3 wo, vec3 Ns, float eta, int MaterialSlotID)
{
    if (!SameHemisphere(wi, wo, Ns))
        return vec3(0.0);

    float NdotL = AbsDot(Ns, wi);
    float NdotV = AbsDot(Ns, wo);
    if (NdotL <= 1e-6 || NdotV <= 1e-6)
        return vec3(0.0);

    BSDFMaterial Mat = DecodeBSDFMaterial(MaterialSlotID);

    float roughness = max(Mat.Roughness, 0.01);
    float alpha = roughness * roughness;

    vec3 H = HalfVectorReflection(wi, wo);
    if (dot(H, H) <= 0.0)
        return vec3(0.0);

    if (dot(H, Ns) < 0.0)
        H = -H;

    float NdotH = AbsDot(Ns, H);
    float VdotH = AbsDot(wo, H);
    if (NdotH <= 1e-6 || VdotH <= 1e-6)
        return vec3(0.0);

    float Fg = FrDielectric(dot(wi, H), eta);
    float Dg = GTR2(NdotH, alpha);
    float Gg = smithG_GGX_Glass(NdotL, NdotV, roughness);

    float denom = 4.0 * NdotL * NdotV;

    return Mat.BaseColor * Fg * Dg * Gg / denom;
}


vec3 EvalGlassTransmission(vec3 wi, vec3 wo, vec3 Ns, float eta_wi2wo, int MaterialSlotID)
{
    if (SameHemisphere(wi, wo, Ns))
        return vec3(0.0);

    BSDFMaterial Mat = DecodeBSDFMaterial(MaterialSlotID);

    float NdotL = AbsDot(Ns, wi);
    float NdotV = AbsDot(Ns, wo);
    if (NdotL <= 1e-6 || NdotV <= 1e-6)
        return vec3(0.0);

    float roughness = max(Mat.Roughness, 0.01);
    float alpha = roughness * roughness;

    vec3 H = HalfVectorTransmission(wi, wo, eta_wi2wo);
    if (dot(H, H) <= 0.0)
        return vec3(0.0);

    if (dot(H, Ns) < 0.0)
        H = -H;

    float cosWi = dot(Ns, wi);
    float cosWo = dot(Ns, wo);
    if (dot(H, wi) * cosWi < 0.0 || dot(H, wo) * cosWo < 0.0)
        return vec3(0.0);

    float NdotH = AbsDot(Ns, H);
    float VdotH = AbsDot(wo, H);
    float LdotH = AbsDot(wi, H);

    if (NdotH <= 1e-6 || VdotH <= 1e-6 || LdotH <= 1e-6)
        return vec3(0.0);

    float Fg = FrDielectric(dot(wi, H), eta_wi2wo);
    float Dg = GTR2(NdotH, alpha);
    float Gg = smithG_GGX_Glass(NdotL, NdotV, roughness);


    float sqrtDenom = eta_wi2wo * LdotH + VdotH;
    float denom = max(NdotL * NdotV * sqrtDenom * sqrtDenom, 1e-6);

    vec3 tint = sqrt(max(Mat.BaseColor, vec3(0.0)));

    return tint * abs((1.0 - Fg) * Dg * Gg * LdotH * VdotH / denom);
}


vec3 BSDF(vec3 wi, vec3 wo, vec3 Ns, vec3 Ng, float p_glass, bool bIsInside, int MaterialSlotID)
{
    BSDFMaterial Mat = DecodeBSDFMaterial(MaterialSlotID);

    float eta_wi2wo = bIsInside ?  (1.0 / max(Mat.IOR, 1.0001)) : max(Mat.IOR, 1.0001);
    bool sameSide = dot(Ng, wi) * dot(Ng, wo) > 0.0;

    vec3 f = vec3(0.0);

    f += (1.0 - p_glass) * EvalBRDF(wi, wo, Ns, MaterialSlotID);

    if (sameSide)
        f += p_glass * EvalGlassReflection(wi, wo, Ns, 1.0 / eta_wi2wo, MaterialSlotID);
    else
        f += p_glass * EvalGlassTransmission(wi, wo, Ns, eta_wi2wo, MaterialSlotID);

    return f;
}

struct SampleBSDFResult
{
    vec3 wi;
    float cosTheta;
    float PDF;
    float p_glass;
};

float PdfDiffuseIS(vec3 wi, vec3 Ns)
{
    float cosTheta = dot(Ns, wi);
    return (cosTheta > 0.0) ? (cosTheta / PI) : 0.0;
}

float PdfSpecularIS(vec3 wi, vec3 V, vec3 Ns, float alpha)
{
    if (dot(Ns, wi) <= 0.0 || dot(Ns, V) <= 0.0)
        return 0.0;

    vec3 H = wi + V;
    float HLen2 = dot(H, H);
    if (HLen2 <= 1e-12)
        return 0.0;
    H *= inversesqrt(HLen2);

    float NdotH = dot(Ns, H);
    float VdotH = dot(V, H);

    if (NdotH <= 0.0 || VdotH <= 1e-6)
        return 0.0;

    return GTR2(NdotH, alpha) * NdotH / (4.0 * VdotH);
}

float PdfClearcoatIS(vec3 wi, vec3 V, vec3 Ns, float alpha)
{
    if (dot(Ns, wi) <= 0.0 || dot(Ns, V) <= 0.0)
        return 0.0;

    vec3 H = wi + V;
    float HLen2 = dot(H, H);
    if (HLen2 <= 1e-12)
        return 0.0;
    H *= inversesqrt(HLen2);

    float NdotH = dot(Ns, H);
    float VdotH = dot(V, H);

    if (NdotH <= 0.0 || VdotH <= 1e-6)
        return 0.0;

    return GTR1(NdotH, alpha) * NdotH / (4.0 * VdotH);
}


float PdfGlassReflectionIS(vec3 wi, vec3 wo, vec3 Ns, float eta_wo2wi, float alpha)
{
    if (!SameHemisphere(wi, wo, Ns))
        return 0.0;

    vec3 H = HalfVectorReflection(wi, wo);
    if (dot(H, H) <= 0.0)
        return 0.0;

    if (dot(H, Ns) < 0.0)
        H = -H;

    float NdotH = AbsDot(Ns, H);
    float VdotH = AbsDot(wo, H);

    if (NdotH <= 1e-6 || VdotH <= 1e-6)
        return 0.0;

    float pdfH = GTR2(NdotH, alpha) * NdotH;
    float Fg = FrDielectric(dot(wo, H), eta_wo2wi);

    return  Fg * pdfH / max(4.0 * VdotH, 1e-6);
}

float PdfGlassTransmissionIS(vec3 wi, vec3 wo, vec3 Ns, float eta_wo2wi, float alpha)
{
    if (SameHemisphere(wi, wo, Ns))
        return 0.0;

    float eta_wi2wo = 1.0/eta_wo2wi;
    vec3 H = HalfVectorTransmission(wi, wo, eta_wi2wo);
    if (dot(H, H) <= 0.0)
        return 0.0;

    if (dot(H, Ns) < 0.0)
        H = -H;

    float NdotH = AbsDot(Ns, H);
    float VdotH = AbsDot(wo, H);
    float LdotH = AbsDot(wi, H);

    if (NdotH <= 1e-6 || VdotH <= 1e-6 || LdotH <= 1e-6)
        return 0.0;

    float pdfH = GTR2(NdotH, alpha) * NdotH;
    float Fg = FrDielectric(dot(wo, H), eta_wo2wi);

    float sqrtDenom = eta_wo2wi * VdotH + LdotH;
    float dwh_dwi = abs((LdotH) / max(sqrtDenom * sqrtDenom, 1e-6));

    return  (1 - Fg) * pdfH * dwh_dwi;
}

float BRDF_PDF_Eval(vec3 wi, vec3 V, vec3 Ns, int MaterialSlotID)
{
    if (dot(Ns, wi) <= 0.0 || dot(Ns, V) <= 0.0)
        return 0.0;

    BSDFMaterial Mat = DecodeBSDFMaterial(MaterialSlotID);

    float alpha_GTR1 = mix(0.1, 0.001, Mat.ClearcoatGloss);
    float alpha_GTR2 = max(0.001, Mat.Roughness * Mat.Roughness);

    float r_diffuse   = (1.0 - Mat.Metallic) * (1.0 - Mat.Transmission);
    float r_specular  = 1.0 - Mat.Transmission * (1.0 - Mat.Metallic);
    float r_clearcoat = 0.25 * Mat.Clearcoat;
    float r_sum       = max(r_diffuse + r_specular + r_clearcoat, 1e-8);

    float p_diffuse   = r_diffuse   / r_sum;
    float p_specular  = r_specular  / r_sum;
    float p_clearcoat = r_clearcoat / r_sum;

    return
        p_diffuse   * PdfDiffuseIS(wi, Ns) +
        p_specular  * PdfSpecularIS(wi, V, Ns, alpha_GTR2) +
        p_clearcoat * PdfClearcoatIS(wi, V, Ns, alpha_GTR1);
}


float BSDF_Glass_PDF_Eval(vec3 wi, vec3 wo, vec3 Ns, bool bIsInside, int MaterialSlotID)
{
    BSDFMaterial Mat = DecodeBSDFMaterial(MaterialSlotID);

    float roughness = max(Mat.Roughness, 0.01);
    float alpha = roughness * roughness;
    float eta_wo2wi = bIsInside ? max(Mat.IOR, 1.0001) : (1.0 / max(Mat.IOR, 1.0001));

    if (SameHemisphere(wi, wo, Ns))
        return PdfGlassReflectionIS(wi, wo, Ns, eta_wo2wi, alpha);
    else
        return PdfGlassTransmissionIS(wi, wo, Ns, eta_wo2wi, alpha);
}

float BSDF_PDF_Eval(vec3 wi, vec3 wo, vec3 Ns, float p_glass, bool bIsInside, int MaterialSlotID)
{
    BSDFMaterial Mat = DecodeBSDFMaterial(MaterialSlotID);

    float pGlass = p_glass;
    float pBRDF  = 1.0 - pGlass;

    float pdf_brdf  = BRDF_PDF_Eval(wi, wo, Ns, MaterialSlotID);
    float pdf_glass = BSDF_Glass_PDF_Eval(wi, wo, Ns, bIsInside, MaterialSlotID);

    return pBRDF * pdf_brdf + pGlass * pdf_glass;
}

float EnvPdf_Eval(vec3 wi)
{
    vec2 uv = SampleSphericalMap(normalize(wi));
    ivec2 size = textureSize(uHDRCache, 0);
    ivec2 p = ivec2(
        min(int(uv.x * float(size.x)), size.x - 1),
        min(int(uv.y * float(size.y)), size.y - 1)
    );
    float pdfUV = texelFetch(uHDRCache, p, 0).a;

    float elev = PI * (uv.y - 0.5);
    float sinThetaPolar = max(cos(elev), 1e-6);

    return pdfUV / (2.0 * PI * PI * sinThetaPolar);
}

SampleBSDFResult SampleBRDF(vec3 V, vec3 Ns, int MaterialSlotID)
{
    BSDFMaterial Mat = DecodeBSDFMaterial(MaterialSlotID);

    float alpha_GTR1 = mix(0.1, 0.001, Mat.ClearcoatGloss);
    float alpha_GTR2 = max(0.001, Mat.Roughness * Mat.Roughness);

    float r_diffuse   = (1.0 - Mat.Metallic) * (1.0 - Mat.Transmission);
    float r_specular  = 1.0 - Mat.Transmission * (1.0 - Mat.Metallic);
    float r_clearcoat = 0.25 * Mat.Clearcoat;
    float r_sum       = max(r_diffuse + r_specular + r_clearcoat, 1e-8);

    float p_diffuse   = r_diffuse   / r_sum;
    float p_specular  = r_specular  / r_sum;
    float p_clearcoat = r_clearcoat / r_sum;

    float xi_1 = rand();
    float xi_2 = rand();
    float xi_3 = rand();

    SampleBSDFResult result;
    result.wi = vec3(0.0);
    result.cosTheta = 0.0;
    result.PDF = 0.0;

    vec3 wi = vec3(0.0);

    if (xi_3 <= p_diffuse)
    {
        wi = SampleCosineHemisphere(xi_1, xi_2, Ns);
    }
    else if (xi_3 <= p_diffuse + p_specular)
    {
        vec3 H = SampleGTR2(xi_1, xi_2, Ns, alpha_GTR2);
        if (dot(V, H) < 0.0) H = -H;

        float VdotH = dot(V, H);
        if (VdotH <= 1e-6)
            return result;

        wi = reflect(-V, H);
    }
    else
    {
        vec3 H = SampleGTR1(xi_1, xi_2, Ns, alpha_GTR1);
        if (dot(V, H) < 0.0) H = -H;

        float VdotH = dot(V, H);
        if (VdotH <= 1e-6)
            return result;

        wi = reflect(-V, H);
    }

    float cosTheta = dot(Ns, wi);
    if (cosTheta <= 0.0)
        return result;

    result.wi = wi;
    result.cosTheta = cosTheta;
    result.PDF = max(BRDF_PDF_Eval(wi, V, Ns, MaterialSlotID), 1e-8);
    return result;
}

SampleBSDFResult SampleBSDF_Glass(vec3 wo, vec3 Ns, bool bIsInside, int MaterialSlotID)
{
    BSDFMaterial Mat = DecodeBSDFMaterial(MaterialSlotID);

    float roughness = max(Mat.Roughness, 0.01);
    float alpha = roughness * roughness;
    float eta_wo2wi = bIsInside ? max(Mat.IOR, 1.0001) : (1.0 / max(Mat.IOR, 1.0001));

    SampleBSDFResult result;
    result.wi = vec3(0.0);
    result.cosTheta = 0.0;
    result.PDF = 0.0;

    vec3 H = SampleGTR2(rand(), rand(), Ns, alpha);

    if (dot(wo, H) < 0.0)
        H = -H;

    float Fg = FrDielectric(dot(wo, H), eta_wo2wi);

    vec3 wi_reflect = reflect(-wo, H);
    vec3 wi_refract = refract(-wo, H, eta_wo2wi);

    bool tir = dot(wi_refract, wi_refract) <= 1e-12;
    float xi = rand();

    if (tir || xi < Fg)
    {
        result.wi = wi_reflect;

        if (dot(Ns, result.wi) <= 0.0)
        {
            result.wi = vec3(0.0);
            return result;
        }
    }
    else
    {
        result.wi = wi_refract;

        if (dot(Ns, result.wi) * dot(Ns, wo) >= 0.0)
        {
            result.wi = vec3(0.0);
            return result;
        }
    }

    result.cosTheta = AbsDot(Ns, result.wi);
    return result;
}

SampleBSDFResult SampleBSDF(vec3 wo, vec3 Ns, vec3 Ng, bool bIsInside, int MaterialSlotID)
{
    SampleBSDFResult result;
    result.wi = vec3(0.0);
    result.cosTheta = 0.0;
    result.PDF = 0.0;
    result.p_glass = 0.0;

    BSDFMaterial Mat = DecodeBSDFMaterial(MaterialSlotID);

    float r_diffuse   = (1.0 - Mat.Metallic) * (1.0 - Mat.Transmission);
    float r_specular  = 1.0 - Mat.Transmission * (1.0 - Mat.Metallic);
    float r_glass = (1.0 - Mat.Metallic) * Mat.Transmission;
    float r_clearcoat = 0.25 * Mat.Clearcoat;
    float r_sum       = max(r_diffuse + r_specular + r_glass + r_clearcoat, 1e-8);

    float p_glass = r_glass / r_sum;

    float xi = rand();

    if (xi < p_glass)
        result = SampleBSDF_Glass(wo, Ns, bIsInside, MaterialSlotID);
    else
        result = SampleBRDF(wo, Ns, MaterialSlotID);

    if (dot(result.wi, result.wi) <= 0.0)
        return result;

    result.cosTheta = abs(dot(Ns, result.wi));
    result.PDF = max(BSDF_PDF_Eval(result.wi, wo, Ns, p_glass, bIsInside, MaterialSlotID), 1e-8);
    result.p_glass = p_glass;

    return result;
}

vec3 PathTracing(Ray ray, int maxBounce)
{
    vec3 finalColor = vec3(0.0);
    vec3 throughput = vec3(1.0);
    Ray currRay = ray;

    uvec2 pix = uvec2(gl_FragCoord.xy);

    float xi_bsdf1 = rand();
    float xi_bsdf2 = rand();
    float xi_env1 = rand();
    float xi_env2 = rand();
    float xi_1 = rand();

    float pdf_bsdf = 0.0;

    for (int bounce = 0; bounce < maxBounce; bounce++)
    {
        HitResult hit_result = HitBVH(currRay);

        vec3 V  = -currRay.Direction;

        if (!hit_result.bIsHit)
        {
            if(uEnableSkybox == 1)
            {
                finalColor += throughput * SampleSkybox(currRay.Direction);
            }
            else
            {
                finalColor += throughput * SkyColor;
            }
            break;
        }

        xi_bsdf1 = rand();
        xi_bsdf2 = rand();
        xi_env1 = rand();
        xi_env2 = rand();
        xi_1 = rand();

        vec3 Ng = normalize(hit_result.GeoNormal);
        vec3 Ns = normalize(hit_result.ShadeNormal);

        SampleBSDFResult sample_result;
        sample_result.wi = vec3(0.0);
        sample_result.cosTheta = 0.0;
        sample_result.PDF = 0.0;

        BSDFMaterial Mat = DecodeBSDFMaterial(hit_result.MaterialSlot);
        finalColor += throughput * Mat.Emissive;

        sample_result = SampleBSDF(V, Ns, Ng, hit_result.bIsInside, hit_result.MaterialSlot);
        
        if (sample_result.cosTheta <= 0.0 || sample_result.PDF <= 1e-8)
            break;

        pdf_bsdf = sample_result.PDF;

        throughput *= BSDF(sample_result.wi, V, Ns, Ng, sample_result.p_glass, hit_result.bIsInside, hit_result.MaterialSlot)
           * sample_result.cosTheta /
           sample_result.PDF;

        currRay.Start = hit_result.HitPoint + sign(dot(sample_result.wi, Ng)) * Ng * 1e-4;
        currRay.Direction = sample_result.wi;

        if (bounce >= 3)
        {
            float p = clamp(max(throughput.r, max(throughput.g, throughput.b)), 0.05, 0.95);
            if (rand() > p)
                break;
            throughput /= p;
        }
    }

    return finalColor;
}

void main()
{
    InitRNG();

    Ray ray;
    ray.Start = UCameraParam.Position.xyz;
    ray.Direction = GetRayDirection(CanvasPos.xy);

    vec4 Color = vec4(PathTracing(ray, 8), 1.0);
    ivec2 pix = ivec2(gl_FragCoord.xy);

    vec4 LastFrameColor = texelFetch(uLastFrame, pix , 0);

    if(uFrameCounter < 4096)
        FragColor = mix(LastFrameColor, Color, 1.0/float(uFrameCounter+1));
    else
        FragColor = LastFrameColor;
}
