#version 450
layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;
layout(set= 1,binding = 0) uniform sampler2D texSampler;
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 worldPos;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec4 outNormal;

vec3 lightPos = vec3(2.0f,6.0f,0.0f);

// struct Phong{
//     float ambient;
//     float diffuse;
//     float specular;
// };

// Phong phongShader(vec3 worldPos, vec3 cameraPos, vec3 lightPos, vec3 normal){
//     Phong phong;
//     phong.ambient = 0.2;
//     vec3 N = normal;
//     vec3 V = cameraPos- worldPos;
//     vec3 L = lightPos- worldPos;
//     phong.diffuse = 0.3*max(dot(normalize(V+L),N),0);
//     phong.specular = 1.4*pow(max(dot(normalize(V+L),N),0),64);
//     return phong;
// }

// float columFrog(vec3 worldPos, vec3 cameraPos){
//     vec3 samplePoint = cameraPos;
//     float res = 0.0f;
//     float step = 0.2;
//     for(int i = 0;i<100;++i){
//         if(length(samplePoint-cameraPos)>length(worldPos-cameraPos)) break;
//         //We can input camera's direction here, and move the sample point along
//         //-side the direction, then we can draw the ugly sky, without using skybox
//         //texture.
//         samplePoint+=step*normalize(worldPos-cameraPos);
//         if(samplePoint.y>=12&&samplePoint.y<=20){
//             res+=0.01f;
//         }
//     }
//     return res;
// }

void main() {
    // vec4 baseColor = texture(texSampler, fragTexCoord);
    // //vec4 baseColor = vec4(0.6,0.6,0.6,1.0);
    // Phong ans = phongShader(worldPos, ubo.cameraPos, lightPos, fragNormal);
    // outColor = vec4(baseColor.xyz*(ans.ambient+ans.diffuse+ans.specular)+vec3(0.0f,1.0f,0.0f)*columFrog(worldPos,ubo.cameraPos),1.0);
    // // outColor.x = outColor.x>0.3?1.0:outColor.x;
    // // outColor.y = outColor.y>0.3?1.0:outColor.y;
    // // outColor.z = outColor.z>0.3?1.0:outColor.z;

    // //outColor = vec4(worldPos,1.0);// worldPos visualization
    // //outColor = vec4(fragNormal*0.5+0.5,1.0); // normal vector visualization
    // //outColor = vec4(1.0,1.0,0.0,1.0);

    outPos = vec4(worldPos,1.0);
    outColor = vec4(texture(texSampler,fragTexCoord).xyz,1.0);
    outNormal = vec4(fragNormal, 1.0);
}