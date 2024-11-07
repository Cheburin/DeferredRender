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
    float3 pos : SV_Position;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT; 
    float4 color    : COLOR0;
    float2 tex      : TEXCOORD0;
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

cbuffer cbMaterial : register( b1 )
{
	float4 	 g_materialAmbientColor;          	// Material's ambient color
	float4 	 g_materialDiffuseColor;          	// Material's diffuse color
	float4 	 g_materialSpecularColor;         	// Material's specular color
	float4 	 g_fSpecularExponent;             	// Material's specular exponent

	float4 	 g_LightPosition;                 	// Light's position in world space
	float4 	 g_LightDiffuse;                  	// Light's diffuse color
	float4 	 g_LightAmbient;                  	// Light's ambient color

	float4 	 g_vEye;					    	// Camera's location
	float4 	 g_fBaseTextureRepeat;		    	// The tiling factor for base and normal map textures
	float4 	 g_fPOMHeightMapScale;		    	// Describes the useful range of values for the height field
	
	float4   g_fShadowSoftening;		    	// Blurring factor for the soft shadows computation
	
	int      g_nMinSamples;				    	// The minimum number of samples for sampling the height field profile
	int      g_nMaxSamples;				    	// The maximum number of samples for sampling the height field profile
};