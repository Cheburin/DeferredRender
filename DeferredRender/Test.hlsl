#include "shader\\inc\\shader_include.hlsl"
struct ClipPosPosNormal
{
    float3 pos            : TEXCOORD0;
    float3 normal         : TEXCOORD1;   // Normal vector in world space
    float4 clip_pos       : SV_POSITION; // Output position
};
ClipPosPosNormal VS( in PosNormalTangetColorTex2d i )
{
  ClipPosPosNormal output;  
      
  matrix mWorldView = mul(g_mWorld, g_mView);      
  output.clip_pos = mul( float4( i.pos, 1.0  ), g_mWorldViewProjection );
  output.pos =    mul(   float4( i.pos, 1.0  ), mWorldView).xyz;
  output.normal = mul( float4( i.normal, 0.0 ), mWorldView).xyz;

  return output;
}   

float4 PS(in float3 pos : TEXCOORD0, in float3 normal : TEXCOORD1):SV_TARGET
{ 
   float3 lightPos = mul( float4( 2, 3, 0.5, 1.0 ), g_mView ).xyz;

   if ( length ( lightPos - pos ) > 2.25 ) discard;

   float3    l  = normalize     ( lightPos - pos );
   float3    n  = normalize     ( normal );

   float3   diff = max( 0.0, dot ( l, n ) )*float3(1,1,1);
    
   return float4( diff, 1.0 );    
}
