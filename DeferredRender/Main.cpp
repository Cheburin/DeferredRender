//--------------------------------------------------------------------------------------
// File: DetailTessellation11.cpp
//
// This sample demonstrates the use of detail tessellation for improving the 
// quality of material surfaces in real-time rendering applications.
//
// Developed by AMD Developer Relations team.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "GeometricPrimitive.h"
#include "Effects.h"
#include "DirectXHelpers.h"
#include "DirectXMath.h"
#include "CommonStates.h"
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
#include "PlatformHelpers.h"
#include "ConstantBuffer.h"
#include "Model.h"
#include <map>
#include <algorithm>
#include <array>
using namespace DirectX;
//--------------------------------------------------------------------------------------
// Macros
//--------------------------------------------------------------------------------------
#define DWORD_POSITIVE_RANDOM(x)    ((DWORD)(( ((x)*rand()) / RAND_MAX )))
#define FLOAT_POSITIVE_RANDOM(x)    ( ((x)*rand()) / RAND_MAX )
#define FLOAT_RANDOM(x)             ((((2.0f*rand())/RAND_MAX) - 1.0f)*(x))

//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------
const float GRID_SIZE               = 200.0f;
const float MAX_TESSELLATION_FACTOR = 15.0f;

class IPostProcess
{
public:
	virtual ~IPostProcess() { }

	virtual void __cdecl Process(_In_ ID3D11DeviceContext* deviceContext, _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) = 0;
};


struct cbCustom
{
	D3DXVECTOR4     textureScale;
};

struct EffectShaderFileDef{
	WCHAR * name;
	WCHAR * entry_point;
	WCHAR * shader_ver;
};

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
DirectX::XMFLOAT3 decal_box_size = XMFLOAT3(1, 2, 2);
std::unique_ptr<DirectX::IEffect> createHlslEffect(ID3D11Device* device, std::map<const WCHAR*, EffectShaderFileDef>& fileDef);
std::unique_ptr<IPostProcess> createPostProcess(ID3D11Device* device, std::map<const WCHAR*, EffectShaderFileDef>& fileDef);
DirectX::XMFLOAT3 stone_box_size = XMFLOAT3(10, 10, 3);

std::unique_ptr<DirectX::ModelMeshPart> decal_box;
std::unique_ptr<DirectX::ModelMeshPart> stone_box;
std::unique_ptr<DirectX::ModelMeshPart> teapot;

std::unique_ptr<GeometricPrimitive> light;

std::unique_ptr<DirectX::IEffect> effect, effectTBN, effectLight;
std::unique_ptr<IPostProcess> postProcess, ambientPostProcess;

std::unique_ptr<CommonStates> states;

Microsoft::WRL::ComPtr<ID3D11InputLayout> box_inputLayout;
Microsoft::WRL::ComPtr<ID3D11InputLayout> teapot_inputLayout;
Microsoft::WRL::ComPtr<ID3D11InputLayout> teapot_tbn_inputLayout;
Microsoft::WRL::ComPtr<ID3D11InputLayout> light_inputLayout;

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv1;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv2;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv3;

Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv1;
Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv2;
Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv3;

Microsoft::WRL::ComPtr<ID3D11DepthStencilView> ds_v;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ds_srv;

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> decal_srv;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> decal_nm_srv;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> stone_srv;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> stone_nm_srv;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> teapot_srv;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> teapot_nm_srv;

DirectX::ConstantBuffer<cbCustom> *scene_state_cb;

Microsoft::WRL::ComPtr<ID3D11DepthStencilState> light_depth_stencil_first_pass_state;
Microsoft::WRL::ComPtr<ID3D11DepthStencilState> light_depth_stencil_second_pass_state;
Microsoft::WRL::ComPtr<ID3D11DepthStencilState> light_depth_stencil_ambient_pass_state;
struct SceneState
{
	DirectX::XMFLOAT4X4    mWorld;                         // World matrix
	DirectX::XMFLOAT4X4    mView;                          // View matrix
	DirectX::XMFLOAT4X4    mProjection;                    // Projection matrix
	DirectX::XMFLOAT4X4    mWorldViewProjection;           // WVP matrix
	DirectX::XMFLOAT4X4    mWorldView;                     // WV matrix
	DirectX::XMFLOAT4X4    mViewProjection;                // VP matrix
	DirectX::XMFLOAT4X4    mInvView;                       // Inverse of view matrix
	DirectX::XMFLOAT4      vScreenResolution;              // Screen resolution

	DirectX::XMFLOAT4    vMeshColor;                     // Mesh color
	DirectX::XMFLOAT4    vTessellationFactor;            // Edge, inside, minimum tessellation factor and 1/desired triangle size
	DirectX::XMFLOAT4    vDetailTessellationHeightScale; // Height scale for detail tessellation of grid surface
	DirectX::XMFLOAT4    vGridSize;                      // Grid size

	DirectX::XMFLOAT4    vDebugColorMultiply;            // Debug colors
	DirectX::XMFLOAT4    vDebugColorAdd;                 // Debug colors

	DirectX::XMFLOAT4    vFrustumPlaneEquation[4];       // View frustum plane equations
};
SceneState main_scene_state;
DirectX::ConstantBuffer<SceneState> *main_scene_state_cb;

struct MATERIAL_CB_STRUCT
{
    D3DXVECTOR4     g_materialAmbientColor;  // Material's ambient color
    D3DXVECTOR4     g_materialDiffuseColor;  // Material's diffuse color
    D3DXVECTOR4     g_materialSpecularColor; // Material's specular color
    D3DXVECTOR4     g_fSpecularExponent;     // Material's specular exponent

    D3DXVECTOR4     g_LightPosition;         // Light's position in world space
    D3DXVECTOR4     g_LightDiffuse;          // Light's diffuse color
    D3DXVECTOR4     g_LightAmbient;          // Light's ambient color

    D3DXVECTOR4     g_vEye;                  // Camera's location
    D3DXVECTOR4     g_fBaseTextureRepeat;    // The tiling factor for base and normal map textures
    D3DXVECTOR4     g_fPOMHeightMapScale;    // Describes the useful range of values for the height field

    D3DXVECTOR4     g_fShadowSoftening;      // Blurring factor for the soft shadows computation

    int             g_nMinSamples;           // The minimum number of samples for sampling the height field
    int             g_nMaxSamples;           // The maximum number of samples for sampling the height field
    int             uDummy1;
    int             uDummy2;
};

struct DETAIL_TESSELLATION_TEXTURE_STRUCT
{
    WCHAR* DiffuseMap;                  // Diffuse texture map
    WCHAR* NormalHeightMap;             // Normal and height map (normal in .xyz, height in .w)
    WCHAR* DisplayName;                 // Display name of texture
    float  fHeightScale;                // Height scale when rendering
    float  fBaseTextureRepeat;          // Base repeat of texture coordinates (1.0f for no repeat)
    float  fDensityScale;               // Density scale (used for density map generation)
    float  fMeaningfulDifference;       // Meaningful difference (used for density map generation)
};

//--------------------------------------------------------------------------------------
// Enums
//--------------------------------------------------------------------------------------
enum
{
    TESSELLATIONMETHOD_DISABLED,
    TESSELLATIONMETHOD_DISABLED_POM,
    TESSELLATIONMETHOD_DETAIL_TESSELLATION, 
} eTessellationMethod;

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
// DXUT resources
CDXUTDialogResourceManager          g_DialogResourceManager;    // Manager for shared resources of dialogs
CFirstPersonCamera                  g_Camera;
CD3DSettingsDlg                     g_D3DSettingsDlg;           // Device settings dialog
CDXUTDialog                         g_HUD;                      // Manages the 3D   
CDXUTDialog                         g_SampleUI;                 // Dialog for sample specific controls
CDXUTTextHelper*                    g_pTxtHelper = NULL;

// Shaders
ID3D11VertexShader*                 g_pPOMVS = NULL;
ID3D11PixelShader*                  g_pPOMPS = NULL;
ID3D11VertexShader*                 g_pDetailTessellationVS = NULL;
ID3D11VertexShader*                 g_pNoTessellationVS = NULL;
ID3D11HullShader*                   g_pDetailTessellationHS = NULL;
ID3D11DomainShader*                 g_pDetailTessellationDS = NULL;
ID3D11PixelShader*                  g_pBumpMapPS = NULL;
ID3D11VertexShader*                 g_pParticleVS = NULL;
ID3D11GeometryShader*               g_pParticleGS = NULL;
ID3D11PixelShader*                  g_pParticlePS = NULL;

// Textures
#define NUM_TEXTURES 7
DETAIL_TESSELLATION_TEXTURE_STRUCT DetailTessellationTextures[NUM_TEXTURES + 1] =
{
//    DiffuseMap              NormalHeightMap                 DisplayName,    fHeightScale fBaseTextureRepeat fDensityScale fMeaningfulDifference
    { L"Textures\\rocks.jpg",    L"Textures\\rocks_NM_height.dds",  L"Rocks",       10.0f,       1.0f,              25.0f,        2.0f/255.0f },
    { L"Textures\\stones.bmp",   L"Textures\\stones_NM_height.dds", L"Stones",      5.0f,        1.0f,              10.0f,        5.0f/255.0f },
    { L"Textures\\wall.jpg",     L"Textures\\wall_NM_height.dds",   L"Wall",        8.0f,        1.0f,              7.0f,         7.0f/255.0f },
    { L"Textures\\wood.jpg",     L"Textures\\four_NM_height.dds",   L"Four shapes", 30.0f,       1.0f,              35.0f,        2.0f/255.0f },
    { L"Textures\\concrete.bmp", L"Textures\\bump_NM_height.dds",   L"Bump",        10.0f,       4.0f,              50.0f,        2.0f/255.0f },
    { L"Textures\\concrete.bmp", L"Textures\\dent_NM_height.dds",   L"Dent",        10.0f,       4.0f,              50.0f,        2.0f/255.0f },
    { L"Textures\\wood.jpg",     L"Textures\\saint_NM_height.dds",  L"Saints" ,     20.0f,       1.0f,              40.0f,        2.0f/255.0f },
    { L"",                       L"",                               L"Custom" ,     5.0f,        1.0f,              10.0f,        2.0f/255.0f },
};
DWORD                               g_dwNumTextures = 0;
ID3D11ShaderResourceView*           g_pDetailTessellationBaseTextureRV[NUM_TEXTURES+1];
ID3D11ShaderResourceView*           g_pDetailTessellationHeightTextureRV[NUM_TEXTURES+1];
ID3D11ShaderResourceView*           g_pDetailTessellationDensityTextureRV[NUM_TEXTURES+1];
ID3D11ShaderResourceView*           g_pLightTextureRV = NULL;
WCHAR                               g_pszCustomTextureDiffuseFilename[MAX_PATH] = L"";
WCHAR                               g_pszCustomTextureNormalHeightFilename[MAX_PATH] = L"";
DWORD                               g_bRecreateCustomTextureDensityMap  = false;

// Geometry
ID3D11Buffer*                       g_pGridTangentSpaceVB = NULL;
ID3D11Buffer*                       g_pGridTangentSpaceIB = NULL;
ID3D11Buffer*                       g_pMainCB = NULL;
ID3D11Buffer*                       g_pMaterialCB = NULL;
ID3D11InputLayout*                  g_pTangentSpaceVertexLayout = NULL;
ID3D11InputLayout*                  g_pPositionOnlyVertexLayout = NULL;
ID3D11Buffer*                       g_pGridDensityVB[NUM_TEXTURES+1];
ID3D11ShaderResourceView*           g_pGridDensityVBSRV[NUM_TEXTURES+1];
ID3D11Buffer*                       g_pParticleVB = NULL;

