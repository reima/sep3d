//--------------------------------------------------------------------------------------
// File: TerrainRenderer.fx
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
cbuffer cb0
{
  float    g_fTime;                   // App's time in seconds
  float4x4 g_mWorld;                  // World matrix for object
  float4x4 g_mWorldViewProjection;    // World * View * Projection matrix
  float3   g_vCamPos;                 // Camera position
}

cbuffer cb1
{
  bool     g_bDynamicMinMax;
  bool     g_bWaveNormals;
}

cbuffer cb2
{
  float   g_fMinHeight;
  float   g_fMaxHeight;
}

cbuffer cbStaticLightParams
{
  uint g_nPointLights;
  uint g_nDirectionalLights;
  uint g_nSpotLights;
  float3 g_vPointLight_Color[8];
  float3 g_vDirectionalLight_Color[8];
  float3 g_vSpotLight_Color[8];
  float2 g_fSpotLight_AngleExp[8];
  float4 g_vMaterialParameters; // = { k_a, k_d, k_s, n }
}

cbuffer cbLightsOnFrameMove
{
  float3 g_vPointLight_Position[8];
  float3 g_vDirectionalLight_Direction[8]; // Guaranteed to be normalized
  float3 g_vSpotLight_Position[8];
  float3 g_vSpotLight_Direction[8]; // Guaranteed to be normalized
}

cbuffer cbEnvironmentOnFrame
{
  float4x4 g_mWorldViewInv;
  float g_fCameraFOV;
}

cbuffer cbEnvironmentSeldom
{
  uint2 g_vBackBufferSize;
}

Texture2D g_tWaves;
Texture2D g_tGround;
Texture3D g_tGround3D;
TextureCube g_tCubeMap;

SamplerState g_ssLinear
{
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = Wrap;
  AddressV = Wrap;
  AddressW = Clamp;
};

#define NUM_SPOTS 8
const int g_nSpots = NUM_SPOTS;
const float g_Spots[NUM_SPOTS] = {
  -1.0,      // Tiefes Wasser
  -0.25,     // Seichtes Wasser
   0.0,      // Küste
   0.0625,   // Strand
   0.125,    // Gras
   0.375,    // Wald
   0.75,     // Gestein
   1.0       // Schnee
};

const float4 g_Colors[NUM_SPOTS] = {
  float4(0.0625, 0.0625, 0.025, 1),
  float4(0.125, 0.125, 0.05, 1),
  float4(0.5, 0.5, 0.1, 1),
  float4(0.9375, 0.9375, 0.25, 1),
  float4(0.125, 0.625, 0, 1),
  float4(0, 0.375, 0, 1),
  float4(0.5, 0.5, 0.5, 1),
  float4(1, 1, 1, 1)
};

const float3 vAttenuation = { 1, 0, 0 }; // Constant, linear, quadratic

const float4 g_vWaterColor = float4(0, 0.25, 0.5, 1.0);
const float4 g_vMaterMaterial = float4(0.05, 0.45, 0.5, 200);

//--------------------------------------------------------------------------------------
// Vertex shader input structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
  float3 Position   : POSITION;   // vertex position
  float3 Normal     : NORMAL;     // vertex normal
};

//--------------------------------------------------------------------------------------
// Vertex shader output structures
//--------------------------------------------------------------------------------------
struct VS_GOURAUD_SHADING_OUTPUT
{
  float4 Position       : SV_Position;
  float  Height         : HEIGHT;
  float3 DiffuseLight   : DIFFUSELIGHT;
  float3 SpecularLight  : SPECULARLIGHT;
};

struct VS_PHONG_SHADING_OUTPUT
{
  float4 Position       : SV_Position;
  float3 WorldPosition  : POSITION;
  float  Height         : HEIGHT;
  float3 Normal         : NORMAL0;
  float2 TexCoord       : TEXCOORD0;
  float2 Wave0          : TEXCOORD2;
  float2 Wave1          : TEXCOORD3;
  float2 Wave2          : TEXCOORD4;
  float2 Wave3          : TEXCOORD5;
  float4 Material       : MATERIAL; // = { k_a, k_d, k_s, n }
};

struct PHONG
{
  float3 DiffuseLight : DIFFUSE;
  float3 SpecularLight : SPECULAR;
};

