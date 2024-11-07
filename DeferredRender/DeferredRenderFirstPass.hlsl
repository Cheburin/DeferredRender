#include "shader\\inc\\shader_include.hlsl"

cbuffer cbCustom : register( b1 )
{
	float4 textureScale; 
}

#include "shader\\src\\vs\\DeferredRenderBump.hlsl"

Texture2D colorTexture : register( t0 );
Texture2D normalTexture : register( t1 );
SamplerState linearSampler : register( s0 );

#include "shader\\src\\ps\\DeferredRenderBump.hlsl"