#include "shader\\inc\\shader_include.hlsl"

#include "shader\\src\\vs\\FullScreenQuad.hlsl"

Texture2D positionTexture : register( t0 );
Texture2D normalTexture : register( t1 );
Texture2D colorTexture : register( t2 );

SamplerState linearSampler : register( s0 );

#include "shader\\src\\ps\\Lambert.hlsl"