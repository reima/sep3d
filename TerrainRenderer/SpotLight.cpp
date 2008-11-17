#include "SpotLight.h"

unsigned int SpotLight::instance_count = 0;
ID3D10EffectVectorVariable *SpotLight::pPos = NULL;
ID3D10EffectVectorVariable *SpotLight::pDir = NULL;
ID3D10EffectVectorVariable *SpotLight::pAngleExp = NULL;
ID3D10EffectVectorVariable *SpotLight::pColor = NULL;
ID3D10EffectScalarVariable *SpotLight::pNumSL = NULL;

SpotLight::SpotLight(const D3DXVECTOR3 &position, const D3DXVECTOR3 &direction,
                     const D3DXVECTOR3 &color, const D3DXVECTOR3 &rotation,
                     float cutoff_angle, float exponent)
    : LightSource(color, rotation),
      position_(position),
      direction_(direction),
      cutoff_angle_(cutoff_angle),
      exponent_(exponent) {
  instance_id_ = SpotLight::instance_count++;
  SpotLight::pColor->SetFloatVectorArray(color_, instance_id_, 1);
  D3DXVECTOR3 temp;
  D3DXVec3Normalize(&temp, &direction);
  SpotLight::pDir->SetFloatVectorArray(temp, instance_id_, 1);
  float angle_exp[] = { cutoff_angle_, exponent_ };
  SpotLight::pAngleExp->SetFloatVectorArray(angle_exp, instance_id_, 1);
  SpotLight::pNumSL->SetInt(SpotLight::instance_count);
}

void SpotLight::GetHandles(ID3D10Effect *effect) {
  SpotLight::pPos =
      effect->GetVariableByName("g_vSpotLight_Position")->AsVector();
  SpotLight::pDir =
      effect->GetVariableByName("g_vSpotLight_Direction")->AsVector();
  SpotLight::pAngleExp =
      effect->GetVariableByName("g_fSpotLight_AngleExp")->AsVector();
  SpotLight::pColor =
      effect->GetVariableByName("g_vSpotLight_Color")->AsVector();
  SpotLight::pNumSL = effect->GetVariableByName("g_nSpotLights")->AsScalar();
}

void SpotLight::OnFrameMove(float elapsed_time) {
  assert(SpotLight::pPos != NULL);
  D3DXMATRIX rotation_matrix;
  D3DXMatrixRotationYawPitchRoll(&rotation_matrix,
    elapsed_time * rotation_.x,
    elapsed_time * rotation_.y,
    elapsed_time * rotation_.z);
  D3DXVECTOR4 new_position;
  D3DXVec3Transform(&new_position, &position_, &rotation_matrix);
  position_ = static_cast<D3DXVECTOR3>(new_position);
  SpotLight::pPos->SetFloatVectorArray(position_, instance_id_, 1);
}
