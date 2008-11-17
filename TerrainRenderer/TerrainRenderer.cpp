//--------------------------------------------------------------------------------------
// File: TerrainRenderer.cpp
//
// Starting point for new Direct3D 10 samples.  For a more basic starting point,
// use the EmptyProject10 sample instead.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "resource.h"

#include "Tile.h"
#include "LODSelector.h"
#include "FixedLODSelector.h"
#include "Scene.h"


//#define DEBUG_VS   // Uncomment this line to debug D3D9 vertex shaders
//#define DEBUG_PS   // Uncomment this line to debug D3D9 pixel shaders

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
int g_nTerrainN = 7;
float g_fTerrainR = 1.0f;
int g_nTerrainLOD = 3;

CFirstPersonCamera          g_Camera;               // A first person camera
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*            g_pTxtHelper = NULL;
CDXUTDialog                 g_HUD;                  // dialog for standard controls
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls
CDXUTDialog                 g_TerrainUI;
CDXUTDialog                 g_SfxUI[3];

// Direct3D 10 resources
ID3DX10Font*                g_pFont10 = NULL;
ID3DX10Sprite*              g_pSprite10 = NULL;
ID3D10Effect*               g_pEffect10 = NULL;
ID3D10EffectTechnique*      g_pTechnique = NULL;
ID3D10InputLayout*          g_pVertexLayout = NULL;
ID3D10EffectMatrixVariable* g_pmWorldViewProj = NULL;
ID3D10EffectMatrixVariable* g_pmWorld = NULL;
ID3D10EffectScalarVariable* g_pfTime = NULL;
ID3D10EffectVectorVariable* g_pvLightPos = NULL;
ID3D10EffectVectorVariable* g_pvLightColor = NULL;
ID3D10EffectScalarVariable* g_pfWaveHeight = NULL;
ID3D10EffectVectorVariable* g_pvWaveScale = NULL;
ID3D10EffectVectorVariable* g_pvWaveSpeed = NULL;
ID3D10EffectVectorVariable* g_pvWaterColor = NULL;
ID3D10EffectScalarVariable* g_piPhongExp = NULL;
ID3D10EffectScalarVariable* g_pfFresnelBias = NULL;
ID3D10EffectScalarVariable* g_piFresnelExp = NULL;
ID3D10EffectScalarVariable* g_pfMinHeight = NULL;
ID3D10EffectScalarVariable* g_pfMaxHeight = NULL;
ID3D10EffectScalarVariable* g_pbDynamicMinMax = NULL;
ID3D10EffectScalarVariable* g_pbDirectionalLight = NULL;
ID3D10EffectShaderResourceVariable* g_ptWaves = NULL;
ID3D10ShaderResourceView*   g_pWavesRV = NULL;

Tile*                       g_pTile = NULL;
LODSelector*                g_pLODSelector = NULL;
Scene*                      g_pScene;

bool                        g_bShowSettings = false;
bool                        g_bWireframe = false;
ID3D10RasterizerState*      g_pRSWireframe = NULL;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN        1
#define IDC_TOGGLEREF               2
#define IDC_CHANGEDEVICE            3
#define IDC_NEWTERRAIN              4
#define IDC_LODSLIDER               5
#define IDC_LODSLIDER_S             6
#define IDC_WIREFRAME               7
#define IDC_DYNAMICMINMAX           8
#define IDC_TECHNIQUE               9
#define IDC_SFX_OPTS                10

#define IDC_NEWTERRAIN_LOD          100
#define IDC_NEWTERRAIN_SIZE         101
#define IDC_NEWTERRAIN_ROUGHNESS    102
#define IDC_NEWTERRAIN_LOD_S        103
#define IDC_NEWTERRAIN_SIZE_S       104
#define IDC_NEWTERRAIN_ROUGHNESS_S  105
#define IDC_NEWTERRAIN_OK           106

#define IDC_LIGHT_S                 200
#define IDC_LIGHTX                  201
#define IDC_LIGHTY                  202
#define IDC_LIGHTZ                  203
#define IDC_LIGHTCOLOR_S            204
#define IDC_LIGHTCOLOR_R            205
#define IDC_LIGHTCOLOR_G            206
#define IDC_LIGHTCOLOR_B            207
#define IDC_LIGHTDIRECTIONAL        208

#define IDC_WAVEHEIGHT_S            300
#define IDC_WAVEHEIGHT              301
#define IDC_WAVESCALE_S             302
#define IDC_WAVESCALEX              303
#define IDC_WAVESCALEY              304
#define IDC_WAVESPEED_S             305
#define IDC_WAVESPEEDX              306
#define IDC_WAVESPEEDY              307

#define IDC_WATERCOLOR_S            400
#define IDC_WATERCOLOR_R            401
#define IDC_WATERCOLOR_G            402
#define IDC_WATERCOLOR_B            403
#define IDC_WATER_PHONG_EXP_S       404
#define IDC_WATER_PHONG_EXP         405
#define IDC_WATER_FRESNEL_BIAS_S    406
#define IDC_WATER_FRESNEL_BIAS      407
#define IDC_WATER_FRESNEL_EXP_S     408
#define IDC_WATER_FRESNEL_EXP       409


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                         bool* pbNoFurtherProcessing, void* pUserContext);
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown,
                         void* pUserContext);
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl,
                         void* pUserContext);
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext);
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings,
                                   void* pUserContext);

bool CALLBACK IsD3D10DeviceAcceptable(UINT Adapter, UINT Output,
                                      D3D10_DRIVER_TYPE DeviceType,
                                      DXGI_FORMAT BackBufferFormat,
                                      bool bWindowed, void* pUserContext);
HRESULT CALLBACK OnD3D10CreateDevice(ID3D10Device* pd3dDevice,
                                     const DXGI_SURFACE_DESC*
                                        pBackBufferSurfaceDesc,
                                     void* pUserContext );
HRESULT CALLBACK OnD3D10ResizedSwapChain(ID3D10Device* pd3dDevice,
                                         IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC*
                                            pBackBufferSurfaceDesc,
                                         void* pUserContext);
