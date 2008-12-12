#include <string>
#include <sstream>
#include "DXUT.h"

ID3D10Effect* LoadEffect(ID3D10Device* pd3dDevice,
                          const std::wstring& filename,
                          const D3D10_SHADER_MACRO *Shader_Macros=NULL,
                          const bool bDebugCompile=false) {

      assert(pd3dDevice != NULL);

      ID3D10Effect* pEffect10 = NULL;

      DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;

      #if defined( DEBUG ) || defined( _DEBUG )
            if (bDebugCompile) dwShaderFlags |= D3D10_SHADER_DEBUG;
      #else
            UNREFERENCED_PARAMETER(bDebugCompile);
      #endif

      ID3D10Blob *errors=NULL;
      HRESULT hr = D3DX10CreateEffectFromFile(filename.c_str(), Shader_Macros, NULL, "fx_4_0", dwShaderFlags, 0, pd3dDevice, NULL, NULL, &pEffect10, &errors, NULL );

      if (FAILED(hr)) {
            std::wstringstream s;
            if (errors == NULL) {
                  s << L"Unknown error loading shader: \"" << filename << L"\".";
            } else {
                  s << L"Error loading shader \"" << filename << "\"\n " << (CHAR*)errors->GetBufferPointer();
            }
            OutputDebugStringW(s.str().c_str());
            MessageBoxW(NULL, s.str().c_str(), L"Shader Load Error", MB_OK);
            return NULL;
      }

#if defined( DEBUG ) || defined( _DEBUG )
      if (errors != NULL) {
            std::wstringstream s;
            s << L"Warnings loading shader \"" << filename << "\"\n " << (CHAR*)errors->GetBufferPointer();
            OutputDebugStringW(s.str().c_str());
            MessageBoxW(NULL, s.str().c_str(), L"Shader Load Warnings", MB_OK);
      }
#endif

      return pEffect10;
}
