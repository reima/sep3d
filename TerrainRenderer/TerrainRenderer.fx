//--------------------------------------------------------------------------------------
// File: TerrainRenderer.fx
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
#define PI (3.14159265)

cbuffer cbPerTilePass
{
  float    g_fTileScale;
  float2   g_vTileTranslate;
  uint     g_uiTileLOD;
}

cbuffer cbPerFrame
{
  // Misc
  float    g_fTime;                   // App's time in seconds
  float    g_fElapsedTime;            // Elapsed time since last frame
  float4x4 g_mWorld;                  // World matrix for object
  float4x4 g_mWorldViewProjection;    // World * View * Projection matrix
  float4x4 g_mViewInv;
  float3   g_vCamPos;                 // Camera position
  float3   g_vCamRight;
  float3   g_vCamUp;
  // Lights
  float3   g_vPointLight_Position[8];
  float3   g_vDirectionalLight_Direction[8]; // Guaranteed to be normalized
  float3   g_vSpotLight_Position[8];
  float4x4 g_mDirectionalLightSpaceTransform;
  float4x4 g_mDirectionalTrapezoidToSquare;
  float4x4 g_mPointLightSpaceTransform[6];
  // Environment
  float4x4 g_mWorldViewInv;
  float    g_fCameraFOV;
}

cbuffer cbOptions
{
  bool     g_bDynamicMinMax;
  bool     g_bWaveNormals;
  bool     g_bPCF;
  float    g_fZEpsilon;
}

cbuffer cbStatic
{
  // Terrain
  float  g_fMinHeight;
  float  g_fMaxHeight;
  uint   g_uiTerrainSize;
  // Lights
  uint   g_nPointLights;
  uint   g_nDirectionalLights;
  uint   g_nSpotLights;
  uint   g_iShadowedPointLight;
  uint   g_iShadowedDirectionalLight;
  bool   g_bShadowedPointLight;
  bool   g_bShadowedDirectionalLight;
  float3 g_vPointLight_Color[8];
  float3 g_vDirectionalLight_Color[8];
  float3 g_vSpotLight_Color[8];
  float2 g_fSpotLight_AngleExp[8];
  float3 g_vSpotLight_Direction[8]; // Guaranteed to be normalized
  float4 g_vMaterialParameters; // = { k_a, k_d, k_s, n }
  // Misc
  uint2  g_vBackBufferSize;
}

Texture2D g_tWaves;
Texture2D g_tGround;
Texture2D g_tTerrain; // Terrain heights
Texture3D g_tGround3D;
Texture2D g_tMesh; // Tree texture
TextureCube g_tCubeMap;
Texture2D g_tDirectionalShadowMap;
Texture2DArray g_tPointShadowMap;
Texture2D g_tGrass;
Texture2D g_tNoise;

SamplerState g_ssLinear
{
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = Wrap;
  AddressV = Wrap;
  AddressW = Clamp;
};

SamplerState g_ssPoint
{
  Filter = MIN_MAG_MIP_POINT;
  AddressU = Wrap;
  AddressV = Wrap;
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

const float4 g_LODColors[] = {
  float4(1, 0, 0, 1),
  float4(0, 1, 0, 1),
  float4(0, 0, 1, 1),
  float4(1, 1, 0, 1),
  float4(0, 1, 1, 1),
  float4(1, 0, 1, 1),
};

const float3 vAttenuation = float3(0, 0, 1); // Constant, linear, quadratic

const float4 g_vWaterColor = float4(0, 0.25, 0.5, 1.0);
const float4 g_vWaterMaterial = float4(0.05, 0.45, 0.5, 200);
const float4 g_vTreeMaterial = float4(0.05, 0.8, 0.15, 10);

//--------------------------------------------------------------------------------------
// Vertex shader input structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
  float3 Position   : POSITION;   // vertex position
};

struct VS_TREES_INPUT
{
  float3 Position : POSITION;
  float3 Normal : NORMAL;
  float2 TexCoord : TEXTURE;
  row_major float4x4 mTransform: mTransform;
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
  float3 WorldPosition  : WORLDPOS;
  float4 LightSpacePos  : LIGHTSPACEPOS;
  float4 TSMPos         : TSMPOS;
  float  Height         : HEIGHT;
  float3 Normal         : NORMAL;
  float2 TexCoord       : TEXCOORD0;
  float2 Wave0          : TEXCOORD2;
  float2 Wave1          : TEXCOORD3;
  float2 Wave2          : TEXCOORD4;
  float2 Wave3          : TEXCOORD5;
  float4 Material       : MATERIAL; // = { k_a, k_d, k_s, n }
};

struct VS_DIRECTIONALSHADOW_OUTPUT
{
  float4 Position      : SV_Position;
  float4 LightSpacePos : LIGHTSPACEPOS;
};

struct VS_TREES_OUTPUT
{
  float4 Position      : SV_Position;
  float3 WorldPosition : WORLDPOS;
  float4 LightSpacePos : LIGHTSPACEPOS;
  float4 TSMPos        : TSMPOS;
  float3 Normal        : NORMAL;
  float2 TexCoord      : TEXCOORD0;
};

struct VS_RTS_OUTPUT
{
  float4 Position : SV_Position;
  float4 Color    : COLOR;
};

struct GS_POINTSHADOW_INPUT
{
  float4 Position   : SV_Position;
};

struct GS_POINTSHADOW_OUTPUT
{
  float4 Position   : SV_Position;
  float2 Depth      : DEPTH;
  uint   RTI        : SV_RenderTargetArrayIndex;
};

struct PHONG
{
  float3 DiffuseLight;
  float3 SpecularLight;
};

struct VS_SEED
{
  float3 Position : POSITION;
  float  Rotation : ROTATION;
  float  Size     : SIZE;
  float3 Normal   : NORMAL;
  uint   VertexID : SV_VertexID;
};

struct GS_SEED
{
  float3 Position : POSITION;
  float  Rotation : ROTATION;
  float  Size     : SIZE;
  float3 Normal   : NORMAL;
  uint   PlantID  : PLANTID;
};