// States
ID3D11RasterizerState*              g_pRasterizerStateSolid = NULL;
ID3D11RasterizerState*              g_pRasterizerStateWireframe = NULL;
ID3D11SamplerState*                 g_pSamplerStateLinear = NULL;
ID3D11BlendState*                   g_pBlendStateNoBlend = NULL;
ID3D11BlendState*                   g_pBlendStateAdditiveBlending = NULL;
ID3D11DepthStencilState*            g_pDepthStencilState = NULL;

// Camera and light parameters
D3DXVECTOR3                         g_vecEye(76.099495f, 43.191154f, -110.108971f);
D3DXVECTOR3                         g_vecAt (75.590889f, 42.676582f, -109.418678f);
D3DXVECTOR4                         g_LightPosition(100.0f, 30.0f, -50.0f, 1.0f);
CDXUTDirectionWidget                g_LightControl; 
D3DXVECTOR4							g_pWorldSpaceFrustumPlaneEquation[6];

// Render settings
int                                 g_nRenderHUD = 2;
DWORD                               g_dwGridTessellation = 50;
bool                                g_bEnableWireFrame = false;
float                               g_fTessellationFactorEdges = 7.0f;
float                               g_fTessellationFactorInside = 7.0f;
int                                 g_nTessellationMethod = TESSELLATIONMETHOD_DETAIL_TESSELLATION;
int                                 g_nCurrentTexture = 0;
bool                                g_bFrameBasedAnimation = FALSE;
bool                                g_bAnimatedCamera = FALSE;
bool                                g_bDisplacementEnabled = TRUE;
D3DXVECTOR3                         g_vDebugColorMultiply(1.0, 1.0, 1.0);
D3DXVECTOR3                         g_vDebugColorAdd(0.0, 0.0, 0.0);
bool                                g_bDensityBasedDetailTessellation = TRUE;
bool                                g_bDistanceAdaptiveDetailTessellation = TRUE;
bool                                g_bScreenSpaceAdaptiveDetailTessellation = FALSE;
bool                                g_bUseFrustumCullingOptimization = TRUE;
bool                                g_bDetailTessellationShadersNeedReloading = FALSE;
bool                                g_bShowFPS = TRUE;
bool                                g_bDrawLightSource = TRUE;
float                               g_fDesiredTriangleSize = 10.0f;

// Frame buffer readback ( 0 means never dump to disk (frame counter starts at 1) )
DWORD                               g_dwFrameNumberToDump = 0; 

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN                        1
#define IDC_TOGGLEREF                               3
#define IDC_CHANGEDEVICE                            4
#define IDC_STATIC_TECHNIQUE                       10
#define IDC_RADIOBUTTON_DISABLED                   11
#define IDC_RADIOBUTTON_POM                        12
#define IDC_RADIOBUTTON_DETAILTESSELLATION         13
#define IDC_STATIC_TEXTURE                         14
#define IDC_COMBOBOX_TEXTURE                       15
#define IDC_CHECKBOX_WIREFRAME                     16
#define IDC_CHECKBOX_DISPLACEMENT                  18
#define IDC_CHECKBOX_DENSITYBASED                  19    
#define IDC_STATIC_ADAPTIVE_SCHEME                 20
#define IDC_RADIOBUTTON_ADAPTIVESCHEME_OFF         21
#define IDC_RADIOBUTTON_ADAPTIVESCHEME_DISTANCE    22
#define IDC_RADIOBUTTON_ADAPTIVESCHEME_SCREENSPACE 23
#define IDC_STATIC_DESIRED_TRIANGLE_SIZE           24
#define IDC_SLIDER_DESIRED_TRIANGLE_SIZE           25
#define IDC_CHECKBOX_FRUSTUMCULLING                26
#define IDC_STATIC_TESS_FACTOR_EDGES               31
#define IDC_SLIDER_TESS_FACTOR_EDGES               32
#define IDC_STATIC_TESS_FACTOR_INSIDE              33
#define IDC_SLIDER_TESS_FACTOR_INSIDE              34
#define IDC_CHECKBOX_LINK_TESS_FACTORS             35
#define IDC_CHECKBOX_ROTATING_CAMERA               36

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, 
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, 
                                      void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain, 
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, 
                                  double fTime, float fElapsedTime, void* pUserContext );

void ExtractPlanesFromFrustum( D3DXVECTOR4* pPlaneEquation, const D3DXMATRIX* pMatrix, bool bNormalize=TRUE );

void InitApp();
void RenderText();
bool IsNextArg( WCHAR*& strCmdLine, WCHAR* strArg );
bool GetCmdParam( WCHAR*& strCmdLine, WCHAR* strFlag );
void CreateDensityMapFromHeightMap( ID3D11Device* pd3dDevice, ID3D11DeviceContext *pDeviceContext, ID3D11Texture2D* pHeightMap, 
                                    float fDensityScale, ID3D11Texture2D** ppDensityMap, ID3D11Texture2D** ppDensityMapStaging=NULL, 
                                    float fMeaningfulDifference=0.0f);
void CreateEdgeDensityVertexStream( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pDeviceContext, D3DXVECTOR2* pTexcoord, 
                                    DWORD dwVertexStride, void *pIndex, DWORD dwIndexSize, DWORD dwNumIndices, 
                                    ID3D11Texture2D* pDensityMap, ID3D11Buffer** ppEdgeDensityVertexStream, 
                                    float fBaseTextureRepeat);
void CreateStagingBufferFromBuffer( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext, ID3D11Buffer* pInputBuffer, 
                                    ID3D11Buffer **ppStagingBuffer);
HRESULT CreateShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                              LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                              ID3DX11ThreadPump* pPump, ID3D11DeviceChild** ppShader, ID3DBlob** ppShaderBlob = NULL, 
                              BOOL bDumpShader = FALSE);
HRESULT CreateVertexShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                                    LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                                    ID3DX11ThreadPump* pPump, ID3D11VertexShader** ppShader, ID3DBlob** ppShaderBlob = NULL, 
                                    BOOL bDumpShader = FALSE);
HRESULT CreateHullShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                                  LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                                  ID3DX11ThreadPump* pPump, ID3D11HullShader** ppShader, ID3DBlob** ppShaderBlob = NULL, 
                                  BOOL bDumpShader = FALSE);
HRESULT CreateDomainShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                                    LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                                    ID3DX11ThreadPump* pPump, ID3D11DomainShader** ppShader, ID3DBlob** ppShaderBlob = NULL, 
                                    BOOL bDumpShader = FALSE);
HRESULT CreateGeometryShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                                      LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                                      ID3DX11ThreadPump* pPump, ID3D11GeometryShader** ppShader, ID3DBlob** ppShaderBlob = NULL, 
                                      BOOL bDumpShader = FALSE);
HRESULT CreatePixelShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                                   LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                                   ID3DX11ThreadPump* pPump, ID3D11PixelShader** ppShader, ID3DBlob** ppShaderBlob = NULL, 
                                   BOOL bDumpShader = FALSE);
HRESULT CreateComputeShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                                     LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                                     ID3DX11ThreadPump* pPump, ID3D11ComputeShader** ppShader, ID3DBlob** ppShaderBlob = NULL, 
                                     BOOL bDumpShader = FALSE);


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D10 or D3D11) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackFrameMove( OnFrameMove );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    DXUTSetCallbackKeyboard( OnKeyboard );

    InitApp();
    DXUTInit( true, true );
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"DeferredRender" );
    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 1024, 768);
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}

D3DXMATRIX& assign(D3DXMATRIX& dest, const DirectX::XMMATRIX& src){
	dest._11 = src.r[0].m128_f32[0]; dest._12 = src.r[0].m128_f32[1]; dest._13 = src.r[0].m128_f32[2]; dest._14 = src.r[0].m128_f32[3];
	dest._21 = src.r[1].m128_f32[0]; dest._22 = src.r[1].m128_f32[1]; dest._23 = src.r[1].m128_f32[2]; dest._24 = src.r[1].m128_f32[3];
	dest._31 = src.r[2].m128_f32[0]; dest._32 = src.r[2].m128_f32[1]; dest._33 = src.r[2].m128_f32[2]; dest._34 = src.r[2].m128_f32[3];
	dest._41 = src.r[3].m128_f32[0]; dest._42 = src.r[3].m128_f32[1]; dest._43 = src.r[3].m128_f32[2]; dest._44 = src.r[3].m128_f32[3];
	return dest;
};
DirectX::XMMATRIX& assign(DirectX::XMMATRIX& dest, const D3DXMATRIX& src){
	dest.r[0].m128_f32[0] = src._11; dest.r[0].m128_f32[1] = src._12; dest.r[0].m128_f32[2] = src._13; dest.r[0].m128_f32[3] = src._14;
	dest.r[1].m128_f32[0] = src._21; dest.r[1].m128_f32[1] = src._22; dest.r[1].m128_f32[2] = src._23; dest.r[1].m128_f32[3] = src._24;
	dest.r[2].m128_f32[0] = src._31; dest.r[2].m128_f32[1] = src._32; dest.r[2].m128_f32[2] = src._33; dest.r[2].m128_f32[3] = src._34;
	dest.r[3].m128_f32[0] = src._41; dest.r[3].m128_f32[1] = src._42; dest.r[3].m128_f32[2] = src._43; dest.r[3].m128_f32[3] = src._44;
	return dest;
};
//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    WCHAR szTemp[256];
    
    // Initialize dialogs
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent );

    // Setup the camera's view parameters
    g_Camera.SetRotateButtons( true, false, false );
    g_Camera.SetEnablePositionMovement( true );
    g_Camera.SetViewParams( &g_vecEye, &g_vecAt );
    g_Camera.SetScalers( 0.005f, 100.0f );
    D3DXVECTOR3 vEyePt, vEyeTo;
	vEyePt.x = -0.5; // 76.099495f;
	vEyePt.y = -0.5; //43.191154f;
	vEyePt.z = 1.5; // -110.108971f;

	vEyeTo.x = -0.5; // 75.590889f;
	vEyeTo.y = -0.5; // 42.676582f;
	vEyeTo.z = 0.0;  // -109.418678f; 

	assign( g_Camera.m_mView, DirectX::XMMatrixLookToLH(XMLoadFloat3(&XMFLOAT3(-0.5, -0.5, 1.5)), XMLoadFloat3(&XMFLOAT3(0.5, 0.5, 0)), XMLoadFloat3(&XMFLOAT3(0, 0, 1))) );
	//assign( g_Camera.m_mView, DirectX::XMMatrixLookToLH(XMLoadFloat3(&XMFLOAT3(0, 0, 18)), XMLoadFloat3(&XMFLOAT3(0, 0, -1)), XMLoadFloat3(&XMFLOAT3(0, 1, 0))));

	// g_Camera.SetViewParams( &vEyePt, &vEyeTo );
    // Initialize the light control
    D3DXVECTOR3 vLightDirection;
    vLightDirection = -(D3DXVECTOR3)g_LightPosition;
    D3DXVec3Normalize( &vLightDirection, &vLightDirection );
    g_LightControl.SetLightDirection( vLightDirection );
    g_LightControl.SetRadius( GRID_SIZE/2.0f );

    // Load default scene
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }

        //// Enable 4xMSAA by default
        //DXGI_SAMPLE_DESC MSAA4xSampleDesc = { 4, 0 };
		DXGI_SAMPLE_DESC SampleDesc = { 1, 0 };
		pDeviceSettings->d3d11.sd.SampleDesc = SampleDesc;
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    // g_Camera.FrameMove( fElapsedTime );

	// dynamic_cast<IEffectMatrices*>(effect.get())->SetView(assign(DirectX::XMMATRIX(), *g_Camera.GetViewMatrix()));

	DirectX::XMStoreFloat4x4(&main_scene_state.mView, DirectX::XMMatrixTranspose(DirectX::XMMATRIX(assign(DirectX::XMMATRIX(), *g_Camera.GetViewMatrix()))));
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( g_bShowFPS && DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );
    
    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    switch( uMsg )
    {
        case WM_KEYDOWN:    // Prevent the camera class to use some prefefined keys that we're already using    
                            switch( (UINT)wParam )
                            {
                                case VK_CONTROL:    
                                case VK_LEFT:        
                                case VK_RIGHT:         
                                case VK_UP:            
                                case VK_DOWN:          
                                case VK_PRIOR:              
                                case VK_NEXT:             

                                case VK_NUMPAD4:     
                                case VK_NUMPAD6:     
                                case VK_NUMPAD8:     
                                case VK_NUMPAD2:     
                                case VK_NUMPAD9:           
                                case VK_NUMPAD3:         

                                case VK_HOME:       return 0;

                                case VK_RETURN:     
                                {
                                    // Reset camera position
                                    g_Camera.SetViewParams( &g_vecEye, &g_vecAt );

                                    // Reset light position
                                    g_LightPosition = D3DXVECTOR4(100.0f, 30.0f, -50.0f, 1.0f);
                                    D3DXVECTOR3 vLightDirection;
                                    vLightDirection = -(D3DXVECTOR3)g_LightPosition;
                                    D3DXVec3Normalize(&vLightDirection, &vLightDirection);
                                    g_LightControl.SetLightDirection(vLightDirection);
                                }
                                break;
                            }
    }

    // Pass all remaining windows messages to camera so it can respond to user input
    // g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    // Pass the message to be handled to the light control:
    // g_LightControl.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
            case 'V':           // Debug views
                                if (g_vDebugColorMultiply.x==1.0)
                                {
                                    g_vDebugColorMultiply.x=0.0;
                                    g_vDebugColorMultiply.y=0.0;
                                    g_vDebugColorMultiply.z=0.0;
                                    g_vDebugColorAdd.x=1.0;
                                    g_vDebugColorAdd.y=1.0;
                                    g_vDebugColorAdd.z=1.0;
                                }
                                else
                                {
                                    g_vDebugColorMultiply.x=1.0;
                                    g_vDebugColorMultiply.y=1.0;
                                    g_vDebugColorMultiply.z=1.0;
                                    g_vDebugColorAdd.x=0.0;
                                    g_vDebugColorAdd.y=0.0;
                                    g_vDebugColorAdd.z=0.0;
                                }
                                break;

            case 'H':
            case VK_F1:         g_nRenderHUD = ( g_nRenderHUD + 1 ) % 3; break;

        }
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    WCHAR szTemp[256];

}

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

	ID3D11DepthStencilState** pResult);
