#version 130
out vec3  Normal;
out vec3  ObjPos;
out vec4  Color;

void main(void) 
{
    gl_Position    = ftransform();
    Color	   = gl_Color;
    Normal         = normalize(gl_NormalMatrix * gl_Normal);
    ObjPos         = (gl_ModelViewMatrix * gl_Vertex).xyz;  
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
}
  
/*
VertToFrag2 main(	float4 position,
          			float3 tangent,
          			float3 biNormal,
          			float4 texUV,
                    float  s_reflectionscale)
{
	float4x4 ModelViewProj = glstate.matrix.mvp;
    float4x4 ModelView     = glstate.matrix.modelview[0];
    float4x4 ModelViewI    = glstate.matrix.inverse.modelview[0];
    float4x4 ModelViewIT   = glstate.matrix.invtrans.modelview[0];
	float4x4 textureMatrix = glstate.matrix.texture[1];

	VertToFrag2 OUT;

	float4x4 viewInv     = glstate.matrix.program[0];
	float3 viewTranslate = viewInv[3].xyz;
	
	//Vertex position in World space
    float4 viewPos  = float4( mul(ModelView, position).xyz, 1.0 );
    float4 worldPos = mul(viewPos, viewInv );

	float4 lightVecmv = float4(glstate.light[0].position.xyz,1);
    float4 worldLightPos = mul(lightVecmv, viewInv );

	//World space view vector relative to vertex
	//Vector from vertex to eye, normalized
	float3 tempvec = viewTranslate - worldPos.xyz;
	float3 vertToEye = normalize(tempvec);

	//World space light vector
	//Vector from vertex to light, normalized
	float3 lightVec = worldLightPos.xyz - worldPos.xyz;
	lightVec = normalize(lightVec);
	
	float3 halfAngle = normalize(vertToEye + lightVec);

	// Tangent, Normal, Binormal in View space
	float3 tangentmv    = normalize( mul( (float3x3)ModelViewIT, tangent)).xyz;   
	float3 biNormalmv   = normalize( mul( (float3x3)ModelViewIT, biNormal)).xyz;  
	float3   normalmv   = cross(biNormalmv, tangentmv);

	//World space normal
	float3 norm = mul( (float3x3)viewInv,  normalmv);
	float3 worldNormal = normalize(norm);

	//Homogeneous position
    OUT.HPosition   = mul(ModelViewProj, position);

	OUT.TexCoord    = texUV.xy;

	OUT.HalfAngle   = halfAngle;
	OUT.LightVec	= lightVec;
	OUT.WorldNormal = worldNormal;
	OUT.WorldView	= vertToEye;
    
    OUT.WorldView.xy *= s_reflectionscale;

	//Multiply tex coords by texture matrix
    //Out.TexCoord0 = float4( mul( (float3x3)textureMatrix, float3(texUV,1.0) ).xy, 0.0, 1.0 );

    return OUT;
}
*/  