struct PLANT_VERTEX
{
  float4 Position       : SV_Position;
  //float3 Normal         : NORMAL;
  //float3 WorldPosition  : WORLDPOS;
  //float4 LightSpacePos  : LIGHTSPACEPOS;
  //float4 TSMPos         : TSMPOS;
  PHONG  Phong          : PHONG;
  float2 TexCoord       : TEXCOORD0;
  uint   PlantID        : PLANTID;
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

PHONG PhongLighting(float3 vPos, float3 vNormal, float4 vMaterial, uniform bool bDeferShadowed=true)
{
  float3 vDiffuseLight = float3(0, 0, 0);
  float3 vSpecularLight = float3(0, 0, 0);
  // Point lights
  uint i;
  float d;
  float fAttenuation;
  float2 vPhong;
  for (i = 0; i < g_nPointLights; i++) {
    // Defer shadowed lighting
    if (bDeferShadowed && g_bShadowedPointLight && i == g_iShadowedPointLight) continue;
    d = length(vPos - g_vPointLight_Position[i]);
    fAttenuation = 1 / dot(vAttenuation, float3(1, d, d*d));
    vPhong = Phong(vPos, g_vPointLight_Position[i] - vPos, vNormal, vMaterial);
    vDiffuseLight += vPhong.x * g_vPointLight_Color[i] * fAttenuation;
    vSpecularLight += vPhong.y * g_vPointLight_Color[i] * fAttenuation;
  }

  // Directional lights
  for (i = 0; i < g_nDirectionalLights; i++) {
    // Defer shadowed lighting
    if (bDeferShadowed && g_bShadowedDirectionalLight &&i == g_iShadowedDirectionalLight) continue;
    vPhong = Phong(vPos, g_vDirectionalLight_Direction[i], vNormal,
                          vMaterial);
    vDiffuseLight += vPhong.x * g_vDirectionalLight_Color[i];
    vSpecularLight += vPhong.y * g_vDirectionalLight_Color[i];
  }

  // Spot lights
  float3 I;
  float IdotL, theta, angle;
  for (i = 0; i < g_nSpotLights; i++) {
    d = length(vPos - g_vSpotLight_Position[i]);
    I = (g_vSpotLight_Position[i] - vPos) / d;
    IdotL = dot(-I, g_vSpotLight_Direction[i]);
    theta = abs(g_fSpotLight_AngleExp[i].x);
    angle = acos(IdotL);
    if (angle < theta) {
      fAttenuation = 1 / dot(vAttenuation, float3(1, d, d*d));
      fAttenuation *= 1 - pow(angle/theta, g_fSpotLight_AngleExp[i].y);
      vPhong = Phong(vPos, g_vSpotLight_Position[i] - vPos, vNormal, vMaterial);
      vDiffuseLight += vPhong.x * g_vSpotLight_Color[i] * fAttenuation;
      vSpecularLight += vPhong.y * g_vSpotLight_Color[i] * fAttenuation;
    }
  }

  PHONG ret;
  ret.DiffuseLight = vDiffuseLight;
  ret.SpecularLight = vSpecularLight;
  return ret;
}

float3 FullLighting(float3 vColor,
                    float3 vWorldPosition,
                    float4 vLightSpacePos,
                    float4 vTSMPos,
                    float3 N,
                    float4 vMaterial)
{
  PHONG phong = PhongLighting(vWorldPosition, N, vMaterial);

  float fShadowMap;
  uint nInShadow;
  float fLightDist;
  float2 vPhong;
  float fLightScale;
  uint uiWidth, uiHeight, uiElements;

  //
  // Shadowed directional light
  //
  if (g_bShadowedDirectionalLight) {
    vLightSpacePos /= vLightSpacePos.w;
    fLightDist = vLightSpacePos.z - g_fZEpsilon;
    vTSMPos /= vTSMPos.w;
    vTSMPos.x = 0.5 * vTSMPos.x + 0.5;
    vTSMPos.y = 1 - (0.5 * vTSMPos.y + 0.5);
    g_tDirectionalShadowMap.GetDimensions(uiWidth, uiHeight);
    vTSMPos.xy *= uint2(uiWidth-1, uiHeight-1);

    if (g_bPCF) {
      nInShadow = 0;
      [unroll] for (int x = -1; x <= 1; ++x) {
        [unroll] for (int y = -1; y <= 1; ++y) {
          fShadowMap = g_tDirectionalShadowMap.Load(int3(vTSMPos.xy + int2(x, y), 0));
          if (fShadowMap < fLightDist) nInShadow++;
        }
      }
      fLightScale = 1 - nInShadow / 9.0f;
    } else {
      fShadowMap = g_tDirectionalShadowMap.Load(int3(vTSMPos.xy, 0));
      if (fShadowMap < fLightDist) fLightScale = 0.0f;
      else fLightScale = 1.0f;
    }

    if (fLightScale > 0) {
      uint i = g_iShadowedDirectionalLight;
      vPhong = Phong(vWorldPosition,
                     g_vDirectionalLight_Direction[i],
                     N, vMaterial);
      phong.DiffuseLight +=
          fLightScale * vPhong.x * g_vDirectionalLight_Color[i];
      phong.SpecularLight +=
          fLightScale * vPhong.y * g_vDirectionalLight_Color[i];
    }
  }

  //
  // Shadowed point light
  //
  if (g_bShadowedPointLight) {
    const uint i = g_iShadowedPointLight;
    float3 vLightDir = vWorldPosition - g_vPointLight_Position[i];
    float3 vLightDirAbs = abs(vLightDir);
    float fMaxCoord = max(vLightDirAbs.x, max(vLightDirAbs.y, vLightDirAbs.z));
    uint uiArraySlice;
    if (vLightDirAbs.x == fMaxCoord) {
      if (vLightDir.x < 0) uiArraySlice = 0;
      else uiArraySlice = 1;
    } else if (vLightDirAbs.y == fMaxCoord) {
      if (vLightDir.y < 0) uiArraySlice = 2;
      else uiArraySlice = 3;
    } else if (vLightDirAbs.z == fMaxCoord) {
      if (vLightDir.z < 0) uiArraySlice = 4;
      else uiArraySlice = 5;
    }
    // Transform in light space
    vLightSpacePos = mul(float4(vWorldPosition, 1), g_mPointLightSpaceTransform[uiArraySlice]);
    // Dehomogenize
    vLightSpacePos /= vLightSpacePos.w;
    fLightDist = vLightSpacePos.z - g_fZEpsilon;
    vLightSpacePos.x = 0.5 * vLightSpacePos.x + 0.5;
    vLightSpacePos.y = 1 - (0.5 * vLightSpacePos.y + 0.5);
    g_tPointShadowMap.GetDimensions(uiWidth, uiHeight, uiElements);
    vLightSpacePos.xy *= uint2(uiWidth-1, uiHeight-1);

    if (g_bPCF) {
      nInShadow = 0;
      [unroll] for (int x = -1; x <= 1; ++x) {
        [unroll] for (int y = -1; y <= 1; ++y) {
          fShadowMap = g_tPointShadowMap.Load(int4(vLightSpacePos.xy + int2(x, y), uiArraySlice, 0));
          if (fShadowMap < fLightDist) nInShadow++;
        }
      }
      fLightScale = 1 - nInShadow / 9.0f;
    } else {
      fShadowMap = g_tPointShadowMap.Load(int4(vLightSpacePos.xy, uiArraySlice, 0));
      if (fShadowMap < fLightDist) fLightScale = 0.0f;
      else fLightScale = 1.0f;
    }

    if (fLightScale > 0) {
      float d = length(vWorldPosition - g_vPointLight_Position[i]);
      float fAttenuation = 1 / dot(vAttenuation, float3(1, d, d*d));
      vPhong = Phong(vWorldPosition,
                     g_vPointLight_Position[i] - vWorldPosition,
                     N,
                     vMaterial);
      phong.DiffuseLight +=
          fLightScale * vPhong.x * g_vPointLight_Color[i] * fAttenuation;
      phong.SpecularLight +=
          fLightScale * vPhong.y * g_vPointLight_Color[i] * fAttenuation;
    }
  }

  return vColor * vMaterial.x +
      vColor * phong.DiffuseLight +
      phong.SpecularLight;
}

float4 GetTileData(float2 vPosition)
{
  return g_tTerrain.Load(float3(vPosition.xy, 0) * (g_uiTerrainSize - 1));
}

float4 GetTileVertex(float2 vPosition)
{
  float4 vVertexData = GetTileData(vPosition);
  vPosition *= g_fTileScale;
  vPosition += g_vTileTranslate;
  return float4(vPosition.x, vVertexData.w, vPosition.y, 1);
}

float4 GetTileVertexAndNormal(float2 vPosition, out float4 vNormal)
{
  float4 vVertexData = GetTileData(vPosition);
  vNormal = float4(vVertexData.xyz, 0);
  vPosition *= g_fTileScale;
  vPosition += g_vTileTranslate;
  return float4(vPosition.x, vVertexData.w, vPosition.y, 1);
}

//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------
VS_GOURAUD_SHADING_OUTPUT GouraudShading_VS( float2 vPosition : POSITION )
{
  VS_GOURAUD_SHADING_OUTPUT Output;
  float4 vNormal;
  float4 vPos = GetTileVertexAndNormal(vPosition, vNormal);
  Output.Height = vPos.y;
  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(vPos, g_mWorldViewProjection);
  PHONG phong = PhongLighting(vPos, vNormal,
                              g_vMaterialParameters);
  Output.DiffuseLight = phong.DiffuseLight;
  Output.SpecularLight = phong.SpecularLight;

  return Output;
}

VS_PHONG_SHADING_OUTPUT PhongShading_VS( float2 vPosition : POSITION )
{
  VS_PHONG_SHADING_OUTPUT Output;
  float4 vPos = GetTileVertexAndNormal(vPosition, Output.Normal);
  if (vPos.y <= 0) {
    Output.Material = g_vWaterMaterial;
  } else {
    Output.Material = g_vMaterialParameters;
  }
  // Transform the position from object space to homogeneous projection space
  Output.Position = mul(vPos, g_mWorldViewProjection);
  Output.Height = vPos.y;
  Output.WorldPosition = vPos;
  Output.LightSpacePos = mul(vPos, g_mDirectionalLightSpaceTransform);
  Output.TSMPos = mul(Output.LightSpacePos, g_mDirectionalTrapezoidToSquare);
  Output.TexCoord = vPos.xz * 5;

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

VS_TREES_OUTPUT Trees_VS( VS_TREES_INPUT Input ) {
  VS_TREES_OUTPUT Output;

  float4 vPos = float4(Input.Position, 1);
  float4x4 trans = Input.mTransform;
  //trans._14 = sin(g_fTime)/30;
  vPos = mul(vPos, trans);
  Output.Position = mul(vPos, g_mWorldViewProjection);
  Output.WorldPosition = Input.Position;
  Output.LightSpacePos = mul(vPos, g_mDirectionalLightSpaceTransform);
  Output.TSMPos = mul(Output.LightSpacePos, g_mDirectionalTrapezoidToSquare);
  Output.Normal = normalize(Input.Normal);
  Output.TexCoord = Input.TexCoord;

  return Output;
}

VS_TREES_OUTPUT TreesShadowMap_VS( VS_TREES_INPUT Input ) {
  VS_TREES_OUTPUT Output;

  float4 vPos = float4(Input.Position, 1);
  float4x4 trans = Input.mTransform;
  //trans._14 = sin(g_fTime)/30;
  vPos = mul(vPos, trans);
  Output.LightSpacePos = mul(vPos, g_mDirectionalLightSpaceTransform);
  Output.Position = mul(Output.LightSpacePos, g_mDirectionalTrapezoidToSquare);
  Output.TexCoord = Input.TexCoord;

  return Output;
}

float4 Environment_VS( float3 vPosition : POSITION ) : SV_Position
{
  return float4(vPosition, 1);
}

VS_DIRECTIONALSHADOW_OUTPUT DirectionalShadow_VS( float2 vPosition : POSITION )
{
  VS_DIRECTIONALSHADOW_OUTPUT Output;
  float4 vPos = GetTileVertex(vPosition);
  Output.LightSpacePos = mul(vPos, g_mDirectionalLightSpaceTransform);
  Output.Position = mul(Output.LightSpacePos, g_mDirectionalTrapezoidToSquare);
  return Output;
}

float4 PointShadow_VS( float2 vPosition : POSITION ) : SV_Position
{
  return GetTileVertex(vPosition);
}

VS_RTS_OUTPUT RenderToScreen_VS( float2 vPosition : POSITION, uint iVertex : SV_VertexID )
{
  VS_RTS_OUTPUT Output;
  Output.Position = float4(vPosition, 0, 1);
  Output.Color = g_LODColors[iVertex % 6];
  return Output;
}

GS_SEED Grass_VS(VS_SEED Input)
{
  GS_SEED Output;
  Output.Position = Input.Position;
  Output.Rotation = Input.Rotation;
  Output.Size = Input.Size;
  Output.Normal = Input.Normal;
  Output.PlantID = Input.VertexID;
  return Output;
}

//--------------------------------------------------------------------------------------
// Geometry Shaders
//--------------------------------------------------------------------------------------
[MaxVertexCount(18)]
void PointShadow_GS( triangle GS_POINTSHADOW_INPUT In[3],
                     inout TriangleStream<GS_POINTSHADOW_OUTPUT> MeshStream )
{
  GS_POINTSHADOW_OUTPUT Output;
  [unroll] for (uint i = 0; i < 6; ++i) {
    Output.RTI = i;
    [unroll] for (uint j = 0; j < 3; ++j) {
      Output.Position = mul(In[j].Position, g_mPointLightSpaceTransform[i]);
      Output.Depth = Output.Position.zw;
      MeshStream.Append(Output);
    }
    MeshStream.RestartStrip();
  }
}

void CreatePlantQuad(float3 vBase, float3 vUp, float3 vRight, PLANT_VERTEX Output,
                     inout TriangleStream <PLANT_VERTEX> PlantStream,
                     float4x4 mTransform)
{
  float4 vVertex;
  vVertex.w = 1;

  // Links oben
  vVertex.xyz = vBase - vRight + vUp;
  Output.Position = mul(vVertex, mTransform);
  Output.TexCoord = float2(0,0);
  PlantStream.Append(Output);
  // Rechts oben
  vVertex.xyz = vBase + vRight + vUp;
  Output.Position = mul(vVertex, mTransform);
  Output.TexCoord = float2(1,0);
  PlantStream.Append(Output);
  // Links unten
  vVertex.xyz = vBase - vRight;
  Output.Position = mul(vVertex, mTransform);
  Output.TexCoord = float2(0,1);
  PlantStream.Append(Output);
  // Rechts unten
  vVertex.xyz = vBase + vRight;
  Output.Position = mul(vVertex, mTransform);
  Output.TexCoord = float2(1,1);
  PlantStream.Append(Output);

  PlantStream.RestartStrip();
}

[MaxVertexCount(8)]
void Grass_GS(point GS_SEED input[1], inout TriangleStream <PLANT_VERTEX> PlantStream)
{
  // Culling
  float4 vSeed = mul(float4(input[0].Position, 1), g_mWorldViewProjection);
  vSeed /= vSeed.w;
  if (vSeed.z < 0 || vSeed.z > 1 ||
      vSeed.x < -1.2 || vSeed.x > 1.2 ||
      vSeed.y < -2.0 || vSeed.y > 1.2) return;

  PLANT_VERTEX Output;
  Output.PlantID = input[0].PlantID;
  //Output.Normal = input[0].Normal;
  //Output.WorldPosition = input[0].Position;
  //Output.LightSpacePos = mul(float4(input[0].Position, 1), g_mDirectionalLightSpaceTransform);
  //Output.TSMPos = mul(Output.LightSpacePos, g_mDirectionalTrapezoidToSquare);
  Output.Phong = PhongLighting(input[0].Position, input[0].Normal, float4(0.05, 0.9, 0.05, 50), false);

  const float s = 0.5*sin(input[0].Rotation);
  const float c = 0.5*cos(input[0].Rotation);

  const float skew = 0.05*(sin(1*(input[0].Position.x+0.5*g_fTime)) +
                           sin(2*(input[0].Position.x+0.5*g_fTime)) +
                           sin(4*(input[0].Position.x+0.5*g_fTime)) +
                           sin(1*(input[0].Position.y+0.5*g_fTime)) +
                           sin(2*(input[0].Position.y+0.5*g_fTime)) +
                           sin(4*(input[0].Position.y+0.5*g_fTime)));

  const float billboardSize = input[0].Size;

  CreatePlantQuad(input[0].Position, float3(skew, 1, 0) * billboardSize,
                  float3(s, 0, c) * billboardSize, Output,
                  PlantStream, g_mWorldViewProjection);
  CreatePlantQuad(input[0].Position, float3(skew, 1, 0) * billboardSize,
                  float3(-c, 0, s) * billboardSize, Output,
                  PlantStream, g_mWorldViewProjection);
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

float4 PhongShading_PS( VS_PHONG_SHADING_OUTPUT In, uniform bool bLODColoring ) : SV_Target
{
  float3 N = normalize(In.Normal);
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
      N = normalize(float3(0, 1, 0) + vNormalPertubation * 0.1);
    }
    vTerrainColor = g_vWaterColor;
  } else {
    //vTerrainColor = GetColorFromHeight(In.Height);
    //vTerrainColor = g_tDirectionalShadowMap.Sample(g_ssLinear, In.TexCoord);
    //vTerrainColor = g_tGround.SampleLevel(g_ssLinear, In.TexCoord, 0);
    float fHeightNormalized = In.Height / g_fMaxHeight + 0.1;
    vTerrainColor = g_tGround3D.SampleLevel(g_ssLinear,
                                            float3(In.TexCoord,
                                                   fHeightNormalized),
                                            0);
  }

  if (bLODColoring) {
    vTerrainColor = g_LODColors[g_uiTileLOD % 6];
  }

  float3 vColor = FullLighting(vTerrainColor,
                               In.WorldPosition,
                               In.LightSpacePos,
                               In.TSMPos,
                               N,
                               In.Material);

  float3 I = normalize(In.WorldPosition - g_vCamPos);
  float3 R = reflect(I, N);
  float4 vReflect = g_tCubeMap.Sample(g_ssLinear, R);

  return float4(vColor, 1) + In.Material.z * vReflect;
}

float4 Trees_PS( VS_TREES_OUTPUT In ) : SV_Target
{
  float3 N = normalize(In.Normal);
  float4 vColor = g_tMesh.Sample(g_ssLinear, In.TexCoord);

  if (vColor.a < 0.15) discard;

  vColor.rgb = FullLighting(vColor.rgb,
                            In.WorldPosition,
                            In.LightSpacePos,
                            In.TSMPos,
                            N,
                            g_vTreeMaterial);

  return vColor;
}

float TreesShadowMap_PS( VS_TREES_OUTPUT In ) : SV_Depth
{
  float4 vColor = g_tMesh.Sample(g_ssLinear, In.TexCoord);
  if (vColor.a < 0.05) discard;

  return In.LightSpacePos.z/In.LightSpacePos.w;
}

float4 Environment_PS( float4 vPos : SV_Position ) : SV_Target
{
  // Transform vPos.xy in NDC
  vPos.xy /= float2(g_vBackBufferSize.x - 1, g_vBackBufferSize.y - 1);
  vPos.x = 2*vPos.x-1;
  vPos.y = 1-2*vPos.y;
  float fRatio = g_vBackBufferSize.x / (float)g_vBackBufferSize.y;
  // Calculate view vector in view space
  float3 vView = float3(fRatio * tan(0.5f * g_fCameraFOV) * vPos.x,
                        tan(0.5f * g_fCameraFOV) * vPos.y,
                        1.0);
  // Transform view vector in world space
  vView = mul(vView, g_mWorldViewInv);
  return g_tCubeMap.Sample(g_ssLinear, vView);
}

float DirectionalShadow_PS( VS_DIRECTIONALSHADOW_OUTPUT In ) : SV_Depth
{
  return In.LightSpacePos.z/In.LightSpacePos.w;
}

float PointShadow_PS( GS_POINTSHADOW_OUTPUT In ) : SV_Depth
{
  return In.Depth.x/In.Depth.y;
}

float4 Color_PS( VS_RTS_OUTPUT In ) : SV_Target
{
  return In.Color;
}

float4 Grass_PS( PLANT_VERTEX In ) : SV_Target
{
  float fNoise = g_tNoise.Sample(g_ssLinear, In.TexCoord);
  const int type = In.PlantID % 4;
  In.TexCoord.x += type;
  In.TexCoord.x *= 0.25;
  float4 vColor = g_tGrass.Sample(g_ssLinear, In.TexCoord);
  vColor.a *= saturate(fNoise+0.1);

  //vColor.rgb = FullLighting(vColor.rgb,
  //                          In.WorldPosition,
  //                          In.LightSpacePos,
  //                          In.TSMPos,
  //                          In.Normal,
  //                          float4(0.05, 0.9, 0.0, 50));

  //PHONG phong = PhongLighting(In.WorldPosition, In.Normal, float4(0.05, 0.9, 0.05, 50));
  //vColor.rgb = vColor.rgb * 0.05 + vColor.rgb * phong.DiffuseLight + phong.SpecularLight;
  vColor.rgb = vColor.rgb * 0.05 + vColor.rgb * In.Phong.DiffuseLight + In.Phong.SpecularLight;

  return vColor;
}

//--------------------------------------------------------------------------------------
// States
//--------------------------------------------------------------------------------------
DepthStencilState dssEnableDepth
{
  DepthEnable = TRUE;
  DepthWriteMask = ALL;
  DepthFunc = LESS;
};

DepthStencilState dssDisableDepthStencil
{
  DepthEnable = FALSE;
  StencilEnable = FALSE;
};

DepthStencilState dssEnableDepthNoWrite
{
  DepthEnable = TRUE;
  DepthWriteMask = 0;
  DepthFunc = LESS;
};

BlendState bsNoColorWrite
{
  RenderTargetWriteMask[0] = 0;
  RenderTargetWriteMask[1] = 0;
  RenderTargetWriteMask[2] = 0;
  RenderTargetWriteMask[3] = 0;
  RenderTargetWriteMask[4] = 0;
  RenderTargetWriteMask[5] = 0;
  RenderTargetWriteMask[6] = 0;
  RenderTargetWriteMask[7] = 0;
};

BlendState bsAlphaToCov
{
  AlphaToCoverageEnable = TRUE;
  RenderTargetWriteMask[0] = 0x0F;
};

BlendState bsSrcAlphaBlendingAdd
{
  BlendEnable[0] = TRUE;
  SrcBlend = SRC_ALPHA;
  DestBlend = INV_SRC_ALPHA;
  BlendOp = ADD;
  RenderTargetWriteMask[0] = 0x0F;
};

BlendState bsSrcAlphaBlendingAddIntense
{
  BlendEnable[0] = TRUE;
  SrcBlend = SRC_ALPHA;
  DestBlend = ONE;
  BlendOp = ADD;
  RenderTargetWriteMask[0] = 0x0F;
};

RasterizerState rsCullNone
{
  CullMode = None;
};

RasterizerState rsWireframe
{
  CullMode = Back;
  FillMode = Wireframe;
};

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------
//technique10 GouraudShading
//{
//  pass P0
//  {
//    SetVertexShader( CompileShader( vs_4_0, GouraudShading_VS() ) );
//    SetGeometryShader( NULL );
//    SetPixelShader( CompileShader( ps_4_0, GouraudShading_PS() ) );
//    SetRasterizerState( NULL );
//    SetBlendState( NULL, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
//  }
//}

technique10 PhongShading
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, PhongShading_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, PhongShading_PS(false) ) );
    SetDepthStencilState( NULL, 0 );
    SetRasterizerState( NULL );
    SetBlendState( NULL, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 LODColoring
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, PhongShading_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, PhongShading_PS(true) ) );
    SetDepthStencilState( NULL, 0 );
    SetRasterizerState( NULL );
    SetBlendState( NULL, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 Trees
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Trees_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, Trees_PS() ) );
    SetDepthStencilState( NULL, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsAlphaToCov, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 TreesShadowMap
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, TreesShadowMap_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, TreesShadowMap_PS() ) );
    SetDepthStencilState( dssEnableDepth, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsNoColorWrite, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 Environment
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Environment_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, Environment_PS() ) );
    SetRasterizerState( NULL );
    SetBlendState( NULL, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 DirectionalShadowMap
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, DirectionalShadow_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, DirectionalShadow_PS() ) );
    SetDepthStencilState( dssEnableDepth, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsNoColorWrite, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 PointShadowMap
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, PointShadow_VS() ) );
    SetGeometryShader( CompileShader( gs_4_0, PointShadow_GS() ) );
    SetPixelShader( CompileShader( ps_4_0, PointShadow_PS() ) );
    SetDepthStencilState( dssEnableDepth, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsNoColorWrite, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

// Nur für Debug-Zwecke gebraucht
technique10 RenderToScreen
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, RenderToScreen_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, Color_PS() ) );
    SetDepthStencilState( NULL, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( NULL, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 Grass
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Grass_VS() ) );
    SetGeometryShader( CompileShader( gs_4_0, Grass_GS() ) );
    SetPixelShader( CompileShader( ps_4_0, Grass_PS() ) );
    SetDepthStencilState( dssEnableDepthNoWrite, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsSrcAlphaBlendingAdd, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}


