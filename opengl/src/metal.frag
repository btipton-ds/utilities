#version 130
#extension GL_ARB_texture_rectangle : enable

uniform float reflectionStrength;   //* 1.0
uniform float directionalShininess; //* 3.0
uniform samplerCube    EnvMap;      //* Patterns\seaworld.dds

uniform float DepthPeel;
uniform sampler2DRect  DepthMap;
uniform int useTex0;
uniform sampler2D tex0;
uniform float opacity; //* 1.0
uniform float useBackFaceColor; //* 0.0
uniform  vec4 BackFaceColor;
uniform float lightOn; //* 1.0
uniform float specularScale; 

in vec3  ObjPos;
in vec3  Normal;
in vec4  Color;
in vec2 TexCoord;

/* 
//If we have subroutine support
subroutine vec4 cololModelType();
subroutine uniform colorModelType colorModel;

subroutine (colorModelType)
vec3 uniformColor()
{
	return Color;
}

subroutine (colorModelType)
vec3 texColor()
{
	return texture(tex0, gl_TexCoord[0].st);
}
*/

void main (void)
{                                                             
  	if(DepthPeel > 0.0)
  	{
  		float depth = texture2DRect( DepthMap, gl_FragCoord.xy ).x;
  		if( depth + 0.000001 > gl_FragCoord.z) 	
  			discard;		 
  	}                                                          

    vec3 N   = normalize( Normal );	                            
    float NdotZ = dot( N, vec3(0.0,0.0,1.0));
    
    vec4 diffuseColor, ambientColor, specularColor;
    float specularStrength;

/*	
	To avoid 'if' statements we compute ff and bf.
	If a face is front facing ff = 1.0 and bf = 0.0
	If a face is back facing ff = 0.0 and bf = 1.0 
	If the face is exactly edge on ff == bf == 0.5 and we get a bogus
	value, but since the face is edge on we will not see it.
*/
	float ff = 0.5 * (sign(NdotZ) + 1.0);
	float bf = 1.0 - ff;

	// The texture lookup is being multiplied by 'Color' in order to pickup the green face hilite color.
	// When a texture is used, 'Color' is either white or green (hilite color). Multiplying by white is a no op.
	vec4 ffc = (1.0 - useTex0) * Color + useTex0 * texture(tex0, gl_TexCoord[0].st) * Color;
	diffuseColor     = ff * ffc						   + bf * gl_BackMaterial.diffuse;
	specularColor    = ff * gl_FrontMaterial.specular  + bf * gl_BackMaterial.specular;         
	specularStrength = ff * gl_FrontMaterial.shininess + bf * gl_BackMaterial.shininess;
	specularStrength *= specularScale;
	N				 = ff * N						   + bf * -N;
	NdotZ			 = ff * NdotZ					   + bf * -NdotZ;

	ambientColor     = diffuseColor * gl_LightModel.ambient; // we don't have ambient material, just ambient lighting ;)

    vec3 V   = normalize(-ObjPos );	                            
    vec3 L1  = normalize(gl_LightSource[0].position.xyz);             
    vec3 L2  = normalize(gl_LightSource[1].position.xyz);                   
    vec3 R   = reflect  (-ObjPos, N ); //reflection vector in world space                     
                                                                 	 	                  
	float NdotHV1 = max(dot(N, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	float NdotHV2 = max(dot(N, normalize(gl_LightSource[1].halfVector.xyz)),0.0);
	                                                  
    float specularIntensity1  = pow( NdotHV1, specularStrength);  
    float specularIntensity2  = pow( NdotHV2, specularStrength);  
       
    specularColor =   specularColor * gl_LightSource[0].specular * specularIntensity1
				    + specularColor * gl_LightSource[1].specular * specularIntensity2;   
    clamp(specularColor, 0.0, 1.0);   
    specularColor.a   = 0.0;                                           
	    
	// reflection    
    float reflectionIntensity = NdotZ > 0.0 ? NdotZ : 0.0;
    reflectionIntensity = 1.0-pow( reflectionIntensity, directionalShininess); 
    
    // a lookup into the reflection map.
	R.y *= -1.0;    
    vec4 reflectionColor = textureCube(EnvMap,  R);            
    reflectionColor.rgb *= diffuseColor.rgb;
         
    // lighting     
 	float frontLightIntensity1 = max( dot( N, L1), 0.0 );
 	float frontLightIntensity2 = max( dot( N, L2), 0.0 ); 	
                                                                                     
    // Add lighting to base color and mix
    vec4 base;
    base =  ambientColor 
			+ frontLightIntensity1 * diffuseColor  * gl_LightSource[0].diffuse
			+ frontLightIntensity2 * diffuseColor  * gl_LightSource[1].diffuse
			+ specularColor;
						 
    reflectionColor = mix(reflectionColor, base, reflectionIntensity); 
    base = mix(reflectionColor, base, 1.0-reflectionStrength);
    
	base.a = diffuseColor.a * opacity;	                                       
                                                               
	gl_FragColor = lightOn * base + (1 - lightOn) * Color;                                              
}  