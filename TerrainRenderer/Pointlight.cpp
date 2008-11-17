#include "PointLight.h"
#include "DXUT.h"

unsigned int PointLight::instance_count = 0;
ID3D10EffectVectorVariable *PointLight::pPos = NULL;
ID3D10EffectVectorVariable *PointLight::pColor = NULL;
ID3D10EffectScalarVariable *PointLight::pNumPL = NULL;

PointLight::PointLight(const D3DXVECTOR3 &position,
                       const D3DXVECTOR3 &color,
                       const D3DXVECTOR3 &rotation)
    : LightSource(color, rotation),
      position_(position) {
  instance_id_ = PointLight::instance_count++;
  PointLight::pColor->SetFloatVectorArray(color_, instance_id_, 1);
  PointLight::pNumPL->SetInt(PointLight::instance_count);
}

void PointLight::GetHandles(ID3D10Effect *effect) {
  PointLight::pPos =
      effect->GetVariableByName("g_vPointLight_Position")->AsVector();
  PointLight::pColor =
      effect->GetVariableByName("g_vPointLight_Color")->AsVector();
  PointLight::pNumPL = effect->GetVariableByName("g_nPointLights")->AsScalar();
}

void PointLight::OnFrameMove(float elapsed_time) {
  D3DXMATRIX rotation_matrix;
  D3DXMatrixRotationYawPitchRoll(&rotation_matrix,
    elapsed_time * rotation_.x,
    elapsed_time * rotation_.y,
    elapsed_time * rotation_.z);
  D3DXVECTOR4 new_position;
  D3DXVec3Transform(&new_position, &position_, &rotation_matrix);
  position_ = static_cast<D3DXVECTOR3>(new_position);
  PointLight::pPos->SetFloatVectorArray(position_, instance_id_, 1);
}