//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
// Particle Simulation
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant buffers & data types
//--------------------------------------------------------------------------------------
cbuffer cbPerPointEmitter
{
  float3 g_vPEPosition;
  float g_fPESpread;
  float4x4 g_mPETransform;
}

cbuffer cbPerBoxEmitter
{
  float3 g_vBEMinVertex;
  float3 g_vBEMaxVertex;
  float3 g_vBEVelocity;
}

struct PARTICLE
{
  float3 Position : POSITION;
  float3 Velocity : VELOCITY;
  float  Age      : AGE;
  float  MaxAge   : MAXAGE;
  float  Size     : SIZE;
  float  Rotation : ROTATION;
  uint   Type     : TYPE;
};

struct PARTICLE_BILLBOARD
{
  float4 Position : SV_Position;
  float2 TexCoord : TEXCOORD0;
  float4 Color    : COLOR;
  uint   Type     : TYPE;
};

struct PARTICLE_POINT
{
  float4 Position : SV_Position;
  float4 Color    : COLOR;
};

const float3x3 g_mId3 = float3x3(1, 0, 0, 0, 1, 0, 0, 0, 1);

const float3 g_vGravity = float3(0, -1, 0);

//--------------------------------------------------------------------------------------
// Texture resources
//--------------------------------------------------------------------------------------
Texture2D g_tRandom;
Texture2D g_tVulcanoFire;
Texture2D g_tVulcanoHighlight;
Texture2D g_tRainDrop;

