#ifndef SHADERCOMPILER_H
#define SHADERCOMPILER_H

#include "dgLog.hpp"
#include "dgpch.h"
#include "glslang/Include/glslang_c_interface.h"
#include "glslang/Public/ShaderLang.h"

namespace dg {
struct ShaderCompiler {
  struct SpvObject {
    std::vector<unsigned int> spvData;
    size_t                    binarySize = 0;
  };

  static void init() {
    bool isInitSuccess = glslang_initialize_process();
    if (isInitSuccess) {
      DG_INFO("Shader Compiler is inited successfully")
    } else {
      DG_ERROR("Shader Compiler init error")
      exit(-1);
    }
  }
  static std::string readShaderFile(const char* path);
  static SpvObject   compileShader(std::string path);
  static void        destroy() {
    glslang_finalize_process();
    DG_INFO("Shader Copmiler destroied done")
  }
};
}// namespace dg
#endif//SHADERCOMPILER_H