#include "Pointlight.h"
#include "DXUT.h"
unsigned int Pointlight::InstanceCount = 0;
ID3D10EffectVectorVariable* Pointlight::pPos = NULL;
ID3D10EffectVectorVariable* Pointlight::pColor = NULL;
ID3D10EffectScalarVariable* Pointlight::pNumPL = NULL;



void Pointlight::OnFrameMove(float fElapsedTime){
  D3DXMATRIX *pOut = new D3DXMATRIX();
  D3DXMatrixRotationYawPitchRoll(pOut ,  0.01f,  0.02f,  0.03f);
  

  D3DXVECTOR4 *vOut = new D3DXVECTOR4();
  D3DXVec3Transform(vOut, &Position, pOut);
  
  Position = D3DXVECTOR3(vOut->x,vOut->y,vOut->z);
  pPos[ID].SetFloatVector(Position);

  delete(pOut);
  delete(vOut);

}

void Pointlight::GetHandles(ID3D10Effect *pFx){
  Pointlight::pPos = pFx->GetVariableByName("g_vPointlight_Position")->AsVector();
  Pointlight::pColor = pFx->GetVariableByName("g_vPointlight_Color")->AsVector();
  Pointlight::pNumPL = pFx->GetVariableByName("g_iNumOfPL")->AsScalar();

}

Pointlight::Pointlight(D3DXVECTOR3 pos, D3DXVECTOR4 color): Position(pos){
  ID = Pointlight::InstanceCount++;
  pColor[ID].SetFloatVector(color);
  pNumPL->SetInt(ID+1);

}

Pointlight::~Pointlight(void)
{
}
