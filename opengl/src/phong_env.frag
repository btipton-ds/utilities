#extension ARB_texture_rectangle : enable                       
             
uniform float reflectionStrength;   //* 0.3     
uniform float directionalShininess; //* 3.0                         
uniform samplerCube    EnvMap;      //* Patterns\seaworld.dds              
                                                                
uniform float DepthPeel;                                        
uniform float BackFaceIntensity;                                
uniform sampler2DRect  org_depth;                               
uniform sampler2DRect  DepthMap;                                                                                                                                      
uniform float opacity; //* 1.0  
                                                                
varying vec3  ObjPos;                                           
varying vec3  Normal;                                           
varying vec4  Color;                                                                
                                                                
void main (void)                                                
{                                                             
    float currentDepth  = gl_FragCoord.z;                       
    float solidDepth    = texture2DRect( org_depth, gl_FragCoord.xy ).x;  
                                                                
    if(currentDepth > solidDepth)                               
        discard;                                                
                                                                
    if(DepthPeel > 0.0)                                         
    {                                                           
        float previousDepth = texture2DRect( DepthMap,  gl_FragCoord.xy ).x + 0.000001; //fudge for later NVidia drivers  
                                                                
        if( currentDepth < previousDepth   )                    
            discard;                                            
    }                                                           
                                                                
    vec4  ambientColor     = gl_FrontMaterial.ambient * gl_LightModel.ambient;          
    vec4  diffuseColor     = Color; //gl_FrontMaterial.diffuse;          
    vec4  specularColor    = gl_FrontMaterial.specular;         
    float specularStrength = gl_FrontMaterial.shininess;
                                                                        
                                                                
    vec3 N   = normalize( Normal );	                            
    vec3 V   = normalize(-ObjPos );	                            
    vec3 L1  = normalize(gl_LightSource[0].position.xyz);             
    vec3 L2  = normalize(gl_LightSource[1].position.xyz);                   
    vec3 R   = reflect  (-ObjPos, N ); //reflection vector in world space                     
                                                                
 	float lightIntensity1 = dot( N, L1);
 	float lightIntensity2 = dot( N, L2); 	
    float NdotZ = dot( N, vec3(0.0,0.0,1.0));                      

	float NdotHV1 = max(dot(N, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	float NdotHV2 = max(dot(N, normalize(gl_LightSource[1].halfVector.xyz)),0.0);
	                                                  
    float specularIntensity1  = pow( NdotHV1, specularStrength);  
    float specularIntensity2  = pow( NdotHV2, specularStrength);  
       
    specularColor =   specularColor * gl_LightSource[0].specular * specularIntensity1
				    + specularColor * gl_LightSource[1].specular * specularIntensity2;   
    clamp(specularColor, 0.0, 1.0);   
    specularColor.a   = 0.0;                                     
     
	R.y *= -1.0; 
	    
    float reflectionIntensity = NdotZ > 0.0 ? NdotZ : 0.0;
    reflectionIntensity = 1.0-pow( reflectionIntensity, directionalShininess); 
    
    // a lookup into the environment map.
    vec4 reflectionColor = textureCube(EnvMap,  R);            
    reflectionColor.rgb *= diffuseColor.rgb;
                                                                                     
    // Add lighting to base color and mix
    vec4 base;
    base.rgb =  ambientColor 
				+ lightIntensity1 * diffuseColor * gl_LightSource[0].diffuse 				   
				+ lightIntensity2 * diffuseColor * gl_LightSource[1].diffuse
				+ specularColor;
							 
    reflectionColor = mix(reflectionColor, base, reflectionIntensity); 
    base = mix(reflectionColor, base, 1.0-reflectionStrength);
    
	base.a = diffuseColor.a * opacity;	                                       
                                                                
    gl_FragColor = base;                                   
}  