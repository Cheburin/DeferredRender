float	atten ( float d )								// compute distance attenuation factor, 0 -> 1, MAX_DIST -> 0
{
	const	float	offs0 = 1.0;
	const	float	offs1 = 1.0 / (1.0 + 2.25);
	const	float	scale = 1.0 / (offs0 - offs1);
	
	return scale * ( 1.0 / (1.0 + d) - offs1 );
}

float4 PS(in float2 tex : TEXCOORD0):SV_TARGET
{ 
   float3 lightPos = mul( float4( 2, 3, 0.5, 1.0 ), g_mView ).xyz;
   //float3    lightPos = float3(0.707106829, -1.00000000, 4.24264050); //0.0, 0.0, 0.0);//0.707106829, -1.00000000, 4.24264050

   float2  ndc = float2(2*tex.x,-2*tex.y) + float2(-1,1);

   float     z = positionTexture.Sample( linearSampler, tex.xy).z;
   float     e = g_vScreenResolution.z;
   float     a = g_vScreenResolution.w;

   float3    p  = z * float3(ndc.x*(a/e), ndc.y*(1/e), 1);
   float3    n  = normalTexture.Sample( linearSampler, tex.xy).xyz;//float3(0,-1,0);//
   //n = mul( float4( n, 0.0 ), g_mView ).xyz;
   float3    c  = colorTexture.Sample( linearSampler, tex.xy).xyz;

   if(length(n)==.0)
     return float4(c, 0.0);

	 float3	ll = lightPos - p;
	 float	d  = length ( ll );

	 if ( d > 2.25 )									// too far away
	   discard;

   float	k  = atten ( d );

   float3    l  = normalize     ( ll );
   float3    v  = normalize     ( - p );
   float3    h  = normalize     ( l + v );
   float   diff = max           ( 0.09, dot ( l, n ) );
   float   spec = pow         ( max ( 0.0, dot ( h, n ) ), 40.0 );
    
   return 5 * k * float4( float3 ( diff*c + spec*float3(1,1,1) ), 1.0 );    

   //return normalTexture.Sample( linearSampler, tex.xy);
}

float4 AMBIENT_PS(in float2 tex : TEXCOORD0):SV_TARGET
{ 
   float3    n  = normalTexture.Sample( linearSampler, tex.xy).xyz;
   float3    c  = colorTexture.Sample( linearSampler, tex.xy).xyz;

   if(length(n)==.0)
     return float4(c, 1.0);

   return float4( 0.09 * c, 1.0 );    
}
