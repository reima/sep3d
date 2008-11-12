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
  bool     g_bDirectionalLight;
  float3   g_vLightPos;               // Light position
  float3   g_vLightColor;             // Light color
  float    g_fWaveHeight;
  float2   g_vWaveScale;
  float2   g_vWaveSpeed;
  float3   g_vWaterColor;
  int      g_iPhongExp;
  float    g_fFresnelBias;
  int      g_iFresnelExp;
}

cbuffer cb2
{
  float   g_fMinHeight;
  float   g_fMaxHeight;
}

cbuffer cbStaticLightParams
{
  uint g_iNumOfPL;
  float4 g_vPointlight_Color[16]; //Lightcolor
}

cbuffer cbLightsOnFrameMove
{
  float3 g_vPointlight_Position[16];//
}

Texture2D g_tWaves;
SamplerState g_ssWaves
{
	Filter = MIN_MAG_MIP_LINEAR;
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


const float k_a = 0.1;
const float k_d = 0.5;
const float k_s = 0.4;
const float k_n = 2;

const float PointLightA = 1;
const float PointLightB = 0;
const float PointLightC = 0;


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
// VSC (Vertex Shader Coloring)
struct VS_VSC_OUTPUT
{
  float4 Position   : SV_Position;   // vertex position
  float4 Color      : COLOR;
};

// PSC (Pixel Shader Coloring)
struct VS_PSC_OUTPUT
{
  float4 Position   : SV_Position;   // vertex position
  float  Height     : HEIGHT;
};

struct VS_SFX_P0_OUTPUT
{
  float4 Position   : SV_Position;
  float3 Normal     : NORMAL0;
  float  Height     : TEXCOORD0;
  float3 LightDir   : TEXCOORD1;
};

struct VS_SFX_P1_OUTPUT
{
  float4 Position   : SV_Position;
  float3 ViewDir    : TEXCOORD0; // In tangent space
  float3 LightDir   : TEXCOORD1; // In tangent space
  float2 Wave0      : TEXCOORD2;
  float2 Wave1      : TEXCOORD3;
  float2 Wave2      : TEXCOORD4;
  float2 Wave3      : TEXCOORD5;
};

struct VS_GOURAUD_SHADING_OUTPUT
{
  float4 Position   : SV_Position;
  float  Height     : height;
  float4 LightColor : LIGHT;
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

//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------
VS_VSC_OUTPUT VertexColoring_VS( VS_INPUT In, uniform bool phong )
{
  VS_VSC_OUTPUT Output;
  float4 pos = float4(In.Position, 1);
  Output.Color = GetColorFromHeight(pos.y);

  // Diffuse Phong
  if (phong) {
    float4 L = normalize(float4(g_vCamPos, 1) - pos);
    Output.Color *= saturate(dot(L, In.Normal));
  }

  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(pos, g_mWorldViewProjection);

  return Output;
}

VS_PSC_OUTPUT PixelColoring_VS( VS_INPUT In )
{
  VS_PSC_OUTPUT Output;
  float4 pos = float4(In.Position, 1);
  Output.Height = pos.y;
  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(pos, g_mWorldViewProjection);

  return Output;
}

VS_VSC_OUTPUT NormalColoring_VS( VS_INPUT In )
{
  VS_VSC_OUTPUT Output;
  Output.Color = float4(In.Normal * 0.5 + 0.5, 0);
  float4 pos = float4(In.Position, 1);
  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(pos, g_mWorldViewProjection);

  return Output;
}

VS_SFX_P0_OUTPUT SFX_P0_VS( VS_INPUT In )
{
  VS_SFX_P0_OUTPUT Output;
  Output.Position = mul(float4(In.Position, 1), g_mWorldViewProjection);
  Output.Height = In.Position.y;
  Output.Normal = In.Normal;
  if (g_bDirectionalLight) {
    Output.LightDir = g_vLightPos;
  } else {
    Output.LightDir = g_vLightPos - In.Position;
  }
  return Output;
}

VS_SFX_P1_OUTPUT SFX_P1_VS( VS_INPUT In )
{
  VS_SFX_P1_OUTPUT Output;
  float3 vPos = In.Position;
  vPos.xz *= 5;
  float2 vArg = g_vWaveScale * vPos.xz + g_vWaveSpeed * g_fTime;
  // Wave function
  vPos.y = g_fWaveHeight * sin(vArg.x) * cos(vArg.y);
  // Tangent and binormal are the partial derivated of the wave function
  float3 vTangent  = float3(1,  g_fWaveHeight*cos(vArg.x)*cos(vArg.y), 0);
  float3 vBinormal = float3(0, -g_fWaveHeight*sin(vArg.x)*sin(vArg.y), 1);
  float3 vNormal = cross(vBinormal, vTangent);

  // Build matrix for transformation in tangent space
  float3x3 mWorldToTangent;
  mWorldToTangent[0] = normalize(vTangent);
  mWorldToTangent[1] = normalize(vBinormal);
  mWorldToTangent[2] = normalize(vNormal);

  Output.ViewDir = mul(mWorldToTangent, g_vCamPos - vPos);
  if (g_bDirectionalLight) {
    Output.LightDir = mul(mWorldToTangent, g_vLightPos);
  } else {
    Output.LightDir = mul(mWorldToTangent, g_vLightPos - vPos);
  }
  Output.Position = mul(float4(vPos, 1), g_mWorldViewProjection);

  // Calculate wave texture coordinates
  float2 vTexCoord = vPos.xz * 0.5;
  float2 vTrans = g_fTime * 0.01;
  Output.Wave0 = vTexCoord * 1 + vTrans * 2;
  Output.Wave1 = vTexCoord * 2 - vTrans * 4;
  Output.Wave2 = vTexCoord * 4 + vTrans * 2;
  Output.Wave3 = vTexCoord * 8 - vTrans * 1;

  return Output;
}

float4 GouraudShaderHilfsFunktion(float3 vPos, float3 lPos, float3 normal, float4 Color)
{

  //Diffuse
  float4 L = normalize(float4(lPos, 1) - float4(vPos, 1));
  float4 outcolor = saturate(dot(L, normal)) * k_d * Color;

  //ambiente
  outcolor += k_a * Color;

  float3 N = normalize(normal.xyz);
  
  float NdotL = saturate(dot(N, L.xyz));
  float3 R = normalize(reflect(-L, N));
  float3 V = normalize(g_vCamPos - vPos.xyz);
  float RdotV = saturate(dot(R, V));

  outcolor += pow(RdotV, k_n) * Color * k_s;
  return outcolor;
}

VS_GOURAUD_SHADING_OUTPUT VS_GOURAUD_SHADING( VS_INPUT In )
{
  VS_GOURAUD_SHADING_OUTPUT Output;
  float4 pos = float4(In.Position, 1);
  Output.Height = pos.y;
  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(pos, g_mWorldViewProjection);
 
  //pointlights
  for (int i = 0; i<g_iNumOfPL; i++)
  {
    float d = length(Output.Position - g_vPointlight_Position[i]);
    float4 Color = g_vPointlight_Color[i] / (PointLightA + PointLightB*d + PointLightC*d*d);
   
    Output.LightColor = GouraudShaderHilfsFunktion(In.Position, g_vPointlight_Position[i], In.Normal, Color);

  }

  return Output;
}

//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------
float4 VertexColoring_PS( VS_VSC_OUTPUT In ) : SV_Target
{
  return In.Color;
}

float4 PixelColoring_PS( VS_PSC_OUTPUT In ) : SV_Target
{
  return GetColorFromHeight(In.Height);
}

float4 SFX_P0_PS( VS_SFX_P0_OUTPUT In ) : SV_Target
{
  float4 vBaseColor = GetColorFromHeight(In.Height);
  float3 N = normalize(In.Normal);
  float3 L = normalize(In.LightDir);
  float NdotL = saturate(dot(N, L));
  return (0.1 + 0.9 * NdotL) * vBaseColor * float4(g_vLightColor, 1);
}

float4 SFX_P1_PS( VS_SFX_P1_OUTPUT In ) : SV_Target
{
  // Waves bump map lookups
  float3 vWave0 = g_tWaves.Sample(g_ssWaves, In.Wave0);
  float3 vWave1 = g_tWaves.Sample(g_ssWaves, In.Wave1);
  float3 vWave2 = g_tWaves.Sample(g_ssWaves, In.Wave2);
  float3 vWave3 = g_tWaves.Sample(g_ssWaves, In.Wave3);
  // Combine waves to get normal pertubation vector
  float3 vNormalPertubation = normalize(vWave0 + vWave1 +
                                        vWave2 + vWave3 - 2);
  // Unpertubed normal is always (0,0,1)^T as we are in tangent space
  float3 N = normalize(float3(0, 0, 1) + vNormalPertubation);
  float3 L = normalize(In.LightDir);
  float NdotL = saturate(dot(N, L));
  float3 R = normalize(reflect(-L, N));
  float3 V = normalize(In.ViewDir);
  float RdotV = saturate(dot(R, V));

  float3 vTotalColor = NdotL * g_vWaterColor +
      pow(RdotV, g_iPhongExp) * g_vLightColor;

  float fFresnel = g_fFresnelBias +
      (1 - g_fFresnelBias) * pow(1 - dot(N, V), g_iFresnelExp);

  return float4(vTotalColor, fFresnel);
}


float4 PS_GOURAUD_SHADING( VS_GOURAUD_SHADING_OUTPUT In ) : SV_Target
{
  return GetColorFromHeight(In.Height) + In.LightColor;
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
technique10 VertexShaderColoring
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, VertexColoring_VS(false) ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, VertexColoring_PS() ) );
  }
}

technique10 VertexShaderColoringPhong
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, VertexColoring_VS(true) ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, VertexColoring_PS() ) );
  }
}

technique10 PixelShaderColoring
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, PixelColoring_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, PixelColoring_PS() ) );
  }
}

technique10 NormalColoring
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, NormalColoring_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, VertexColoring_PS() ) );
  }
}

technique10 SpecialFX
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, SFX_P0_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, SFX_P0_PS() ) );
  }
  pass P1
  {
    SetVertexShader( CompileShader( vs_4_0, SFX_P1_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, SFX_P1_PS() ) );
    SetBlendState( SrcColorBlendingAdd, float4(1, 1, 1, 1), 0xffffffff );
  }
}

technique10 GouraudShading
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, VS_GOURAUD_SHADING() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, PS_GOURAUD_SHADING() ) );
  }
}