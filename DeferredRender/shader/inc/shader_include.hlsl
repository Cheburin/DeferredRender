//--------------------------------------------------------------------------------------
// Defines
//--------------------------------------------------------------------------------------                  
#define ADD_SPECULAR 0
                                          
//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

struct PosNormalTex2d
{
    float3 pos : SV_Position;
    float3 normal   : NORMAL;
    float2 tex      : TEXCOORD0;
};

struct PosNormalTangetColorTex2d
{
    float3 pos      : SV_Position;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT; 
    float4 color    : COLOR0;
    float2 tex      : TEXCOORD0;
};

struct ExpandPosNormalTangetColorTex2d
{
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT; 
    float4 color    : COLOR0;
    float2 tex      : TEXCOORD0;
    float4 pos      : SV_POSITION;
};

struct ClipPosTex2d
{
    float2 tex            : TEXCOORD0;
    float4 clip_pos       : SV_POSITION; // Output position
};

struct ClipPosPosNormalTex2d
{
    float3 pos            : TEXCOORD0;
    float3 normal         : TEXCOORD1;   // Normal vector in world space
    float2 tex            : TEXCOORD2;
    float4 clip_pos       : SV_POSITION; // Output position
};

struct ClipPosPosNormalTangentBitangentTex2d
{
    float3 pos            : TEXCOORD0;
    float3 normal         : TEXCOORD1;
    float3 tangent        : TEXCOORD2;
    float3 bitangent      : TEXCOORD3;
    float2 tex            : TEXCOORD4;
    float4 clip_pos       : SV_POSITION; // Output position
};

struct ClipPosColor
{
    float4 color          : TEXCOORD0;
    float4 clip_pos       : SV_POSITION; // Output position
};

struct ThreeTargets
{
    float4 target0: SV_Target0;

    float4 target1: SV_Target1;

    float4 target2: SV_Target2;
};

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
cbuffer cbMain : register( b0 )
{
	matrix g_mWorld;                            // World matrix
	matrix g_mView;                             // View matrix
	matrix g_mProjection;                       // Projection matrix
    matrix g_mWorldViewProjection;              // WVP matrix
    matrix g_mWorldView;                        // WV matrix
    matrix g_mViewProjection;                   // VP matrix
    matrix g_mInvView;                          // Inverse of view matrix
    float4 g_vScreenResolution;                 // Screen resolution
    
    float4 g_vMeshColor;                        // Mesh color
    float4 g_vTessellationFactor;               // Edge, inside, minimum tessellation factor and 1/desired triangle size
    float4 g_vDetailTessellationHeightScale;    // Height scale for detail tessellation of grid surface
    float4 g_vGridSize;                         // Grid size
    
    float4 g_vDebugColorMultiply;               // Debug colors
    float4 g_vDebugColorAdd;                    // Debug colors
    
    float4 g_vFrustumPlaneEquation[4];          // View frustum plane equations
};