//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}
template<class A> inline char& getMember(A &a, size_t offset){
	return (char&)((char *)&a)[offset];
}
template<class T> inline void assignMember(T &to, char& from){
	to = (T&)from;
}
template<class T> inline void assignMember(char &to, T& from){
	(T&)to = from;
}
HRESULT ComputeTangentFrame(const uint16_t* indices, size_t nFaces, const XMFLOAT3* positions, const XMFLOAT3* normals, const XMFLOAT2* texcoords, size_t nVerts, XMFLOAT4* tangents);
void CreateBuffer(_In_ ID3D11Device* device, std::vector<VertexPositionNormalTangentColorTexture> const& data, D3D11_BIND_FLAG bindFlags, _Outptr_ ID3D11Buffer** pBuffer);
void CreateBuffer(_In_ ID3D11Device* device, std::vector<uint16_t> const& data, D3D11_BIND_FLAG bindFlags, _Outptr_ ID3D11Buffer** pBuffer);
std::unique_ptr<DirectX::ModelMeshPart> CreateModelMeshPart(ID3D11Device* device, std::function<void(std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices)> createGeometry){
	std::vector<VertexPositionNormalTexture> vertices;
	std::vector<uint16_t> indices;

	createGeometry(vertices, indices);

	size_t nVerts = vertices.size();

	std::vector<VertexPositionNormalTangentColorTexture> out_vertices(nVerts);

	std::unique_ptr<XMFLOAT3> positions(new XMFLOAT3[nVerts]); auto p = positions.get();
	std::unique_ptr<XMFLOAT3> normals(new XMFLOAT3[nVerts]); auto n = normals.get();
	std::unique_ptr<XMFLOAT2> texcoords(new XMFLOAT2[nVerts]); auto tex = texcoords.get();
	std::unique_ptr<XMFLOAT4> tangents(new XMFLOAT4[nVerts]); auto tan = tangents.get();

	std::for_each(vertices.begin(), vertices.end(), [p, n, tex](VertexPositionNormalTexture &v) mutable {
		assignMember(*(p++), getMember(v, offsetof(VertexPositionNormalTexture, VertexPositionNormalTexture::position)));
		assignMember(*(n++), getMember(v, offsetof(VertexPositionNormalTexture, VertexPositionNormalTexture::normal)));
		assignMember(*(tex++), getMember(v, offsetof(VertexPositionNormalTexture, VertexPositionNormalTexture::textureCoordinate)));
	});

	ComputeTangentFrame(indices.data(), indices.size() / 3, positions.get(), normals.get(), texcoords.get(), nVerts, tangents.get());

	p = positions.get();
	n = normals.get();
	tex = texcoords.get();
	tan = tangents.get();

	std::for_each(out_vertices.begin(), out_vertices.end(), [p, n, tex, tan](VertexPositionNormalTangentColorTexture &v) mutable {
		assignMember(getMember(v, offsetof(VertexPositionNormalTangentColorTexture, VertexPositionNormalTangentColorTexture::position)), *(p++));
		assignMember(getMember(v, offsetof(VertexPositionNormalTangentColorTexture, VertexPositionNormalTangentColorTexture::normal)), *(n++));
		assignMember(getMember(v, offsetof(VertexPositionNormalTangentColorTexture, VertexPositionNormalTangentColorTexture::textureCoordinate)), *(tex++));
		assignMember(getMember(v, offsetof(VertexPositionNormalTangentColorTexture, VertexPositionNormalTangentColorTexture::tangent)), *(tan++));
	});

	//std::vector<XMFLOAT3> _a(positions.get(), positions.get() + nVerts);
	//std::vector<XMFLOAT3> _b(normals.get(), normals.get() + nVerts);
	//std::vector<XMFLOAT2> _c(texcoords.get(), texcoords.get() + nVerts);
	std::unique_ptr<DirectX::ModelMeshPart> modelMeshPArt(new DirectX::ModelMeshPart());

	modelMeshPArt->indexCount = indices.size();
	modelMeshPArt->startIndex = 0;
	modelMeshPArt->vertexOffset = 0;
	modelMeshPArt->vertexStride = sizeof(VertexPositionNormalTangentColorTexture);
	modelMeshPArt->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	modelMeshPArt->indexFormat = DXGI_FORMAT_R16_UINT;
	modelMeshPArt->vbDecl = std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>>(
		new std::vector<D3D11_INPUT_ELEMENT_DESC>(
			&VertexPositionNormalTangentColorTexture::InputElements[0],
			&VertexPositionNormalTangentColorTexture::InputElements[VertexPositionNormalTangentColorTexture::InputElementCount]
		)
	);

	CreateBuffer(device, out_vertices, D3D11_BIND_VERTEX_BUFFER, modelMeshPArt->vertexBuffer.ReleaseAndGetAddressOf());
	
	CreateBuffer(device, indices, D3D11_BIND_INDEX_BUFFER, modelMeshPArt->indexBuffer.ReleaseAndGetAddressOf());

	return modelMeshPArt;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
	HRESULT hr;
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"DeferredRenderFirstPass.hlsl", L"VS", L"vs_5_0" };
		shaderDef[L"PS"] = { L"DeferredRenderFirstPass.hlsl", L"PS", L"ps_5_0" };

		effect = createHlslEffect(pd3dDevice, shaderDef);
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"RenderTBN.hlsl", L"MODEL", L"vs_5_0" };
		shaderDef[L"GS"] = { L"RenderTBN.hlsl", L"GS", L"gs_5_0" };
		shaderDef[L"PS"] = { L"RenderTBN.hlsl", L"PS", L"ps_5_0" };

		effectTBN = createHlslEffect(pd3dDevice, shaderDef);
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"DeferredRenderSecondPass.hlsl", L"VS", L"vs_5_0" };
		shaderDef[L"PS"] = { L"DeferredRenderSecondPass.hlsl", L"PS", L"ps_5_0" };

		postProcess = createPostProcess(pd3dDevice, shaderDef);
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"SimplePosThenConstColor.hlsl", L"SIMPLE_MODEL", L"vs_5_0" };
		shaderDef[L"PS"] = { L"SimplePosThenConstColor.hlsl", L"CONST_COLOR", L"ps_5_0" };

		effectLight = createHlslEffect(pd3dDevice, shaderDef);
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"DeferredRenderSecondPass.hlsl", L"VS", L"vs_5_0" };
		shaderDef[L"PS"] = { L"DeferredRenderSecondPass.hlsl", L"AMBIENT_PS", L"ps_5_0" };

		ambientPostProcess = createPostProcess(pd3dDevice, shaderDef);
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	states = std::make_unique<CommonStates>(pd3dDevice);

	scene_state_cb = new DirectX::ConstantBuffer<cbCustom>(pd3dDevice);

	main_scene_state_cb = new DirectX::ConstantBuffer<SceneState>(pd3dDevice);

	// Get device context
	ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();

	decal_box = CreateModelMeshPart(pd3dDevice, [=](std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices){
		GeometricPrimitive::CreateBox(_vertices, _indices, decal_box_size, false);
	});

	stone_box = CreateModelMeshPart(pd3dDevice, [=](std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices){
		GeometricPrimitive::CreateBox(_vertices, _indices, stone_box_size, false, true);
	});

	teapot = CreateModelMeshPart(pd3dDevice, [=](std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices){
		GeometricPrimitive::CreateTeapot(_vertices, _indices, 0.5, 4U, false);
	});

	light = GeometricPrimitive::CreateSphere(pd3dImmediateContext, 0.5, 32, false);

	decal_box->CreateInputLayout(pd3dDevice, effect.get(), &box_inputLayout);

	teapot->CreateInputLayout(pd3dDevice, effect.get(), &teapot_inputLayout);

	teapot->CreateInputLayout(pd3dDevice, effectTBN.get(), &teapot_tbn_inputLayout);
	
	light->CreateInputLayout(effectLight.get(), &light_inputLayout);

	// GUI creation
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

    // Light control
    g_LightControl.StaticOnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext );

	Microsoft::WRL::ComPtr<ID3D11Texture2D> t1;
	WCHAR wcPath1[256];
	DXUTFindDXSDKMediaFileCch(wcPath1, 256, L"Textures\\wood.png");
	hr = D3DX11CreateShaderResourceViewFromFile(pd3dDevice, wcPath1, NULL, NULL, decal_srv.ReleaseAndGetAddressOf(), NULL);
	DXUTFindDXSDKMediaFileCch(wcPath1, 256, L"Textures\\wood_normal.png");
	hr = D3DX11CreateShaderResourceViewFromFile(pd3dDevice, wcPath1, NULL, NULL, decal_nm_srv.ReleaseAndGetAddressOf(), NULL);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> t2;
	WCHAR wcPath2[256];
	DXUTFindDXSDKMediaFileCch(wcPath2, 256, L"Textures\\brick.dds");
	hr = D3DX11CreateShaderResourceViewFromFile(pd3dDevice, wcPath2, NULL, NULL, stone_srv.ReleaseAndGetAddressOf(), NULL);
	DXUTFindDXSDKMediaFileCch(wcPath2, 256, L"Textures\\brick_nm.bmp");
	hr = D3DX11CreateShaderResourceViewFromFile(pd3dDevice, wcPath2, NULL, NULL, stone_nm_srv.ReleaseAndGetAddressOf(), NULL);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> t3;
	WCHAR wcPath3[256];
	DXUTFindDXSDKMediaFileCch(wcPath3, 256, L"Textures\\Oxidated.jpg");
	hr = D3DX11CreateShaderResourceViewFromFile(pd3dDevice, wcPath3, NULL, NULL, teapot_srv.ReleaseAndGetAddressOf(), NULL);
	DXUTFindDXSDKMediaFileCch(wcPath3, 256, L"Textures\\wall_bump.dds");
	hr = D3DX11CreateShaderResourceViewFromFile(pd3dDevice, wcPath3, NULL, NULL, teapot_nm_srv.ReleaseAndGetAddressOf(), NULL);

	CreateDepthStencilState(pd3dDevice,
		/*BOOL DepthEnable*/true,
		/*D3D11_DEPTH_WRITE_MASK DepthWriteMask*/D3D11_DEPTH_WRITE_MASK_ZERO,
		/*D3D11_COMPARISON_FUNC DepthFunc*/D3D11_COMPARISON_LESS,

		/*BOOL StencilEnable*/true,
		/*UINT8 StencilReadMask*/D3D11_DEFAULT_STENCIL_READ_MASK,
		/*UINT8 StencilWriteMask*/D3D11_DEFAULT_STENCIL_READ_MASK,

		/*D3D11_COMPARISON_FUNC FrontFaceStencilFunc*/D3D11_COMPARISON_ALWAYS,
		/*D3D11_STENCIL_OP FrontFaceStencilPassOp*/D3D11_STENCIL_OP_KEEP,
		/*D3D11_STENCIL_OP FrontFaceStencilFailOp*/D3D11_STENCIL_OP_KEEP,
		/*D3D11_STENCIL_OP FrontFaceStencilDepthFailOp*/D3D11_STENCIL_OP_INCR_SAT,

		/*D3D11_COMPARISON_FUNC BackFaceStencilFunc*/D3D11_COMPARISON_ALWAYS,
		/*D3D11_STENCIL_OP BackFaceStencilPassOp*/D3D11_STENCIL_OP_KEEP,
		/*D3D11_STENCIL_OP BackFaceStencilFailOp*/D3D11_STENCIL_OP_KEEP,
		/*D3D11_STENCIL_OP BackFaceStencilDepthFailOp*/D3D11_STENCIL_OP_INCR_SAT,

		light_depth_stencil_first_pass_state.ReleaseAndGetAddressOf());

	CreateDepthStencilState(pd3dDevice,
		/*BOOL DepthEnable*/false,
		/*D3D11_DEPTH_WRITE_MASK DepthWriteMask*/D3D11_DEPTH_WRITE_MASK_ZERO,
		/*D3D11_COMPARISON_FUNC DepthFunc*/D3D11_COMPARISON_LESS,

		/*BOOL StencilEnable*/true,
		/*UINT8 StencilReadMask*/D3D11_DEFAULT_STENCIL_READ_MASK,
		/*UINT8 StencilWriteMask*/D3D11_DEFAULT_STENCIL_READ_MASK,

		/*D3D11_COMPARISON_FUNC FrontFaceStencilFunc*/D3D11_COMPARISON_NOT_EQUAL,
		/*D3D11_STENCIL_OP FrontFaceStencilPassOp*/D3D11_STENCIL_OP_KEEP,
		/*D3D11_STENCIL_OP FrontFaceStencilFailOp*/D3D11_STENCIL_OP_KEEP,
		/*D3D11_STENCIL_OP FrontFaceStencilDepthFailOp*/D3D11_STENCIL_OP_KEEP,

		/*D3D11_COMPARISON_FUNC BackFaceStencilFunc*/D3D11_COMPARISON_NOT_EQUAL,
		/*D3D11_STENCIL_OP BackFaceStencilPassOp*/D3D11_STENCIL_OP_KEEP,
		/*D3D11_STENCIL_OP BackFaceStencilFailOp*/D3D11_STENCIL_OP_KEEP,
		/*D3D11_STENCIL_OP BackFaceStencilDepthFailOp*/D3D11_STENCIL_OP_KEEP,

		light_depth_stencil_second_pass_state.ReleaseAndGetAddressOf());

	CreateDepthStencilState(pd3dDevice,
		/*BOOL DepthEnable*/false,
		/*D3D11_DEPTH_WRITE_MASK DepthWriteMask*/D3D11_DEPTH_WRITE_MASK_ZERO,
		/*D3D11_COMPARISON_FUNC DepthFunc*/D3D11_COMPARISON_LESS,

		/*BOOL StencilEnable*/true,
		/*UINT8 StencilReadMask*/D3D11_DEFAULT_STENCIL_READ_MASK,
		/*UINT8 StencilWriteMask*/D3D11_DEFAULT_STENCIL_READ_MASK,

		/*D3D11_COMPARISON_FUNC FrontFaceStencilFunc*/D3D11_COMPARISON_EQUAL,
		/*D3D11_STENCIL_OP FrontFaceStencilPassOp*/D3D11_STENCIL_OP_KEEP,
		/*D3D11_STENCIL_OP FrontFaceStencilFailOp*/D3D11_STENCIL_OP_KEEP,
		/*D3D11_STENCIL_OP FrontFaceStencilDepthFailOp*/D3D11_STENCIL_OP_KEEP,

		/*D3D11_COMPARISON_FUNC BackFaceStencilFunc*/D3D11_COMPARISON_EQUAL,
		/*D3D11_STENCIL_OP BackFaceStencilPassOp*/D3D11_STENCIL_OP_KEEP,
		/*D3D11_STENCIL_OP BackFaceStencilFailOp*/D3D11_STENCIL_OP_KEEP,
		/*D3D11_STENCIL_OP BackFaceStencilDepthFailOp*/D3D11_STENCIL_OP_KEEP,

		light_depth_stencil_ambient_pass_state.ReleaseAndGetAddressOf());

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI/3, fAspectRatio, 0.1f, 100000.0f );

	//dynamic_cast<IEffectMatrices*>(effect.get())->SetProjection(assign(DirectX::XMMATRIX(), *g_Camera.GetProjMatrix()));
	DirectX::XMStoreFloat4x4(&main_scene_state.mProjection, DirectX::XMMatrixTranspose(assign(DirectX::XMMATRIX(), *g_Camera.GetProjMatrix())));
	DirectX::XMStoreFloat4(&main_scene_state.vScreenResolution, XMLoadFloat4(&XMFLOAT4(0, 0, (*g_Camera.GetProjMatrix())(1,1), fAspectRatio)));
    // Set GUI size and locations
    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 245, pBackBufferSurfaceDesc->Height - 660 );
    g_SampleUI.SetSize( 245, 660 );


	Microsoft::WRL::ComPtr<ID3D11Texture2D> t1, t2, t3, t4;
	/// /// 
	CD3D11_TEXTURE2D_DESC fragmentDesc(DXGI_FORMAT_R32G32B32A32_FLOAT, pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);// , D3D11_USAGE_DEFAULT, 0, 1, 0, 0);

	ThrowIfFailed(pd3dDevice->CreateTexture2D(&fragmentDesc, nullptr, t1.ReleaseAndGetAddressOf()));

	ThrowIfFailed(pd3dDevice->CreateTexture2D(&fragmentDesc, nullptr, t2.ReleaseAndGetAddressOf()));

	ThrowIfFailed(pd3dDevice->CreateTexture2D(&fragmentDesc, nullptr, t3.ReleaseAndGetAddressOf()));

	ThrowIfFailed(pd3dDevice->CreateShaderResourceView(t1.Get(), nullptr, srv1.ReleaseAndGetAddressOf()));

	ThrowIfFailed(pd3dDevice->CreateShaderResourceView(t2.Get(), nullptr, srv2.ReleaseAndGetAddressOf()));

	ThrowIfFailed(pd3dDevice->CreateShaderResourceView(t3.Get(), nullptr, srv3.ReleaseAndGetAddressOf()));

	ThrowIfFailed(pd3dDevice->CreateRenderTargetView(t1.Get(), nullptr, rtv1.ReleaseAndGetAddressOf()));

	ThrowIfFailed(pd3dDevice->CreateRenderTargetView(t2.Get(), nullptr, rtv2.ReleaseAndGetAddressOf()));

	ThrowIfFailed(pd3dDevice->CreateRenderTargetView(t3.Get(), nullptr, rtv3.ReleaseAndGetAddressOf()));

	/// ///
	
	CD3D11_TEXTURE2D_DESC depthDesc(DXGI_FORMAT_D24_UNORM_S8_UINT, pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height, 1, 1, D3D11_BIND_DEPTH_STENCIL);// , D3D11_USAGE_DEFAULT, 0, 1, 0, 0);

	ThrowIfFailed(pd3dDevice->CreateTexture2D(&depthDesc, nullptr, t4.ReleaseAndGetAddressOf()));

	// ThrowIfFailed(pd3dDevice->CreateShaderResourceView(t3.Get(), nullptr, ds_srv.ReleaseAndGetAddressOf())); not support

	ThrowIfFailed(pd3dDevice->CreateDepthStencilView(t4.Get(), nullptr, ds_v.ReleaseAndGetAddressOf()));

    return S_OK;
}

