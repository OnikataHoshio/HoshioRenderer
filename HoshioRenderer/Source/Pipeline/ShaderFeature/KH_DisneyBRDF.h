#pragma once

#include "Pipeline/KH_ShaderFeature.h"

class KH_DisneyBRDF : public KH_ShaderFeatureBase
{
public:
	KH_DisneyBRDF() = default;

	KH_DisneyBRDF(const KH_Shader& shader);

	~KH_DisneyBRDF() override = default;

	void DrawControlPanel() override;

	void ApplyUniforms() override;

private:
	int uEnableSobol = 0;
	int uEnableSkybox = 0;
	int uEnableImportanceSampling = 0;
	int uEnableMIS = 0;
	int uAllowSingleIS = 0;
	int uEnableDiffuseIS = 0;
	int uEnableSpecularIS = 0;
	int uEnableClearcoatIS = 0;

	void SetEnableSobol(bool bEnable);

	void SetEnableSkybox(bool bEnable);

	void SetEnableImportanceSampling(bool bEnable);

	void SetEnableMIS(bool bEnable);

	void SetEnableDiffuseIS(bool bEnable);

	void SetEnableSpecularIS(bool bEnable);

	void SetEnableClearcoatIS(bool bEnable);
};