//--------------------------------------------------------------------------------------
// Particle types
//--------------------------------------------------------------------------------------
#define PT_SPAWNER   0
#define PT_FIRE      1
#define PT_SMOKE     2
#define PT_HIGHLIGHT 3
#define PT_CHILD_SPAWNER 4

#define PT_RAIN_DROP 5

struct PARTICLE_TYPE
{
  float Life;
  float LifeVariation;
  float Size;
  float SizeVariation;
  float Velocity;
  float VelocityVariation;
  float Rotation;
  float RotationVariation;
  bool  Visible;
  bool  Intense;
};

const PARTICLE_TYPE ptSpawner =
{
  5.0, 1.0,
  0, 0,
  2, 1,
  0, 0,
  false, false
};

const PARTICLE_TYPE ptFire =
{
  1.0, 1.5,
  3.0, 1.5,
  0.0, 0.12,
  0, PI,
  true, true
};

float Fire_Size(float fAge)
{
  if (fAge < 0.15) return 11*fAge;
  else return -0.5*fAge*(fAge-2)+1.5;
}

float Fire_Velocity(float fAge)
{
  return 2*exp2(-10*fAge);
}

float4 Fire_Color(float fAge)
{
  float4 vColor = float4(1, 0.435, 0.212, 1);
  if (fAge > 0.5) vColor.a = lerp(1, 0, (fAge - 0.5) / 0.5);
  return vColor;
}


