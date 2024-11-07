void axis(in float4 color, in float4 position, in float4 direction, inout LineStream<ClipPosColor> SpriteStream){
    ClipPosColor output;

    output.color = color;
    output.clip_pos = mul( position, g_mWorldViewProjection );
    SpriteStream.Append( output );

    output.color = color;
    output.clip_pos = mul( position + 0.05 * direction, g_mWorldViewProjection );
    SpriteStream.Append( output );

    SpriteStream.RestartStrip();
}

[maxvertexcount(6)]
void GS(point ExpandPosNormalTangetColorTex2d input[1], uint primID : SV_PrimitiveID, inout LineStream<ClipPosColor> SpriteStream){
  float  w = input[0].tangent.w; 
  float3 normal = input[0].normal.xyz;  
  float3 tangent = input[0].tangent.xyz;

  float3 bitangent = cross(normal, tangent) * w;  

  axis(float4(1,0,0,1), input[0].pos.xyzw, float4(tangent, 0), SpriteStream); 
  axis(float4(0,1,0,1), input[0].pos.xyzw, float4(bitangent, 0), SpriteStream);
  axis(float4(0,0,1,1), input[0].pos.xyzw, float4(normal, 0),  SpriteStream);
}