#include "GeometricPrimitive.h"
#include "Effects.h"
#include "DirectXHelpers.h"
#include "DirectXMath.h"
// #include <DirectXHelpers.h>
// #include <DDSTextureLoader.h>
// #include "pch.h"

#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"

#include "strsafe.h"
#include "resource.h"
#include "Grid_Creation11.h"

/*
ID3D11RasterizerState*              g_pRasterizerStateSolid;
ID3D11RasterizerState*              g_pRasterizerStateWireframe;
ID3D11SamplerState*                 g_pSamplerStateLinear;
ID3D11BlendState*                   g_pBlendStateNoBlend;
ID3D11BlendState*                   g_pBlendStateAdditiveBlending;
ID3D11DepthStencilState*            g_pDepthStencilState;
ID3DBlob*                   pBlobVS;
ID3D11ShaderResourceView*           decalMap;
ID3D11ShaderResourceView*           stoneMap;
ID3D11ShaderResourceView*           teapotMap;
ID3D11ShaderResourceView*   pSRV;
ID3D11InputLayout*                  g_VertexLayout;


WCHAR wcPath[256];
DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
DXUTFindDXSDKMediaFileCch(wcPath, 256, L"firstPass.hlsl");
CreateVertexShaderFromFile(pd3dDevice, wcPath, NULL, NULL, "RenderSceneVS", "vs_5_0", dwShaderFlags, 0, NULL,
	&g_VS, &pBlobVS, false);
CreatePixelShaderFromFile(pd3dDevice, wcPath, NULL, NULL, "RenderScenePS", "ps_5_0", dwShaderFlags, 0, NULL,
	&g_PS, 0, false);

DXUTFindDXSDKMediaFileCch(wcPath, 256, L"Textures\\oak.bmp");
hr = D3DX11CreateShaderResourceViewFromFile(pd3dDevice, wcPath, NULL, NULL, &decalMap, NULL);
DXUTFindDXSDKMediaFileCch(wcPath, 256, L"Textures\\block.bmp");
hr = D3DX11CreateShaderResourceViewFromFile(pd3dDevice, wcPath, NULL, NULL, &stoneMap, NULL);
DXUTFindDXSDKMediaFileCch(wcPath, 256, L"Textures\\Oxidated.jpg");
hr = D3DX11CreateShaderResourceViewFromFile(pd3dDevice, wcPath, NULL, NULL, &teapotMap, NULL);

const D3D11_INPUT_ELEMENT_DESC layout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};
pd3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout),
	pBlobVS->GetBufferPointer(),
	pBlobVS->GetBufferSize(),
	&g_VertexLayout);

pBlobVS->Release();

// Create solid and wireframe rasterizer state objects
D3D11_RASTERIZER_DESC RasterDesc;
ZeroMemory(&RasterDesc, sizeof(D3D11_RASTERIZER_DESC));
RasterDesc.FillMode = D3D11_FILL_SOLID;
RasterDesc.CullMode = D3D11_CULL_BACK;
RasterDesc.DepthClipEnable = TRUE;
pd3dDevice->CreateRasterizerState(&RasterDesc, &g_pRasterizerStateSolid);

RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
pd3dDevice->CreateRasterizerState(&RasterDesc, &g_pRasterizerStateWireframe);

// Create sampler state for heightmap and normal map
D3D11_SAMPLER_DESC SSDesc;
ZeroMemory(&SSDesc, sizeof(D3D11_SAMPLER_DESC));
SSDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
SSDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
SSDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
SSDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
SSDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
SSDesc.MaxAnisotropy = 16;
SSDesc.MinLOD = 0;
SSDesc.MaxLOD = D3D11_FLOAT32_MAX;
pd3dDevice->CreateSamplerState(&SSDesc, &g_pSamplerStateLinear);

// Create a blend state to disable alpha blending
D3D11_BLEND_DESC BlendState;
ZeroMemory(&BlendState, sizeof(D3D11_BLEND_DESC));
BlendState.IndependentBlendEnable = FALSE;
BlendState.RenderTarget[0].BlendEnable = FALSE;
BlendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
hr = pd3dDevice->CreateBlendState(&BlendState, &g_pBlendStateNoBlend);

BlendState.RenderTarget[0].BlendEnable = TRUE;
BlendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
BlendState.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
BlendState.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
BlendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
BlendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
BlendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
BlendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
hr = pd3dDevice->CreateBlendState(&BlendState, &g_pBlendStateAdditiveBlending);

// Create a depthstencil state
D3D11_DEPTH_STENCIL_DESC DSDesc;
DSDesc.DepthEnable = TRUE;
DSDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
DSDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
DSDesc.StencilEnable = FALSE;
hr = pd3dDevice->CreateDepthStencilState(&DSDesc, &g_pDepthStencilState);


decalMap->Release();
stoneMap->Release();;
teapotMap->Release();

g_pRasterizerStateSolid->Release();
g_pRasterizerStateWireframe->Release();
g_pSamplerStateLinear->Release();
g_pBlendStateNoBlend->Release();
g_pBlendStateAdditiveBlending->Release();
g_pDepthStencilState->Release();

/////
pd3dImmediateContext->VSSetSamplers(0, 1, &g_pSamplerStateLinear);

pd3dImmediateContext->OMSetBlendState(g_pBlendStateNoBlend, 0, 0xffffffff);
pd3dImmediateContext->RSSetState(g_pRasterizerStateSolid);
pd3dImmediateContext->OMSetDepthStencilState(g_pDepthStencilState, 0);

pd3dImmediateContext->VSSetShaderResources(0, 1, &pSRV);
*/
/*
float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

pd3dImmediateContext->ClearRenderTargetView(DXUTGetD3D11RenderTargetView(), ClearColor);
ID3D11DepthStencilView* dsv = DXUTGetD3D11DepthStencilView();
pd3dImmediateContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
pd3dImmediateContext->OMSetRenderTargets(1, renderTargetViewToArray(DXUTGetD3D11RenderTargetView()), dsv);

{
DirectX::XMMATRIX wvp = DirectX::XMMatrixTranslationFromVector(XMLoadFloat3(&XMFLOAT3(-5, -5, 0)) + 1 / 2.0 * XMVECTOR(XMLoadFloat3(&stone_box_size)));
DirectX::XMStoreFloat4x4(&main_scene_state.mWorld, DirectX::XMMatrixTranspose(wvp));

wvp = wvp * XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mView));
DirectX::XMStoreFloat4x4(&main_scene_state.mWorldView, DirectX::XMMatrixTranspose(wvp));

wvp = wvp * XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mProjection));
DirectX::XMStoreFloat4x4(&main_scene_state.mWorldViewProjection, DirectX::XMMatrixTranspose(wvp));

main_scene_state_cb->SetData(pd3dImmediateContext, main_scene_state);

stone_box->Draw(pd3dImmediateContext, effect.get(), box_inputLayout.Get(), [=]
{
pd3dImmediateContext->VSSetConstantBuffers(0, 1, constantBuffersToArray(*main_scene_state_cb));
pd3dImmediateContext->PSSetConstantBuffers(0, 1, constantBuffersToArray(*main_scene_state_cb));

pd3dImmediateContext->OMSetBlendState(states->Opaque(), Colors::Black, 0xFFFFFFFF);
pd3dImmediateContext->RSSetState(states->CullClockwise());
pd3dImmediateContext->OMSetDepthStencilState(states->DepthDefault(), 0);
});
}
{
DirectX::XMMATRIX wvp = DirectX::XMMatrixTranslationFromVector(XMLoadFloat3(&XMFLOAT3(3, 2, 0.5)) + 1 / 2.0 * XMVECTOR(XMLoadFloat3(&decal_box_size)));
DirectX::XMStoreFloat4x4(&main_scene_state.mWorld, DirectX::XMMatrixTranspose(wvp));

wvp = wvp * XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mView));
DirectX::XMStoreFloat4x4(&main_scene_state.mWorldView, DirectX::XMMatrixTranspose(wvp));

wvp = wvp * XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mProjection));
DirectX::XMStoreFloat4x4(&main_scene_state.mWorldViewProjection, DirectX::XMMatrixTranspose(wvp));

main_scene_state_cb->SetData(pd3dImmediateContext, main_scene_state);

decal_box->Draw(pd3dImmediateContext, effect.get(), box_inputLayout.Get(), [=]
{
pd3dImmediateContext->VSSetConstantBuffers(0, 1, constantBuffersToArray(*main_scene_state_cb));
pd3dImmediateContext->PSSetConstantBuffers(0, 1, constantBuffersToArray(*main_scene_state_cb));

pd3dImmediateContext->OMSetBlendState(states->Opaque(), Colors::Black, 0xFFFFFFFF);
pd3dImmediateContext->RSSetState(states->CullCounterClockwise());
pd3dImmediateContext->OMSetDepthStencilState(states->DepthDefault(), 0);
});
}
*/
HRESULT CreateDepthStencilState(ID3D11Device* device, 
	BOOL DepthEnable,
	D3D11_DEPTH_WRITE_MASK DepthWriteMask,
	D3D11_COMPARISON_FUNC DepthFunc,

	BOOL StencilEnable,
	UINT8 StencilReadMask,
	UINT8 StencilWriteMask,

	D3D11_COMPARISON_FUNC FrontFaceStencilFunc,
	D3D11_STENCIL_OP FrontFaceStencilPassOp,
	D3D11_STENCIL_OP FrontFaceStencilFailOp,
	D3D11_STENCIL_OP FrontFaceStencilDepthFailOp,

	D3D11_COMPARISON_FUNC BackFaceStencilFunc,
	D3D11_STENCIL_OP BackFaceStencilPassOp,
	D3D11_STENCIL_OP BackFaceStencilFailOp,
	D3D11_STENCIL_OP BackFaceStencilDepthFailOp,

	ID3D11DepthStencilState** pResult)
{
	D3D11_DEPTH_STENCIL_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	desc.DepthEnable = DepthEnable;
	desc.DepthWriteMask = DepthWriteMask;
	desc.DepthFunc = DepthFunc;

	desc.StencilEnable = StencilEnable;
	desc.StencilReadMask = StencilReadMask;
	desc.StencilWriteMask = StencilWriteMask;

	desc.FrontFace.StencilFunc = FrontFaceStencilFunc;
	desc.FrontFace.StencilPassOp = FrontFaceStencilPassOp;
	desc.FrontFace.StencilFailOp = FrontFaceStencilFailOp;
	desc.FrontFace.StencilDepthFailOp = FrontFaceStencilDepthFailOp;

	desc.BackFace.StencilFunc = BackFaceStencilFunc;
	desc.BackFace.StencilPassOp = BackFaceStencilPassOp;
	desc.BackFace.StencilFailOp = BackFaceStencilFailOp;
	desc.BackFace.StencilDepthFailOp = BackFaceStencilDepthFailOp;

	return device->CreateDepthStencilState(&desc, pResult);
}