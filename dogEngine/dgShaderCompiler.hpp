#ifndef SHADERCOMPILER_H
#define SHADERCOMPILER_H

#include "glslang_c_interface.h"
#include "Public/resource_limits_c.h"
#include "dgpch.h"
#include "dgLog.hpp"
namespace dg{
struct ShaderCompiler{
    struct spvObject{
        std::vector<unsigned int> spvData;
        size_t  binarySize = 0;
    };

    static void init(){
       bool isInitSuccess = glslang_initialize_process();
       if(isInitSuccess){
           DG_INFO("Shader Compiler is inited successfully")
       }else{
           DG_ERROR("Shader Compiler init error")
           exit(-1);
       }
    }
    static std::string readShaderFile(const char* path);
    static spvObject compileShader(std::string code);
    static void destroy(){
        glslang_finalize_process();
        DG_INFO("Shader Copmiler destroied done")
    }
};

bool endsWith(const char* s, const char* part){
    return (strstr( s, part ) - s) == (strlen( s ) - strlen( part ));
}

glslang_stage_t getShaderStageFromShaderName(const char* fileName){
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
    std::ifstream  file(path, std::ios::ate);
    if(!file.is_open()){
        DG_ERROR("Failed to open shader file");
        exit(-1);
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(),fileSize);
    file.close();
    std::string code = std::string(buffer.data());
    while(code.find("#include")!=code.npos){
        const auto pos = code.find("#include");
        const auto p1 = code.find('<',pos);
        const auto p2 = code.find('>',pos);
        if(p1==code.npos||p2==code.npos||p2<p1){
            DG_ERROR("Failed to build shader including");
            return "";
        }
        const std::string name = code.substr(p1+1,p2-p1-1);
        std::string strPath = std::string(path);
        auto find = strPath.find_last_of("/\\");
        const std::string currFilePath =strPath.substr(0,find+1);
        const std::string include = readShaderFile((currFilePath+name).c_str());
        code.replace(pos,p2-pos+1,include.c_str());
    }
    return code;
}

ShaderCompiler::spvObject ShaderCompiler::compileShader(std::string path) {
    std::string shaderSourceCode= readShaderFile(path.c_str());
    if(!shaderSourceCode.empty()){
        glslang_stage_t shaderStage = getShaderStageFromShaderName(path.c_str());
        glslang_input_t input{};
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
            input.resource = glslang_default_resource();
        }
        glslang_shader_t * shader = glslang_shader_create(&input);
        if(!glslang_shader_preprocess(shader,&input)){
            DG_ERROR("GLSL preprocessing failed in {}\n",path);
            DG_ERROR(glslang_shader_get_info_log(shader));
            DG_ERROR(glslang_shader_get_info_debug_log(shader));
            exit(-1);
        }

        if(!glslang_shader_parse(shader,&input)){
            DG_ERROR("GLSL parsing failed in {}\n", path);
            DG_ERROR(glslang_shader_get_info_log(shader));
            DG_ERROR(glslang_shader_get_info_debug_log(shader));
            exit(-1);
        }
        glslang_program_t * program = glslang_program_create();
        glslang_program_add_shader(program, shader);
        if(!glslang_program_link(program,GLSLANG_MSG_SPV_RULES_BIT|GLSLANG_MSG_VULKAN_RULES_BIT)){
            DG_ERROR("GLSL linking failed in {} \n",path);
            DG_ERROR(glslang_shader_get_info_log(shader));
            DG_ERROR(glslang_shader_get_info_debug_log(shader));
            exit(-1);
        }

        glslang_program_SPIRV_generate(program,shaderStage);
        spvObject res{};
        res.spvData.resize(glslang_program_SPIRV_get_size(program));
        glslang_program_SPIRV_get(program,res.spvData.data());
        {
            const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
            if(spirv_messages){
                DG_ERROR(spirv_messages);
            }
        }
        glslang_program_delete(program);
        glslang_shader_delete(shader);
        res.binarySize = res.spvData.size();
        DG_INFO("Shader ' {} ' compiled successfully",path);
        return res;
    }
    return {};
}

}
#endif //SHADERCOMPILER_H