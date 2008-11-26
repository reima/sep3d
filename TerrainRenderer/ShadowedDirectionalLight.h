#pragma once
#include "DirectionalLight.h"
#include "Tile.h"

class ShadowedDirectionalLight : public DirectionalLight {
 public:
  ShadowedDirectionalLight(const D3DXVECTOR3 &direction,
                           const D3DXVECTOR3 &color,
                           const D3DXVECTOR3 &rotation);
  virtual ~ShadowedDirectionalLight(void);

  HRESULT OnCreateDevice(ID3D10Device *device);
  void OnDestroyDevice(void);

  void GetShaderHandles(ID3D10Effect *effect);
  void SetShaderVariables(void);

  void UpdateMatrices(Tile *tile) ;

  void OnFrameMove(float elapsed_time, Tile *tile);

 private:
  ID3D10Device *device_;

  ID3D10Texture2D *shadow_map_;
  ID3D10DepthStencilView *depth_stencil_view_;
  ID3D10ShaderResourceView *shader_resource_view_;

  D3DXMATRIX light_space_transform_;

  ID3D10Effect *effect_;
  ID3D10EffectMatrixVariable *lst_effect_;
  ID3D10EffectShaderResourceVariable *shadow_map_effect_;

  // TODO: Shader Handles
};
