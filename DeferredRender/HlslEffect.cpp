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
#include <wrl.h>
#include "ConstantBuffer.h"
#include <map>
#include <algorithm>

#include <locale>
#include <codecvt>
#include <string>
#include <array>
#include <locale>  
// #pragma pack(push, 1)
// __declspec(align(16))

// #pragma pack(pop)

HRESULT CreateVertexShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11VertexShader** ppShader, ID3DBlob** ppShaderBlob,
	BOOL bDumpShader);

HRESULT CreatePixelShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11PixelShader** ppShader, ID3DBlob** ppShaderBlob,
	BOOL bDumpShader);

HRESULT CreateShaderFromFile(ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines,
	LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2,
	ID3DX11ThreadPump* pPump, ID3D11DeviceChild** ppShader, ID3DBlob** ppShaderBlob,
	BOOL bDumpShader);

template<class T> inline ID3D11Buffer** constantBuffersToArray(DirectX::ConstantBuffer<T> &cb){
	static ID3D11Buffer* cbs[10];
	cbs[0] = cb.GetBuffer();
	return cbs;
};

struct EffectShaderFileDef{
	WCHAR * name;
	WCHAR * entry_point;
	WCHAR * profile;
};

class HlslEffect : public DirectX::IEffect
{
protected:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;

	Microsoft::WRL::ComPtr<ID3DBlob>         blob_vs;

	Microsoft::WRL::ComPtr<ID3D11HullShader> hs;

	Microsoft::WRL::ComPtr<ID3D11DomainShader> ds;

	Microsoft::WRL::ComPtr<ID3D11GeometryShader> gs;

	Microsoft::WRL::ComPtr<ID3D11PixelShader>  ps;

public:
	//Constructor
	HlslEffect(ID3D11Device* device, std::map<const WCHAR*, EffectShaderFileDef>& fileDef){
		std::map<const WCHAR*, ID3D11DeviceChild*> shaders;

		std::for_each(fileDef.begin(), fileDef.end(), [this, device, &shaders](std::pair<const WCHAR*, EffectShaderFileDef> p) {
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

			ID3D11DeviceChild* pShader;

			if (FAILED(CreateShaderFromFile(
				device,
				p.second.name,
				NULL,
				NULL,
				converter.to_bytes(p.second.entry_point).data(),
				converter.to_bytes(p.second.profile).data(),
				D3DCOMPILE_ENABLE_STRICTNESS,
				0,
				NULL,
				&pShader,
				(std::wstring(p.first) == std::wstring(L"VS") ? this->blob_vs.ReleaseAndGetAddressOf() : NULL),
				false
			)))
			   throw std::exception("HlslEffect");

			shaders.insert(std::pair<const WCHAR*, ID3D11DeviceChild*>(p.first, pShader));
		});

		vs.Attach((ID3D11VertexShader*)shaders[L"VS"]);
		hs.Attach((ID3D11HullShader*)shaders[L"HS"]);
		ds.Attach((ID3D11DomainShader*)shaders[L"DS"]);
		gs.Attach((ID3D11GeometryShader*)shaders[L"GS"]);
		ps.Attach((ID3D11PixelShader*)shaders[L"PS"]);
	}

	//Destructor
	virtual ~HlslEffect(){

	}

	//IEffect
	virtual void __cdecl Apply(_In_ ID3D11DeviceContext* context) {
		context->VSSetShader(vs.Get(), NULL, 0);
		context->HSSetShader(hs.Get(), NULL, 0);
		context->DSSetShader(ds.Get(), NULL, 0);
		context->GSSetShader(gs.Get(), NULL, 0);
		context->PSSetShader(ps.Get(), NULL, 0);
	}

	virtual void __cdecl GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength) {
		*pShaderByteCode = blob_vs->GetBufferPointer();
		*pByteCodeLength = blob_vs->GetBufferSize();
	}
};

std::unique_ptr<DirectX::IEffect> createHlslEffect(ID3D11Device* device, std::map<const WCHAR*, EffectShaderFileDef>& fileDef){
	return std::unique_ptr<DirectX::IEffect>(new HlslEffect(device, fileDef));
}