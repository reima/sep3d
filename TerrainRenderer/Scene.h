#pragma once
#include <vector>
#include "DXUT.h"
#include "LightSource.h"

class Scene {
 public:
  Scene(float ambient, float diffuse, float specular, float exponent);
  ~Scene(void);

  void AddPointLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &color,
                     const D3DXVECTOR3 &rotation);
  void AddDirectionalLight(const D3DXVECTOR3 &direction,
                           const D3DXVECTOR3 &color,
                           const D3DXVECTOR3 &rotation);
  void AddSpotLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &direction,
                    const D3DXVECTOR3 &color, const D3DXVECTOR3 &rotation,
                    float cutoff_angle, float exponent);

  void OnFrameMove(float elapsed_time, const D3DXVECTOR3 &cam_pos);
  void GetShaderHandles(ID3D10Effect* effect);

 private:
  std::vector<LightSource *> light_sources_;
  float ambient_;
  float diffuse_;
  float specular_;
  float exponent_;
  D3DXVECTOR3 cam_pos_;
  ID3D10EffectVectorVariable *pMaterialParameters;
  ID3D10EffectVectorVariable *pCameraPosition;
};
