#include "ShadowedPointLight.h"
#include "LODSelector.h"
#include "Scene.h"
#include "DXUTCamera.h"

ShadowedPointLight::ShadowedPointLight(const D3DXVECTOR3 &position,
                                       const D3DXVECTOR3 &color,
                                       const D3DXVECTOR3 &rotation,
                                       Scene *scene,
                                       UINT map_width,
                                       UINT map_height,
                                       bool high_precision)
  : PointLight(position, color, rotation),
    map_width_(map_width),
    map_height_(map_height),
    high_precision_(high_precision),
    scene_(scene),
    device_(NULL),
    shadow_map_(NULL),
    depth_stencil_view_(NULL),
    shader_resource_view_(NULL),
    technique_(NULL),
    shadowed_idx_ev_(NULL),
    lst_ev_(NULL),
    shadow_map_ev_(NULL) {
}

ShadowedPointLight::~ShadowedPointLight(void) {
  OnDestroyDevice();
}

HRESULT ShadowedPointLight::OnCreateDevice(ID3D10Device *device) {
  HRESULT hr;
  device_ = device;

  D3D10_TEXTURE2D_DESC tex2d_desc;
  tex2d_desc.Width = map_width_;
  tex2d_desc.Height = map_height_;
  tex2d_desc.MipLevels = 1;
  tex2d_desc.ArraySize = 6;
  tex2d_desc.Format = high_precision_ ? DXGI_FORMAT_R32_TYPELESS
                                      : DXGI_FORMAT_R16_TYPELESS;
  tex2d_desc.SampleDesc.Count = 1;
  tex2d_desc.SampleDesc.Quality = 0;
  tex2d_desc.Usage = D3D10_USAGE_DEFAULT;
  tex2d_desc.BindFlags = D3D10_BIND_DEPTH_STENCIL | D3D10_BIND_SHADER_RESOURCE;
  tex2d_desc.CPUAccessFlags = 0;
  tex2d_desc.MiscFlags = 0;
  V_RETURN(device->CreateTexture2D(&tex2d_desc, NULL, &shadow_map_));

  D3D10_DEPTH_STENCIL_VIEW_DESC dsv_desc;
  dsv_desc.Format = high_precision_ ? DXGI_FORMAT_D32_FLOAT
                                    : DXGI_FORMAT_D16_UNORM;
  dsv_desc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2DARRAY;
  dsv_desc.Texture2DArray.ArraySize = 6;
  dsv_desc.Texture2DArray.FirstArraySlice = 0;
  dsv_desc.Texture2DArray.MipSlice = 0;
  V_RETURN(device->CreateDepthStencilView(shadow_map_, &dsv_desc,
                                          &depth_stencil_view_));

  D3D10_SHADER_RESOURCE_VIEW_DESC srv_desc;
  srv_desc.Format = high_precision_ ? DXGI_FORMAT_R32_FLOAT
                                    : DXGI_FORMAT_R16_FLOAT;
  srv_desc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2DARRAY;
  srv_desc.Texture2DArray.ArraySize = 6;
  srv_desc.Texture2DArray.FirstArraySlice = 0;
  srv_desc.Texture2DArray.MipLevels = 1;
  srv_desc.Texture2DArray.MostDetailedMip = 0;
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
  shadowed_idx_ev_ =
      effect->GetVariableByName("g_iShadowedPointLight")->AsScalar();
  technique_ = effect->GetTechniqueByName("PointShadowMap");
  lst_ev_ = effect->GetVariableByName("g_mPointLightSpaceTransform")->AsMatrix();
  shadow_map_ev_ =
      effect->GetVariableByName("g_tPointShadowMap")->AsShaderResource();
}

void ShadowedPointLight::SetShaderVariables(void) {
  assert(shadowed_idx_ev_ != NULL);
  assert(lst_ev_ != NULL);
  shadowed_idx_ev_->SetInt(instance_id_);
  lst_ev_->SetMatrixArray((float *)light_space_transforms_, 0, 6);
}

void ShadowedPointLight::SetShadowMapDimensions(UINT width, UINT height) {
  map_width_ = width;
  map_height_ = height;
  OnDestroyDevice();
  OnCreateDevice(device_);
}

void ShadowedPointLight::SetShadowMapPrecision(bool high_precision) {
  high_precision_ = high_precision;
  OnDestroyDevice();
  OnCreateDevice(device_);
}

void ShadowedPointLight::UpdateMatrices(void) {
  D3DXVECTOR3 upvec = D3DXVECTOR3(0, 1, 0);

  D3DXVECTOR3 lookat = position_+ D3DXVECTOR3(-1,0,0);
  D3DXMatrixLookAtLH(&light_space_transforms_[0], &position_, &lookat, &upvec);
  
  lookat = position_+ D3DXVECTOR3(+1,0,0);
  D3DXMatrixLookAtLH(&light_space_transforms_[1], &position_, &lookat, &upvec);
  
  upvec = D3DXVECTOR3(0, 0, 1);
  lookat = position_+ D3DXVECTOR3(0,-1,0);
  D3DXMatrixLookAtLH(&light_space_transforms_[2], &position_, &lookat, &upvec);

  upvec = D3DXVECTOR3(0, 0, -1);
  lookat = position_+ D3DXVECTOR3(0,+1,0);
  D3DXMatrixLookAtLH(&light_space_transforms_[3], &position_, &lookat, &upvec);

  upvec = D3DXVECTOR3(0, 1, 0);
  lookat = position_+ D3DXVECTOR3(0,0,-1);
  D3DXMatrixLookAtLH(&light_space_transforms_[4], &position_, &lookat, &upvec);
  
  lookat = position_+ D3DXVECTOR3(0,0,1);
  D3DXMatrixLookAtLH(&light_space_transforms_[5], &position_, &lookat, &upvec);

  D3DXMATRIX proj;
  D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI/2, 1, 0.1f, 10);

  for (int i = 0; i < 6; ++i)
    light_space_transforms_[i] *= proj;
}

void ShadowedPointLight::OnFrameMove(float elapsed_time) {
  assert(technique_ != NULL);
  assert(depth_stencil_view_ != NULL);
  assert(device_ != NULL);
  assert(scene_ != NULL);
  assert(shadow_map_ev_ != NULL);

  PointLight::OnFrameMove(elapsed_time);
  UpdateMatrices();
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

  // Shader Resource ausbinden
  // DEVICE_OMSETRENDERTARGETS_HAZARD tritt aber trotzdem auf :(
  shadow_map_ev_->SetResource(NULL);  

  // Unsere Textur als Depth-Stencil-Target setzen
  device_->OMSetRenderTargets(0, NULL, depth_stencil_view_);
  // Inhalt zurücksetzen
  device_->ClearDepthStencilView(depth_stencil_view_, D3D10_CLEAR_DEPTH, 1.0f, 0);

  // Viewport setzen
  D3D10_VIEWPORT viewport;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = map_width_;
  viewport.Height = map_height_;
  viewport.MaxDepth = 1.0f;
  viewport.MinDepth = 0.0f;
  device_->RSSetViewports(1, &viewport);

  technique_->GetPassByIndex(0)->Apply(0);
  scene_->Draw();

  // Alte Render Targets wieder setzen
  device_->OMSetRenderTargets(D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT,
                              rtv_old, dsv_old);

  // Alte Viewports wieder setzen
  device_->RSSetViewports(num_viewports, viewports_old);

  // Referenzen auf alte Render Targets freigeben
  for (UINT i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    SAFE_RELEASE(rtv_old[i]);

  SAFE_RELEASE(dsv_old);

  shadow_map_ev_->SetResource(shader_resource_view_);
}