const PARTICLE_TYPE ptSmoke =
{
  6.0, 0.0,
  5.0, 0.2,
  0.15, 0.48,
  0, PI,
  true, false
};

float Smoke_Size(float fAge)
{
  return 2-2*exp2(-20*fAge);
}

float Smoke_Velocity(float fAge)
{
  if (fAge < 0.04) return 1 - 20*fAge;
  else return lerp(0.2, 0, (fAge-0.04)/0.96);
}

float4 Smoke_Color(float fAge)
{
  float4 vColor;
  if (fAge < 0.8) vColor.a = 1;
  else vColor.a = lerp(1, 0, (fAge - 0.8) * 5);
  if (fAge > 0.66666) vColor.rgb = float3(31.0/255, 30.0/255, 24.0/255);
  else vColor.rgb = lerp(float3(57.0/255, 54.0/255, 40.0/255),
                         float3(31.0/255, 30.0/255, 24.0/255),
                         fAge * 1.5);
  vColor.a *= 0.5;
  return vColor;
}


const PARTICLE_TYPE ptHighlight =
{
  5.0, 1.0,
  4.8, 0.1,
  0.15, 0.48,
  0, PI,
  true, false
};

float Highlight_Size(float fAge)
{
  return 2-2*exp2(-20*fAge);
}

