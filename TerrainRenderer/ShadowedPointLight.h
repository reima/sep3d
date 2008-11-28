#pragma once
#include "PointLight.h"

class Scene;

class ShadowedPointLight : public PointLight {
 public:
  ShadowedPointLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &color,
                     const D3DXVECTOR3 &rotation);
  virtual ~ShadowedPointLight(void);

  HRESULT OnCreateDevice(ID3D10Device *device);
  void OnDestroyDevice(void);

  void GetShaderHandles(ID3D10Effect *effect);
  void SetShaderVariables(void);

  void UpdateMatrices(Scene *scene);

  void OnFrameMove(float elapsed_time, Scene *scene);

 private:
  ID3D10Device *device_;

  ID3D10Texture2D *shadow_map_;
  ID3D10DepthStencilView *depth_stencil_view_;
  ID3D10ShaderResourceView *shader_resource_view_;

  D3DXMATRIX light_space_transforms_[6];


  ID3D10EffectTechnique *technique_;
  ID3D10EffectMatrixVariable *lst_effect_;
  ID3D10EffectShaderResourceVariable *shadow_map_effect_;
};
