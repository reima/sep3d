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
#include "Environment.h"
#include "ShadowedDirectionalLight.h"
#include "ShadowedPointLight.h"


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

// Direct3D 10 resources
ID3DX10Font*                g_pFont10 = NULL;
ID3DX10Sprite*              g_pSprite10 = NULL;
ID3D10Effect*               g_pEffect10 = NULL;
ID3D10EffectTechnique*      g_pTechnique = NULL;
ID3D10InputLayout*          g_pVertexLayout = NULL;
ID3D10EffectMatrixVariable* g_pmWorldViewProj = NULL;
ID3D10EffectMatrixVariable* g_pmWorld = NULL;
ID3D10EffectScalarVariable* g_pfTime = NULL;
ID3D10EffectScalarVariable* g_pfMinHeight = NULL;
ID3D10EffectScalarVariable* g_pfMaxHeight = NULL;
ID3D10EffectScalarVariable* g_pbDynamicMinMax = NULL;
ID3D10EffectShaderResourceVariable* g_ptWaves = NULL;
ID3D10ShaderResourceView*   g_pWavesRV = NULL;
ID3D10EffectShaderResourceVariable* g_ptGround = NULL;
ID3D10ShaderResourceView*   g_pGroundRV = NULL;
ID3D10EffectShaderResourceVariable* g_ptGround3D = NULL;
ID3D10ShaderResourceView*   g_pGround3DRV = NULL;
ID3D10EffectShaderResourceVariable* g_ptCubeMap = NULL;
ID3D10ShaderResourceView*   g_pCubeMapRV = NULL;
ID3D10EffectScalarVariable* g_pbWaveNormals = NULL;
ID3D10EffectScalarVariable* g_pfZEpsilon = NULL;

LODSelector*                g_pLODSelector = NULL;
Scene*                      g_pScene = NULL;
Environment*                g_pEnvironment = NULL;

ShadowedDirectionalLight*   g_pShadowedDirectionalLight = NULL;
ShadowedPointLight*         g_pShadowedPointLight = NULL;

bool                        g_bShowSettings = false;
bool                        g_bWireframe = false;
bool                        g_bPaused = false;
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
#define IDC_PAUSED                  7
#define IDC_WIREFRAME               8
#define IDC_DYNAMICMINMAX           9
#define IDC_WAVENORMALS             10
#define IDC_TECHNIQUE               11
#define IDC_SHADOWMAPS_RESOLUTION   12
#define IDC_SHADOWMAPS_RESOLUTION_S 13
#define IDC_SHADOWMAPS_PRECISION    14
#define IDC_SHADOWMAPS_ZEPSILON     15
#define IDC_SHADOWMAPS_ZEPSILON_S   16

