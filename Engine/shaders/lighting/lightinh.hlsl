#include "lighting.h"


/*
    point light rendering
*/
PS_Output_Simple PS_Point_Light(PS_Input_Simple ps_input)
{
    PS_Output_Simple output = (PS_Output_Simple)0;
    // sample gbuffer
    GBuffer gbuffer = GetGBuffer(ps_input.TexCoord);
    // shadow value
    float shadow_value = shadow_value(gbuffer);
    // lighting color
    float3 lighting_color = deferred_lighting(gbuffer); 
    // adjust lighting by distance and light intensity
    float radius = gRadiusIntensity.x;
    float intensity = gRadiusIntensity.y;
    // pixel postion
    float3 position = gbuffer.Position;
    // get vectors
    float4 normal = gbuffer.Normal;
    // get L, V, vectors
    float3 L = gLightPosition - position.xyz;
    L = normalize(L);
    // distance from light
    float d = distance(gLightPosition, position.xyz);
    // an value
    float3 an = gLightColor * intensity * saturate(1 - d / radius);
    // final value
    output.Color = float4(lighting_color * an * saturate(dot(normal, L)), 0) * shadow_value;
    return output;
}


/*
    direction lighing
*/
PS_Output_Simple PS_Direction_Light(PS_Input_Simple ps_input)
{
    PS_Output_Simple output = (PS_Output_Simple)0;
    GBuffer gbuffer = GetGBuffer(ps_input.TexCoord);
    // get vectors
    float4 normal = gbuffer.Normal;
    float3 position = gbuffer.Position;
    // get light view space texcoord
    float intensity = gRadiusIntensity.y;
    float3 L = -gLightDirection;
    L = normalize(L);
    // deferred lighting
    float3 lighting_color = deferred_lighting(gbuffer);
    // final color
    output.Color = float4(intensity * lighting_color * gLightColor * saturate(dot(normal, L)), 0)
    return output;
}

PS_Output_Simple PS_ImageBasedLight(PS_Input_Simple ps_input)
{
    PS_Output_Simple output = (PS_Output_Simple)0;
    // get gbuffer
    GBuffer gbuffer = GetGBuffer(ps_input.TexCoord);
    // get normal and position
    float4 N = gbuffer.Normal;
    float3 position = gbuffer.Position;

    if (length(position) < 0.001) {
        // draw the light probe
        float4 look = GetLookVector(input.TexCoord);
        look = mul(look, gInvertViewMaxtrix);
        output.Light = gLightProbe.Sample(gSam, look.xyz);
        return output;
    }
    float3 V = -position.xyz;
    V = normalize(V);
    float NoV = saturate(dot(N, V));

    // get params
    float4 rm = gSpecularBuffer.Sample(gSam, input.TexCoord);
    float3 F0 = float3(rm.x, rm.x, rm.x);
    float metallic = gbuffer.Metallic;
    float roughness = gbuffer.Roughness;

    float4 albedo = gbuffer.Diffuse;
    float4 WorldNormal = mul(float4(N.xyz, 0), gInvertViewMaxtrix);
    float3 irradiance = gLightProbeIrradiance.Sample(gSam, WorldNormal).rgb;

    // IBL Specular
    float3 SpecularColor = gbuffer.Specular;
    float3 kS = FresnelSchlickRoughness(SpecularColor, roughness, NoV);
    float3 specular = SpecularIBL(SpecularColor, roughness, N, V);


    // IBL Diffuse
    float3 diffuse = irradiance * albedo.xyz;
    float3 kD = 1 - kS;
    kD = lerp(kD, 0, metallic);

    float intensity = gRadiusIntensity.y;
    float4 color;
    color.rgb = (kD * diffuse + specular) * intensity;
    color.a = 1;
    output.Color = color;
    return output;
}
   