float Highlight_Velocity(float fAge)
{
  if (fAge < 0.04) return 1 - 20*fAge;
  else return lerp(0.2, 0, (fAge-0.04)/0.96);
}

float4 Highlight_Color(float fAge)
{
  float4 vColor;
  vColor.rgb = float3(1, 1, 1);
  if (fAge < 0.2) vColor.a = 1;
  else vColor.a = lerp(1, 0, (fAge - 0.2) / 0.8);
  vColor.a *= 0.1;
  return vColor;
}


const PARTICLE_TYPE ptRainDrop =
{
  10.0, 0.0,
  0.5, 0.1,
  1, 0,
  0, 0,
  true, false
};

float4 Rain_Color(float fAge)
{
  float4 vColor = float4(0.9, 0.9, 1, 1);
  if (fAge < 0.1) vColor.a = fAge*10;
  return vColor;
}

PARTICLE_TYPE ParticleType(uint uiType)
{
  switch (uiType) {
    case PT_SPAWNER:
    case PT_CHILD_SPAWNER:
                       return ptSpawner;
    case PT_FIRE:      return ptFire;
    case PT_SMOKE:     return ptSmoke;
    case PT_HIGHLIGHT: return ptHighlight;
    case PT_RAIN_DROP: return ptRainDrop;
    default:           return (PARTICLE_TYPE)0;
  }
}

float4 ParticleColor(PARTICLE p, float fAge)
{
  switch (p.Type) {
    case PT_FIRE:      return Fire_Color(fAge);
    case PT_SMOKE:     return Smoke_Color(fAge);
    case PT_HIGHLIGHT: return Highlight_Color(fAge);
    case PT_SPAWNER:   return float4(1, 0, 0, 1);
    case PT_CHILD_SPAWNER: return float4(0, 1, 0, 1);
    case PT_RAIN_DROP: return Rain_Color(fAge);
    default: return 0;
  }
}

//--------------------------------------------------------------------------------------
// Global functions
//--------------------------------------------------------------------------------------
float Random(float fOffset)
{
  fOffset += g_fTime*5000;
  fOffset /= 1234;
  return g_tRandom.SampleLevel(g_ssPoint, float2(fmod(fOffset, 1024), fOffset/1024), 0);
}

float UniformRandom(float fOffset, float fMin, float fMax)
{
  return fMin + Random(fOffset) * (fMax - fMin);
}

float UniformRandomWithVariation(float fOffset, float fMean, float fVariation)
{
  return UniformRandom(fOffset, fMean - fVariation, fMean + fVariation);
}

//
// Simulation functions
//
PARTICLE EulerStep(PARTICLE In)
{
  float3 vVelocity;
  switch (In.Type) {
    case PT_FIRE:
      vVelocity = Fire_Velocity(In.Age/In.MaxAge)*In.Velocity;
      break;
    case PT_SMOKE:
      vVelocity = Smoke_Velocity(In.Age/In.MaxAge)*In.Velocity;
      break;
    case PT_HIGHLIGHT:
      vVelocity = Highlight_Velocity(In.Age/In.MaxAge)*In.Velocity;
      break;
    case PT_RAIN_DROP:
      vVelocity = In.Velocity;
      float3 fWindForce = 0.1*float3(
        sin(1*(0.8*g_fTime)) +
        sin(4*(0.2*g_fTime)),
        0,
        cos(1*(0.1*g_fTime)) +
        cos(2*(0.4*g_fTime)));
      In.Velocity += g_fElapsedTime * (g_vGravity + fWindForce);
      break;
    default:
      vVelocity = In.Velocity;
      In.Velocity += g_fElapsedTime * g_vGravity;
      break;
  }
  In.Position += g_fElapsedTime * vVelocity;
  return In;
}

