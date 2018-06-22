#version 330 core
layout(location=0) out vec4 fragColor;
in vec3 localPos;

uniform sampler2D equirectangularMap;
uniform samplerCube irradiance;
const float PI = 3.14159265359;
uniform int mode=0;
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 sampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}



void main()
{
    vec2 uv = sampleSphericalMap(normalize(localPos)); // make sure to normalize localPos
//    vec3 color = vec3(0);
     if(mode==0)
      fragColor = vec4(texture(equirectangularMap, uv).rgb, 1.0);
    else
      fragColor=vec4(texture(irradiance,localPos).rgb, 1.0);
}