void CALLBACK OnD3D10FrameRender(ID3D10Device* pd3dDevice, double fTime,
                                 float fElapsedTime, void* pUserContext);
void CALLBACK OnD3D10ReleasingSwapChain(void* pUserContext);
void CALLBACK OnD3D10DestroyDevice(void* pUserContext);

void InitApp();
void RenderText();

ID3D10Effect* LoadEffect(ID3D10Device* pd3dDevice,
                         const std::wstring& filename,
                         const D3D10_SHADER_MACRO *Shader_Macros = NULL,
                         const bool bDebugCompile = false);


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow) {
  // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  // Set DXUT callbacks
  DXUTSetCallbackMsgProc(MsgProc);
  DXUTSetCallbackKeyboard(OnKeyboard);
  DXUTSetCallbackFrameMove(OnFrameMove);
  DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);

  DXUTSetCallbackD3D10DeviceAcceptable(IsD3D10DeviceAcceptable);
  DXUTSetCallbackD3D10DeviceCreated(OnD3D10CreateDevice);
  DXUTSetCallbackD3D10SwapChainResized(OnD3D10ResizedSwapChain);
  DXUTSetCallbackD3D10SwapChainReleasing(OnD3D10ReleasingSwapChain);
  DXUTSetCallbackD3D10DeviceDestroyed(OnD3D10DestroyDevice);
  DXUTSetCallbackD3D10FrameRender(OnD3D10FrameRender);

  InitApp();
  DXUTInit(true, true, NULL); // Parse the command line, show msgboxes on error,
                              // no extra command line params
  DXUTSetCursorSettings(true, true);
  DXUTCreateWindow(L"Terrain Renderer");
  DXUTCreateDevice(true, 800, 600);
  DXUTMainLoop(); // Enter into the DXUT render loop

  return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app