bool CollisionDetect(PARTICLE In)
{
  float2 vTexCoord = (In.Position.xz - g_vTileTranslate) / g_fTileScale;
  if (all(vTexCoord >= float2(0, 0) && vTexCoord <= float2(1, 1))) {
    float4 vHeightMap = g_tTerrain.SampleLevel(g_ssLinear, vTexCoord, 0);
    return vHeightMap.w >= In.Position.y;
  } else return false;
}

//--------------------------------------------------------------------------------------
// Emitters
//--------------------------------------------------------------------------------------
PARTICLE InitParticle(float fRandomOffset, PARTICLE p)
{
  PARTICLE_TYPE pt = ParticleType(p.Type);
  p.Size = UniformRandomWithVariation(fRandomOffset+731, pt.Size, pt.SizeVariation);
  p.Velocity *= UniformRandomWithVariation(fRandomOffset+107, pt.Velocity, pt.VelocityVariation);
  p.MaxAge = UniformRandomWithVariation(fRandomOffset+541, pt.Life, pt.LifeVariation);
  p.Age = 0;
  p.Rotation = UniformRandomWithVariation(fRandomOffset+381, pt.Rotation, pt.RotationVariation);
  return p;
}

PARTICLE PointEmitterCreate(float fRandomOffset, uint uiType, float3 vPosition,
                            float3x3 mTransform, float fSpread)
{
  PARTICLE p;
  p.Type = uiType;
  p.Position = vPosition;
  float fAzimuth = Random(fRandomOffset)*PI*2;
  float fZenith = Random(fRandomOffset+23)*fSpread;
  float3 vDir = float3(cos(fAzimuth)*sin(fZenith), sin(fAzimuth)*sin(fZenith), cos(fZenith));
  p.Velocity = mul(vDir, mTransform);
  return InitParticle(fRandomOffset, p);
}

PARTICLE VolcanoParticleCreate(float3 vPosition, float fRandomOffset)
{
  float fRand = Random(fRandomOffset+1245);
  uint uiType = 0;
  if (fRand < 0.45)      uiType = 1;
  else if (fRand < 0.9)  uiType = 2;
  else                   uiType = 3;  
  return PointEmitterCreate(fRandomOffset, uiType, vPosition, g_mId3, PI*2);
}

PARTICLE BoxEmitterCreate(float fRandomOffset, uint uiType, float3 vMinVertex,
                          float3 vMaxVertex, float3 vVelocity)
{
  PARTICLE p;
  p.Type = uiType;
  p.Position = lerp(vMinVertex,
                    vMaxVertex,
                    float3(Random(fRandomOffset),
                           Random(fRandomOffset+17),
                           Random(fRandomOffset+109)));
  p.Velocity = vVelocity;
  return InitParticle(fRandomOffset, p);
}

//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------
PARTICLE Particles_VS(PARTICLE Input)
{
  return Input;
}

PARTICLE_POINT ParticlePoint_VS(PARTICLE Input)
{
  PARTICLE_POINT Output;
  Output.Position = mul(float4(Input.Position, 1), g_mWorldViewProjection);
  float fAge = Input.Age / Input.MaxAge;
  Output.Color = ParticleColor(Input, fAge);
  return Output;
}

//--------------------------------------------------------------------------------------
// Geometry Shaders
//--------------------------------------------------------------------------------------
[MaxVertexCount(10)]
void Volcano_GS(point PARTICLE input[1], uint ID : SV_PrimitiveID,
                  inout PointStream<PARTICLE> ParticleStream)
{
  PARTICLE p = input[0];
  if (p.Type == -1) {
    // First simulation step
    p = PointEmitterCreate(ID, PT_SPAWNER, g_vPEPosition, g_mPETransform, g_fPESpread);      
    ParticleStream.Append(p);
    return;
  } else if (p.Type == PT_SPAWNER || p.Type == PT_CHILD_SPAWNER) {
    if (Random(ID) > 0.1 && g_fElapsedTime > 0) ParticleStream.Append(VolcanoParticleCreate(p.Position, ID));
  }
  p.Age += g_fElapsedTime;
  if (p.Age > p.MaxAge || p.Position.y < 0.1) {
    if (p.Type == PT_SPAWNER) {
      if (p.Age > p.MaxAge) {
        [unroll] for (uint i = 0; i < 3; ++i) {
          PARTICLE p0 = PointEmitterCreate(ID+i, PT_CHILD_SPAWNER, p.Position, g_mId3, PI);
          p0.Velocity = p0.Velocity*0.5 + p.Velocity;
          ParticleStream.Append(p0);
        }
      }
      p = PointEmitterCreate(ID, PT_SPAWNER, g_vPEPosition, g_mPETransform, g_fPESpread);      
      ParticleStream.Append(p);
    }
    return;
  }
  p = EulerStep(p);
  if (p.Type == PT_SPAWNER || p.Type == PT_CHILD_SPAWNER) {
    if (CollisionDetect(p)) {
      p.Velocity = 0;
    }
  }
  ParticleStream.Append(p);
}

[MaxVertexCount(1)]
void Rain_GS(point PARTICLE input[1], uint ID : SV_PrimitiveID,
                  inout PointStream<PARTICLE> ParticleStream)
{
  PARTICLE p = input[0];
  p.Age += g_fElapsedTime;
  if (p.Type == -1) {
    float fAge = p.Age;
    p = BoxEmitterCreate(ID, PT_RAIN_DROP, g_vBEMinVertex, g_vBEMaxVertex, g_vBEVelocity);
    p.Age = fAge;
    ParticleStream.Append(p);
    return;
  }
  if (p.Age > p.MaxAge) {
    p = BoxEmitterCreate(ID, PT_RAIN_DROP, g_vBEMinVertex, g_vBEMaxVertex, g_vBEVelocity);
    ParticleStream.Append(p);
    return;
  }
  if (p.Age > 0) {
    p = EulerStep(p);
    if (CollisionDetect(p)) {
      p = BoxEmitterCreate(ID, PT_RAIN_DROP, g_vBEMinVertex, g_vBEMaxVertex, g_vBEVelocity);
    }
  }
  ParticleStream.Append(p);
}

