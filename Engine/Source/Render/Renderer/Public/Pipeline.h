#pragma once

struct FPipelineInfo
{
	ID3D11InputLayout* InputLayout;
	ID3D11VertexShader* VertexShader;
	ID3D11RasterizerState* RasterizerState;
	ID3D11DepthStencilState* DepthStencilState;
	ID3D11PixelShader* PixelShader;
	ID3D11BlendState* BlendState;
	D3D11_PRIMITIVE_TOPOLOGY Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};

class UPipeline
{
public:
	UPipeline(ID3D11DeviceContext* InDeviceContext);
	~UPipeline();

	void UpdatePipeline(FPipelineInfo Info);

	void SetIndexBuffer(ID3D11Buffer* indexBuffer, uint32 stride);

	void SetVertexBuffer(ID3D11Buffer* VertexBuffer, uint32 Stride);

	void SetConstantBuffer(uint32 Slot, bool bIsVS, ID3D11Buffer* ConstantBuffer);

	void SetTexture(uint32 Slot, bool bIsVS, ID3D11ShaderResourceView* Srv);

	void SetSamplerState(uint32 Slot, bool bIsVS, ID3D11SamplerState* SamplerState);

	void Draw(uint32 VertexCount, uint32 StartLocation);

	void DrawIndexed(uint32 indexCount, uint32 startIndexLocation, uint32 baseVertexLocation);

private:
	FPipelineInfo LastPipelineInfo{};
	ID3D11DeviceContext* DeviceContext;
};
