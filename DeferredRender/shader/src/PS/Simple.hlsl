ThreeTargets PS(in float4 color : TEXCOORD0)
{ 
   ThreeTargets output;

   output.target0 = float4( 0, 0, 0, 1.0 );

   output.target1 = float4( 0, 0, 0, 1.0 );

   output.target2 = color;

   return  output;
}

float4 CONST_COLOR(in float2 tex : TEXCOORD0):SV_TARGET
{ 
   return  float4( 1.0, 1.0, 1.0, 0.0 );;
}