[MaxVertexCount(4)]
void ParticleBillboard_GS(point PARTICLE input[1], inout TriangleStream<PARTICLE_BILLBOARD> ParticleStream, uniform bool bIntense)
{
  PARTICLE_TYPE pt = ParticleType(input[0].Type);
  if (!pt.Visible) return;
  if (bIntense) {
    if (!pt.Intense) return;
  } else {
    if (pt.Intense) return;
  }

  PARTICLE_BILLBOARD Output;
  PARTICLE p = input[0];
  Output.Type = p.Type;
  float fAge = p.Age / p.MaxAge;
  float fParticleSize = 0.1 * p.Size;
  Output.Color = ParticleColor(p, fAge);
  
  switch (p.Type) {    
    case PT_FIRE:
      fParticleSize *= Fire_Size(fAge);
      break;
    case PT_SMOKE:
      fParticleSize *= Smoke_Size(fAge);
      break;
    case PT_HIGHLIGHT:
      fParticleSize *= Highlight_Size(fAge);
      break;
  }
  
  float4 vVertex;
  vVertex.w = 1;  
  const float s = sin(p.Rotation);
  const float c = cos(p.Rotation);
  float3 vBase = p.Position;
  float3 vRight = normalize(mul(float3(c, s, 0), (float3x3)g_mViewInv)) * fParticleSize;
  float3 vUp = normalize(mul(float3(-s, c, 0), (float3x3)g_mViewInv)) * fParticleSize;

  // Links oben
  vVertex.xyz = vBase - vRight + vUp;
  Output.Position = mul(vVertex, g_mWorldViewProjection);
  Output.TexCoord = float2(0,0);
  ParticleStream.Append(Output);
  // Rechts oben
  vVertex.xyz = vBase + vRight + vUp;
  Output.Position = mul(vVertex, g_mWorldViewProjection);
  Output.TexCoord = float2(1,0);
  ParticleStream.Append(Output);
  // Links unten
  vVertex.xyz = vBase - vRight - vUp;
  Output.Position = mul(vVertex, g_mWorldViewProjection);
  Output.TexCoord = float2(0,1);
  ParticleStream.Append(Output);
  // Rechts unten
  vVertex.xyz = vBase + vRight - vUp;
  Output.Position = mul(vVertex, g_mWorldViewProjection);
  Output.TexCoord = float2(1,1);
  ParticleStream.Append(Output);

  ParticleStream.RestartStrip();
}

[MaxVertexCount(4)]
void RainBillboard_GS(point PARTICLE input[1], inout TriangleStream<PARTICLE_BILLBOARD> ParticleStream)
{
  PARTICLE_BILLBOARD Output;
  PARTICLE p = input[0];
  if (p.Age <= 0) return;
  Output.Type = p.Type;
  float fParticleSize = 0.1 * p.Size;
  float fAge = p.Age / p.MaxAge;
  Output.Color = ParticleColor(p, fAge);
  
  float4 vVertex;
  vVertex.w = 1;    
  float3 vBase = p.Position;
  float3 vRight = normalize(mul(float3(1, 0, 0), (float3x3)g_mViewInv)) * fParticleSize;
  float3 vUp = normalize(-p.Velocity) * fParticleSize;

  // Links oben
  vVertex.xyz = vBase - vRight + vUp;
  Output.Position = mul(vVertex, g_mWorldViewProjection);
  Output.TexCoord = float2(0,0);
  ParticleStream.Append(Output);
  // Rechts oben
  vVertex.xyz = vBase + vRight + vUp;
  Output.Position = mul(vVertex, g_mWorldViewProjection);
  Output.TexCoord = float2(1,0);
  ParticleStream.Append(Output);
  // Links unten
  vVertex.xyz = vBase - vRight - vUp;
  Output.Position = mul(vVertex, g_mWorldViewProjection);
  Output.TexCoord = float2(0,1);
  ParticleStream.Append(Output);
  // Rechts unten
  vVertex.xyz = vBase + vRight - vUp;
  Output.Position = mul(vVertex, g_mWorldViewProjection);
  Output.TexCoord = float2(1,1);
  ParticleStream.Append(Output);

  ParticleStream.RestartStrip();
}

//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------
float4 ParticlePoint_PS( PARTICLE_POINT In ) : SV_Target
{
  return In.Color;
}

float4 ParticleBillboard_PS( PARTICLE_BILLBOARD In ) : SV_Target
{
  float4 vColor;
  switch (In.Type) {
    case PT_HIGHLIGHT:
      vColor = g_tVulcanoHighlight.Sample(g_ssLinear, In.TexCoord);
      break;
    case PT_FIRE:
    case PT_SMOKE:
      vColor = g_tVulcanoFire.Sample(g_ssLinear, In.TexCoord);
      break;
    default:
      vColor = 0;
      break;
  }
  vColor *= In.Color;
  return vColor;
}

float4 RainBillboard_PS( PARTICLE_BILLBOARD In ) : SV_Target
{
  float4 vColor = g_tRainDrop.Sample(g_ssLinear, In.TexCoord);  
  vColor *= In.Color;
  return vColor;
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------
GeometryShader gsVolcanoSim = ConstructGSWithSO(
  CompileShader( gs_4_0, Volcano_GS() ),
  "POSITION.xyz; VELOCITY.xyz; AGE.x; MAXAGE.x; SIZE.x; ROTATION.x; TYPE.x");

technique10 VolcanoSim
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Particles_VS() ) );
    SetGeometryShader( gsVolcanoSim );
    SetPixelShader( NULL );
    SetDepthStencilState( dssDisableDepthStencil, 0 );
  }
}

GeometryShader gsRainSim = ConstructGSWithSO(
  CompileShader( gs_4_0, Rain_GS() ),
  "POSITION.xyz; VELOCITY.xyz; AGE.x; MAXAGE.x; SIZE.x; ROTATION.x; TYPE.x");

technique10 RainSim
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Particles_VS() ) );
    SetGeometryShader( gsRainSim );
    SetPixelShader( NULL );
    SetDepthStencilState( dssDisableDepthStencil, 0 );
  }
}

technique10 RenderParticlesPoint
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, ParticlePoint_VS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, ParticlePoint_PS() ) );
    SetDepthStencilState( dssEnableDepthNoWrite, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( NULL, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 RenderParticlesBillboard
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Particles_VS() ) );
    SetGeometryShader( CompileShader( gs_4_0, ParticleBillboard_GS(false) ) );
    SetPixelShader( CompileShader( ps_4_0, ParticleBillboard_PS() ) );
    SetDepthStencilState( dssEnableDepthNoWrite, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsSrcAlphaBlendingAdd, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 RenderParticlesBillboardIntense
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Particles_VS() ) );
    SetGeometryShader( CompileShader( gs_4_0, ParticleBillboard_GS(true) ) );
    SetPixelShader( CompileShader( ps_4_0, ParticleBillboard_PS() ) );
    SetDepthStencilState( dssEnableDepthNoWrite, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsSrcAlphaBlendingAddIntense, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 RenderRainBillboard
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, Particles_VS() ) );
    SetGeometryShader( CompileShader( gs_4_0, RainBillboard_GS() ) );
    SetPixelShader( CompileShader( ps_4_0, RainBillboard_PS() ) );
    SetDepthStencilState( dssEnableDepthNoWrite, 0 );
    SetRasterizerState( rsCullNone );
    SetBlendState( bsSrcAlphaBlendingAdd, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}