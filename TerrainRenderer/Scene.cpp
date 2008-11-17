#include "Scene.h"
#include "PointLight.h"
#include "DirectionalLight.h"
#include "SpotLight.h"

Scene::Scene(float ambient, float diffuse, float specular, float exponent)
    : ambient_(ambient),
      diffuse_(diffuse),
      specular_(specular),
      exponent_(exponent) {
}

Scene::~Scene(void) {
  std::vector<LightSource *>::iterator it;
  for (it = light_sources_.begin(); it != light_sources_.end(); ++it) {
    delete (*it);
  }
}

void Scene::AddPointLight(const D3DXVECTOR3 &position,
                          const D3DXVECTOR3 &color,
                          const D3DXVECTOR3 &rotation) {
  light_sources_.push_back(new PointLight(position, color, rotation));
}

void Scene::AddDirectionalLight(const D3DXVECTOR3 &direction,
                                const D3DXVECTOR3 &color,
                                const D3DXVECTOR3 &rotation) {
  light_sources_.push_back(new DirectionalLight(direction, color, rotation));
}

void Scene::AddSpotLight(const D3DXVECTOR3 &position,
                         const D3DXVECTOR3 &direction,
                         const D3DXVECTOR3 &color,
                         const D3DXVECTOR3 &rotation,
                         float cutoff_angle, float exponent) {
  light_sources_.push_back(new SpotLight(position, direction, color, rotation,
                                         cutoff_angle, exponent));
}

void Scene::OnFrameMove(float fTime) {
  std::vector<LightSource *>::iterator it;
  for (it = light_sources_.begin(); it != light_sources_.end(); ++it) {
    (*it)->OnFrameMove(fTime);
  }
}

void Scene::GetShaderHandles(ID3D10Effect* effect) {
  PointLight::GetHandles(effect);
  DirectionalLight::GetHandles(effect);
  SpotLight::GetHandles(effect);
  pMaterialParameters =
      effect->GetVariableByName("g_vMaterialParameters")->AsVector();
  float material_parameters[] = { ambient_, diffuse_, specular_, exponent_ };
  pMaterialParameters->SetFloatVector(material_parameters);
}
