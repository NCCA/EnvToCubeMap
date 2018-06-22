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
  if(mode==0)
  {
    vec2 uv = sampleSphericalMap(normalize(localPos)); // make sure to normalize localPos
    fragColor = texture(equirectangularMap, uv);
  }

  else
  {
    fragColor=texture(irradiance,normalize(localPos));

  }
}

