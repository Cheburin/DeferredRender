ClipPosPosNormalTangentBitangentTex2d VS( in PosNormalTangetColorTex2d i )
{
    ClipPosPosNormalTangentBitangentTex2d Out;
        
    // Transform and output input position 
    Out.clip_pos = mul( float4( i.pos, 1.0 ), g_mWorldViewProjection );
       
    float3 ts = textureScale.xyz;//float3(10, 10, 3);   

    if(abs(i.normal.z) > 0.001)
        Out.tex = float2(ts.x*i.tex.x, ts.y*i.tex.y);
    else if(abs(i.normal.y) > 0.001)
        Out.tex = float2(ts.x*i.tex.x, ts.z*i.tex.y);
    else if(abs(i.normal.x) > 0.001)
        Out.tex = float2(ts.z*i.tex.x, ts.y*i.tex.y);

    matrix mWorldView = mul(g_mWorld, g_mView);
    Out.tangent = mul( float4( i.tangent.xyz, 0.0 ), mWorldView ).xyz;
    Out.bitangent = mul( float4( cross(i.normal, i.tangent.xyz) * i.tangent.w, 0.0 ), mWorldView ).xyz;
    Out.normal = mul( float4( i.normal, 0.0 ), mWorldView ).xyz;

    // Transform object space vertex to world space
    Out.pos = mul( float4( i.pos, 1.0 ), mWorldView ).xyz;

    return Out;
}   
