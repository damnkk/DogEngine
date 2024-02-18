#include "dgShaderCompiler.h"

namespace dg {
TBuiltInResource make_default_builtin_rsc() {
  TBuiltInResource builtin_rsc{};
  builtin_rsc.maxLights = 32;
  builtin_rsc.maxClipPlanes = 6;
  builtin_rsc.maxTextureUnits = 32;
  builtin_rsc.maxTextureCoords = 32;
  builtin_rsc.maxVertexAttribs = 64;
  builtin_rsc.maxVertexUniformComponents = 4096;
  builtin_rsc.maxVaryingFloats = 64;
  builtin_rsc.maxVertexTextureImageUnits = 32;
  builtin_rsc.maxCombinedTextureImageUnits = 80;
  builtin_rsc.maxTextureImageUnits = 32;
  builtin_rsc.maxFragmentUniformComponents = 4096;
  builtin_rsc.maxDrawBuffers = 32;
  builtin_rsc.maxVertexUniformVectors = 128;
  builtin_rsc.maxVaryingVectors = 8;
  builtin_rsc.maxFragmentUniformVectors = 16;
  builtin_rsc.maxVertexOutputVectors = 16;
  builtin_rsc.maxFragmentInputVectors = 15;
  builtin_rsc.minProgramTexelOffset = -8;
  builtin_rsc.maxProgramTexelOffset = 7;
  builtin_rsc.maxClipDistances = 8;
  builtin_rsc.maxComputeWorkGroupCountX = 65535;
  builtin_rsc.maxComputeWorkGroupCountY = 65535;
  builtin_rsc.maxComputeWorkGroupCountZ = 65535;
  builtin_rsc.maxComputeWorkGroupSizeX = 1024;
  builtin_rsc.maxComputeWorkGroupSizeY = 1024;
  builtin_rsc.maxComputeWorkGroupSizeZ = 64;
  builtin_rsc.maxComputeUniformComponents = 1024;
  builtin_rsc.maxComputeTextureImageUnits = 16;
  builtin_rsc.maxComputeImageUniforms = 8;
  builtin_rsc.maxComputeAtomicCounters = 8;
  builtin_rsc.maxComputeAtomicCounterBuffers = 1;
  builtin_rsc.maxVaryingComponents = 60;
  builtin_rsc.maxVertexOutputComponents = 64;
  builtin_rsc.maxGeometryInputComponents = 64;
  builtin_rsc.maxGeometryOutputComponents = 128;
  builtin_rsc.maxFragmentInputComponents = 128;
  builtin_rsc.maxImageUnits = 8;
  builtin_rsc.maxCombinedImageUnitsAndFragmentOutputs = 8;
  builtin_rsc.maxCombinedShaderOutputResources = 8;
  builtin_rsc.maxImageSamples = 0;
  builtin_rsc.maxVertexImageUniforms = 0;
  builtin_rsc.maxTessControlImageUniforms = 0;
  builtin_rsc.maxTessEvaluationImageUniforms = 0;
  builtin_rsc.maxGeometryImageUniforms = 0;
  builtin_rsc.maxFragmentImageUniforms = 8;
  builtin_rsc.maxCombinedImageUniforms = 8;
  builtin_rsc.maxGeometryTextureImageUnits = 16;
  builtin_rsc.maxGeometryOutputVertices = 256;
  builtin_rsc.maxGeometryTotalOutputComponents = 1024;
  builtin_rsc.maxGeometryUniformComponents = 1024;
  builtin_rsc.maxGeometryVaryingComponents = 64;
  builtin_rsc.maxTessControlInputComponents = 128;
  builtin_rsc.maxTessControlOutputComponents = 128;
  builtin_rsc.maxTessControlTextureImageUnits = 16;
  builtin_rsc.maxTessControlUniformComponents = 1024;
  builtin_rsc.maxTessControlTotalOutputComponents = 4096;
  builtin_rsc.maxTessEvaluationInputComponents = 128;
  builtin_rsc.maxTessEvaluationOutputComponents = 128;
  builtin_rsc.maxTessEvaluationTextureImageUnits = 16;
  builtin_rsc.maxTessEvaluationUniformComponents = 1024;
  builtin_rsc.maxTessPatchComponents = 120;
  builtin_rsc.maxPatchVertices = 32;
  builtin_rsc.maxTessGenLevel = 64;
  builtin_rsc.maxViewports = 16;
  builtin_rsc.maxVertexAtomicCounters = 0;
  builtin_rsc.maxTessControlAtomicCounters = 0;
  builtin_rsc.maxTessEvaluationAtomicCounters = 0;
  builtin_rsc.maxGeometryAtomicCounters = 0;
  builtin_rsc.maxFragmentAtomicCounters = 8;
  builtin_rsc.maxCombinedAtomicCounters = 8;
  builtin_rsc.maxAtomicCounterBindings = 1;
  builtin_rsc.maxVertexAtomicCounterBuffers = 0;
  builtin_rsc.maxTessControlAtomicCounterBuffers = 0;
  builtin_rsc.maxTessEvaluationAtomicCounterBuffers = 0;
  builtin_rsc.maxGeometryAtomicCounterBuffers = 0;
  builtin_rsc.maxFragmentAtomicCounterBuffers = 1;
  builtin_rsc.maxCombinedAtomicCounterBuffers = 1;
  builtin_rsc.maxAtomicCounterBufferSize = 16384;
  builtin_rsc.maxTransformFeedbackBuffers = 4;
  builtin_rsc.maxTransformFeedbackInterleavedComponents = 64;
  builtin_rsc.maxCullDistances = 8;
  builtin_rsc.maxCombinedClipAndCullDistances = 8;
  builtin_rsc.maxSamples = 4;
  builtin_rsc.maxMeshOutputVerticesNV = 256;
  builtin_rsc.maxMeshOutputPrimitivesNV = 512;
  builtin_rsc.maxMeshWorkGroupSizeX_NV = 32;
  builtin_rsc.maxMeshWorkGroupSizeY_NV = 1;
  builtin_rsc.maxMeshWorkGroupSizeZ_NV = 1;
  builtin_rsc.maxTaskWorkGroupSizeX_NV = 32;
  builtin_rsc.maxTaskWorkGroupSizeY_NV = 1;
  builtin_rsc.maxTaskWorkGroupSizeZ_NV = 1;
  builtin_rsc.maxMeshViewCountNV = 4;
  builtin_rsc.maxDualSourceDrawBuffersEXT = 1;
  builtin_rsc.limits.nonInductiveForLoops = 1;
  builtin_rsc.limits.whileLoops = 1;
  builtin_rsc.limits.doWhileLoops = 1;
  builtin_rsc.limits.generalUniformIndexing = 1;
  builtin_rsc.limits.generalAttributeMatrixVectorIndexing = 1;
  builtin_rsc.limits.generalVaryingIndexing = 1;
  builtin_rsc.limits.generalSamplerIndexing = 1;
  builtin_rsc.limits.generalVariableIndexing = 1;
  builtin_rsc.limits.generalConstantMatrixVectorIndexing = 1;
  return builtin_rsc;
}

bool endsWith(const char* s, const char* part) {
  return (strstr(s, part) - s) == (strlen(s) - strlen(part));
}

glslang_stage_t getShaderStageFromShaderName(const char* fileName) {
  if (endsWith(fileName, ".vert"))
    return GLSLANG_STAGE_VERTEX;

  if (endsWith(fileName, ".frag"))
    return GLSLANG_STAGE_FRAGMENT;

  if (endsWith(fileName, ".geom"))
    return GLSLANG_STAGE_GEOMETRY;

  if (endsWith(fileName, ".comp"))
    return GLSLANG_STAGE_COMPUTE;

  if (endsWith(fileName, ".tesc"))
    return GLSLANG_STAGE_TESSCONTROL;

  if (endsWith(fileName, ".tese"))
    return GLSLANG_STAGE_TESSEVALUATION;

  return GLSLANG_STAGE_VERTEX;
}

std::string ShaderCompiler::readShaderFile(const char* path) {
  std::ifstream file(path, std::ios::ate);
  if (!file.is_open()) {
    DG_ERROR("Failed to open shader file");
    exit(-1);
  }
  size_t            fileSize = (size_t) file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();
  std::string code = std::string(buffer.data());
  while (code.find("#include") != code.npos) {
    const auto pos = code.find("#include");
    const auto p1 = code.find('<', pos);
    const auto p2 = code.find('>', pos);
    if (p1 == code.npos || p2 == code.npos || p2 < p1) {
      DG_ERROR("Failed to build shader including");
      return "";
    }
    const std::string name = code.substr(p1 + 1, p2 - p1 - 1);
    std::string       strPath = std::string(path);
    auto              find = strPath.find_last_of("/\\");
    const std::string currFilePath = strPath.substr(0, find + 1);
    const std::string include = readShaderFile((currFilePath + name).c_str());
    code.replace(pos, p2 - pos + 1, include.c_str());
  }
  return code;
}

ShaderCompiler::SpvObject ShaderCompiler::compileShader(std::string path) {
  std::string shaderSourceCode = readShaderFile(path.c_str());
  if (!shaderSourceCode.empty()) {
    glslang_stage_t  shaderStage = getShaderStageFromShaderName(path.c_str());
    TBuiltInResource defaultResouce = make_default_builtin_rsc();
    glslang_input_t  input{};
    {
      input.language = GLSLANG_SOURCE_GLSL;
      input.stage = shaderStage;
      input.client = GLSLANG_CLIENT_VULKAN;
      input.client_version = GLSLANG_TARGET_VULKAN_1_3;
      input.target_language = GLSLANG_TARGET_SPV;
      input.target_language_version = GLSLANG_TARGET_SPV_1_6;
      input.code = shaderSourceCode.c_str();
      input.default_version = 100;
      //这个选项存疑
      input.default_profile = GLSLANG_NO_PROFILE;
      input.force_default_version_and_profile = false;
      input.forward_compatible = false;
      input.messages = GLSLANG_MSG_DEFAULT_BIT;
      input.resource = reinterpret_cast<const glslang_resource_t*>(&defaultResouce);
    }
    glslang_shader_t* shader = glslang_shader_create(&input);
    if (!glslang_shader_preprocess(shader, &input)) {
      DG_ERROR("GLSL preprocessing failed in {}\n", path);
      DG_ERROR(glslang_shader_get_info_log(shader));
      DG_ERROR(glslang_shader_get_info_debug_log(shader));
      exit(-1);
    }

    if (!glslang_shader_parse(shader, &input)) {
      DG_ERROR("GLSL parsing failed in {}\n", path);
      DG_ERROR(glslang_shader_get_info_log(shader));
      DG_ERROR(glslang_shader_get_info_debug_log(shader));
      exit(-1);
    }
    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);
    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
      DG_ERROR("GLSL linking failed in {} \n", path);
      DG_ERROR(glslang_shader_get_info_log(shader));
      DG_ERROR(glslang_shader_get_info_debug_log(shader));
      exit(-1);
    }

    glslang_program_SPIRV_generate(program, shaderStage);
    SpvObject res{};
    res.spvData.resize(glslang_program_SPIRV_get_size(program));
    glslang_program_SPIRV_get(program, res.spvData.data());
    {
      const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
      if (spirv_messages) {
        DG_ERROR(spirv_messages);
      }
    }
    glslang_program_delete(program);
    glslang_shader_delete(shader);
    res.binarySize = res.spvData.size();
    DG_INFO("Shader ' {} ' compiled successfully", path);
    return res;
  }
  return {};
}
}// namespace dg