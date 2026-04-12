#version 460 core
out vec4 FragColor;

in vec3 CanvasPos; 

struct Primitive{
    vec4 P1, P2, P3;
    vec4 N1, N2, N3;
    ivec2 PrimitiveType;
    ivec2 MaterialSlot;
};

struct EncodedBRDFMaterial{
	vec4 Emissive;  
	vec4 BaseColor;
    vec4 Param1;
    vec4 Param2;
    vec4 Param3;
};

struct BRDFMaterial{
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
layout(std430, binding = 4) buffer EncodedBRDFMaterialSSBO{ EncodedBRDFMaterial Materials[]; };

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

uniform int uEnableSkybox;
uniform int uEnableImportanceSampling;
uniform int uEnableMIS;

const vec3 SkyColor = vec3(0.05, 0.05, 0.05);

struct Ray
{
	vec3 Start;
	vec3 Direction;
};

struct HitResult {
	bool bIsHit;
    // bool BIsInside;
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

const uint V[16*32] = {
	2147483648, 1073741824, 536870912, 268435456, 134217728, 67108864, 33554432, 
	16777216, 8388608, 4194304, 2097152, 1048576, 524288, 262144, 131072, 65536, 
	32768, 16384, 8192, 4096, 2048, 1024, 512, 256, 128, 64, 32, 16, 8, 4, 2, 1,
	2147483648, 3221225472, 2684354560, 4026531840, 2281701376, 3422552064, 
	2852126720, 4278190080, 2155872256, 3233808384, 2694840320, 4042260480, 
	2290614272, 3435921408, 2863267840, 4294901760, 2147516416, 3221274624, 
	2684395520, 4026593280, 2281736192, 3422604288, 2852170240, 4278255360, 
	2155905152, 3233857728, 2694881440, 4042322160, 2290649224, 3435973836, 
	2863311530, 4294967295,
	2147483648, 3221225472, 1610612736, 2415919104, 3892314112, 1543503872, 
	2382364672, 3305111552, 1753219072, 2629828608, 3999268864, 1435500544, 
	2154299392, 3231449088, 1626210304, 2421489664, 3900735488, 1556135936, 
	2388680704, 3314585600, 1751705600, 2627492864, 4008611328, 1431684352, 
	2147543168, 3221249216, 1610649184, 2415969680, 3892340840, 1543543964, 
	2382425838, 3305133397,
	2147483648, 3221225472, 536870912, 1342177280, 4160749568, 1946157056, 
	2717908992, 2466250752, 3632267264, 624951296, 1507852288, 3872391168, 
	2013790208, 3020685312, 2181169152, 3271884800, 546275328, 1363623936, 
	4226424832, 1977167872, 2693105664, 2437829632, 3689389568, 635137280, 
	1484783744, 3846176960, 2044723232, 3067084880, 2148008184, 3222012020, 
	537002146, 1342505107,
	2147483648, 1073741824, 536870912, 2952790016, 4160749568, 3690987520, 
	2046820352, 2634022912, 1518338048, 801112064, 2707423232, 4038066176, 
	3666345984, 1875116032, 2170683392, 1085997056, 579305472, 3016343552, 
	4217741312, 3719483392, 2013407232, 2617981952, 1510979072, 755882752, 
	2726789248, 4090085440, 3680870432, 1840435376, 2147625208, 1074478300, 
	537900666, 2953698205,
	2147483648, 1073741824, 1610612736, 805306368, 2818572288, 335544320, 
	2113929216, 3472883712, 2290089984, 3829399552, 3059744768, 1127219200, 
	3089629184, 4199809024, 3567124480, 1891565568, 394297344, 3988799488, 
	920674304, 4193267712, 2950604800, 3977188352, 3250028032, 129093376, 
	2231568512, 2963678272, 4281226848, 432124720, 803643432, 1633613396, 
	2672665246, 3170194367,
	2147483648, 3221225472, 2684354560, 3489660928, 1476395008, 2483027968, 
	1040187392, 3808428032, 3196059648, 599785472, 505413632, 4077912064, 
	1182269440, 1736704000, 2017853440, 2221342720, 3329785856, 2810494976, 
	3628507136, 1416089600, 2658719744, 864310272, 3863387648, 3076993792, 
	553150080, 272922560, 4167467040, 1148698640, 1719673080, 2009075780, 
	2149644390, 3222291575,
	2147483648, 1073741824, 2684354560, 1342177280, 2281701376, 1946157056, 
	436207616, 2566914048, 2625634304, 3208642560, 2720006144, 2098200576, 
	111673344, 2354315264, 3464626176, 4027383808, 2886631424, 3770826752, 
	1691164672, 3357462528, 1993345024, 3752330240, 873073152, 2870150400, 
	1700563072, 87021376, 1097028000, 1222351248, 1560027592, 2977959924, 
	23268898, 437609937,
    2147483648, 1073741824, 2684354560, 1342177280, 671088640, 3556769792,
    1778384896, 1895825408, 947912704, 1480589312, 3927965696, 823132160,
    2561146880, 139722752, 3257532416, 3844407296, 4071784448, 2034778112,
    4205060096, 3178434560, 413665280, 1213465600, 1646922240, 3039102208,
    3669131904, 2907196736, 2426676896, 3430094608, 539495304, 269746564,
    2282357922, 2218067473,
    2147483648, 1073741824, 3758096384, 2952790016, 2550136832, 2483027968,
    2315255808, 1526726656, 864026624, 3653238784, 1914699776, 1058013184,
    3250061312, 2800484352, 1401290752, 703922176, 171606016, 455786496,
    3549618176, 1778348032, 3929540608, 2871788544, 1269173760, 4259646208,
    1610779008, 4026976576, 2016733344, 605713840, 305826616, 3475687836,
    3113412898, 2197780721,
    2147483648, 1073741824, 2684354560, 268435456, 134217728, 1811939328,
    2650800128, 587202560, 1468006400, 2915041280, 2141192192, 2446327808,
    1233649664, 3470000128, 2282356736, 739180544, 1041072128, 857194496,
    1605394432, 3254300672, 3784148992, 3000484864, 504392192, 1663611136,
    4152723584, 3183723200, 2008703968, 4260868912, 3615493624, 3988785180,
    3751805978, 2177894957,
    2147483648, 1073741824, 536870912, 805306368, 1476395008, 2885681152,
    2516582400, 721420288, 3565158400, 155189248, 3802136576, 1380974592,
    1311244288, 3340500992, 1654521856, 308740096, 1846771712, 4147232768,
    983080960, 3192164352, 4164651008, 3693986816, 3993412096, 3072561920,
    447221120, 2388397760, 2688420704, 1882653104, 2017167560, 2620246612,
    3456542538, 2267256725,
    2147483648, 3221225472, 2684354560, 1342177280, 4160749568, 2348810240,
    3791650816, 855638016, 260046848, 557842432, 2510290944, 1584398336,
    3624402944, 472121344, 3122003968, 4013359104, 361136128, 2658123776,
    2015059968, 1278513152, 1108248576, 1661717504, 4155337216, 2910033152,
    2004879232, 1832912064, 3617588256, 1030751792, 797446008, 2976123604,
    3451258746, 2185692887,
    2147483648, 3221225472, 1610612736, 2415919104, 939524096, 3288334336,
    1107296256, 2734686208, 4051697664, 2856321024, 4242538496, 2232418304,
    3758620672, 1342963712, 1476788224, 1409875968, 2047049728, 1728856064,
    3011780608, 155856896, 225384448, 794469376, 484953600, 3574878464,
    3087007872, 67109056, 570425440, 855638160, 3380609080, 1849688260,
    3202351170, 638582947,
    2147483648, 1073741824, 536870912, 4026531840, 2818572288, 1409286144,
    2583691264, 2634022912, 511705088, 1556086784, 2099249152, 2366636032,
    612892672, 1908670464, 3953262592, 1977548800, 1805811712, 902905856,
    1269014528, 3318927360, 3819005952, 2447084544, 2041508352, 215957760,
    1730838656, 1343569984, 436314144, 3708670192, 1048832168, 2898971732,
    3576517274, 3642593693,
    2147483648, 3221225472, 536870912, 3489660928, 3623878656, 3288334336,
    1174405120, 2231369728, 2776629248, 1992294400, 2912944128, 1789919232,
    765984768, 2864447488, 229244928, 2058420224, 3584524288, 3200073728,
    2476990464, 1001721856, 908703744, 1299348480, 2609078784, 667211520,
    3056187520, 2373090496, 3145949728, 4156872656, 1848227928, 1232239620,
    4253246054, 1925502805
};

uint GrayCode(uint i) {
	return i ^ (i>>1);
}

float Sobol(uint d, uint i) {
	uint result = 0;
	uint offset = d * 32;
	for(uint j = 0; i > 0; i >>= 1, j++) 
	{
		if((i & 1) == 1)
			result ^= V[j+offset];
	}

	return float(result) * (1.0f/float(0xFFFFFFFFU));
}

vec2 SobolVec2(uint i, uint b) {
	float u = Sobol(b*2, GrayCode(i));
	float v = Sobol(b*2+1, GrayCode(i));
	return vec2(u, v);
}

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

float UintTo01(uint x)
{
    return float(x) / 4294967296.0;
}

vec2 PixelRotation(uvec2 pix, uint bounce)
{
    uint s0 = wang_hash(pix.x * 1973u ^ pix.y * 9277u ^ bounce * 26699u ^ 0x68bc21ebu);
    uint s1 = wang_hash(s0 ^ 0x02e5be93u);
    return vec2(UintTo01(s0), UintTo01(s1));
}

vec2 CranleyPattersonRotation(vec2 sobol2, uvec2 pix, uint bounce)
{
    return fract(sobol2 + PixelRotation(pix, bounce));
}

BRDFMaterial DecodeBRDFMaterial(int MaterialSlotID)
{
    BRDFMaterial Material;
    EncodedBRDFMaterial EncodedMaterial = Materials[MaterialSlotID];

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

    if (dot(Ng, ray.Direction) > 0.0)
        Ng = -Ng;

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


vec3 FetchHDRCache(float xi1, float xi2)
{
    ivec2 size = textureSize(uHDRCache, 0);
    ivec2 p = ivec2(
        min(int(xi1 * float(size.x)), size.x - 1),
        min(int(xi2 * float(size.y)), size.y - 1)
    );
    return texelFetch(uHDRCache, p, 0).rgb;
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

float PowerHeuristic(float a, float b)
{
    float a2 = a * a;
    float b2 = b * b;
    float s = a2 + b2;
    return (s > 1e-20) ? (a2 / s) : 0.0;
}

vec3 SampleHDR(float xi_env1, float xi_env2,  vec3 V, HitResult hit_result){
    vec3 values = FetchHDRCache(xi_env1, xi_env2);
    
    vec2 uv = values.xy;
    float pdfUV = values.z;

    float phi  = 2.0 * PI * (uv.x - 0.5);
    float elev = PI * (uv.y - 0.5);

    float cosElev = cos(elev);
    float sinElev = sin(elev);

    vec3 wi = normalize(vec3(
        cosElev * cos(phi),
        sinElev,
        cosElev * sin(phi)
    ));

    vec3 Ng = normalize(hit_result.GeoNormal);
    vec3 Ns = normalize(hit_result.ShadeNormal);

    float NdotL = dot(Ns, wi);
    if (NdotL <= 0.0 || dot(Ng, wi) <= 0.0)
        return vec3(0.0);

    Ray shadowRay;
    shadowRay.Start = hit_result.HitPoint + Ng * 1e-4;
    shadowRay.Direction = wi;

    HitResult occ = HitBVH(shadowRay);
    if (occ.bIsHit)
        return vec3(0.0);

    vec3 envColor = texture(uSkybox, uv).rgb;

    float sinThetaPolar = max(cosElev, 1e-6);
    float pdfEnv = pdfUV / (2.0 * PI * PI * sinThetaPolar);

    float weight = 1.0;
    
//    if(uEnableMIS == 1)
//    {
//        float pdfBRDF = BRDF_PDF_Eval(wi, V, Ns, hit_result.MaterialSlot);
//        weight = PowerHeuristic(pdfEnv, pdfBRDF);
//    }
//
//    return weight * envColor
//         * BRDF(wi, V, Ns, hit_result.MaterialSlot)
//         * NdotL
//         / max(pdfEnv, 1e-6);
    return SkyColor;
}

vec3 PathTracing(Ray ray, int maxBounce)
{
    vec3 finalColor = vec3(0.0);
    vec3 throughput = vec3(1.0);
    Ray currRay = ray;

    uvec2 pix = uvec2(gl_FragCoord.xy);

    float xi_brdf1 = rand();
    float xi_brdf2 = rand();
    float xi_env1 = rand();
    float xi_env2 = rand();
    float xi_1 = rand();
//
//    float pdf_BRDF = 0.0;
//
//    for (int bounce = 0; bounce < maxBounce; bounce++)
//    {
//        HitResult hit_result = HitBVH(currRay);
//        vec3 V  = -currRay.Direction;
//
//        if (!hit_result.bIsHit)
//        {
//            if(uEnableSkybox == 1)
//            {
//                if (bounce == 0)
//                    finalColor += throughput * SampleSkybox(currRay.Direction);
//                else{
//                    if(uEnableMIS > 0)
//                    {
//                        float pdfEnv = EnvPdf_Eval(currRay.Direction);
//                        float weight = PowerHeuristic(pdf_BRDF, pdfEnv);
//                        finalColor += weight * throughput * SampleSkybox(currRay.Direction);
//                    }
//                }
//
//            }
//            else
//            {
//                finalColor += throughput * SkyColor;
//            }
//            break;
//        }
//
//        xi_brdf1 = rand();
//        xi_brdf2 = rand();
//        xi_env1 = rand();
//        xi_env2 = rand();
//        xi_1 = rand();
//
//        vec3 Ng = normalize(hit_result.GeoNormal);
//        vec3 Ns = normalize(hit_result.ShadeNormal);
//
//        SampleBRDFResult sample_result;
//        sample_result.wi = vec3(0.0);
//        sample_result.cosTheta = 0.0;
//        sample_result.PDF = 0.0;
//
//        if (uEnableSobol == 1)
//        {
//            vec2 sobol2 = SobolVec2(uFrameCounter + 1u, 2 * bounce);
//            vec2 p = CranleyPattersonRotation(sobol2, pix, 2 * bounce);
//            xi_brdf1 = p.x;
//            xi_brdf2 = p.y;
//            sobol2 = SobolVec2(uFrameCounter + 1u, 2 * bounce + 1);
//            p = CranleyPattersonRotation(sobol2, pix, 2 * bounce + 1);
//            xi_env1 = p.x;
//            xi_env2 = p.y;
//        }
//
//        BRDFMaterial Mat = DecodeBRDFMaterial(hit_result.MaterialSlot);
//        finalColor += throughput * Mat.Emissive;
//
//        if(uEnableSkybox == 1)
//        {
//            finalColor += throughput * SampleHDR(xi_env1, xi_env2, V, hit_result);
//        }
//
//        if (uEnableImportanceSampling == 1)
//        {
//            sample_result = SampleBSSRDF();
//        }
//
//        if (sample_result.cosTheta <= 0.0 || sample_result.PDF <= 1e-8)
//            break;
//
//        if (dot(sample_result.wi, Ng) <= 0.0)
//            break;
//
//        pdf_BRDF = sample_result.PDF;
//
//        throughput *= BRDF(sample_result.wi, V, Ns, hit_result.MaterialSlot)
//                   * sample_result.cosTheta
//                   / sample_result.PDF;
//
//        currRay.Start = hit_result.HitPoint + Ng * 1e-4;
//        currRay.Direction = sample_result.wi;
//
//        if (bounce >= 3)
//        {
//            float p = clamp(max(throughput.r, max(throughput.g, throughput.b)), 0.05, 0.95);
//            if (rand() > p)
//                break;
//            throughput /= p;
//        }
//    }
//
    return finalColor;
}

void main()
{
    InitRNG();

    Ray ray;
    ray.Start = UCameraParam.Position.xyz;
    ray.Direction = GetRayDirection(CanvasPos.xy);

    vec4 Color = vec4(PathTracing(ray, 4), 1.0);
    ivec2 pix = ivec2(gl_FragCoord.xy);

    vec4 LastFrameColor = texelFetch(uLastFrame, pix , 0);

    if(uFrameCounter < 2048)
        FragColor = mix(LastFrameColor, Color, 1.0/float(uFrameCounter+1));
    else
        FragColor = LastFrameColor;
 
}
