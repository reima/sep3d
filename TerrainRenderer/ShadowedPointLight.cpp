#include "ShadowedPointLight.h"
#include "LODSelector.h"
#include "Tile.h"
#include "DXUTCamera.h"

extern LODSelector *g_pLODSelector;
extern CFirstPersonCamera g_Camera;
extern ID3D10InputLayout* g_pVertexLayout;

ShadowedPointLight::ShadowedPointLight(const D3DXVECTOR3 &position,
                                       const D3DXVECTOR3 &color,
                                       const D3DXVECTOR3 &rotation)
  : PointLight(position, color, rotation),
    device_(NULL),
    shadow_map_(NULL),
    depth_stencil_view_(NULL),
    shader_resource_view_(NULL) {
}

ShadowedPointLight::~ShadowedPointLight(void) {
  OnDestroyDevice();
}

HRESULT ShadowedPointLight::OnCreateDevice(ID3D10Device *device) {
  HRESULT hr;
  device_ = device;

  D3D10_TEXTURE2D_DESC tex2d_desc;
  tex2d_desc.Width = 1024;
  tex2d_desc.Height = 1024;
  tex2d_desc.MipLevels = 1;
  tex2d_desc.ArraySize = 6;
  tex2d_desc.Format = DXGI_FORMAT_R32_TYPELESS;
  tex2d_desc.SampleDesc.Count = 1;
  tex2d_desc.SampleDesc.Quality = 0;
  tex2d_desc.Usage = D3D10_USAGE_DEFAULT;
  tex2d_desc.BindFlags = D3D10_BIND_DEPTH_STENCIL | D3D10_BIND_SHADER_RESOURCE;
  tex2d_desc.CPUAccessFlags = 0;
  tex2d_desc.MiscFlags = D3D10_RESOURCE_MISC_TEXTURECUBE;
  V_RETURN(device->CreateTexture2D(&tex2d_desc, NULL, &shadow_map_));

  D3D10_DEPTH_STENCIL_VIEW_DESC dsv_desc;
  dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
  dsv_desc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2DARRAY;
  dsv_desc.Texture2DArray.ArraySize = 6;
  dsv_desc.Texture2DArray.FirstArraySlice = 0;
  dsv_desc.Texture2DArray.MipSlice = 0;  
  V_RETURN(device->CreateDepthStencilView(shadow_map_, &dsv_desc,
                                          &depth_stencil_view_));

  D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
  srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
  srv_desc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURECUBE;
  srv_desc.TextureCube.MipLevels = 1;
  srv_desc.TextureCube.MostDetailedMip = 0;
  V_RETURN(device->CreateShaderResourceView(shadow_map_, &srv_desc,
                                            &shader_resource_view_));
  return S_OK;
}

void ShadowedPointLight::OnDestroyDevice(void) {
  SAFE_RELEASE(shader_resource_view_);
  SAFE_RELEASE(depth_stencil_view_);
  SAFE_RELEASE(shadow_map_);
}

void ShadowedPointLight::GetShaderHandles(ID3D10Effect *effect) {
  PointLight::GetHandles(effect);
  technique_ = effect->GetTechniqueByName("PointShadowMap");
}

void ShadowedPointLight::SetShaderVariables(void) {
  // TODO: Implementierung
}

void ShadowedPointLight::UpdateMatrices(Tile *tile) {
  // TODO: Implementierung
}

void ShadowedPointLight::OnFrameMove(float elapsed_time, Tile *tile) {
  return;
  assert(technique_ != NULL);
  PointLight::OnFrameMove(elapsed_time);
  UpdateMatrices(tile);
  SetShaderVariables();

  // Render Targets sichern
  ID3D10RenderTargetView *rtv_old[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
  ID3D10DepthStencilView *dsv_old;
  device_->OMGetRenderTargets(D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT,
                              rtv_old, &dsv_old);

  // Viewports sichern
  D3D10_VIEWPORT viewports_old[D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT];
  UINT num_viewports = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;
  device_->RSGetViewports(&num_viewports, viewports_old);

  // Unsere Textur als Depth-Stencil-Target setzen
  device_->OMSetRenderTargets(0, NULL, depth_stencil_view_);
  // Inhalt zur�cksetzen
  device_->ClearDepthStencilView(depth_stencil_view_, D3D10_CLEAR_DEPTH, 1.0f, 0);

  // Viewport setzen
  D3D10_VIEWPORT viewport;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = 1024;
  viewport.Height = 1024;
  viewport.MaxDepth = 1.0f;
  viewport.MinDepth = 0.0f;
  device_->RSSetViewports(1, &viewport);

  device_->IASetInputLayout(g_pVertexLayout);
  technique_->GetPassByIndex(0)->Apply(0);
  tile->Draw(device_, g_pLODSelector, g_Camera.GetEyePt());

  // Alte Render Targets wieder setzen
  device_->OMSetRenderTargets(D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT,
                              rtv_old, dsv_old);

  // Alte Viewports wieder setzen
  device_->RSSetViewports(num_viewports, viewports_old);

  // Referenzen auf alte Render Targets freigeben
  for (UINT i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    SAFE_RELEASE(rtv_old[i]);

  SAFE_RELEASE(dsv_old);
}