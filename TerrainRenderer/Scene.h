#pragma once
#include <vector>
#include "DXUT.h"
#include "LightSource.h"

class Scene {
 public:
  Scene(void);
  ~Scene(void);

  void AddPointLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &color,
                     const D3DXVECTOR3 &rotation);
  void AddDirectionalLight(const D3DXVECTOR3 &direction,
                           const D3DXVECTOR3 &color,
                           const D3DXVECTOR3 &rotation);
  void AddSpotLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &direction,
                    const D3DXVECTOR3 &color, const D3DXVECTOR3 &rotation,
                    float cutoff_angle, float exponent);

  void OnFrameMove(float fTime);
  void GetShaderHandles(ID3D10Effect* pFx);

 private:
  std::vector<LightSource *> light_sources_;
};
