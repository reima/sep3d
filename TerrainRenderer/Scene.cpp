#include <algorithm>
#include "Scene.h"
#include "PointLight.h"
#include "DirectionalLight.h"
#include "SpotLight.h"
#include "ShadowedPointLight.h"
#include "ShadowedDirectionalLight.h"
#include "Terrain.h"
#include "LODSelector.h"
#include "DXUTCamera.h"
#include "Environment.h"

#undef min
#undef max

extern const float g_fFOV;

Scene::Scene(void)
    : cam_pos_(D3DXVECTOR3(0, 0, 0)),
      camera_(NULL),
      terrain_(NULL),
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
      pShadowedDirectionalLight(NULL),
      movement_(SCENE_MOVEMENT_FLY) {
}

Scene::~Scene(void) {
  OnDestroyDevice();
  std::vector<LightSource *>::iterator it;
  for (it = light_sources_.begin(); it != light_sources_.end(); ++it) {
    delete (*it);
  }
  SAFE_DELETE(terrain_);
  SAFE_DELETE(environment_);
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

void Scene::OnFrameMove(float elapsed_time) {
  assert(pCameraPosition != NULL);

  cam_pos_ = *camera_->GetEyePt();
  float terrain_height = terrain_->GetHeightAt(cam_pos_) + 0.5f;
  if (movement_ == SCENE_MOVEMENT_WALK ||
      (movement_ == SCENE_MOVEMENT_FLY && cam_pos_.y < terrain_height)) {
    D3DXVECTOR3 lookat = *camera_->GetLookAtPt();
    D3DXVECTOR3 view_dir = lookat - cam_pos_;
    cam_pos_.y = terrain_height;
    lookat = cam_pos_ + view_dir;
    camera_->SetViewParams(&cam_pos_, &lookat);
  }

  pCameraPosition->SetFloatVector(cam_pos_);
  std::vector<LightSource *>::iterator it;
  for (it = light_sources_.begin(); it != light_sources_.end(); ++it) {
    (*it)->OnFrameMove(elapsed_time);
  }

  if (environment_) {
    D3DXMATRIX mView = *camera_->GetViewMatrix();
    environment_->OnFrameMove(&mView, g_fFOV);
  }
}

HRESULT Scene::OnCreateDevice(ID3D10Device *device) {
  HRESULT hr;
  device_ = device;
  std::vector<LightSource *>::iterator it;
  for (it = light_sources_.begin(); it != light_sources_.end(); ++it) {
    V_RETURN((*it)->OnCreateDevice(device));
  }
  if (terrain_) {
    V_RETURN(terrain_->CreateBuffers(device_));
  }
  environment_ = new Environment(device);
  return S_OK;
}

void Scene::OnResizedSwapChain(UINT width, UINT height) {
  if (environment_) environment_->SetBackBufferSize(width, height);
}

void Scene::GetShaderHandles(ID3D10Effect* effect) {
  effect_ = effect;
  PointLight::GetHandles(effect);
  DirectionalLight::GetHandles(effect);
  SpotLight::GetHandles(effect);
  environment_->GetShaderHandles(effect);
  if (shadowed_point_light_)
    shadowed_point_light_->GetShaderHandles(effect);
  if (shadowed_directional_light_)
    shadowed_directional_light_->GetShaderHandles(effect);
  if (terrain_)
    terrain_->GetShaderHandles(effect);
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
  device_ = NULL;
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

void Scene::CreateTerrain(int n, float roughness, int num_lod, float scale) {
  SAFE_DELETE(terrain_);
  terrain_ = new Terrain(n, roughness, num_lod, scale, true);
  terrain_->TriangulateZOrder();
  if (device_)
    terrain_->CreateBuffers(device_);
  if (effect_)
    terrain_->GetShaderHandles(effect_);
  //terrain_->FreeMemory();
}

void Scene::GetBoundingBox(D3DXVECTOR3 *box, D3DXVECTOR3 *mid) {
  if (terrain_) {
    terrain_->GetBoundingBox(box, mid);
  } else {
    for (int i = 0; i < 8; ++i) {
      box[i] = D3DXVECTOR3(0, 0, 0);
    }
    *mid = D3DXVECTOR3(0, 0, 0);
  }
}

void Scene::Draw(ID3D10EffectTechnique *technique, bool shadow_pass) {
  assert(device_ != NULL);
  if (terrain_) {
    assert(lod_selector_ != NULL);
    terrain_->Draw(technique, lod_selector_, camera_, shadow_pass);
  }
  if (environment_) {
    environment_->Draw();
  }
  if (terrain_) {
    terrain_->DrawPlants(shadow_pass);
  }
}

void Scene::SetMovement(SceneMovement movement) {
  movement_ = movement;
}
