#extension ARB_texture_rectangle : enable                       
                                                                                                                             
uniform float DepthPeel;                                                                       
uniform sampler2DRect  org_depth;                               
uniform sampler2DRect  DepthMap; 
  
uniform sampler2DRect  DistanceMap1;                                                                                                                                 
uniform float opacity; //* 1.0  
   
varying vec3  RawObjPos;                                                                
varying vec3  ObjPos;                                           
varying vec3  Normal;                                           
varying vec4  Color;   
                                        
uniform float distRange1;                                       
uniform float distRange2;                                       
uniform float distRange3;                                       
uniform float distRange4;                                       
                                                                
uniform vec4  distColor1;                                       
uniform vec4  distColor2;                                       
uniform vec4  distColor3;                                       
uniform vec4  distColor4;                                       

uniform vec4  distanceMapDim; 
uniform float distanceMap1ToMM;	
uniform float distanceMap1MMOffset;	//* 0.0 	 
uniform mat4  distanceMap1Matrix;                                                                                                                                                                  
                                                                
void main (void)                                                
{                                                                                                                                                                          
    if(DepthPeel > 0.0)                                         
    {                                                           
        float currentDepth  = gl_FragCoord.z;                       
		float solidDepth    = texture2DRect( org_depth, gl_FragCoord.xy ).x;  
	                                                                
		if(currentDepth > solidDepth)                               
			discard;   
        
        float previousDepth = texture2DRect( DepthMap,  gl_FragCoord.xy ).x + 0.000001; //fudge for later NVidia drivers  
                                                                
        if( currentDepth < previousDepth   )                    
            discard;                                            
    }  
                                                            
    vec4  ambientColor     = Color * gl_LightModel.ambient;  // BZ 8286 patch clay colors        
    vec4  diffuseColor     = Color; //gl_FrontMaterial.diffuse;          
    vec4  specularColor    = gl_FrontMaterial.specular;         
    float specularStrength = gl_FrontMaterial.shininess;
                                                                
    vec3 lightPos = gl_LightSource[0].position.xyz;             
                                                                
    vec3 N   = normalize( Normal );	                            
    vec3 V   = normalize(-ObjPos );	                                               
    vec3 L1  = normalize(gl_LightSource[0].position.xyz);             
    vec3 L2  = normalize(gl_LightSource[1].position.xyz);  
                                                                          
    float NdotZ = dot( N, vec3(0.0,0.0,1.0)); 
	float NdotHV1 = max(dot(N, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	float NdotHV2 = max(dot(N, normalize(gl_LightSource[1].halfVector.xyz)),0.0);
	                                                  
    float specularIntensity1  = pow( NdotHV1, specularStrength);  
    float specularIntensity2  = pow( NdotHV2, specularStrength);  
       
    specularColor =   specularColor * gl_LightSource[0].specular * specularIntensity1
				    + specularColor * gl_LightSource[1].specular * specularIntensity2;   
    clamp(specularColor, 0.0, 1.0);   
    specularColor.a   = 0.0;  
                                       
 	float lightIntensity1 = abs( dot( N, L1) );
 	float lightIntensity2 = abs( dot( N, L2) ); 	                               
                            
    vec4 LCol0 = lightIntensity1 * gl_LightSource[0].diffuse;           
    vec4 LCol1 = lightIntensity2 * gl_LightSource[1].diffuse;                              
                                                                      
    vec4 base = vec4(0.0,0.0,0.0,1.0);	                        
    if( NdotZ < 0.0 ) //back face                               
    {                                                           
        //light back faces by half                                                                                     
        base =  ambientColor 
				+ diffuseColor * LCol0 				   
				+ diffuseColor * LCol1
				+ specularColor;      								                               
    }                                                           
    else  //front face                                          
    {                                                                                                                       
        // Add lighting to base color and mix                   
        base =    ambientColor 
				+ diffuseColor * LCol0 				   
			    + diffuseColor * LCol1;
				+ specularColor;                                                      
                                                                
        // Transform the current pixel into the depth compare coordinate system   
        vec3 lookup = (distanceMap1Matrix * vec4(RawObjPos,1.0)).xyz;    
        lookup.x = distanceMapDim.x + distanceMapDim.z*(lookup.x+1.0)/2.0;
		lookup.y = distanceMapDim.y + distanceMapDim.w*(lookup.y+1.0)/2.0; 
        lookup.z = (lookup.z+1.0)/2.0;
         
        // Retrieve the depth difference in mm              
        float biteDepth = texture2DRect( DistanceMap1, lookup.xy ).x;      
        float surfDist  = (biteDepth - lookup.z ) * distanceMap1ToMM + distanceMap1MMOffset; 
                                          
        vec4 color1 = distColor1 * LCol0 + distColor1 * LCol1;                                    
        vec4 color2 = distColor2 * LCol0 + distColor2 * LCol1; 
        vec4 color3 = distColor3 * LCol0 + distColor3 * LCol1;   
        vec4 color4 = distColor4 * LCol0 + distColor4 * LCol1;               
                       
        if( surfDist >= distRange4 || surfDist < -10.0 || // assuming -10 is the same as no depth to compare to             
			biteDepth >= 1.0 || biteDepth <= 0.0 )        // this means the depth is 'out of range'
        {                                                        
            specularStrength = abs( pow( lightIntensity1, specularStrength) );  
            specularColor    = clamp(specularColor * specularStrength, 0.0, 1.0);   
            specularColor.a  = 0.0;                              
                                                                 
            base += specularColor;                               
        }                                                                                                               
        else if( surfDist < distRange1 )                         
            base = mix( color1, color2, surfDist / distRange1 );  
                                                                 
        else if ( surfDist < distRange2 )                        
            base = mix( color2, color3, (surfDist - distRange1) / (distRange2 - distRange1) );   
                                                                 
        else if ( surfDist < distRange3 )                        
            base = mix( color3, color4, (surfDist - distRange2) / (distRange3 - distRange2) );  
                                                                 
        else if ( surfDist < distRange4 )                        
            base = mix( color4, base,   (surfDist - distRange3) / (distRange4 - distRange3) );                                                                                    
    }  

	base.w = opacity;                                                                                                  
    gl_FragColor = base;                                   
}  