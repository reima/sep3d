#pragma once
#include "DirectionalLight.h"

class Scene;

class ShadowedDirectionalLight : public DirectionalLight {
 public:
  ShadowedDirectionalLight(const D3DXVECTOR3 &direction,
                           const D3DXVECTOR3 &color,
                           const D3DXVECTOR3 &rotation,
                           Scene *scene);
  virtual ~ShadowedDirectionalLight(void);

  virtual HRESULT OnCreateDevice(ID3D10Device *device);
  virtual void OnDestroyDevice(void);

  void GetShaderHandles(ID3D10Effect *effect);

  virtual void OnFrameMove(float elapsed_time);

 private:
  void SetShaderVariables(void);
  void UpdateMatrices(void);

  Scene *scene_;
  ID3D10Device *device_;

  ID3D10Texture2D *shadow_map_;
  ID3D10DepthStencilView *depth_stencil_view_;
  ID3D10ShaderResourceView *shader_resource_view_;

  D3DXMATRIX light_space_transform_;

  ID3D10EffectTechnique *technique_;
  ID3D10EffectScalarVariable *shadowed_idx_;
  ID3D10EffectMatrixVariable *lst_effect_;
  ID3D10EffectShaderResourceVariable *shadow_map_effect_;
};
