#pragma once
#include "DirectionalLight.h"

class Scene;

class ShadowedDirectionalLight : public DirectionalLight {
 public:
  ShadowedDirectionalLight(const D3DXVECTOR3 &direction,
                           const D3DXVECTOR3 &color,
                           const D3DXVECTOR3 &rotation,
                           Scene *scene,
                           UINT map_width = 1024, UINT map_height = 1024,
                           bool high_precision = true);
  virtual ~ShadowedDirectionalLight(void);

  HRESULT OnCreateDevice(ID3D10Device *device);
  void OnDestroyDevice(void);
  void GetShaderHandles(ID3D10Effect *effect);

  virtual void OnFrameMove(float elapsed_time);

  void SetShadowMapDimensions(UINT width, UINT height);
  void SetShadowMapPrecision(bool high_precision);

 private:
  void SetShaderVariables(void);
  void UpdateMatrices(void);
  void TSM_UpdateMatrices(void);
  D3DXMATRIX TSM_TrapezoidToSquare(const D3DXVECTOR2 &t0,
                             const D3DXVECTOR2 &t1,
                             const D3DXVECTOR2 &t2,
                             const D3DXVECTOR2 &t3);

  UINT map_width_;
  UINT map_height_;
  bool high_precision_;

  Scene *scene_;
  ID3D10Device *device_;

  ID3D10Texture2D *shadow_map_;
  ID3D10DepthStencilView *depth_stencil_view_;
  ID3D10ShaderResourceView *shader_resource_view_;

  D3DXMATRIX light_space_transform_;
  D3DXMATRIX trapezoid_to_square_;

  ID3D10EffectTechnique *technique_;
  ID3D10EffectScalarVariable *shadowed_idx_ev_;
  ID3D10EffectMatrixVariable *lst_ev_;
  ID3D10EffectMatrixVariable *tts_ev_;
  ID3D10EffectShaderResourceVariable *shadow_map_ev_;
};