#define IDC_NEWTERRAIN_LOD          100
#define IDC_NEWTERRAIN_SIZE         101
#define IDC_NEWTERRAIN_ROUGHNESS    102
#define IDC_NEWTERRAIN_LOD_S        103
#define IDC_NEWTERRAIN_SIZE_S       104
#define IDC_NEWTERRAIN_ROUGHNESS_S  105
#define IDC_NEWTERRAIN_OK           106

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
  g_SampleUI.AddCheckBox(IDC_WIREFRAME, L"Wireframe (F5)", 35, iY += 24, 125,
                         22, g_bWireframe, VK_F5);
  g_SampleUI.AddCheckBox(IDC_PAUSED, L"Paused (F6)", 35, iY += 24, 125, 22,
                         g_bPaused, VK_F6);
  g_SampleUI.AddCheckBox(IDC_DYNAMICMINMAX, L"Dynamic Min/Max (F7)", 35,
                         iY += 24, 125, 22, false, VK_F7);
  g_SampleUI.AddCheckBox(IDC_WAVENORMALS, L"Wave normals (F8)", 35, iY += 24,
                         125, 22, true, VK_F8);


  WCHAR sz[100];
  StringCchPrintf(sz, 100, L"LOD (+/-): %d", 0);
  g_SampleUI.AddStatic(IDC_LODSLIDER_S, sz, 35, iY += 24, 125, 22);
  g_SampleUI.AddSlider(IDC_LODSLIDER, 35, iY += 24, 125, 22, 0, g_nTerrainLOD, 0);

  g_SampleUI.AddStatic(0, L"Technique:", 35, iY += 24, 125, 22);
  g_SampleUI.AddComboBox(IDC_TECHNIQUE, 35, iY += 24, 125, 22);
  //g_SampleUI.GetComboBox(IDC_TECHNIQUE)->AddItem(L"GouraudShading", "GouraudShading");
  g_SampleUI.GetComboBox(IDC_TECHNIQUE)->AddItem(L"PhongShading", "PhongShading");

  StringCchPrintf(sz, 100, L"SM Res.: %dx%d", 1024, 1024);
  g_SampleUI.AddStatic(IDC_SHADOWMAPS_RESOLUTION_S, sz, 35, iY += 24, 125, 22);
  g_SampleUI.AddSlider(IDC_SHADOWMAPS_RESOLUTION, 35, iY += 24, 125, 22, 4, 12, 10);
  
  g_SampleUI.AddCheckBox(IDC_SHADOWMAPS_PRECISION, L"High Prec. SM", 35,
                         iY += 24, 125, 22, true);

  StringCchPrintf(sz, 100, L"Z epsilon: %.4f", 0.010f);
  g_SampleUI.AddStatic(IDC_SHADOWMAPS_ZEPSILON_S, sz, 35, iY += 24, 125, 22);
  g_SampleUI.AddSlider(IDC_SHADOWMAPS_ZEPSILON, 35, iY += 24, 125, 22, 0, 1000, 100);


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
  V_RETURN(D3DX10CreateShaderResourceViewFromFile(pd3dDevice,
      L"Textures/waves.dds", NULL, NULL, &g_pWavesRV, NULL));

  // Load ground texture
  V_RETURN(D3DX10CreateShaderResourceViewFromFile(pd3dDevice,
     L"Textures/checker1.png", NULL, NULL, &g_pGroundRV, NULL));
  
  // Load ground texture 3d
  V_RETURN(D3DX10CreateShaderResourceViewFromFile(pd3dDevice,
     L"Textures/terrain.dds", NULL, NULL, &g_pGround3DRV, NULL));

  // Load cube map
  V_RETURN(D3DX10CreateShaderResourceViewFromFile(pd3dDevice,
      L"Textures/SouthernSea.dds", NULL, NULL, &g_pCubeMapRV, NULL));

  // Get effects variables
  g_pmWorldViewProj =
      g_pEffect10->GetVariableByName("g_mWorldViewProjection")->AsMatrix();
  g_pmWorld = g_pEffect10->GetVariableByName("g_mWorld")->AsMatrix();
  g_pfTime = g_pEffect10->GetVariableByName("g_fTime")->AsScalar();
  g_ptWaves = g_pEffect10->GetVariableByName("g_tWaves")->AsShaderResource();
  V_RETURN(g_ptWaves->SetResource(g_pWavesRV));
  g_ptGround = g_pEffect10->GetVariableByName("g_tGround")->AsShaderResource();
  V_RETURN(g_ptGround->SetResource(g_pGroundRV));
  g_ptGround3D = g_pEffect10->GetVariableByName("g_tGround3D")->AsShaderResource();
  V_RETURN(g_ptGround3D->SetResource(g_pGround3DRV));
  g_ptCubeMap =
      g_pEffect10->GetVariableByName("g_tCubeMap")->AsShaderResource();
  V_RETURN(g_ptCubeMap->SetResource(g_pCubeMapRV));
  g_pfMinHeight = g_pEffect10->GetVariableByName("g_fMinHeight")->AsScalar();
  g_pfMaxHeight = g_pEffect10->GetVariableByName("g_fMaxHeight")->AsScalar();
  g_pbDynamicMinMax =
      g_pEffect10->GetVariableByName("g_bDynamicMinMax")->AsScalar();
  g_pbWaveNormals =
      g_pEffect10->GetVariableByName("g_bWaveNormals")->AsScalar();
  g_pfZEpsilon =
      g_pEffect10->GetVariableByName("g_fZEpsilon")->AsScalar();

  // Flush effect vars/init GUI text
  OnGUIEvent(0, IDC_DYNAMICMINMAX, NULL, NULL);
  OnGUIEvent(0, IDC_WAVENORMALS, NULL, NULL);
  OnGUIEvent(0, IDC_SHADOWMAPS_ZEPSILON, NULL, NULL);

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

  pd3dDevice->IASetInputLayout(g_pVertexLayout);

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
  g_pScene = new Scene();
  g_pScene->OnCreateDevice(pd3dDevice);
  g_pScene->GetShaderHandles(g_pEffect10);
  g_pScene->SetMaterial(0.05f, 0.9f, 0.05f, 50);
  g_pScene->SetCamera(&g_Camera);
  g_pScene->SetLODSelector(g_pLODSelector);
  
  // Terrain erzeugen
  g_pScene->CreateTerrain(g_nTerrainN, g_fTerrainR, g_nTerrainLOD);  
  Tile *terrain = g_pScene->GetTerrain();
  g_pfMinHeight->SetFloat(terrain->GetMinHeight());
  g_pfMaxHeight->SetFloat(terrain->GetMaxHeight());

  // Lichter hinzufügen
  //g_pScene->AddPointLight(D3DXVECTOR3(-2.5f, 0.0f, 0.0f),
  //                        D3DXVECTOR3(1, 0, 0),
  //                        D3DXVECTOR3(0, 0, 1));
  //g_pScene->AddPointLight(D3DXVECTOR3(0.0f, 0.0f, -2.5f),
  //                        D3DXVECTOR3(0, 1, 0),
  //                        D3DXVECTOR3(1, 0, 0));
  //g_pScene->AddDirectionalLight(D3DXVECTOR3(1.0f, 1.0f, 0.0f),
  //                              D3DXVECTOR3(1, 0.75f, 0.5f),
  //                              D3DXVECTOR3(0, 1, 0));
  //g_pScene->AddSpotLight(D3DXVECTOR3(2.5f, 3.0f, 0.0f),
  //                       D3DXVECTOR3(0, -1, 0),
  //                       D3DXVECTOR3(1, 1, 0),
  //                       D3DXVECTOR3(0, 1, 0),
  //                       .5f, 5);
  //g_pScene->AddSpotLight(D3DXVECTOR3(-2.5f, 3.0f, 0.0f),
  //                       D3DXVECTOR3(0, -1, 0),
  //                       D3DXVECTOR3(0, 1, 1),
  //                       D3DXVECTOR3(0, 1, 0),
  //                       .5f, 5);
  //g_pScene->AddSpotLight(D3DXVECTOR3(0.0f, 3.0f, 2.5f),
  //                       D3DXVECTOR3(0, -1, 0),
  //                       D3DXVECTOR3(1, 0, 1),
  //                       D3DXVECTOR3(0, 1, 0),
  //                       .5f, 5);
  //g_pScene->AddSpotLight(D3DXVECTOR3(0.0f, 3.0f, -2.5f),
  //                       D3DXVECTOR3(0, -1, 0),
  //                       D3DXVECTOR3(1, 1, 1),
  //                       D3DXVECTOR3(0, 1, 0),
  //                       .5f, 5);
  //g_pScene->AddDirectionalLight(
  //    D3DXVECTOR3(1.0f, 0.25f, 0.0f),
  //    D3DXVECTOR3(1, 0.75f, 0.5f),
  //    D3DXVECTOR3(0, 0.2f, 0),
  //    true);
  g_pScene->AddPointLight(
      D3DXVECTOR3(-1, 1, 0),
      D3DXVECTOR3(1, 0, 0),
      D3DXVECTOR3(0, 1, 0),
      true);

  // Environment erstellen
  g_pEnvironment = new Environment(pd3dDevice);
  g_pEnvironment->GetShaderHandles(g_pEffect10);  

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

  if (g_pEnvironment) {
    g_pEnvironment->SetBackBufferSize(pBackBufferSurfaceDesc->Width,
                                      pBackBufferSurfaceDesc->Height);
  }

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

  //
  // Render the scene.
  //

  pd3dDevice->IASetInputLayout(g_pVertexLayout);
  if (g_bWireframe) pd3dDevice->RSSetState(g_pRSWireframe);

  D3D10_TECHNIQUE_DESC tech_desc;
  g_pTechnique->GetDesc(&tech_desc);
  for (UINT p = 0; p < tech_desc.Passes; ++p) {
    g_pTechnique->GetPassByIndex(p)->Apply(0);
    g_pScene->Draw();
  }

  //
  // Render the environment
  //
  g_pEnvironment->OnFrameMove(&mView, D3DX_PI / 4);
  g_pEnvironment->Draw(pd3dDevice);

  if (g_bWireframe) pd3dDevice->RSSetState(NULL);

  DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"HUD / Stats");
  RenderText();
  g_HUD.OnRender(fElapsedTime);
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
  SAFE_RELEASE(g_pGroundRV);
  SAFE_RELEASE(g_pGround3DRV);
  SAFE_RELEASE(g_pWavesRV);
  SAFE_RELEASE(g_pCubeMapRV);
  SAFE_DELETE(g_pTxtHelper);
  SAFE_DELETE(g_pLODSelector);
  SAFE_DELETE(g_pScene);
  SAFE_DELETE(g_pEnvironment);
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings,
                                   void* pUserContext) {
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
  pDeviceSettings->d3d10.SyncInterval = 0;

  return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double fTime, float fElapsedTime,
                          void* pUserContext) {
  // Update the camera's position based on user input
  g_Camera.FrameMove(fElapsedTime);
  if (g_bPaused) fElapsedTime = 0;
  DXUTGetD3D10Device()->IASetInputLayout(g_pVertexLayout);
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
  g_pScene->SetLODSelector(g_pLODSelector);
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
  WCHAR sz[100];
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
    case IDC_PAUSED:
      g_bPaused = g_SampleUI.GetCheckBox(IDC_PAUSED)->GetChecked();
      break;
    case IDC_DYNAMICMINMAX:
      g_pbDynamicMinMax->SetBool(
          g_SampleUI.GetCheckBox(IDC_DYNAMICMINMAX)->GetChecked());
      break;
    case IDC_WAVENORMALS:
      g_pbWaveNormals->SetBool(
          g_SampleUI.GetCheckBox(IDC_WAVENORMALS)->GetChecked());
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
    case IDC_SHADOWMAPS_RESOLUTION: {
      int value =
          (1 << g_SampleUI.GetSlider(IDC_SHADOWMAPS_RESOLUTION)->GetValue());
      StringCchPrintf(sz, 100, L"SM Res.: %dx%d", value, value);
      g_SampleUI.GetStatic(IDC_SHADOWMAPS_RESOLUTION_S)->SetText(sz);
      g_pScene->SetShadowMapDimensions(value, value);
      break;
    }
    case IDC_SHADOWMAPS_PRECISION:
      g_pScene->SetShadowMapPrecision(
          g_SampleUI.GetCheckBox(IDC_SHADOWMAPS_PRECISION)->GetChecked());
      break;
    case IDC_SHADOWMAPS_ZEPSILON: {
      float value =
          g_SampleUI.GetSlider(IDC_SHADOWMAPS_ZEPSILON)->GetValue() / 10000.0f;
      StringCchPrintf(sz, 100, L"Z epsilon: %.4f", value);
      g_SampleUI.GetStatic(IDC_SHADOWMAPS_ZEPSILON_S)->SetText(sz);
      g_pfZEpsilon->SetFloat(value);
      break;
    }

    /**
     * Terrain UI
     */
    case IDC_NEWTERRAIN_SIZE: {
      int value =
          (1 << g_TerrainUI.GetSlider(IDC_NEWTERRAIN_SIZE)->GetValue()) + 1;
      StringCchPrintf(sz, 100, L"Size: %dx%d", value, value);
      g_TerrainUI.GetStatic(IDC_NEWTERRAIN_SIZE_S)->SetText(sz);
      break;
    }
    case IDC_NEWTERRAIN_ROUGHNESS: {
      float value =
          g_TerrainUI.GetSlider(IDC_NEWTERRAIN_ROUGHNESS)->GetValue() / 100.0f;
      StringCchPrintf(sz, 100, L"Roughness: %.2f", value);
      g_TerrainUI.GetStatic(IDC_NEWTERRAIN_ROUGHNESS_S)->SetText(sz);
      break;
    }
    case IDC_NEWTERRAIN_LOD: {
      int value = g_TerrainUI.GetSlider(IDC_NEWTERRAIN_LOD)->GetValue();
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

      g_pScene->CreateTerrain(g_nTerrainN, g_fTerrainR, g_nTerrainLOD);
      Tile *terrain = g_pScene->GetTerrain();
      g_pfMinHeight->SetFloat(terrain->GetMinHeight());
      g_pfMaxHeight->SetFloat(terrain->GetMaxHeight());

      g_SampleUI.GetSlider(IDC_LODSLIDER)->SetValue(0);
      g_SampleUI.GetSlider(IDC_LODSLIDER)->SetRange(0, g_nTerrainLOD);
      g_SampleUI.GetStatic(IDC_LODSLIDER_S)->SetText(L"LOD (+/-): 0");
      SAFE_DELETE(g_pLODSelector);
      g_pLODSelector = new FixedLODSelector(0);
      break;
    }
  }
}
