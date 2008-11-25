#pragma once
#include "PointLight.h"

class ShadowedPointLight : public PointLight {
 public:
  ShadowedPointLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &color,
                     const D3DXVECTOR3 &rotation);
  virtual ~ShadowedPointLight(void);

  HRESULT OnCreateDevice(ID3D10Device *device);
  void OnDestroyDevice(void);

  void GetShaderHandles(ID3D10Effect *effect);
  void SetShaderVariables(void);

  void UpdateMatrices();

  void OnFrameMove(float elapsed_time);

 private:
  ID3D10Device *device_;

  ID3D10Texture2D *shadow_map_;
  ID3D10DepthStencilView *depth_stencil_view_;
  ID3D10ShaderResourceView *shader_resource_view_;

  D3DXMATRIX light_space_transforms_[6];

  // TODO: Shader Handles
};