inline ID3D11RenderTargetView** renderTargetViewToArray(ID3D11RenderTargetView* rtv1, ID3D11RenderTargetView* rtv2 = 0, ID3D11RenderTargetView* rtv3 = 0){
	static ID3D11RenderTargetView* rtvs[10];
	rtvs[0] = rtv1;
	rtvs[1] = rtv2;
	rtvs[2] = rtv3;
	return rtvs;
};

inline ID3D11ShaderResourceView** shaderResourceViewToArray(ID3D11ShaderResourceView* rtv1, ID3D11ShaderResourceView* rtv2 = 0, ID3D11ShaderResourceView* rtv3 = 0){
	static ID3D11ShaderResourceView* srvs[10];
	srvs[0] = rtv1;
	srvs[1] = rtv2;
	srvs[2] = rtv3;
	return srvs;
};


inline ID3D11SamplerState** samplerStateToArray(ID3D11SamplerState* ss1, ID3D11SamplerState* ss2 = 0){
	static ID3D11SamplerState* sss[10];
	sss[0] = ss1;
	sss[1] = ss2;
	return sss;
};

template<class T> inline ID3D11Buffer** constantBuffersToArray(DirectX::ConstantBuffer<T> &cb){
	static ID3D11Buffer* cbs[10];
	cbs[0] = cb.GetBuffer();
	return cbs;
};


template<class T1, class T2 > inline ID3D11Buffer** constantBuffersToArray(DirectX::ConstantBuffer<T1> &cb1, DirectX::ConstantBuffer<T2> &cb2){
	static ID3D11Buffer* cbs[10];
	cbs[0] = cb1.GetBuffer();
	cbs[1] = cb2.GetBuffer();
	return cbs;
};

