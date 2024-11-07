ClipPosTex2d VS(uint VertexID : SV_VERTEXID)
{
    ClipPosTex2d output;

	float2 tmp = float2( (VertexID << 1) & 2, VertexID & 2 );

	float4 pos = float4( tmp * float2( 2.0f, -2.0f ) + float2( -1.0f, 1.0f), 0.0f, 1.0f );

    output.tex = float2(pos.x * 0.5 + 0.5f, pos.y * -0.5 + 0.5f);

    output.clip_pos = pos;

    return output;
} 