//--------------------------------------------------------------------------------------
// Global functions
//--------------------------------------------------------------------------------------
float4 GetColorFromHeight(float height)
{
  if (g_bDynamicMinMax) {
    height = (height - g_fMinHeight) / (g_fMaxHeight - g_fMinHeight) *
      (g_Spots[g_nSpots - 1] - g_Spots[0]) + g_Spots[0];
  } else {
    height = clamp(height, g_Spots[0], g_Spots[g_nSpots - 1]);
  }
  float4 color = float4(0, 0, 0, 1);
  for (int i = 0; i < g_nSpots - 1; ++i) {
    if (height <= g_Spots[i+1]) {
      color = lerp(g_Colors[i], g_Colors[i+1],
                   (height - g_Spots[i]) / (g_Spots[i+1] - g_Spots[i]));
      break;
    }
  }
  return color;
}

float2 Phong(float3 vPos, float3 vLightDir, float3 vNormal, float4 vMaterial)
{
  // Diffuse
  float3 L = normalize(vLightDir);
  float3 N = normalize(vNormal);
  float NdotL = saturate(dot(N, L));
  float fDiffuse = NdotL * vMaterial.y;

  // Specular
  float3 R = normalize(reflect(-L, N));
  float3 V = normalize(g_vCamPos - vPos);
  float RdotV = saturate(dot(R, V));
  float fSpecular = pow(RdotV, vMaterial.w) * vMaterial.z;

  return float2(fDiffuse, fSpecular);
}

PHONG PhongLighting(float3 vPos, float3 vNormal, float4 vMaterial)
{
  float3 vDiffuseLight = float3(0, 0, 0);
  float3 vSpecularLight = float3(0, 0, 0);
  // Point lights
  uint i;
  for (i = 0; i < g_nPointLights; i++) {
    float d = length(vPos - g_vPointLight_Position[i]);
    float fAttenuation = 1 / dot(vAttenuation, float3(1, d, d*d));
    float2 vPhong = Phong(vPos, g_vPointLight_Position[i] - vPos, vNormal,
                          vMaterial);
    vDiffuseLight += vPhong.x * g_vPointLight_Color[i] * fAttenuation;
    vSpecularLight += vPhong.y * g_vPointLight_Color[i] * fAttenuation;
  }

  // Directional lights
  for (i = 0; i < g_nDirectionalLights; i++) {
    float2 vPhong = Phong(vPos, g_vDirectionalLight_Direction[i], vNormal,
                          vMaterial);
    vDiffuseLight += vPhong.x * g_vDirectionalLight_Color[i];
    vSpecularLight += vPhong.y * g_vDirectionalLight_Color[i];
  }

  // Spot lights
  for (i = 0; i < g_nSpotLights; i++) {
    float d = length(vPos - g_vSpotLight_Position[i]);
    float3 I = (g_vSpotLight_Position[i] - vPos) / d;
    float IdotL = dot(-I, g_vSpotLight_Direction[i]);
    float theta = g_fSpotLight_AngleExp[i].x;
    float angle = acos(IdotL);
    if (angle < theta) {
      float fAttenuation = 1 / dot(vAttenuation, float3(1, d, d*d));
      fAttenuation *= 1 - pow(angle/theta, g_fSpotLight_AngleExp[i].y);
      float2 vPhong = Phong(vPos, g_vSpotLight_Position[i] - vPos, vNormal,
                            vMaterial);
      vDiffuseLight += vPhong.x * g_vSpotLight_Color[i] * fAttenuation;
      vSpecularLight += vPhong.y * g_vSpotLight_Color[i] * fAttenuation; 
    }
  }

  PHONG ret;
  ret.DiffuseLight = vDiffuseLight;
  ret.SpecularLight = vSpecularLight;
  return ret;
}

//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------
VS_GOURAUD_SHADING_OUTPUT GouraudShading_VS( VS_INPUT In )
{
  VS_GOURAUD_SHADING_OUTPUT Output;
  float4 vPos = float4(In.Position, 1);
  Output.Height = vPos.y;
  if (vPos.y < 0) {
    vPos.y = 0;
    In.Normal = float3(0, 1, 0);
  }
  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(vPos, g_mWorldViewProjection);
  PHONG phong = PhongLighting(In.Position, normalize(In.Normal),
                              g_vMaterialParameters);
  Output.DiffuseLight = phong.DiffuseLight;
  Output.SpecularLight = phong.SpecularLight;

  return Output;
}

