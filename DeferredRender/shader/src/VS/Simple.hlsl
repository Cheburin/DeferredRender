ExpandPosNormalTangetColorTex2d MODEL( in PosNormalTangetColorTex2d i )
{
  ExpandPosNormalTangetColorTex2d output;  
  
  output.pos = float4( i.pos, 1.0 );
  output.normal = i.normal;
  output.tangent = i.tangent;
  output.color = i.color;
  output.tex = i.tex;

  return output;
}   

ClipPosTex2d SIMPLE_MODEL( in PosNormalTex2d i )
{
  ClipPosTex2d output;  
  
  output.clip_pos = mul( float4( i.pos, 1.0 ), g_mWorldViewProjection );
  output.tex = i.tex;

  return output;
} 