//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, 
                                  double fTime, float fElapsedTime, void* pUserContext )
{
    HRESULT                     hr;
    static DWORD                s_dwFrameNumber = 1;
    ID3D11Buffer*               pBuffers[2];
    ID3D11ShaderResourceView*   pSRV[4];
    ID3D11SamplerState*         pSS[1];
    D3DXVECTOR3                 vFrom;

	struct Et {
		IEffect* e;
		D3D_PRIMITIVE_TOPOLOGY t;
		ID3D11RasterizerState* s;
	} et[2] = {
		{ effect.get(), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, states->CullCounterClockwise() },
		{ effectTBN.get(), D3D_PRIMITIVE_TOPOLOGY_POINTLIST, states->Wireframe() }
	};

	// Clear the render target and depth stencil
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	ID3D11DepthStencilView* dsv = ds_v.Get();// // DXUTGetD3D11DepthStencilView();// ds_v.Get();//

	pd3dImmediateContext->ClearRenderTargetView(rtv1.Get(), ClearColor);
	pd3dImmediateContext->ClearRenderTargetView(rtv2.Get(), ClearColor);
	pd3dImmediateContext->ClearRenderTargetView(rtv3.Get(), ClearColor);

	pd3dImmediateContext->ClearRenderTargetView(DXUTGetD3D11RenderTargetView(), ClearColor);

	pd3dImmediateContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

	pd3dImmediateContext->OMSetRenderTargets(3, renderTargetViewToArray(rtv1.Get(), rtv2.Get(), rtv3.Get()), dsv);
	if(true){
		DirectX::XMMATRIX wvp = DirectX::XMMatrixTranslationFromVector(XMLoadFloat3(&XMFLOAT3(3, 2, 0.5)) + 1 / 2.0 * XMVECTOR(XMLoadFloat3(&decal_box_size)));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorld, DirectX::XMMatrixTranspose(wvp));
		wvp = wvp * XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mView));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorldView, DirectX::XMMatrixTranspose(wvp));
		wvp = wvp * XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mProjection));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorldViewProjection, DirectX::XMMatrixTranspose(wvp));
		cbCustom cb;
		cb.textureScale = D3DXVECTOR4(decal_box_size.x, decal_box_size.y, decal_box_size.z, 0);
		scene_state_cb->SetData(pd3dImmediateContext, cb);
		main_scene_state_cb->SetData(pd3dImmediateContext, main_scene_state);

		std::for_each(&et[0], &et[2], [=](Et et) {
			decal_box->primitiveType = et.t;
			decal_box->Draw(pd3dImmediateContext, et.e, box_inputLayout.Get(), [=]
			{
				pd3dImmediateContext->VSSetConstantBuffers(0, 2, constantBuffersToArray(*main_scene_state_cb, *scene_state_cb));
				pd3dImmediateContext->GSSetConstantBuffers(0, 1, constantBuffersToArray(*main_scene_state_cb));

				pd3dImmediateContext->PSSetShaderResources(0, 2, shaderResourceViewToArray(decal_srv.Get(), decal_nm_srv.Get()));
				pd3dImmediateContext->PSSetSamplers(0, 1, samplerStateToArray(states->LinearWrap()));

				pd3dImmediateContext->OMSetBlendState(states->Opaque(), Colors::Black, 0xFFFFFFFF);
				pd3dImmediateContext->RSSetState(et.s);
				pd3dImmediateContext->OMSetDepthStencilState(states->DepthDefault(), 0);
			});
		});
	}
	if (true){
		DirectX::XMMATRIX wvp = DirectX::XMMatrixTranslationFromVector(XMLoadFloat3(&XMFLOAT3(-5, -5, 0)) + 1 / 2.0 * XMVECTOR(XMLoadFloat3(&stone_box_size)));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorld, DirectX::XMMatrixTranspose(wvp));
		wvp = wvp * XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mView));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorldView, DirectX::XMMatrixTranspose(wvp));
		wvp = wvp * XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mProjection));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorldViewProjection, DirectX::XMMatrixTranspose(wvp));
		cbCustom cb;
		cb.textureScale = D3DXVECTOR4(stone_box_size.x, stone_box_size.y, stone_box_size.z, 0);
		scene_state_cb->SetData(pd3dImmediateContext, cb);
		main_scene_state_cb->SetData(pd3dImmediateContext, main_scene_state);

		stone_box->Draw(pd3dImmediateContext, effect.get(), box_inputLayout.Get(), [=]
		{
			pd3dImmediateContext->VSSetConstantBuffers(0, 2, constantBuffersToArray(*main_scene_state_cb, *scene_state_cb));

			pd3dImmediateContext->PSSetShaderResources(0, 2, shaderResourceViewToArray(stone_srv.Get(), stone_nm_srv.Get()));
			pd3dImmediateContext->PSSetSamplers(0, 1, samplerStateToArray(states->LinearWrap()));

			pd3dImmediateContext->OMSetBlendState(states->Opaque(), Colors::Black, 0xFFFFFFFF);
			pd3dImmediateContext->RSSetState(states->CullClockwise());
			pd3dImmediateContext->OMSetDepthStencilState(states->DepthDefault(), 0);
		});
	}
	if (true){//
		DirectX::XMMATRIX wvp = DirectX::XMMatrixRotationY(1 * 57.2) * DirectX::XMMatrixRotationX(1 * 45.3) * DirectX::XMMatrixTranslationFromVector(XMLoadFloat3(&XMFLOAT3(1.0, 2.3, 1.5)));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorld, DirectX::XMMatrixTranspose(wvp));
		wvp = wvp * XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mView));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorldView, DirectX::XMMatrixTranspose(wvp));
		wvp = wvp * XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mProjection));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorldViewProjection, DirectX::XMMatrixTranspose(wvp));
		cbCustom cb;
		cb.textureScale = D3DXVECTOR4(1, 1, 1, 0);
		scene_state_cb->SetData(pd3dImmediateContext, cb);
		main_scene_state_cb->SetData(pd3dImmediateContext, main_scene_state);

		std::for_each(&et[0], &et[1], [=](Et et) {
			teapot->primitiveType = et.t;
			teapot->Draw(pd3dImmediateContext, et.e, teapot_inputLayout.Get(), [=]
			{
				pd3dImmediateContext->VSSetConstantBuffers(0, 2, constantBuffersToArray(*main_scene_state_cb, *scene_state_cb));
				pd3dImmediateContext->GSSetConstantBuffers(0, 1, constantBuffersToArray(*main_scene_state_cb));

				pd3dImmediateContext->PSSetShaderResources(0, 2, shaderResourceViewToArray(teapot_srv.Get(), teapot_nm_srv.Get()));
				pd3dImmediateContext->PSSetSamplers(0, 1, samplerStateToArray(states->LinearWrap()));

				pd3dImmediateContext->OMSetBlendState(states->Opaque(), Colors::Black, 0xFFFFFFFF);
				pd3dImmediateContext->RSSetState(et.s);
				pd3dImmediateContext->OMSetDepthStencilState(states->DepthDefault(), 0);
			});
		});
	}
	pd3dImmediateContext->OMSetRenderTargets(3, renderTargetViewToArray(nullptr, nullptr, nullptr), dsv);

	pd3dImmediateContext->OMSetRenderTargets(1, renderTargetViewToArray(DXUTGetD3D11RenderTargetView()), dsv);
	if (true){
		auto L = XMVector3TransformCoord(XMLoadFloat3(&XMFLOAT3(2, 3, 0.5)), XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mView)));

		DirectX::XMMATRIX wvp = DirectX::XMMatrixScalingFromVector(XMLoadFloat3(&XMFLOAT3(9, 9, 9))) * DirectX::XMMatrixTranslationFromVector(XMLoadFloat3(&XMFLOAT3(2, 3, 0.5)));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorld, DirectX::XMMatrixTranspose(wvp));
		wvp = wvp * XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mView));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorldView, DirectX::XMMatrixTranspose(wvp));
		wvp = wvp * XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mProjection));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorldViewProjection, DirectX::XMMatrixTranspose(wvp));

		main_scene_state_cb->SetData(pd3dImmediateContext, main_scene_state);

		light->Draw(effectLight.get(), light_inputLayout.Get(), false, false, [=]{
			pd3dImmediateContext->VSSetConstantBuffers(0, 1, constantBuffersToArray(*main_scene_state_cb));
			pd3dImmediateContext->PSSetConstantBuffers(0, 1, constantBuffersToArray(*main_scene_state_cb));

			pd3dImmediateContext->OMSetBlendState(states->Additive(), Colors::Black, 0xFFFFFFFF);
			pd3dImmediateContext->RSSetState(states->CullClockwise());
			pd3dImmediateContext->OMSetDepthStencilState(light_depth_stencil_first_pass_state.Get(), 0);
		});
	}
	if (true){
		DirectX::XMMATRIX wvp = DirectX::XMMatrixScalingFromVector(XMLoadFloat3(&XMFLOAT3(0.2, 0.2, 0.2))) * DirectX::XMMatrixTranslationFromVector(XMLoadFloat3(&XMFLOAT3(2, 3, 0.5)));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorld, DirectX::XMMatrixTranspose(wvp));
		wvp = wvp * XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mView));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorldView, DirectX::XMMatrixTranspose(wvp));
		wvp = wvp * XMMatrixTranspose(XMLoadFloat4x4(&main_scene_state.mProjection));
		DirectX::XMStoreFloat4x4(&main_scene_state.mWorldViewProjection, DirectX::XMMatrixTranspose(wvp));

		main_scene_state_cb->SetData(pd3dImmediateContext, main_scene_state);

		light->Draw(effectLight.get(), light_inputLayout.Get(), false, false, [=]{
			pd3dImmediateContext->VSSetConstantBuffers(0, 1, constantBuffersToArray(*main_scene_state_cb));
			pd3dImmediateContext->PSSetConstantBuffers(0, 1, constantBuffersToArray(*main_scene_state_cb));

			pd3dImmediateContext->OMSetBlendState(states->Opaque(), Colors::Black, 0xFFFFFFFF);
			pd3dImmediateContext->RSSetState(states->CullCounterClockwise());
			pd3dImmediateContext->OMSetDepthStencilState(states->DepthDefault(), 0);
		});
	}
	pd3dImmediateContext->OMSetRenderTargets(3, renderTargetViewToArray(nullptr, nullptr, nullptr), dsv);

	pd3dImmediateContext->OMSetRenderTargets(1, renderTargetViewToArray(DXUTGetD3D11RenderTargetView()), dsv);
	if (true){
		ambientPostProcess->Process(pd3dImmediateContext, [=]
		{
			pd3dImmediateContext->PSSetConstantBuffers(0, 1, constantBuffersToArray(*main_scene_state_cb));

			pd3dImmediateContext->PSSetShaderResources(0, 3, shaderResourceViewToArray(srv1.Get(), srv2.Get(), srv3.Get()));
			pd3dImmediateContext->PSSetSamplers(0, 1, samplerStateToArray(states->LinearWrap()));

			pd3dImmediateContext->OMSetBlendState(states->Additive(), nullptr, 0xffffffff);
			pd3dImmediateContext->RSSetState(states->CullNone());
			pd3dImmediateContext->OMSetDepthStencilState(states->DepthNone(), 0);
		});

	}
	if (true){
		postProcess->Process(pd3dImmediateContext, [=]
		{
			pd3dImmediateContext->PSSetConstantBuffers(0, 1, constantBuffersToArray(*main_scene_state_cb));

			pd3dImmediateContext->PSSetShaderResources(0, 3, shaderResourceViewToArray(srv1.Get(), srv2.Get(), srv3.Get()));
			pd3dImmediateContext->PSSetSamplers(0, 1, samplerStateToArray(states->LinearWrap()));

			pd3dImmediateContext->OMSetBlendState(states->Additive(), nullptr, 0xffffffff);
			pd3dImmediateContext->RSSetState(states->CullNone());
			pd3dImmediateContext->OMSetDepthStencilState(light_depth_stencil_second_pass_state.Get(), 0);
		});

	}
	{
		// You can't have the same texture bound as both an input and an output at the same time. The solution is to explicitly clear one binding before applying the new binding.

		ID3D11ShaderResourceView* null[] = { nullptr, nullptr, nullptr };
		pd3dImmediateContext->PSSetShaderResources(0, 3, null);
	}
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }
    // Render the HUD
    if ( g_nRenderHUD > 0 )
    {
        DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
        if ( g_nRenderHUD > 1 )
        {
            g_HUD.OnRender( fElapsedTime );
            g_SampleUI.OnRender( fElapsedTime );
        }
        RenderText();
        DXUT_EndPerfEvent();
    }

    // Check if current frame needs to be dumped to disk
    if ( s_dwFrameNumber == g_dwFrameNumberToDump )
    {
        // Retrieve resource for current render target
        ID3D11Resource *pRTResource;
        DXUTGetD3D11RenderTargetView()->GetResource( &pRTResource );

        // Retrieve a Texture2D interface from resource
        ID3D11Texture2D* RTTexture;
        hr = pRTResource->QueryInterface( __uuidof( ID3D11Texture2D ), ( LPVOID* )&RTTexture );

        // Check if RT is multisampled or not
        D3D11_TEXTURE2D_DESC TexDesc;
        RTTexture->GetDesc( &TexDesc );
        if ( TexDesc.SampleDesc.Count > 1 )
        {
            // RT is multisampled: need resolving before dumping to disk

            // Create single-sample RT of the same type and dimensions
            DXGI_SAMPLE_DESC SingleSample = { 1, 0 };
            TexDesc.CPUAccessFlags =    D3D11_CPU_ACCESS_READ;
            TexDesc.MipLevels =         1;
            TexDesc.Usage =             D3D11_USAGE_DEFAULT;
            TexDesc.CPUAccessFlags =    0;
            TexDesc.BindFlags =         0;
            TexDesc.SampleDesc =        SingleSample;

            // Create single-sample texture
            ID3D11Texture2D *pSingleSampleTexture;
            hr = pd3dDevice->CreateTexture2D( &TexDesc, NULL, &pSingleSampleTexture );
            DXUT_SetDebugName( pSingleSampleTexture, "Single Sample" );

            // Resolve multisampled render target into single-sample texture
            pd3dImmediateContext->ResolveSubresource( pSingleSampleTexture, 0, RTTexture, 0, TexDesc.Format );

            // Save texture to disk
            hr = D3DX11SaveTextureToFile( pd3dImmediateContext, pSingleSampleTexture, D3DX11_IFF_BMP, L"RTOutput.bmp" );

            // Release single-sample version of texture
            SAFE_RELEASE(pSingleSampleTexture);
            
        }
        else
        {
            // Single sample case
            hr = D3DX11SaveTextureToFile( pd3dImmediateContext, RTTexture, D3DX11_IFF_BMP, L"RTOutput.bmp" );
        }

        // Release texture and resource
        SAFE_RELEASE(RTTexture);
        SAFE_RELEASE(pRTResource);
    }

    // Increase frame number
    s_dwFrameNumber++;
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();

	srv1.ReleaseAndGetAddressOf();

	srv2.ReleaseAndGetAddressOf();

	srv3.ReleaseAndGetAddressOf();

	rtv1.ReleaseAndGetAddressOf();

	rtv2.ReleaseAndGetAddressOf();

	rtv3.ReleaseAndGetAddressOf();

	ds_v.ReleaseAndGetAddressOf();

	ds_srv.ReleaseAndGetAddressOf();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
	decal_box = 0;
	stone_box = 0;
	teapot = 0;
	light = 0;

	effect = 0;
	effectTBN = 0;
	postProcess = 0;
	ambientPostProcess = 0;
	effectLight = 0;

	states = 0;

	box_inputLayout.Reset();
	teapot_inputLayout.Reset();
	teapot_tbn_inputLayout.Reset();
	light_inputLayout.Reset();

	decal_srv.ReleaseAndGetAddressOf();
	decal_nm_srv.ReleaseAndGetAddressOf();
	stone_srv.ReleaseAndGetAddressOf();
	stone_nm_srv.ReleaseAndGetAddressOf();
	teapot_srv.ReleaseAndGetAddressOf();
	teapot_nm_srv.ReleaseAndGetAddressOf();

	delete scene_state_cb;
	delete main_scene_state_cb;

	light_depth_stencil_first_pass_state.ReleaseAndGetAddressOf();
	light_depth_stencil_second_pass_state.ReleaseAndGetAddressOf();
	light_depth_stencil_ambient_pass_state.ReleaseAndGetAddressOf();
	///////////////////////////////////////////////////////////////
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    g_LightControl.StaticOnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );

    SAFE_RELEASE( g_pLightTextureRV );
    for ( int i=0; i<(int)g_dwNumTextures; ++i )
    {
        SAFE_RELEASE( g_pGridDensityVBSRV[i] );
        SAFE_RELEASE( g_pGridDensityVB[i] );
        SAFE_RELEASE( g_pDetailTessellationDensityTextureRV[i] );
        SAFE_RELEASE( g_pDetailTessellationHeightTextureRV[i] );
        SAFE_RELEASE( g_pDetailTessellationBaseTextureRV[i] );
    }
    SAFE_RELEASE( g_pParticleVB );
    SAFE_RELEASE( g_pGridTangentSpaceIB )
    SAFE_RELEASE( g_pGridTangentSpaceVB );
    
    SAFE_RELEASE( g_pPositionOnlyVertexLayout );
    SAFE_RELEASE( g_pTangentSpaceVertexLayout );

    SAFE_RELEASE( g_pParticlePS );
    SAFE_RELEASE( g_pParticleGS );
    SAFE_RELEASE( g_pParticleVS );
    SAFE_RELEASE( g_pBumpMapPS );
    SAFE_RELEASE( g_pDetailTessellationDS );
    SAFE_RELEASE( g_pDetailTessellationHS );
    SAFE_RELEASE( g_pDetailTessellationVS );
    SAFE_RELEASE( g_pNoTessellationVS );

    SAFE_RELEASE( g_pPOMPS );
    SAFE_RELEASE( g_pPOMVS );

    SAFE_RELEASE( g_pMaterialCB );
    SAFE_RELEASE( g_pMainCB );

    SAFE_RELEASE( g_pDepthStencilState );
    SAFE_RELEASE( g_pBlendStateAdditiveBlending );
    SAFE_RELEASE( g_pBlendStateNoBlend );
    SAFE_RELEASE( g_pRasterizerStateSolid );
    SAFE_RELEASE( g_pRasterizerStateWireframe );
    SAFE_RELEASE( g_pSamplerStateLinear );
}


