#include "DirectionalLight.h"

unsigned int DirectionalLight::instance_count = 0;
ID3D10EffectVectorVariable *DirectionalLight::pDir = NULL;
ID3D10EffectVectorVariable *DirectionalLight::pColor = NULL;
ID3D10EffectScalarVariable *DirectionalLight::pNumDL = NULL;

DirectionalLight::DirectionalLight(const D3DXVECTOR3 &direction,
                                   const D3DXVECTOR3 &color,
                                   const D3DXVECTOR3 &rotation)
    : LightSource(color, rotation),
      direction_(direction) {
  instance_id_ = DirectionalLight::instance_count++;
  D3DXVec3Normalize(&direction_, &direction_);
  DirectionalLight::pColor->SetFloatVectorArray(color_, instance_id_, 1);
  DirectionalLight::pNumDL->SetInt(DirectionalLight::instance_count);
}

void DirectionalLight::GetHandles(ID3D10Effect *effect) {
  DirectionalLight::pDir =
      effect->GetVariableByName("g_vDirectionalLight_Direction")->AsVector();
  DirectionalLight::pColor =
      effect->GetVariableByName("g_vDirectionalLight_Color")->AsVector();
  DirectionalLight::pNumDL =
      effect->GetVariableByName("g_nDirectionalLights")->AsScalar();
}

void DirectionalLight::OnFrameMove(float elapsed_time) {
  D3DXMATRIX rotation_matrix;
  D3DXMatrixRotationYawPitchRoll(&rotation_matrix,
    elapsed_time * rotation_.x,
    elapsed_time * rotation_.y,
    elapsed_time * rotation_.z);
  D3DXVECTOR4 new_direction;
  D3DXVec3Transform(&new_direction, &direction_, &rotation_matrix);
  direction_ = static_cast<D3DXVECTOR3>(new_direction);
  DirectionalLight::pDir->SetFloatVectorArray(direction_, instance_id_, 1);
}