//--------------------------------------------------------------------------------------
void InitApp() {
  g_SettingsDlg.Init(&g_DialogResourceManager);
  g_HUD.Init(&g_DialogResourceManager);
  g_SampleUI.Init(&g_DialogResourceManager);
  g_TerrainUI.Init(&g_DialogResourceManager);
  g_SfxUI[0].Init(&g_DialogResourceManager);
  g_SfxUI[1].Init(&g_DialogResourceManager);
  g_SfxUI[2].Init(&g_DialogResourceManager);

  g_HUD.SetCallback(OnGUIEvent);
  int iY = 10;
  g_HUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22);
  g_HUD.AddButton(IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22,
                  VK_F3);
  g_HUD.AddButton(IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125,
                  22, VK_F2);

  /**
   * Sample UI
   */
  g_SampleUI.SetCallback(OnGUIEvent);
  iY = 10;
  g_SampleUI.AddButton(IDC_NEWTERRAIN, L"New Terrain...", 35, iY, 125,
                  22, VK_F2);
  g_SampleUI.AddCheckBox(IDC_WIREFRAME, L"Wireframe (F5)", 35, iY += 24, 125, 22,
                    g_bWireframe, VK_F5);
  g_SampleUI.AddCheckBox(IDC_DYNAMICMINMAX, L"Dynamic Min/Max", 35, iY += 24, 125,
                    22, false);

  WCHAR sz[100];
  StringCchPrintf(sz, 100, L"LOD (+/-): %d", 0);
  g_SampleUI.AddStatic(IDC_LODSLIDER_S, sz, 35, iY += 24, 125, 22);
  g_SampleUI.AddSlider(IDC_LODSLIDER, 35, iY += 24, 125, 22, 0, g_nTerrainLOD, 0);

  g_SampleUI.AddStatic(0, L"Technique:", 35, iY += 24, 125, 22);
  g_SampleUI.AddComboBox(IDC_TECHNIQUE, 35, iY += 24, 125, 22);
  g_SampleUI.GetComboBox(IDC_TECHNIQUE)->AddItem(L"Vertex Coloring", "VertexShaderColoring");
  g_SampleUI.GetComboBox(IDC_TECHNIQUE)->AddItem(L"Vertex Col. + Diffuse", "VertexShaderColoringPhong");
  g_SampleUI.GetComboBox(IDC_TECHNIQUE)->AddItem(L"Pixel Coloring", "PixelShaderColoring");
  g_SampleUI.GetComboBox(IDC_TECHNIQUE)->AddItem(L"Normal Coloring", "NormalColoring");
  g_SampleUI.GetComboBox(IDC_TECHNIQUE)->AddItem(L"Special FX", "SpecialFX");
  g_SampleUI.GetComboBox(IDC_TECHNIQUE)->AddItem(L"GouraudShading", "GouraudShading");
  g_SampleUI.GetComboBox(IDC_TECHNIQUE)->AddItem(L"PhongShading", "PhongShading");
  

  g_SampleUI.AddStatic(0, L"SFX Settings:", 35, iY += 24, 125, 22);
  g_SampleUI.AddComboBox(IDC_SFX_OPTS, 35, iY += 24, 125, 22);
  g_SampleUI.GetComboBox(IDC_SFX_OPTS)->AddItem(L"Light", NULL);
  g_SampleUI.GetComboBox(IDC_SFX_OPTS)->AddItem(L"Waves", NULL);
  g_SampleUI.GetComboBox(IDC_SFX_OPTS)->AddItem(L"Water", NULL);

  /**
   * Sfx UI 0
   */
  g_SfxUI[0].SetCallback(OnGUIEvent);
  iY = 10;
  g_SfxUI[0].AddCheckBox(IDC_LIGHTDIRECTIONAL, L"Directional?", 35, iY, 125,
      22, true);

  g_SfxUI[0].AddStatic(IDC_LIGHT_S, L"", 35, iY += 24, 125, 22);
  g_SfxUI[0].AddSlider(IDC_LIGHTX, 35, iY += 24, 125, 22, -100, 100, 14);
  g_SfxUI[0].AddSlider(IDC_LIGHTY, 35, iY += 24, 125, 22, -100, 100, 30);
  g_SfxUI[0].AddSlider(IDC_LIGHTZ, 35, iY += 24, 125, 22, -100, 100, 28);

  g_SfxUI[0].AddStatic(IDC_LIGHTCOLOR_S, L"", 35, iY += 24, 125, 22);
  g_SfxUI[0].AddSlider(IDC_LIGHTCOLOR_R, 35, iY += 24, 125, 22, 0, 100, 100);
  g_SfxUI[0].AddStatic(0, L"R:", 0, iY, 20, 22);
  g_SfxUI[0].AddSlider(IDC_LIGHTCOLOR_G, 35, iY += 24, 125, 22, 0, 100, 75);
  g_SfxUI[0].AddStatic(0, L"G:", 0, iY, 20, 22);
  g_SfxUI[0].AddSlider(IDC_LIGHTCOLOR_B, 35, iY += 24, 125, 22, 0, 100, 50);
  g_SfxUI[0].AddStatic(0, L"B:", 0, iY, 20, 22);

  /**
   * Sfx UI 1
   */
  g_SfxUI[1].SetCallback(OnGUIEvent);
  g_SfxUI[1].SetVisible(false);
  iY = 10;
  g_SfxUI[1].AddStatic(IDC_WAVEHEIGHT_S, L"", 35, iY, 125, 22);
  g_SfxUI[1].AddSlider(IDC_WAVEHEIGHT, 35, iY += 24, 125, 22, 0, 100, 3);

  g_SfxUI[1].AddStatic(IDC_WAVESCALE_S, L"", 35, iY += 24, 125, 22);
  g_SfxUI[1].AddSlider(IDC_WAVESCALEX, 35, iY += 24, 125, 22, 0, 100, 30);
  g_SfxUI[1].AddSlider(IDC_WAVESCALEY, 35, iY += 24, 125, 22, 0, 100, 25);

  g_SfxUI[1].AddStatic(IDC_WAVESPEED_S, L"", 35, iY += 24, 125, 22);
  g_SfxUI[1].AddSlider(IDC_WAVESPEEDX, 35, iY += 24, 125, 22, -100, 100, -20);
  g_SfxUI[1].AddSlider(IDC_WAVESPEEDY, 35, iY += 24, 125, 22, -100, 100, 30);

  /**
   * Sfx UI 2
   */
  g_SfxUI[2].SetCallback(OnGUIEvent);
  g_SfxUI[2].SetVisible(false);
  iY = 10;
  g_SfxUI[2].AddStatic(IDC_WATERCOLOR_S, L"", 35, iY, 125, 22);
  g_SfxUI[2].AddSlider(IDC_WATERCOLOR_R, 35, iY += 24, 125, 22, 0, 100, 50);
  g_SfxUI[2].AddStatic(0, L"R:", 0, iY, 20, 22);
  g_SfxUI[2].AddSlider(IDC_WATERCOLOR_G, 35, iY += 24, 125, 22, 0, 100, 75);
  g_SfxUI[2].AddStatic(0, L"G:", 0, iY, 20, 22);
  g_SfxUI[2].AddSlider(IDC_WATERCOLOR_B, 35, iY += 24, 125, 22, 0, 100, 100);
  g_SfxUI[2].AddStatic(0, L"B:", 0, iY, 20, 22);

  g_SfxUI[2].AddStatic(IDC_WATER_PHONG_EXP_S, L"", 35, iY += 24, 125, 22);
  g_SfxUI[2].AddSlider(IDC_WATER_PHONG_EXP, 35, iY += 24, 125, 22, 0, 1000, 200);

  g_SfxUI[2].AddStatic(IDC_WATER_FRESNEL_BIAS_S, L"", 35, iY += 24, 125, 22);
  g_SfxUI[2].AddSlider(IDC_WATER_FRESNEL_BIAS, 35, iY += 24, 125, 22, 0, 100, 40);

  g_SfxUI[2].AddStatic(IDC_WATER_FRESNEL_EXP_S, L"", 35, iY += 24, 125, 22);
  g_SfxUI[2].AddSlider(IDC_WATER_FRESNEL_EXP, 35, iY += 24, 125, 22, 0, 100, 5);

  /**
   * Terrain UI
   */
  g_TerrainUI.SetCallback(OnGUIEvent);
  iY = 10;
  StringCchPrintf(sz, 100, L"Size: %dx%d", (1<<g_nTerrainN)+1, (1<<g_nTerrainN)+1);
  g_TerrainUI.AddStatic(IDC_NEWTERRAIN_SIZE_S, sz, 0, iY, 125, 22);
  g_TerrainUI.AddSlider(IDC_NEWTERRAIN_SIZE, 0, iY += 24, 125, 22, 1, 10,
                  g_nTerrainN);

  StringCchPrintf(sz, 100, L"Roughness: %.2f", g_fTerrainR);
  g_TerrainUI.AddStatic(IDC_NEWTERRAIN_ROUGHNESS_S, sz, 0, iY += 24, 125, 22);
  g_TerrainUI.AddSlider(IDC_NEWTERRAIN_ROUGHNESS, 0, iY += 24, 125, 22, 0, 1000,
                  (int)(g_fTerrainR*100));

  StringCchPrintf(sz, 100, L"LOD Levels: %d", g_nTerrainLOD);
  g_TerrainUI.AddStatic(IDC_NEWTERRAIN_LOD_S, sz, 0, iY += 24, 125, 22);
  g_TerrainUI.AddSlider(IDC_NEWTERRAIN_LOD, 0, iY += 24, 125, 22, 0, 5,
                  g_nTerrainLOD);

  g_TerrainUI.AddButton(IDC_NEWTERRAIN_OK, L"Generate", 0, iY += 24, 125, 22);

  g_TerrainUI.SetVisible(false);

 
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText() {
  g_pTxtHelper->Begin();
  g_pTxtHelper->SetInsertionPos(5, 5);
  g_pTxtHelper->SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
  g_pTxtHelper->DrawTextLine(DXUTGetFrameStats(DXUTIsVsyncEnabled()));
  g_pTxtHelper->DrawTextLine(DXUTGetDeviceStats());

  if (g_bShowSettings) {
    g_pTxtHelper->DrawTextLine(L"Terrain Settings:");
    WCHAR sz[100];
    StringCchPrintf(sz, 100, L"Size: %dx%d", (1<<g_nTerrainN)+1, (1<<g_nTerrainN)+1);
    g_pTxtHelper->DrawTextLine(sz);
    StringCchPrintf(sz, 100, L"Roughness: %.2f", g_fTerrainR);
    g_pTxtHelper->DrawTextLine(sz);
    StringCchPrintf(sz, 100, L"LOD Levels: %d", g_nTerrainLOD);
    g_pTxtHelper->DrawTextLine(sz);
  }

  g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Reject any D3D10 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D10DeviceAcceptable(UINT Adapter, UINT Output,
                                      D3D10_DRIVER_TYPE DeviceType,
                                      DXGI_FORMAT BackBufferFormat,
                                      bool bWindowed, void* pUserContext) {
  return true;
}

/**
 * Erzeugt ein neues Terrain mit den übergebenen Parametern und bereitet es auf
 * das Rendering vor.
 */
HRESULT CreateTerrain(ID3D10Device *pd3dDevice) {
  HRESULT hr;
  SAFE_DELETE(g_pTile);
  // Tile erzeugen
  g_pTile = new Tile(g_nTerrainN, g_fTerrainR, g_nTerrainLOD);
  g_pTile->TriangulateZOrder();
  g_pTile->CalculateNormals();
  // Tile-Daten in D3D10-Buffer speichern
  V_RETURN(g_pTile->CreateBuffers(pd3dDevice));
  // Minimum und Maximum an Effekt übergeben
  g_pfMinHeight->SetFloat(g_pTile->GetMinHeight());
  g_pfMaxHeight->SetFloat(g_pTile->GetMaxHeight());
  // Lokale Tile-Daten freigeben
  g_pTile->FreeMemory();
  return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D10 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10CreateDevice(ID3D10Device* pd3dDevice,
                                     const DXGI_SURFACE_DESC*
                                        pBackBufferSurfaceDesc,
                                     void* pUserContext) {
  HRESULT hr;

  V_RETURN(D3DX10CreateSprite(pd3dDevice, 500, &g_pSprite10));
  V_RETURN(g_DialogResourceManager.OnD3D10CreateDevice(pd3dDevice));
  V_RETURN(g_SettingsDlg.OnD3D10CreateDevice(pd3dDevice));
  V_RETURN(D3DX10CreateFont(pd3dDevice, 15, 0, FW_BOLD, 1, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                            L"Arial", &g_pFont10));
  g_pTxtHelper = new CDXUTTextHelper(NULL, NULL, g_pFont10, g_pSprite10, 15);

  // Read the D3DX effect file
  g_pEffect10 = LoadEffect(pd3dDevice, L"TerrainRenderer.fx");
  g_pTechnique = g_pEffect10->GetTechniqueByIndex(0);

  // Load wave normal map
  V_RETURN(D3DX10CreateShaderResourceViewFromFile(pd3dDevice, L"waves.dds",
      NULL, NULL, &g_pWavesRV, NULL));

  // Get effects variables
  g_pmWorldViewProj =
      g_pEffect10->GetVariableByName("g_mWorldViewProjection")->AsMatrix();
  g_pmWorld = g_pEffect10->GetVariableByName("g_mWorld")->AsMatrix();
  g_pfTime = g_pEffect10->GetVariableByName("g_fTime")->AsScalar();
  g_pvLightPos = g_pEffect10->GetVariableByName("g_vLightPos")->AsVector();
  g_pvLightColor = g_pEffect10->GetVariableByName("g_vLightColor")->AsVector();
  g_pfWaveHeight = g_pEffect10->GetVariableByName("g_fWaveHeight")->AsScalar();
  g_pvWaveScale = g_pEffect10->GetVariableByName("g_vWaveScale")->AsVector();
  g_pvWaveSpeed = g_pEffect10->GetVariableByName("g_vWaveSpeed")->AsVector();
  g_pvWaterColor = g_pEffect10->GetVariableByName("g_vWaterColor")->AsVector();
  g_piPhongExp = g_pEffect10->GetVariableByName("g_iPhongExp")->AsScalar();
  g_piFresnelExp = g_pEffect10->GetVariableByName("g_iFresnelExp")->AsScalar();
  g_pfFresnelBias =
      g_pEffect10->GetVariableByName("g_fFresnelBias")->AsScalar();
  g_ptWaves = g_pEffect10->GetVariableByName("g_tWaves")->AsShaderResource();
  V_RETURN(g_ptWaves->SetResource(g_pWavesRV));
  g_pfMinHeight = g_pEffect10->GetVariableByName("g_fMinHeight")->AsScalar();
  g_pfMaxHeight = g_pEffect10->GetVariableByName("g_fMaxHeight")->AsScalar();
  g_pbDynamicMinMax =
      g_pEffect10->GetVariableByName("g_bDynamicMinMax")->AsScalar();
  g_pbDirectionalLight =
      g_pEffect10->GetVariableByName("g_bDirectionalLight")->AsScalar();

  // Flush effect vars/init GUI text
  OnGUIEvent(0, IDC_DYNAMICMINMAX, NULL, NULL);
  OnGUIEvent(0, IDC_LIGHTDIRECTIONAL, NULL, NULL);
  OnGUIEvent(0, IDC_LIGHTX, NULL, NULL);
  OnGUIEvent(0, IDC_LIGHTCOLOR_R, NULL, NULL);
  OnGUIEvent(0, IDC_WAVEHEIGHT, NULL, NULL);
  OnGUIEvent(0, IDC_WAVESCALEX, NULL, NULL);
  OnGUIEvent(0, IDC_WAVESPEEDX, NULL, NULL);
  OnGUIEvent(0, IDC_WATERCOLOR_R, NULL, NULL);
  OnGUIEvent(0, IDC_WATER_PHONG_EXP, NULL, NULL);
  OnGUIEvent(0, IDC_WATER_FRESNEL_BIAS, NULL, NULL);
  OnGUIEvent(0, IDC_WATER_FRESNEL_EXP, NULL, NULL);

  // Setup the camera's view parameters
  D3DXVECTOR3 vecEye(0.0f, 5.0f, -5.0f);
  D3DXVECTOR3 vecAt (0.0f, 0.0f, -0.0f);
  g_Camera.SetViewParams(&vecEye, &vecAt);

  // Vertex Layout festlegen
  D3D10_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
      D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0,
      D3D10_INPUT_PER_VERTEX_DATA, 0 },
  };
  UINT num_elements = sizeof(layout) / sizeof(layout[0]);
  D3D10_PASS_DESC pass_desc;
  g_pTechnique->GetPassByIndex(0)->GetDesc(&pass_desc);
  V_RETURN(pd3dDevice->CreateInputLayout(layout, num_elements,
                                         pass_desc.pIAInputSignature,
                                         pass_desc.IAInputSignatureSize,
                                         &g_pVertexLayout));

  // Terrain erzeugen
  V_RETURN(CreateTerrain(pd3dDevice));

  // FixedLODSelector mit LOD-Stufe 0
  g_pLODSelector = new FixedLODSelector(0);

  // RasterizerState für Wireframe erzeugen
  D3D10_RASTERIZER_DESC rast_desc = {
    D3D10_FILL_WIREFRAME, D3D10_CULL_BACK,
    FALSE,
    0, 0.0f, 0.0f,
    TRUE, FALSE, FALSE, FALSE
  };
  V_RETURN(pd3dDevice->CreateRasterizerState(&rast_desc, &g_pRSWireframe));

  // Szene erstellen
  g_pScene = new Scene(0.05f, 0.45f, 0.5f, 100);
  g_pScene->GetShaderHandles(g_pEffect10);

  // Lichter hinzufügen
  /*g_pScene->AddPointLight(D3DXVECTOR3(-2.5f, 1.0f, 2.5f),
                          D3DXVECTOR3(1, 0, 1),
                          D3DXVECTOR3(1, 0, 0));
  g_pScene->AddPointLight(D3DXVECTOR3(2.5f, 1.0f, -2.5f),
                          D3DXVECTOR3(0, 1, 1),
                          D3DXVECTOR3(1, 0, 0));
  g_pScene->AddDirectionalLight(D3DXVECTOR3(1.0f, 0.0f, 0.0f),
                                D3DXVECTOR3(1, 1, 0),
                                D3DXVECTOR3(1, 0, 0));*/
  g_pScene->AddSpotLight(D3DXVECTOR3(2.5f, 3.0f, 0.0f),
                         D3DXVECTOR3(0, -1, 0),
                         D3DXVECTOR3(1, 1, 0),
                         D3DXVECTOR3(1, 0, 0),
                         .5f, 5);
  g_pScene->AddSpotLight(D3DXVECTOR3(-2.5f, 3.0f, 0.0f),
                         D3DXVECTOR3(0, -1, 0),
                         D3DXVECTOR3(0, 1, 1),
                         D3DXVECTOR3(1, 0, 0),
                         .5f, 5);
  g_pScene->AddSpotLight(D3DXVECTOR3(0.0f, 3.0f, 2.5f),
                         D3DXVECTOR3(0, -1, 0),
                         D3DXVECTOR3(1, 0, 1),
                         D3DXVECTOR3(1, 0, 0),
                         .5f, 5);
  g_pScene->AddSpotLight(D3DXVECTOR3(0.0f, 3.0f, -2.5f),
                         D3DXVECTOR3(0, -1, 0),
                         D3DXVECTOR3(1, 1, 1),
                         D3DXVECTOR3(1, 0, 0),
                         .5f, 5);
  return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D10 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10ResizedSwapChain(ID3D10Device* pd3dDevice,
                                         IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC*
                                            pBackBufferSurfaceDesc,
                                         void* pUserContext) {
  HRESULT hr;

  V_RETURN(g_DialogResourceManager.OnD3D10ResizedSwapChain(
      pd3dDevice,
      pBackBufferSurfaceDesc));
  V_RETURN(g_SettingsDlg.OnD3D10ResizedSwapChain(pd3dDevice,
                                                 pBackBufferSurfaceDesc));

  // Setup the camera's projection parameters
  float fAspectRatio = pBackBufferSurfaceDesc->Width /
      (FLOAT)pBackBufferSurfaceDesc->Height;
  g_Camera.SetProjParams(D3DX_PI / 4, fAspectRatio, 0.1f, 1000.0f);

  g_HUD.SetLocation(pBackBufferSurfaceDesc->Width - 170, 0);
  g_HUD.SetSize(170, 170);
  g_SampleUI.SetLocation(pBackBufferSurfaceDesc->Width - 170, 80);
  g_SampleUI.SetSize(170, 300);
  g_TerrainUI.SetLocation(pBackBufferSurfaceDesc->Width - 320, 0);
  g_TerrainUI.SetSize(150, 300);
  g_SfxUI[0].SetLocation(pBackBufferSurfaceDesc->Width - 170, 300);
  g_SfxUI[0].SetSize(170, 300);
  g_SfxUI[1].SetLocation(pBackBufferSurfaceDesc->Width - 170, 300);
  g_SfxUI[1].SetSize(170, 300);
  g_SfxUI[2].SetLocation(pBackBufferSurfaceDesc->Width - 170, 300);
  g_SfxUI[2].SetSize(170, 300);

  return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D10 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10FrameRender(ID3D10Device* pd3dDevice, double fTime,
                                 float fElapsedTime, void* pUserContext) {
  D3DXMATRIX mWorldViewProjection;
  D3DXMATRIX mWorld;
  D3DXMATRIX mView;
  D3DXMATRIX mProj;

  float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  ID3D10RenderTargetView* pRTV = DXUTGetD3D10RenderTargetView();
  pd3dDevice->ClearRenderTargetView(pRTV, ClearColor);

  // Clear the depth stencil
  ID3D10DepthStencilView* pDSV = DXUTGetD3D10DepthStencilView();
  pd3dDevice->ClearDepthStencilView(pDSV, D3D10_CLEAR_DEPTH, 1.0, 0);

  // If the settings dialog is being shown, then render it instead of rendering the app's scene
  if (g_SettingsDlg.IsActive()) {
    g_SettingsDlg.OnRender(fElapsedTime);
    return;
  }

  // Get the projection & view matrix from the camera class
  mWorld = *g_Camera.GetWorldMatrix();
  mProj = *g_Camera.GetProjMatrix();
  mView = *g_Camera.GetViewMatrix();
  mWorldViewProjection = mView * mProj;

  // Update the effect's variables
  g_pmWorldViewProj->SetMatrix((float*)&mWorldViewProjection);
  g_pmWorld->SetMatrix((float*)&mWorld);
  g_pfTime->SetFloat((float)fTime);
  
  // Set vertex Layout
  pd3dDevice->IASetInputLayout(g_pVertexLayout);

  if (g_bWireframe) pd3dDevice->RSSetState(g_pRSWireframe);

  D3D10_TECHNIQUE_DESC tech_desc;
  g_pTechnique->GetDesc(&tech_desc);
  for (UINT p = 0; p < tech_desc.Passes; ++p) {
    g_pTechnique->GetPassByIndex(p)->Apply(0);
    g_pTile->Draw(pd3dDevice, g_pLODSelector, g_Camera.GetEyePt());
  }

  if (g_bWireframe) pd3dDevice->RSSetState(NULL);

  DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"HUD / Stats");
  RenderText();
  g_HUD.OnRender(fElapsedTime);
  g_SfxUI[0].OnRender(fElapsedTime);
  g_SfxUI[1].OnRender(fElapsedTime);
  g_SfxUI[2].OnRender(fElapsedTime);
  g_SampleUI.OnRender(fElapsedTime);
  g_TerrainUI.OnRender(fElapsedTime);
  DXUT_EndPerfEvent();
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10ResizedSwapChain
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10ReleasingSwapChain(void* pUserContext) {
  g_DialogResourceManager.OnD3D10ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10CreateDevice
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10DestroyDevice(void* pUserContext) {
  g_DialogResourceManager.OnD3D10DestroyDevice();
  g_SettingsDlg.OnD3D10DestroyDevice();
  SAFE_RELEASE(g_pFont10);
  SAFE_RELEASE(g_pEffect10);
  SAFE_RELEASE(g_pVertexLayout);
  SAFE_RELEASE(g_pSprite10);
  SAFE_RELEASE(g_pRSWireframe);
  SAFE_RELEASE(g_pWavesRV);
  SAFE_DELETE(g_pTxtHelper);
  SAFE_DELETE(g_pTile);
  SAFE_DELETE(g_pLODSelector);
  SAFE_DELETE(g_pScene);
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings,
                                   void* pUserContext) {
  if(pDeviceSettings->ver == DXUT_D3D9_DEVICE) {
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    D3DCAPS9 Caps;
    pD3D->GetDeviceCaps(pDeviceSettings->d3d9.AdapterOrdinal,
                        pDeviceSettings->d3d9.DeviceType, &Caps);

    // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW
    // then switch to SWVP.
    if ((Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
        Caps.VertexShaderVersion < D3DVS_VERSION(1, 1)) {
      pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }

    // Debugging vertex shaders requires either REF or software vertex processing
    // and debugging pixel shaders requires REF.
#ifdef DEBUG_VS
    if (pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF) {
      pDeviceSettings->d3d9.BehaviorFlags &=
          ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
      pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
      pDeviceSettings->d3d9.BehaviorFlags |=
          D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }
#endif
#ifdef DEBUG_PS
    pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
  }

  // For the first device created if its a REF device, optionally display a warning dialog box
  static bool s_bFirstTime = true;
  if (s_bFirstTime) {
    s_bFirstTime = false;
    if ((DXUT_D3D9_DEVICE == pDeviceSettings->ver &&
         pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF) ||
        (DXUT_D3D10_DEVICE == pDeviceSettings->ver &&
        pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE)) {
      DXUTDisplaySwitchingToREFWarning(pDeviceSettings->ver);
    }
  }

  return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime,
                          void* pUserContext) {
  // Update the camera's position based on user input
  g_Camera.FrameMove(fElapsedTime);
  g_pScene->OnFrameMove(fElapsedTime, *g_Camera.GetEyePt());
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                         bool* pbNoFurtherProcessing, void* pUserContext) {
  // Pass messages to dialog resource manager calls so GUI state is updated correctly
  *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc(hWnd,
                                                           uMsg,
                                                           wParam,
                                                           lParam);
  if (*pbNoFurtherProcessing)
    return 0;

  // Pass messages to settings dialog if its active
  if (g_SettingsDlg.IsActive()) {
    g_SettingsDlg.MsgProc(hWnd, uMsg, wParam, lParam);
    return 0;
  }

  // Give the dialogs a chance to handle the message first
  *pbNoFurtherProcessing = g_HUD.MsgProc(hWnd, uMsg, wParam, lParam);
  if (*pbNoFurtherProcessing)
    return 0;
  *pbNoFurtherProcessing = g_SampleUI.MsgProc(hWnd, uMsg, wParam, lParam);
  if (*pbNoFurtherProcessing)
    return 0;
  *pbNoFurtherProcessing = g_TerrainUI.MsgProc(hWnd, uMsg, wParam, lParam);
  if (*pbNoFurtherProcessing)
    return 0;
  *pbNoFurtherProcessing = g_SfxUI[0].MsgProc(hWnd, uMsg, wParam, lParam);
  if (*pbNoFurtherProcessing)
    return 0;
  *pbNoFurtherProcessing = g_SfxUI[1].MsgProc(hWnd, uMsg, wParam, lParam);
  if (*pbNoFurtherProcessing)
    return 0;
  *pbNoFurtherProcessing = g_SfxUI[2].MsgProc(hWnd, uMsg, wParam, lParam);
  if (*pbNoFurtherProcessing)
    return 0;

  // Pass all remaining windows messages to camera so it can respond to user input
  g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);

  return 0;
}

void SetLOD(int lod) {
  g_SampleUI.GetSlider(IDC_LODSLIDER)->SetValue(lod);
  // Wert neu auslesen, da er evtl. auf den gültigen Bereich geclampt wurde
  lod = g_SampleUI.GetSlider(IDC_LODSLIDER)->GetValue();
  SAFE_DELETE(g_pLODSelector);
  g_pLODSelector = new FixedLODSelector(lod);
  WCHAR sz[100];
  StringCchPrintf(sz, 100, L"LOD (+/-): %d", lod);
  g_SampleUI.GetStatic(IDC_LODSLIDER_S)->SetText(sz);
}

//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown,
                         void* pUserContext) {
  if (!bKeyDown) return;
  switch (nChar) {
    case VK_ADD:
      SetLOD(g_SampleUI.GetSlider(IDC_LODSLIDER)->GetValue() + 1);
      break;
    case VK_SUBTRACT:
      SetLOD(g_SampleUI.GetSlider(IDC_LODSLIDER)->GetValue() - 1);
      break;
    case 'H':
    case 'h':
      g_bShowSettings = !g_bShowSettings;
      break;
  }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl,
                         void* pUserContext) {
  switch (nControlID) {
    case IDC_TOGGLEFULLSCREEN:
      DXUTToggleFullScreen(); break;
    case IDC_TOGGLEREF:
      DXUTToggleREF(); break;
    case IDC_CHANGEDEVICE:
      g_SettingsDlg.SetActive(!g_SettingsDlg.IsActive()); break;

    /**
     * Sample UI
     */
    case IDC_LODSLIDER:
      SetLOD(g_SampleUI.GetSlider(IDC_LODSLIDER)->GetValue());
      break;
    case IDC_WIREFRAME:
      g_bWireframe = g_SampleUI.GetCheckBox(IDC_WIREFRAME)->GetChecked();
      break;
    case IDC_DYNAMICMINMAX:
      g_pbDynamicMinMax->SetBool(
          g_SampleUI.GetCheckBox(IDC_DYNAMICMINMAX)->GetChecked());
      break;
    case IDC_NEWTERRAIN:
      g_TerrainUI.SetVisible(true);
      break;
    case IDC_TECHNIQUE: {
      char *tech =
          (char *)g_SampleUI.GetComboBox(IDC_TECHNIQUE)->GetSelectedData();
      g_pTechnique = g_pEffect10->GetTechniqueByName(tech);
      break;
    }
    case IDC_SFX_OPTS: {
      int ui = g_SampleUI.GetComboBox(IDC_SFX_OPTS)->GetSelectedIndex();
      for (int i = 0; i < 3; ++i) g_SfxUI[i].SetVisible(false);
      g_SfxUI[ui].SetVisible(true);
      break;
    }

    /**
     * Terrain UI
     */
    case IDC_NEWTERRAIN_SIZE: {
      int value =
          (1 << g_TerrainUI.GetSlider(IDC_NEWTERRAIN_SIZE)->GetValue()) + 1;
      WCHAR sz[100];
      StringCchPrintf(sz, 100, L"Size: %dx%d", value, value);
      g_TerrainUI.GetStatic(IDC_NEWTERRAIN_SIZE_S)->SetText(sz);
      break;
    }
    case IDC_NEWTERRAIN_ROUGHNESS: {
      float value =
          g_TerrainUI.GetSlider(IDC_NEWTERRAIN_ROUGHNESS)->GetValue() / 100.0f;
      WCHAR sz[100];
      StringCchPrintf(sz, 100, L"Roughness: %.2f", value);
      g_TerrainUI.GetStatic(IDC_NEWTERRAIN_ROUGHNESS_S)->SetText(sz);
      break;
    }
    case IDC_NEWTERRAIN_LOD: {
      int value = g_TerrainUI.GetSlider(IDC_NEWTERRAIN_LOD)->GetValue();
      WCHAR sz[100];
      StringCchPrintf(sz, 100, L"LOD Levels: %d", value);
      g_TerrainUI.GetStatic(IDC_NEWTERRAIN_LOD_S)->SetText(sz);
      break;
    }
    case IDC_NEWTERRAIN_OK: {
      g_TerrainUI.SetVisible(false);

      g_nTerrainN = g_TerrainUI.GetSlider(IDC_NEWTERRAIN_SIZE)->GetValue();
      g_fTerrainR =
          g_TerrainUI.GetSlider(IDC_NEWTERRAIN_ROUGHNESS)->GetValue()/100.0f;
      g_nTerrainLOD = g_TerrainUI.GetSlider(IDC_NEWTERRAIN_LOD)->GetValue();

      CreateTerrain(DXUTGetD3D10Device());

      g_SampleUI.GetSlider(IDC_LODSLIDER)->SetValue(0);
      g_SampleUI.GetSlider(IDC_LODSLIDER)->SetRange(0, g_nTerrainLOD);
      g_SampleUI.GetStatic(IDC_LODSLIDER_S)->SetText(L"LOD (+/-): 0");
      SAFE_DELETE(g_pLODSelector);
      g_pLODSelector = new FixedLODSelector(0);
      break;
    }

    /**
     * Sfx UI 0
     */
    case IDC_LIGHTDIRECTIONAL:
      g_pbDirectionalLight->SetBool(
          g_SfxUI[0].GetCheckBox(IDC_LIGHTDIRECTIONAL)->GetChecked());
      // Lazy fallthrough for updating IDC_LIGHT_S
    case IDC_LIGHTX:
    case IDC_LIGHTY:
    case IDC_LIGHTZ: {
      D3DXVECTOR3 light_pos = D3DXVECTOR3(
        g_SfxUI[0].GetSlider(IDC_LIGHTX)->GetValue() / 10.f,
        g_SfxUI[0].GetSlider(IDC_LIGHTY)->GetValue() / 10.f,
        g_SfxUI[0].GetSlider(IDC_LIGHTZ)->GetValue() / 10.f
      );
      WCHAR sz[100];
      bool dir = g_SfxUI[0].GetCheckBox(IDC_LIGHTDIRECTIONAL)->GetChecked();
      if (dir) {
        StringCchPrintf(sz, 100, L"Direction: (%.1f, %.1f, %.1f)",
                      light_pos.x, light_pos.y, light_pos.z);
      } else {
        StringCchPrintf(sz, 100, L"Position: (%.1f, %.1f, %.1f)",
                      light_pos.x, light_pos.y, light_pos.z);
      }
      g_SfxUI[0].GetStatic(IDC_LIGHT_S)->SetText(sz);
      g_pvLightPos->SetFloatVector(light_pos);
      break;
    }

    case IDC_LIGHTCOLOR_R:
    case IDC_LIGHTCOLOR_G:
    case IDC_LIGHTCOLOR_B: {
      D3DXVECTOR3 light_col = D3DXVECTOR3(
        g_SfxUI[0].GetSlider(IDC_LIGHTCOLOR_R)->GetValue() / 100.f,
        g_SfxUI[0].GetSlider(IDC_LIGHTCOLOR_G)->GetValue() / 100.f,
        g_SfxUI[0].GetSlider(IDC_LIGHTCOLOR_B)->GetValue() / 100.f
      );
      WCHAR sz[100];
      StringCchPrintf(sz, 100, L"Color: (%.2f, %.2f, %.2f)",
                      light_col.x, light_col.y, light_col.z);
      g_SfxUI[0].GetStatic(IDC_LIGHTCOLOR_S)->SetText(sz);
      g_pvLightColor->SetFloatVector(light_col);
      break;
    }

    /**
     * Sfx UI 1
     */
    case IDC_WAVEHEIGHT: {
      float value = g_SfxUI[1].GetSlider(IDC_WAVEHEIGHT)->GetValue() / 100.0f;
      WCHAR sz[100];
      StringCchPrintf(sz, 100, L"Wave Height: %.2f", value);
      g_SfxUI[1].GetStatic(IDC_WAVEHEIGHT_S)->SetText(sz);
      g_pfWaveHeight->SetFloat(value);
      break;
    }

    case IDC_WAVESCALEX:
    case IDC_WAVESCALEY: {
      D3DXVECTOR2 wave_scale = D3DXVECTOR2(
        g_SfxUI[1].GetSlider(IDC_WAVESCALEX)->GetValue() / 10.f,
        g_SfxUI[1].GetSlider(IDC_WAVESCALEY)->GetValue() / 10.f
      );
      WCHAR sz[100];
      StringCchPrintf(sz, 100, L"Wave Scale: (%.1f, %.1f)",
                      wave_scale.x, wave_scale.y);
      g_SfxUI[1].GetStatic(IDC_WAVESCALE_S)->SetText(sz);
      g_pvWaveScale->SetFloatVector(wave_scale);
      break;
    }

    case IDC_WAVESPEEDX:
    case IDC_WAVESPEEDY: {
      D3DXVECTOR2 wave_speed = D3DXVECTOR2(
        g_SfxUI[1].GetSlider(IDC_WAVESPEEDX)->GetValue() / 10.f,
        g_SfxUI[1].GetSlider(IDC_WAVESPEEDY)->GetValue() / 10.f
      );
      WCHAR sz[100];
      StringCchPrintf(sz, 100, L"Wave Speed: (%.1f, %.1f)",
                      wave_speed.x, wave_speed.y);
      g_SfxUI[1].GetStatic(IDC_WAVESPEED_S)->SetText(sz);
      g_pvWaveSpeed->SetFloatVector(wave_speed);
      break;
    }

    /**
     * Sfx UI 2
     */
    case IDC_WATERCOLOR_R:
    case IDC_WATERCOLOR_G:
    case IDC_WATERCOLOR_B: {
      D3DXVECTOR3 water_col = D3DXVECTOR3(
        g_SfxUI[2].GetSlider(IDC_WATERCOLOR_R)->GetValue() / 100.f,
        g_SfxUI[2].GetSlider(IDC_WATERCOLOR_G)->GetValue() / 100.f,
        g_SfxUI[2].GetSlider(IDC_WATERCOLOR_B)->GetValue() / 100.f
      );
      WCHAR sz[100];
      StringCchPrintf(sz, 100, L"Color: (%.2f, %.2f, %.2f)",
                      water_col.x, water_col.y, water_col.z);
      g_SfxUI[2].GetStatic(IDC_WATERCOLOR_S)->SetText(sz);
      g_pvWaterColor->SetFloatVector(water_col);
      break;
    }

    case IDC_WATER_PHONG_EXP: {
      int value = g_SfxUI[2].GetSlider(IDC_WATER_PHONG_EXP)->GetValue();
      WCHAR sz[100];
      StringCchPrintf(sz, 100, L"Phong Exponent: %d", value);
      g_SfxUI[2].GetStatic(IDC_WATER_PHONG_EXP_S)->SetText(sz);
      g_piPhongExp->SetInt(value);
      break;
    }

    case IDC_WATER_FRESNEL_BIAS: {
      float value =
          g_SfxUI[2].GetSlider(IDC_WATER_FRESNEL_BIAS)->GetValue() / 100.f;
      WCHAR sz[100];
      StringCchPrintf(sz, 100, L"Fresnel Bias: %.2f", value);
      g_SfxUI[2].GetStatic(IDC_WATER_FRESNEL_BIAS_S)->SetText(sz);
      g_pfFresnelBias->SetFloat(value);
      break;
    }

    case IDC_WATER_FRESNEL_EXP: {
      int value = g_SfxUI[2].GetSlider(IDC_WATER_FRESNEL_EXP)->GetValue();
      WCHAR sz[100];
      StringCchPrintf(sz, 100, L"Fresnel Exponent: %d", value);
      g_SfxUI[2].GetStatic(IDC_WATER_FRESNEL_EXP_S)->SetText(sz);
      g_piFresnelExp->SetInt(value);
      break;
    }
  }
}