//--------------------------------------------------------------------------------------
// Helper function for command line retrieval
//--------------------------------------------------------------------------------------
bool IsNextArg( WCHAR*& strCmdLine, WCHAR* strArg )
{
   int nArgLen = (int)wcslen( strArg );
   int nCmdLen = (int)wcslen( strCmdLine );

   if( ( nCmdLen >= nArgLen ) && 
       ( _wcsnicmp( strCmdLine, strArg, nArgLen ) == 0 ) && 
       ( (strCmdLine[nArgLen] == 0 || strCmdLine[nArgLen] == L':' || strCmdLine[nArgLen] == L'=' ) ) )
   {
      strCmdLine += nArgLen;
      return true;
   }

   return false;
}


//--------------------------------------------------------------------------------------
// Helper function for command line retrieval.  Updates strCmdLine and strFlag 
//      Example: if strCmdLine=="-width:1024 -forceref"
// then after: strCmdLine==" -forceref" and strFlag=="1024"
//--------------------------------------------------------------------------------------
bool GetCmdParam( WCHAR*& strCmdLine, WCHAR* strFlag )
{
   if( *strCmdLine == L':' || *strCmdLine == L'=' )
   {       
      strCmdLine++; // Skip ':'

      // Place NULL terminator in strFlag after current token
      wcscpy_s( strFlag, 256, strCmdLine );
      WCHAR* strSpace = strFlag;
      while ( *strSpace && (*strSpace > L' ') )
         strSpace++;
      *strSpace = 0;

      // Update strCmdLine
      strCmdLine += wcslen( strFlag );
      return true;
   }
   else
   {
      strFlag[0] = 0;
      return false;
   }
}






//--------------------------------------------------------------------------------------
// Create a density texture map from a height map
//--------------------------------------------------------------------------------------
void CreateDensityMapFromHeightMap( ID3D11Device* pd3dDevice, ID3D11DeviceContext *pDeviceContext, ID3D11Texture2D* pHeightMap, 
                                    float fDensityScale, ID3D11Texture2D** ppDensityMap, ID3D11Texture2D** ppDensityMapStaging, 
                                    float fMeaningfulDifference )
{
#define ReadPixelFromMappedSubResource(x, y)       ( *( (RGBQUAD *)((BYTE *)MappedSubResourceRead.pData + (y)*MappedSubResourceRead.RowPitch + (x)*sizeof(DWORD)) ) )
#define WritePixelToMappedSubResource(x, y, value) ( ( *( (DWORD *)((BYTE *)MappedSubResourceWrite.pData + (y)*MappedSubResourceWrite.RowPitch + (x)*sizeof(DWORD)) ) ) = (DWORD)(value) )

    // Get description of source texture
    D3D11_TEXTURE2D_DESC Desc;
    pHeightMap->GetDesc( &Desc );    

    // Create density map with the same properties
    pd3dDevice->CreateTexture2D( &Desc, NULL, ppDensityMap );
    DXUT_SetDebugName( *ppDensityMap, "Density Map" );

    // Create STAGING height map texture
    ID3D11Texture2D* pHeightMapStaging;
    Desc.CPUAccessFlags =   D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    Desc.Usage =            D3D11_USAGE_STAGING;
    Desc.BindFlags =        0;
    pd3dDevice->CreateTexture2D( &Desc, NULL, &pHeightMapStaging );
    DXUT_SetDebugName( pHeightMapStaging, "Height Map" );

    // Create STAGING density map
    ID3D11Texture2D* pDensityMapStaging;
    pd3dDevice->CreateTexture2D( &Desc, NULL, &pDensityMapStaging );
    DXUT_SetDebugName( pDensityMapStaging, "Density Map" );

    // Copy contents of height map into staging version
    pDeviceContext->CopyResource( pHeightMapStaging, pHeightMap );

    // Compute density map and store into staging version
    D3D11_MAPPED_SUBRESOURCE MappedSubResourceRead, MappedSubResourceWrite;
    pDeviceContext->Map( pHeightMapStaging, 0, D3D11_MAP_READ, 0, &MappedSubResourceRead );

    pDeviceContext->Map( pDensityMapStaging, 0, D3D11_MAP_WRITE, 0, &MappedSubResourceWrite );

    // Loop map and compute derivatives
    for ( int j=0; j <= (int)Desc.Height-1; ++j )
    {
        for ( int i=0; i <= (int)Desc.Width-1; ++i )
        {
            // Edges are set to the minimum value
            if ( ( j == 0 ) || 
                 ( i == 0 ) || 
                 ( j == ( (int)Desc.Height-1 ) ) || 
                 ( i == ((int)Desc.Width-1 ) ) )
            {
                WritePixelToMappedSubResource( i, j, (DWORD)1 );
                continue;
            }

            // Get current pixel
            RGBQUAD CurrentPixel =     ReadPixelFromMappedSubResource( i, j );

            // Get left pixel
            RGBQUAD LeftPixel =        ReadPixelFromMappedSubResource( i-1, j );

            // Get right pixel
            RGBQUAD RightPixel =       ReadPixelFromMappedSubResource( i+1, j );

            // Get top pixel
            RGBQUAD TopPixel =         ReadPixelFromMappedSubResource( i, j-1 );

            // Get bottom pixel
            RGBQUAD BottomPixel =      ReadPixelFromMappedSubResource( i, j+1 );

            // Get top-right pixel
            RGBQUAD TopRightPixel =    ReadPixelFromMappedSubResource( i+1, j-1 );

            // Get bottom-right pixel
            RGBQUAD BottomRightPixel = ReadPixelFromMappedSubResource( i+1, j+1 );
            
            // Get top-left pixel
            RGBQUAD TopLeftPixel =     ReadPixelFromMappedSubResource( i-1, j-1 );
            
            // Get bottom-left pixel
            RGBQUAD BottomLeft =       ReadPixelFromMappedSubResource( i-1, j+1 );

            float fCurrentPixelHeight =            ( CurrentPixel.rgbReserved     / 255.0f );
            float fCurrentPixelLeftHeight =        ( LeftPixel.rgbReserved        / 255.0f );
            float fCurrentPixelRightHeight =       ( RightPixel.rgbReserved       / 255.0f );
            float fCurrentPixelTopHeight =         ( TopPixel.rgbReserved         / 255.0f );
            float fCurrentPixelBottomHeight =      ( BottomPixel.rgbReserved      / 255.0f );
            float fCurrentPixelTopRightHeight =    ( TopRightPixel.rgbReserved    / 255.0f );
            float fCurrentPixelBottomRightHeight = ( BottomRightPixel.rgbReserved / 255.0f );
            float fCurrentPixelTopLeftHeight =     ( TopLeftPixel.rgbReserved     / 255.0f );
            float fCurrentPixelBottomLeftHeight =  ( BottomLeft.rgbReserved       / 255.0f );

            float fDensity = 0.0f;
            float fHorizontalVariation = fabs( ( fCurrentPixelRightHeight - fCurrentPixelHeight ) - 
                                               ( fCurrentPixelHeight - fCurrentPixelLeftHeight ) );
            float fVerticalVariation = fabs( ( fCurrentPixelBottomHeight - fCurrentPixelHeight ) - 
                                             ( fCurrentPixelHeight - fCurrentPixelTopHeight ) );
            float fFirstDiagonalVariation = fabs( ( fCurrentPixelTopRightHeight - fCurrentPixelHeight ) - 
                                                  ( fCurrentPixelHeight - fCurrentPixelBottomLeftHeight ) );
            float fSecondDiagonalVariation = fabs( ( fCurrentPixelBottomRightHeight - fCurrentPixelHeight ) - 
                                                   ( fCurrentPixelHeight - fCurrentPixelTopLeftHeight ) );
            if ( fHorizontalVariation     >= fMeaningfulDifference) fDensity += 0.293f * fHorizontalVariation;
            if ( fVerticalVariation       >= fMeaningfulDifference) fDensity += 0.293f * fVerticalVariation;
            if ( fFirstDiagonalVariation  >= fMeaningfulDifference) fDensity += 0.207f * fFirstDiagonalVariation;
            if ( fSecondDiagonalVariation >= fMeaningfulDifference) fDensity += 0.207f * fSecondDiagonalVariation;
                
            // Scale density with supplied scale                
            fDensity *= fDensityScale;

            // Clamp density
            fDensity = max( min( fDensity, 1.0f ), 1.0f/255.0f );

            // Write density into density map
            WritePixelToMappedSubResource( i, j, (DWORD)( fDensity * 255.0f ) );
        }
    }

    pDeviceContext->Unmap( pDensityMapStaging, 0 );

    pDeviceContext->Unmap( pHeightMapStaging, 0 );
    SAFE_RELEASE( pHeightMapStaging );

    // Copy contents of density map into DEFAULT density version
    pDeviceContext->CopyResource( *ppDensityMap, pDensityMapStaging );
    
    // If a pointer to a staging density map was provided then return it with staging density map
    if ( ppDensityMapStaging )
    {
        *ppDensityMapStaging = pDensityMapStaging;
    }
    else
    {
        SAFE_RELEASE( pDensityMapStaging );
    }
}


