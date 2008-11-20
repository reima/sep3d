#pragma once
#include "DXUT.h"

class Environment {
 public:
  Environment(ID3D10Device *device);
  ~Environment(void);
  
  void SetBackBufferSize(UINT width, UINT height);
  void OnFrameMove(const D3DXMATRIX *world_view_matrix, float camera_fov);
  void GetShaderHandles(ID3D10Effect* effect);
  void Draw(ID3D10Device *device);

 private:
  void Init(ID3D10Device *device);

  /**
   * Zeiger auf den D3D10-Vertex-Buffer.
   */
  ID3D10Buffer *vertex_buffer_;

  /**
   * Handles auf die Effektvariablen.
   */
  ID3D10EffectMatrixVariable *world_view_inverse_mat_;
  ID3D10EffectVectorVariable *back_buffer_size_;
  ID3D10EffectScalarVariable *camera_fov_;

  ID3D10Effect* effect_;
};
