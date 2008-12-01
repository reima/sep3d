#include "Scene.h"
#include "PointLight.h"
#include "DirectionalLight.h"
#include "SpotLight.h"
#include "ShadowedPointLight.h"
#include "ShadowedDirectionalLight.h"
#include "Tile.h"
#include "LODSelector.h"
#include "DXUTShapes.h"

extern LODSelector *g_pLODSelector;

Scene::Scene(void)
    : cam_pos_(D3DXVECTOR3(0, 0, 0)),
      camera_(NULL),
      tile_(NULL),
      lod_selector_(NULL),
      device_(NULL),
      effect_(NULL),
      shadowed_point_light_(NULL),
      shadowed_directional_light_(NULL),
      shadow_map_width_(1024),
      shadow_map_height_(1024),
      shadow_map_high_precision_(true),
      pMaterialParameters(NULL),
      pCameraPosition(NULL),
      pShadowedPointLight(NULL),
      pShadowedDirectionalLight(NULL) {
}

Scene::~Scene(void) {
  OnDestroyDevice();
  std::vector<LightSource *>::iterator it;
  for (it = light_sources_.begin(); it != light_sources_.end(); ++it) {
    delete (*it);
  }
  SAFE_DELETE(tile_);
}

void Scene::SetMaterial(float ambient, float diffuse, float specular,
                        float exponent) {
  assert(pMaterialParameters != NULL);
  float material_parameters[] = { ambient, diffuse, specular, exponent };
  pMaterialParameters->SetFloatVector(material_parameters);
}

void Scene::AddPointLight(const D3DXVECTOR3 &position,
                          const D3DXVECTOR3 &color,
                          const D3DXVECTOR3 &rotation,
                          bool shadowed) {
  if (shadowed) {
    SAFE_DELETE(shadowed_point_light_);
    shadowed_point_light_ = new ShadowedPointLight(position,
                                                   color,
                                                   rotation,
                                                   this,
                                                   shadow_map_width_,
                                                   shadow_map_height_,
                                                   shadow_map_high_precision_);
    if (device_)
      shadowed_point_light_->OnCreateDevice(device_);
    if (effect_) {
      shadowed_point_light_->GetShaderHandles(effect_);
      pShadowedPointLight->SetBool(true);
    }
    light_sources_.push_back(shadowed_point_light_);
  } else {
    light_sources_.push_back(new PointLight(position, color, rotation));
  }
}

void Scene::AddDirectionalLight(const D3DXVECTOR3 &direction,
                                const D3DXVECTOR3 &color,
                                const D3DXVECTOR3 &rotation,
                                bool shadowed) {
  if (shadowed) {
    SAFE_DELETE(shadowed_directional_light_);
    shadowed_directional_light_ =
        new ShadowedDirectionalLight(direction, color, rotation, this,
                                     shadow_map_width_, shadow_map_height_,
                                     shadow_map_high_precision_);
    if (device_)
      shadowed_directional_light_->OnCreateDevice(device_);
    if (effect_) {
      shadowed_directional_light_->GetShaderHandles(effect_);
      pShadowedDirectionalLight->SetBool(true);
    }
    light_sources_.push_back(shadowed_directional_light_);
  } else {
    light_sources_.push_back(new DirectionalLight(direction, color, rotation));
  }
}

void Scene::AddSpotLight(const D3DXVECTOR3 &position,
                         const D3DXVECTOR3 &direction,
                         const D3DXVECTOR3 &color,
                         const D3DXVECTOR3 &rotation,
                         float cutoff_angle, float exponent) {
  light_sources_.push_back(new SpotLight(position, direction, color, rotation,
                                         cutoff_angle, exponent));
}

void Scene::OnFrameMove(float elapsed_time, const D3DXVECTOR3 &cam_pos) {
  assert(pCameraPosition != NULL);
  cam_pos_ = cam_pos;
  pCameraPosition->SetFloatVector(cam_pos_);
  std::vector<LightSource *>::iterator it;
  for (it = light_sources_.begin(); it != light_sources_.end(); ++it) {
    (*it)->OnFrameMove(elapsed_time);
  }
}

HRESULT Scene::OnCreateDevice(ID3D10Device *device) {
  HRESULT hr;
  device_ = device;
  std::vector<LightSource *>::iterator it;
  for (it = light_sources_.begin(); it != light_sources_.end(); ++it) {
    V_RETURN((*it)->OnCreateDevice(device));
  }
  return S_OK;
}

void Scene::GetShaderHandles(ID3D10Effect* effect) {
  effect_ = effect;
  PointLight::GetHandles(effect);
  DirectionalLight::GetHandles(effect);
  SpotLight::GetHandles(effect);
  if (shadowed_point_light_)
    shadowed_point_light_->GetShaderHandles(effect);
  if (shadowed_directional_light_)
    shadowed_directional_light_->GetShaderHandles(effect);
  pMaterialParameters =
      effect->GetVariableByName("g_vMaterialParameters")->AsVector();  
  pCameraPosition =
      effect->GetVariableByName("g_vCamPos")->AsVector();
  pShadowedPointLight =
      effect->GetVariableByName("g_bShadowedPointLight")->AsScalar();
  pShadowedPointLight->SetBool(shadowed_point_light_ != NULL);
  pShadowedDirectionalLight =
      effect->GetVariableByName("g_bShadowedDirectionalLight")->AsScalar();
  pShadowedDirectionalLight->SetBool(shadowed_directional_light_ != NULL);
}

void Scene::OnDestroyDevice(void) {
  std::vector<LightSource *>::iterator it;
  for (it = light_sources_.begin(); it != light_sources_.end(); ++it) {
    (*it)->OnDestroyDevice();
  }
}

void Scene::SetShadowMapDimensions(UINT width, UINT height) {
  shadow_map_width_ = width;
  shadow_map_height_ = height;
  if (shadowed_point_light_)
    shadowed_point_light_->SetShadowMapDimensions(width, height);
  if (shadowed_directional_light_)
    shadowed_directional_light_->SetShadowMapDimensions(width, height);
}

void Scene::SetShadowMapPrecision(bool high_precision) {
  shadow_map_high_precision_ = high_precision;
  if (shadowed_point_light_)
    shadowed_point_light_->SetShadowMapPrecision(high_precision);
  if (shadowed_directional_light_)
    shadowed_directional_light_->SetShadowMapPrecision(high_precision);
}

void Scene::CreateTerrain(int n, float roughness, int num_lod) {
  assert(device_ != NULL);
  HRESULT hr;
  SAFE_DELETE(tile_);
  tile_ = new Tile(n, roughness, num_lod);
  tile_->TriangulateZOrder();
  tile_->CalculateNormals();
  V(tile_->CreateBuffers(device_));
  tile_->FreeMemory();
}

void Scene::GetBoundingBox(D3DXVECTOR3 *box, D3DXVECTOR3 *mid) {
  if (tile_) {
    tile_->GetBoundingBox(box, mid);
  } else {
    for (int i = 0; i < 8; ++i) {
      box[i] = D3DXVECTOR3(0, 0, 0);      
    }
    *mid = D3DXVECTOR3(0, 0, 0);
  }
}

void Scene::Draw(void) {
  assert(device_ != NULL);  
  if (tile_) {
    assert(lod_selector_ != NULL);
    tile_->Draw(device_, lod_selector_, &cam_pos_);
  }
}