//--------------------------------------------------------------------------------------
// Sample a 32-bit texture at the specified texture coordinate (point sampling only)
//--------------------------------------------------------------------------------------
__inline RGBQUAD SampleTexture2D_32bit( DWORD *pTexture2D, DWORD dwWidth, DWORD dwHeight, D3DXVECTOR2 fTexcoord, DWORD dwStride )
{
#define FROUND(x)    ( (int)( (x) + 0.5 ) )

    // Convert texture coordinates to texel coordinates using round-to-nearest
    int nU = FROUND( fTexcoord.x * ( dwWidth-1 ) );
    int nV = FROUND( fTexcoord.y * ( dwHeight-1 ) );

    // Clamp texcoord between 0 and [width-1, height-1]
    nU = nU % dwWidth;
    nV = nV % dwHeight;

    // Return value at this texel coordinate
    return *(RGBQUAD *)( ( (BYTE *)pTexture2D) + ( nV*dwStride ) + ( nU*sizeof(DWORD) ) );
}


//--------------------------------------------------------------------------------------
// Sample density map along specified edge and return maximum value
//--------------------------------------------------------------------------------------
float GetMaximumDensityAlongEdge( DWORD *pTexture2D, DWORD dwStride, DWORD dwWidth, DWORD dwHeight, 
                                  D3DXVECTOR2 fTexcoord0, D3DXVECTOR2 fTexcoord1 )
{
#define GETTEXEL(x, y)       ( *(RGBQUAD *)( ( (BYTE *)pTexture2D ) + ( (y)*dwStride ) + ( (x)*sizeof(DWORD) ) ) )
#define LERP(x, y, a)        ( (x)*(1.0f-(a)) + (y)*(a) )

    // Convert texture coordinates to texel coordinates using round-to-nearest
    int nU0 = FROUND( fTexcoord0.x * ( dwWidth  - 1 )  );
    int nV0 = FROUND( fTexcoord0.y * ( dwHeight - 1 ) );
    int nU1 = FROUND( fTexcoord1.x * ( dwWidth  - 1 )  );
    int nV1 = FROUND( fTexcoord1.y * ( dwHeight - 1 ) );

    // Wrap texture coordinates
    nU0 = nU0 % dwWidth;
    nV0 = nV0 % dwHeight;
    nU1 = nU1 % dwWidth;
    nV1 = nV1 % dwHeight;

    // Find how many texels on edge
    int nNumTexelsOnEdge = max( abs( nU1 - nU0 ), abs( nV1 - nV0 ) ) + 1;

    float fU, fV;
    float fMaxDensity = 0.0f;
    for ( int i=0; i<nNumTexelsOnEdge; ++i )
    {
        // Calculate current texel coordinates
        fU = LERP( (float)nU0, (float)nU1, ( (float)i ) / ( nNumTexelsOnEdge - 1 ) );
        fV = LERP( (float)nV0, (float)nV1, ( (float)i ) / ( nNumTexelsOnEdge - 1 ) );

        // Round texel texture coordinates to nearest
        int nCurrentU = FROUND( fU );
        int nCurrentV = FROUND( fV );

        // Update max density along edge
        fMaxDensity = max( fMaxDensity, GETTEXEL( nCurrentU, nCurrentV ).rgbBlue / 255.0f );
    }
        
    return fMaxDensity;
}


//--------------------------------------------------------------------------------------
// Calculate the maximum density along a triangle edge and
// store it in the resulting vertex stream.
//--------------------------------------------------------------------------------------
void CreateEdgeDensityVertexStream( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pDeviceContext, D3DXVECTOR2* pTexcoord, 
                                    DWORD dwVertexStride, void *pIndex, DWORD dwIndexSize, DWORD dwNumIndices, 
                                    ID3D11Texture2D* pDensityMap, ID3D11Buffer** ppEdgeDensityVertexStream,
                                    float fBaseTextureRepeat )
{
    HRESULT                hr;
    D3D11_SUBRESOURCE_DATA InitData;
    ID3D11Texture2D*       pDensityMapReadFrom;
    DWORD                  dwNumTriangles = dwNumIndices / 3;

    // Create host memory buffer
    D3DXVECTOR4 *pEdgeDensityBuffer = new D3DXVECTOR4[dwNumTriangles];

    // Retrieve density map description
    D3D11_TEXTURE2D_DESC Tex2DDesc;
    pDensityMap->GetDesc( &Tex2DDesc );
 
    // Check if provided texture can be Mapped() for reading directly
    BOOL bCanBeRead = Tex2DDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ;
    if ( !bCanBeRead )
    {
        // Texture cannot be read directly, need to create STAGING texture and copy contents into it
        Tex2DDesc.CPUAccessFlags =   D3D11_CPU_ACCESS_READ;
        Tex2DDesc.Usage =            D3D11_USAGE_STAGING;
        Tex2DDesc.BindFlags =        0;
        pd3dDevice->CreateTexture2D( &Tex2DDesc, NULL, &pDensityMapReadFrom );
        DXUT_SetDebugName( pDensityMapReadFrom, "DensityMap Read" );

        // Copy contents of height map into staging version
        pDeviceContext->CopyResource( pDensityMapReadFrom, pDensityMap );
    }
    else
    {
        pDensityMapReadFrom = pDensityMap;
    }

    // Map density map for reading
    D3D11_MAPPED_SUBRESOURCE MappedSubResource;
    pDeviceContext->Map( pDensityMapReadFrom, 0, D3D11_MAP_READ, 0, &MappedSubResource );

    // Loop through all triangles
    for ( DWORD i=0; i<dwNumTriangles; ++i )
    {
        DWORD wIndex0, wIndex1, wIndex2;

        // Retrieve indices of current triangle
        if ( dwIndexSize == sizeof(WORD) )
        {
            wIndex0 = (DWORD)( (WORD *)pIndex )[3*i+0];
            wIndex1 = (DWORD)( (WORD *)pIndex )[3*i+1];
            wIndex2 = (DWORD)( (WORD *)pIndex )[3*i+2];
        }
        else
        {
            wIndex0 = ( (DWORD *)pIndex )[3*i+0];
            wIndex1 = ( (DWORD *)pIndex )[3*i+1];
            wIndex2 = ( (DWORD *)pIndex )[3*i+2];
        }

        // Retrieve texture coordinates of each vertex making up current triangle
        D3DXVECTOR2    vTexcoord0 = *(D3DXVECTOR2 *)( (BYTE *)pTexcoord + wIndex0 * dwVertexStride );
        D3DXVECTOR2    vTexcoord1 = *(D3DXVECTOR2 *)( (BYTE *)pTexcoord + wIndex1 * dwVertexStride );
        D3DXVECTOR2    vTexcoord2 = *(D3DXVECTOR2 *)( (BYTE *)pTexcoord + wIndex2 * dwVertexStride );

        // Adjust texture coordinates with texture repeat
        vTexcoord0 *= fBaseTextureRepeat;
        vTexcoord1 *= fBaseTextureRepeat;
        vTexcoord2 *= fBaseTextureRepeat;

        // Sample density map at vertex location
        float fEdgeDensity0 = GetMaximumDensityAlongEdge( (DWORD *)MappedSubResource.pData, MappedSubResource.RowPitch, 
                                                          Tex2DDesc.Width, Tex2DDesc.Height, vTexcoord0, vTexcoord1 );
        float fEdgeDensity1 = GetMaximumDensityAlongEdge( (DWORD *)MappedSubResource.pData, MappedSubResource.RowPitch, 
                                                          Tex2DDesc.Width, Tex2DDesc.Height, vTexcoord1, vTexcoord2 );
        float fEdgeDensity2 = GetMaximumDensityAlongEdge( (DWORD *)MappedSubResource.pData, MappedSubResource.RowPitch, 
                                                          Tex2DDesc.Width, Tex2DDesc.Height, vTexcoord2, vTexcoord0 );

        // Store edge density in x,y,z and store maximum density in .w
        pEdgeDensityBuffer[i] = D3DXVECTOR4( fEdgeDensity0, fEdgeDensity1, fEdgeDensity2, 
                                             max( max( fEdgeDensity0, fEdgeDensity1 ), fEdgeDensity2 ) );
    }

    // Unmap density map
    pDeviceContext->Unmap( pDensityMapReadFrom, 0 );

    // Release staging density map if we had to create it
    if ( !bCanBeRead )
    {
        SAFE_RELEASE( pDensityMapReadFrom );
    }
    
    // Set source buffer for initialization data
    InitData.pSysMem = (void *)pEdgeDensityBuffer;

    // Create density vertex stream buffer: 1 float per entry
    D3D11_BUFFER_DESC bd;
    bd.Usage =            D3D11_USAGE_DEFAULT;
    bd.ByteWidth =        dwNumTriangles * sizeof( D3DXVECTOR4 );
    bd.BindFlags =        D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_SHADER_RESOURCE;
    bd.CPUAccessFlags =   0;
    bd.MiscFlags =        0;
    hr = pd3dDevice->CreateBuffer( &bd, &InitData, ppEdgeDensityVertexStream );
    if( FAILED( hr ) )
    {
        OutputDebugString( L"Failed to create vertex buffer.\n" );
    }
    DXUT_SetDebugName( *ppEdgeDensityVertexStream, "Edge Density" );

    // Free host memory buffer
    delete [] pEdgeDensityBuffer;
}


