// SamplerState Sampler : register(s2);
// Texture2D Albedo : register(t3);
// Texture2D Normal : register(t4);
//
// #define MAX_LIGHTS 50
//
// struct PointLight
// {
//     float3 Position;
//     float Padding1;
//     // 16 bytes boundary 
//     float4 Color;
//     // 16 bytes boundary 
//     float ConstantAttenuation;
//     float LinearAttenuation;
//     float QuadraticAttenuation;
//     float Padding3;
//     // 16 bytes boundary
//     bool Enabled;
//     float3 Padding4;
//     // 16 bytes boundary
// }; // size 48 bytes (16 * 3)
//
// cbuffer LightsCbuf : register(b5)
// {
//     PointLight PointLights[MAX_LIGHTS];
// };
//
// struct PixelIn
// {
//     float4 Position : SV_POSITION;
//     float3 PositionWS : TEXCOORD0;
//     float3 normal : NORMAL;
//     float2 uv : TEXCOORD1;
//     float time : TEXCOORD2;
//     bool HasAlbedo : TEXCOORD3;
//     bool HasNormalMap : TEXCOORD4;
//     float3 CameraPosition : TEXCOORD5;
//     int Mode : TEXCOORD6;
//     float3x3 tbn : TEXCOORD7;
// };
//
// float3 DoDiffuse(float4 lightColor, float3 lightVector, float3 normal)
// {
//     return max(0.0f, dot(lightVector, normal)) * lightColor.xyz;
// }
//
// float3 DoSpecular(float4 lightColor, float3 lightVector, float3 normal, float3 view, float3 inputNormal)
// {
//     float3 reflectedLight = normalize(reflect(-lightVector, normal));
//     float amountSpecularLight = 0;
//     if(dot(lightVector, inputNormal) > 0) // we use vertex normal instead of normal one
//         amountSpecularLight = pow(max(0.0f, dot(reflectedLight, view)), 10.0f /* Shininess */);
//
//     return amountSpecularLight * lightColor.xyz;
// }
//
// float DoAttenuation(PointLight light, float distance)
// {
//     return 1.0f / (light.ConstantAttenuation + light.LinearAttenuation * distance + light.QuadraticAttenuation * distance * distance);
// }
//
// float4 Main(PixelIn Input) : SV_TARGET
// {
//     float4 lightColor = float4(1.0, 1.0, 1.0, 1.0);
//     float3 dirLightVec = normalize(float3(1.0, 1.0, 0.0)); // l (norm vec pointing toward light direction)
//     float4 albedo = float4(1.0, 1.0, 1.0, 1.0);
//     float3 normal = Input.normal;
//
//     if(Input.HasAlbedo)
//         albedo = Albedo.Sample(Sampler, Input.uv);
//
//     if(Input.HasNormalMap)
//     {
//         float4 normalSampled = Normal.Sample(Sampler, Input.uv);
//         normalSampled.xyz = (normalSampled.xyz * 2.0) - 1.0;
//         normalSampled.xyz = mul(normalSampled.xyz, Input.tbn);
//         normal = normalSampled.xyz;
//     }
//     
//     normal = normalize(normal);
//
//     // basic Phong lighting for directional
//     float3 ambiant = 0.1f * albedo.xyz;
//     float3 dirDiffuse = DoDiffuse(lightColor, dirLightVec, normal);
//     
//     float3 view = normalize(Input.Position.xyz - Input.CameraPosition);
//     float3 dirSpecular = DoSpecular(lightColor, dirLightVec, normal, view, Input.normal);
//
//     float3 dirLight = dirDiffuse + dirSpecular + ambiant;
//     float3 lightSum = float3(0.0, 0.0, 0.0);
//
//     // Point Lights
//     for(int i = 0; i < MAX_LIGHTS; i++)
//     {
//         PointLight light = PointLights[i];
//         
//         if(!light.Enabled)
//             continue;
//         
//         float3 l = (light.Position - Input.PositionWS);
//         float3 lightVector = normalize(l);
//         float distance = length(l);
//         
//         float attenuation = DoAttenuation(light, distance);
//         float3 diffuse = DoDiffuse(light.Color, lightVector, normal) * attenuation;
//         float3 specular = DoSpecular(light.Color, lightVector, normal, view, Input.normal) * attenuation;
//
//         lightSum += diffuse + specular;
//     }
//
//     float3 finalLight = dirLight + lightSum;
//
//     switch (Input.Mode)
//     {
//     case 0:
//         return albedo * float4(finalLight, 0.5);
//     case 1:
//         return albedo * float4(dirDiffuse, 1.0);
//     case 2:
//         return albedo * float4(dirSpecular, 1.0);
//     case 3:
//         return albedo;
//     case 4:
//         return float4(normal, 1.0f);
//     case 5:
//         return albedo * float4(lightSum, 1.0); // Debug
//     default:
//         return albedo * float4(finalLight, 1.0);
//     }
//     
//     // safety Yellow
//     // return float4(255.0f, 240.0f, 0.0f, 1.0f);
// }