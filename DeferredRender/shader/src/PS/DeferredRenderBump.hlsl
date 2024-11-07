ThreeTargets PS( in float3 pos: TEXCOORD0, in float3 normal: TEXCOORD1, in float3 tangent: TEXCOORD2, in float3 bitangent: TEXCOORD3, in float2 tex: TEXCOORD4  )
{ 
   ThreeTargets output;

   float3 t = normalize(tangent);
   float3 b = normalize(bitangent);
   float3 n = normalize(normal);

   output.target0 = float4( pos.xyz, 1.0 );

   float3 N = 2*normalTexture.Sample( linearSampler, tex.xy) - float3(1, 1, 1);
   
   // output.target1 = float4( 0.5*n + float3(0.5, 0.5, 0.5), 1.0 );

   // output.target1 = float4( n, 1.0 );

   output.target1 = float4( N.x*t + N.y*b + N.z*n, 1.0 );

   output.target2 = colorTexture.Sample( linearSampler, tex.xy);

   return output;
}