//--------------------------------------------------------------------------------------
// Helper function to create a staging buffer from a buffer resource
//--------------------------------------------------------------------------------------
void CreateStagingBufferFromBuffer( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext, 
                                   ID3D11Buffer* pInputBuffer, ID3D11Buffer **ppStagingBuffer )
{
    D3D11_BUFFER_DESC BufferDesc;

    // Get buffer description
    pInputBuffer->GetDesc( &BufferDesc );

    // Modify description to create STAGING buffer
    BufferDesc.BindFlags =          0;
    BufferDesc.CPUAccessFlags =     D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
    BufferDesc.Usage =              D3D11_USAGE_STAGING;

    // Create staging buffer
    pd3dDevice->CreateBuffer( &BufferDesc, NULL, ppStagingBuffer );

    // Copy buffer into STAGING buffer
    pContext->CopyResource( *ppStagingBuffer, pInputBuffer );
}


//--------------------------------------------------------------------------------------
// Helper function to create a shader from the specified filename
// This function is called by the shader-specific versions of this
// function located after the body of this function.
//--------------------------------------------------------------------------------------
HRESULT CreateShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                              LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                              ID3DX11ThreadPump* pPump, ID3D11DeviceChild** ppShader, ID3DBlob** ppShaderBlob, 
                              BOOL bDumpShader)
{
    HRESULT   hr = D3D_OK;
    ID3DBlob* pShaderBlob = NULL;
    ID3DBlob* pErrorBlob = NULL;
    WCHAR     wcFullPath[256];
    
    DXUTFindDXSDKMediaFileCch( wcFullPath, 256, pSrcFile );
    // Compile shader into binary blob
    hr = D3DX11CompileFromFile( wcFullPath, pDefines, pInclude, pFunctionName, pProfile, 
                                Flags1, Flags2, pPump, &pShaderBlob, &pErrorBlob, NULL );
    if( FAILED( hr ) )
    {
        OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        SAFE_RELEASE( pErrorBlob );
        return hr;
    }
    
    // Create shader from binary blob
    if ( ppShader )
    {
        hr = E_FAIL;
        if ( strstr( pProfile, "vs" ) )
        {
            hr = pd3dDevice->CreateVertexShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11VertexShader**)ppShader );
        }
        else if ( strstr( pProfile, "hs" ) )
        {
            hr = pd3dDevice->CreateHullShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11HullShader**)ppShader ); 
        }
        else if ( strstr( pProfile, "ds" ) )
        {
            hr = pd3dDevice->CreateDomainShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11DomainShader**)ppShader );
        }
        else if ( strstr( pProfile, "gs" ) )
        {
            hr = pd3dDevice->CreateGeometryShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11GeometryShader**)ppShader ); 
        }
        else if ( strstr( pProfile, "ps" ) )
        {
            hr = pd3dDevice->CreatePixelShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11PixelShader**)ppShader ); 
        }
        else if ( strstr( pProfile, "cs" ) )
        {
            hr = pd3dDevice->CreateComputeShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11ComputeShader**)ppShader );
        }
        if ( FAILED( hr ) )
        {
            OutputDebugString( L"Shader creation failed\n" );
            SAFE_RELEASE( pErrorBlob );
            SAFE_RELEASE( pShaderBlob );
            return hr;
        }
    }

    DXUT_SetDebugName( *ppShader, pFunctionName );

    // If blob was requested then pass it otherwise release it
    if ( ppShaderBlob )
    {
        *ppShaderBlob = pShaderBlob;
    }
    else
    {
        pShaderBlob->Release();
    }

    // Return error code
    return hr;
}


//--------------------------------------------------------------------------------------
// Create a vertex shader from the specified filename
//--------------------------------------------------------------------------------------
HRESULT CreateVertexShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                                    LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                                    ID3DX11ThreadPump* pPump, ID3D11VertexShader** ppShader, ID3DBlob** ppShaderBlob, 
                                    BOOL bDumpShader )
{
    return CreateShaderFromFile( pd3dDevice, pSrcFile, pDefines, pInclude, pFunctionName, pProfile, 
                                 Flags1, Flags2, pPump, (ID3D11DeviceChild **)ppShader, ppShaderBlob, 
                                 bDumpShader );
}


//--------------------------------------------------------------------------------------
// Create a hull shader from the specified filename
//--------------------------------------------------------------------------------------
HRESULT CreateHullShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                                  LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                                  ID3DX11ThreadPump* pPump, ID3D11HullShader** ppShader, ID3DBlob** ppShaderBlob, 
                                  BOOL bDumpShader )
{
    return CreateShaderFromFile( pd3dDevice, pSrcFile, pDefines, pInclude, pFunctionName, pProfile, 
                                 Flags1, Flags2, pPump, (ID3D11DeviceChild **)ppShader, ppShaderBlob, 
                                 bDumpShader );
}
//--------------------------------------------------------------------------------------
// Create a domain shader from the specified filename
//--------------------------------------------------------------------------------------
HRESULT CreateDomainShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                                    LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                                    ID3DX11ThreadPump* pPump, ID3D11DomainShader** ppShader, ID3DBlob** ppShaderBlob, 
                                    BOOL bDumpShader )
{
    return CreateShaderFromFile( pd3dDevice, pSrcFile, pDefines, pInclude, pFunctionName, pProfile, 
                                 Flags1, Flags2, pPump, (ID3D11DeviceChild **)ppShader, ppShaderBlob, 
                                 bDumpShader );
}


//--------------------------------------------------------------------------------------
// Create a geometry shader from the specified filename
//--------------------------------------------------------------------------------------
HRESULT CreateGeometryShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                                      LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                                      ID3DX11ThreadPump* pPump, ID3D11GeometryShader** ppShader, ID3DBlob** ppShaderBlob, 
                                      BOOL bDumpShader )
{
    return CreateShaderFromFile( pd3dDevice, pSrcFile, pDefines, pInclude, pFunctionName, pProfile, 
                                 Flags1, Flags2, pPump, (ID3D11DeviceChild **)ppShader, ppShaderBlob, 
                                 bDumpShader );
}


//--------------------------------------------------------------------------------------
// Create a pixel shader from the specified filename
//--------------------------------------------------------------------------------------
HRESULT CreatePixelShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                                   LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                                   ID3DX11ThreadPump* pPump, ID3D11PixelShader** ppShader, ID3DBlob** ppShaderBlob, 
                                   BOOL bDumpShader )
{
    return CreateShaderFromFile( pd3dDevice, pSrcFile, pDefines, pInclude, pFunctionName, pProfile, 
                                 Flags1, Flags2, pPump, (ID3D11DeviceChild **)ppShader, ppShaderBlob, 
                                 bDumpShader );
}


//--------------------------------------------------------------------------------------
// Create a compute shader from the specified filename
//--------------------------------------------------------------------------------------
HRESULT CreateComputeShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D_SHADER_MACRO* pDefines, 
                                     LPD3DINCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                                     ID3DX11ThreadPump* pPump, ID3D11ComputeShader** ppShader, ID3DBlob** ppShaderBlob, 
                                     BOOL bDumpShader)
{
    return CreateShaderFromFile( pd3dDevice, pSrcFile, pDefines, pInclude, pFunctionName, pProfile, 
                                 Flags1, Flags2, pPump, (ID3D11DeviceChild **)ppShader, ppShaderBlob, 
                                 bDumpShader );
}


//--------------------------------------------------------------------------------------
// Helper function to normalize a plane
//--------------------------------------------------------------------------------------
void NormalizePlane( D3DXVECTOR4* pPlaneEquation )
{
    float mag;
    
    mag = sqrt( pPlaneEquation->x * pPlaneEquation->x + 
                pPlaneEquation->y * pPlaneEquation->y + 
                pPlaneEquation->z * pPlaneEquation->z );
    
    pPlaneEquation->x = pPlaneEquation->x / mag;
    pPlaneEquation->y = pPlaneEquation->y / mag;
    pPlaneEquation->z = pPlaneEquation->z / mag;
    pPlaneEquation->w = pPlaneEquation->w / mag;
}


//--------------------------------------------------------------------------------------
// Extract all 6 plane equations from frustum denoted by supplied matrix
//--------------------------------------------------------------------------------------
void ExtractPlanesFromFrustum( D3DXVECTOR4* pPlaneEquation, const D3DXMATRIX* pMatrix, bool bNormalize )
{
    // Left clipping plane
    pPlaneEquation[0].x = pMatrix->_14 + pMatrix->_11;
    pPlaneEquation[0].y = pMatrix->_24 + pMatrix->_21;
    pPlaneEquation[0].z = pMatrix->_34 + pMatrix->_31;
    pPlaneEquation[0].w = pMatrix->_44 + pMatrix->_41;
    
    // Right clipping plane
    pPlaneEquation[1].x = pMatrix->_14 - pMatrix->_11;
    pPlaneEquation[1].y = pMatrix->_24 - pMatrix->_21;
    pPlaneEquation[1].z = pMatrix->_34 - pMatrix->_31;
    pPlaneEquation[1].w = pMatrix->_44 - pMatrix->_41;
    
    // Top clipping plane
    pPlaneEquation[2].x = pMatrix->_14 - pMatrix->_12;
    pPlaneEquation[2].y = pMatrix->_24 - pMatrix->_22;
    pPlaneEquation[2].z = pMatrix->_34 - pMatrix->_32;
    pPlaneEquation[2].w = pMatrix->_44 - pMatrix->_42;
    
    // Bottom clipping plane
    pPlaneEquation[3].x = pMatrix->_14 + pMatrix->_12;
    pPlaneEquation[3].y = pMatrix->_24 + pMatrix->_22;
    pPlaneEquation[3].z = pMatrix->_34 + pMatrix->_32;
    pPlaneEquation[3].w = pMatrix->_44 + pMatrix->_42;
    
    // Near clipping plane
    pPlaneEquation[4].x = pMatrix->_13;
    pPlaneEquation[4].y = pMatrix->_23;
    pPlaneEquation[4].z = pMatrix->_33;
    pPlaneEquation[4].w = pMatrix->_43;
    
    // Far clipping plane
    pPlaneEquation[5].x = pMatrix->_14 - pMatrix->_13;
    pPlaneEquation[5].y = pMatrix->_24 - pMatrix->_23;
    pPlaneEquation[5].z = pMatrix->_34 - pMatrix->_33;
    pPlaneEquation[5].w = pMatrix->_44 - pMatrix->_43;
    
    // Normalize the plane equations, if requested
    if ( bNormalize )
    {
        NormalizePlane( &pPlaneEquation[0] );
        NormalizePlane( &pPlaneEquation[1] );
        NormalizePlane( &pPlaneEquation[2] );
        NormalizePlane( &pPlaneEquation[3] );
        NormalizePlane( &pPlaneEquation[4] );
        NormalizePlane( &pPlaneEquation[5] );
    }
}

