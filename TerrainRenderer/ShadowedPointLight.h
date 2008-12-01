#pragma once
#include "PointLight.h"

class Scene;

class ShadowedPointLight : public PointLight {
 public:
  ShadowedPointLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &color,
                     const D3DXVECTOR3 &rotation, Scene *scene,
                     UINT map_width = 1024, UINT map_height = 1024,
                     bool high_precision = true);
  virtual ~ShadowedPointLight(void);

  virtual HRESULT OnCreateDevice(ID3D10Device *device);
  virtual void OnDestroyDevice(void);
  void GetShaderHandles(ID3D10Effect *effect);

  virtual void OnFrameMove(float elapsed_time);

  void SetShadowMapDimensions(UINT width, UINT height);
  void SetShadowMapPrecision(bool high_precision);

 private:
  void SetShaderVariables(void);
  void UpdateMatrices(void);

  UINT map_width_;
  UINT map_height_;
  bool high_precision_;

  Scene *scene_;
  ID3D10Device *device_;

  ID3D10Texture2D *shadow_map_;
  ID3D10DepthStencilView *depth_stencil_view_;
  ID3D10ShaderResourceView *shader_resource_view_;

  D3DXMATRIX light_space_transforms_[6];

  ID3D10EffectTechnique *technique_;
  ID3D10EffectScalarVariable *shadowed_idx_ev_;
  ID3D10EffectMatrixVariable *lst_ev_;
  ID3D10EffectShaderResourceVariable *shadow_map_ev_;
};
