#pragma once
#include "DXUT.h"

class Environment {
 public:
  Environment(ID3D10Device *device);
  ~Environment(void);
  
  void SetBackBufferSize(UINT width, UINT height);
  void OnFrameMove(const D3DXMATRIX *world_view_matrix, float camera_fov);
  void GetShaderHandles(ID3D10Effect* effect);
  void Draw(void);

 private:
  void Init(void);

  /**
   * Zeiger auf den D3D10-Vertex-Buffer.
   */
  ID3D10Buffer *vertex_buffer_;
  ID3D10Device *device_;
  ID3D10InputLayout* vertex_layout_;

  /**
   * Handles auf die Effektvariablen.
   */
  ID3D10EffectMatrixVariable *world_view_inverse_mat_;
  ID3D10EffectVectorVariable *back_buffer_size_;
  ID3D10EffectScalarVariable *camera_fov_;

  ID3D10EffectTechnique* technique_;
};