VS_PHONG_SHADING_OUTPUT PhongShading_VS( VS_INPUT In )
{
  VS_PHONG_SHADING_OUTPUT Output;
  float4 vPos = float4(In.Position, 1);
  Output.Height = vPos.y;
  if (vPos.y < 0) {
    vPos.y = 0;
    In.Normal = float3(0, 1, 0);
    Output.Material = g_vMaterMaterial;
  } else {
    Output.Material = g_vMaterialParameters;
  }
  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(vPos, g_mWorldViewProjection);
  Output.Normal = normalize(In.Normal);
  Output.WorldPosition = vPos;
  Output.TexCoord = vPos.xz * 2;

  if (g_bWaveNormals) {
    float2 vTexCoord = vPos.xz * 0.5;
    float2 vTrans = g_fTime * 0.01;
    Output.Wave0 = vTexCoord * 1 + vTrans * 2;
    Output.Wave1 = vTexCoord * 2 - vTrans * 4;
    Output.Wave2 = vTexCoord * 4 + vTrans * 2;
    Output.Wave3 = vTexCoord * 8 - vTrans * 1;
  }

  return Output;
}

float4 Environment_VS( VS_INPUT In ) : SV_Position
{
  return float4(In.Position, 1);
}

//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------
float4 GouraudShading_PS( VS_GOURAUD_SHADING_OUTPUT In ) : SV_Target
{
  float4 vTerrainColor = GetColorFromHeight(In.Height);
  return vTerrainColor * g_vMaterialParameters.x +
    vTerrainColor * float4(In.DiffuseLight, 1) +
    float4(In.SpecularLight, 1);
}

float4 PhongShading_PS( VS_PHONG_SHADING_OUTPUT In ) : SV_Target
{
  float3 N = 0;
  float4 vReflect = 0;
  float4 vTerrainColor = 0;
  if (In.Height <= 0) {
    if (g_bWaveNormals) {
      // Waves bump map lookups
      float3 vWave0 = g_tWaves.Sample(g_ssLinear, In.Wave0);
      float3 vWave1 = g_tWaves.Sample(g_ssLinear, In.Wave1);
      float3 vWave2 = g_tWaves.Sample(g_ssLinear, In.Wave2);
      float3 vWave3 = g_tWaves.Sample(g_ssLinear, In.Wave3);
      // Combine waves to get normal pertubation vector
      float3 vNormalPertubation = normalize(vWave0 + vWave1 +
                                            vWave2 + vWave3 - 2);
      vNormalPertubation = vNormalPertubation.xzy;
      N = normalize(float3(0, 1, 0) + vNormalPertubation);
    } else {
      N = float3(0, 1, 0);
    }
    float3 I = normalize(In.WorldPosition - g_vCamPos);
    float3 R = reflect(I, N);
    vTerrainColor = g_vWaterColor;
    vReflect = g_tCubeMap.Sample(g_ssLinear, R);
  } else {
    //float4 vTerrainColor = GetColorFromHeight(In.Height);
    //float4 vTerrainColor = g_tGround.Sample(g_ssLinear, In.TexCoord);
    //float4 vTerrainColor = g_tGround.SampleLevel(g_ssLinear, In.TexCoord, 0);
    float fHeightNormalized = In.Height / g_fMaxHeight + 0.1;
    vTerrainColor = g_tGround3D.SampleLevel(g_ssLinear,
                                            float3(In.TexCoord,
                                                   fHeightNormalized),
                                            0);
    N = normalize(In.Normal);
  }
  
  PHONG phong = PhongLighting(In.WorldPosition, N, In.Material);
  return vTerrainColor * In.Material.x +
    vTerrainColor * float4(phong.DiffuseLight, 1) +
    float4(phong.SpecularLight, 1) + In.Material.z * vReflect;
}

float4 Environment_PS( float4 vPos : SV_Position ) : SV_Target
{
  // Transform vPos.xy in NDC
  vPos.xy /= float2(g_vBackBufferSize.x - 1, g_vBackBufferSize.y - 1);
  vPos.x = 2*vPos.x-1;
  vPos.y = 1-2*vPos.y;
  float fRatio = g_vBackBufferSize.y / (float)g_vBackBufferSize.x;
  // Calculate view vector in view space
  float3 vView = float3(fRatio * tan(0.5f * g_fCameraFOV) * vPos.x,
                        tan(0.5f * g_fCameraFOV) * vPos.y,
                        1.0);
  // Transform view vector in world space
  vView = mul(vView, g_mWorldViewInv);
  return g_tCubeMap.Sample(g_ssLinear, vView);
}


BlendState SrcColorBlendingAdd
{
  BlendEnable[0] = TRUE;
  BlendOp = ADD;
  SrcBlend = SRC_ALPHA;
  DestBlend = INV_SRC_ALPHA;
};

//--------------------------------------------------------------------------------------
// Renders scene
//--------------------------------------------------------------------------------------
technique10 GouraudShading
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, GouraudShading_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, GouraudShading_PS() ) );
  }
}

technique10 PhongShading
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, PhongShading_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, PhongShading_PS() ) );
  }
}

technique10 Environment
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Environment_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, Environment_PS() ) );
  }
}
