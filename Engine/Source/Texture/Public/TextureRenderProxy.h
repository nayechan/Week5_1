#pragma once
#include "pch.h"

struct FTextureRenderProxy
{
public:
	FTextureRenderProxy(ComPtr<ID3D11ShaderResourceView> InSRV, ComPtr<ID3D11SamplerState> InSampler, uint32 InWidth = 0, uint32 InHeight = 0)
		: SRV(std::move(InSRV)), Sampler(std::move(InSampler)), Width(InWidth), Height(InHeight) {}

	ID3D11ShaderResourceView* GetSRV() const { return SRV.Get(); }
	ID3D11SamplerState* GetSampler() const { return Sampler.Get(); }

private:
	ComPtr<ID3D11ShaderResourceView> SRV;
	ComPtr<ID3D11SamplerState> Sampler;
	uint32 Width = 0;
	uint32 Height = 0;
};
