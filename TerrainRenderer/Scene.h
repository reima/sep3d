#pragma once
#include <vector>
#include "DXUT.h"
#include "LightSource.h"

class Scene {
 public:
  void AddPointLight(D3DXVECTOR3 &position, D3DXVECTOR3 &color);
  void AddDirectionalLight(D3DXVECTOR3 &direction, D3DXVECTOR3 &color);
  void AddSpotLight(D3DXVECTOR3 &position, D3DXVECTOR3 &direction,
                    D3DXVECTOR3 &color, float cutoff_angle, float exponent);

  void OnFrameMove(float fTime);
  void GetShaderHandles(ID3D10Effect* pFx);

  Scene(void);
  ~Scene(void);

 private:
  std::vector<LightSource *> light_sources_;
};
