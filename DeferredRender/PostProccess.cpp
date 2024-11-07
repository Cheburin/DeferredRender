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

class IPostProcess
{
public:
	virtual ~IPostProcess() { }

	virtual void __cdecl Process(_In_ ID3D11DeviceContext* deviceContext, _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) = 0;
};

struct EffectShaderFileDef{
	WCHAR * name;
	WCHAR * entry_point;
	WCHAR * shader_ver;
};

class PostProcess : public IPostProcess{
protected:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;

	Microsoft::WRL::ComPtr<ID3D11PixelShader>  ps;
public:
	//Constructor
	PostProcess(ID3D11Device* device, std::map<const WCHAR*, EffectShaderFileDef>& fileDef){
		std::map<const WCHAR*, ID3D11DeviceChild*> shaders;

		std::for_each(fileDef.begin(), fileDef.end(), [device, &shaders](std::pair<const WCHAR*, EffectShaderFileDef> p) {
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

			ID3D11DeviceChild* pShader;

			if (FAILED(CreateShaderFromFile(
				device,
				p.second.name,
				NULL,
				NULL,
				converter.to_bytes(p.second.entry_point).data(),
				converter.to_bytes(p.second.shader_ver).data(),
				D3DCOMPILE_ENABLE_STRICTNESS,
				0,
				NULL,
				&pShader,
				NULL,
				false
			)))
			   throw std::exception("HlslEffect");

			shaders.insert(std::pair<const WCHAR*, ID3D11DeviceChild*>(p.first, pShader));
		});

		vs.Attach((ID3D11VertexShader*)shaders[L"VS"]);
		ps.Attach((ID3D11PixelShader*)shaders[L"PS"]);
	}

	//Destructor
	virtual ~PostProcess() {
	
	}

	//IPostProcess
	virtual void __cdecl Process(_In_ ID3D11DeviceContext* deviceContext, _In_opt_ std::function<void __cdecl()> setCustomState = nullptr)
	{
		deviceContext->VSSetShader(vs.Get(), nullptr, 0);
		deviceContext->HSSetShader(NULL, NULL, 0);
		deviceContext->DSSetShader(NULL, NULL, 0);
		deviceContext->GSSetShader(NULL, NULL, 0);
		deviceContext->PSSetShader(ps.Get(), nullptr, 0);

		if (setCustomState)
		{
			setCustomState();
		}

		// Draw quad.
		deviceContext->IASetInputLayout(nullptr);
		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		deviceContext->Draw(4, 0);
	};
};

std::unique_ptr<IPostProcess> createPostProcess(ID3D11Device* device, std::map<const WCHAR*, EffectShaderFileDef>& fileDef){
	return std::unique_ptr<IPostProcess>(new PostProcess(device, fileDef));
}
