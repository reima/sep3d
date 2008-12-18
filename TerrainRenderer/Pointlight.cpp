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

PointLight::~PointLight(void) {
}

void PointLight::GetHandles(ID3D10Effect *effect) {
  PointLight::pPos =
      effect->GetVariableByName("g_vPointLight_Position")->AsVector();
  PointLight::pColor =
      effect->GetVariableByName("g_vPointLight_Color")->AsVector();
  PointLight::pNumPL = effect->GetVariableByName("g_nPointLights")->AsScalar();
}

void PointLight::OnFrameMove(float elapsed_time) {
  assert(PointLight::pPos != NULL);
  D3DXMATRIX rotation_matrix;
  D3DXMatrixRotationYawPitchRoll(&rotation_matrix,
    elapsed_time * rotation_.y,
    elapsed_time * rotation_.x,
    elapsed_time * rotation_.z);
  D3DXVec3TransformCoord(&position_, &position_, &rotation_matrix);
  PointLight::pPos->SetFloatVectorArray(position_, instance_id_, 1);
}

void PointLight::OnDestroyDevice(void) {
  PointLight::instance_count = 0;
  PointLight::pPos = NULL;
  PointLight::pColor = NULL;
  PointLight::pNumPL = NULL;
}
