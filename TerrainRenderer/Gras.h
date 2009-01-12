#pragma once
#include <vector>
#include "vegetation.h"

class Gras :  public Vegetation {
 public:
  Gras(void);
  virtual ~Gras(void);
    
  virtual void PlaceSeed(const D3DXVECTOR3 &position,
                         float normalized_height,
                         const D3DXVECTOR3 &normal);
  virtual HRESULT CreateBuffers(ID3D10Device *device);
  virtual void GetShaderHandles(ID3D10Effect *effect);
  virtual void Draw(void);

  static HRESULT CreateStaticBuffers(ID3D10Device *device);
  static void GetStaticShaderHandles(ID3D10Device *device, ID3D10Effect *effect);
  static void ReleaseStaticBuffers(void);

 private:
  void ReleaseBuffers(void);

  ID3D10Buffer *seeds_buffer_;
  ID3D10Device *device_;
  std::vector<D3DXVECTOR3> seeds_;

  static ID3D10EffectTechnique *technique_;
  static ID3D10InputLayout* vertex_layout_;
  static ID3D10EffectShaderResourceVariable *texture_ev_;
  static ID3D10ShaderResourceView *texture_srv_;
};
