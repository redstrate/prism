layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outColor;

layout (binding = 2) uniform sampler2D colorSampler;

vec4 toSRGB(vec4 v) {
    return vec4(pow(v.rgb, vec3(2.2)), v.a);
}

void main() {
   if(inColor.a == 0.0)
       discard;
       
   if(textureSize(colorSampler, 0).x > 1)
       outColor = texture(colorSampler, inUV);
   else
       outColor = inColor;
       
   outColor = toSRGB(outColor);
}
