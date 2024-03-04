#ifndef DGRESOURCE_H
#define DGRESOURCE_H
#include "dgLog.hpp"
#include "dgplatform.h"
#include "gpuResource.hpp"
namespace dg {
struct Renderer;

struct Resource {
  void addReference() { ++references; }
  void removeReference() {
    DGASSERT(references == 0);
    --references;
  }
  u64         references = 0;
  std::string name;
};

struct TextureResource : public Resource {
  u32                      poolIdx;
  TextureHandle            handle;
  static const std::string k_type;
};

struct BufferResource : public Resource {
  u32                      poolIdx;
  BufferHandle             handle;
  static const std::string k_type;
};

struct SamplerResource : public Resource {
  u32                      poolIdx;
  SamplerHandle            handle;
  static const std::string k_type;
};

struct ProgramPassCreateInfo {
  PipelineCreateInfo pipelineInfo;
};

struct ProgramPass {
  PipelineHandle                         pipeline;
  std::vector<DescriptorSetLayoutHandle> descriptorSetLayout;
};

struct Program {
  Program() {}
  Program(std::shared_ptr<DeviceContext> context, std::string name);
  ~Program();
  void                           addPass(ProgramPassCreateInfo& pipeInfo);
  std::string                    name;
  std::vector<ProgramPass>       passes;
  std::shared_ptr<DeviceContext> context;
};

struct uniformRenderData {
  alignas(16) glm::vec3 lightDirection;
};

struct TextureBind {
  TextureHandle texture = {k_invalid_index};
  u32           bindIdx = -1;
};

struct MaterialCreateInfo {
  MaterialCreateInfo& setRenderOrder(int renderOrder);
  MaterialCreateInfo& setName(std::string name);
  std::string         name;
  int                 renderOrder = -1;
  Renderer*           renderer;
};

struct Material : public Resource {
  struct alignas(16) aliInt {
    int idx;
  };

  struct alignas(16) UniformMaterial {
    alignas(16) glm::mat4 modelMatrix = glm::mat4(1.0f);
    alignas(16) glm::vec4 baseColorFactor = glm::vec4(glm::vec3(0.7f), 1.0f);
    alignas(16) glm::vec3 emissiveFactor = glm::vec3(0.0f);

    //envrotate, envExposure, envGamma
    alignas(16) glm::vec3 envFactor = glm::vec3(1.0f, 2.5f, 1.2f);
    //metallic, roughness, ao intensity
    alignas(16) glm::vec3 mrFactor = glm::vec3(1.0f, 1.0f, 1.0f);
    //normal intensity,emissive intensity,______
    alignas(16) glm::vec4 intensity = glm::vec4(1.0f);
    //use albedo,normal/mrao/emissive
    alignas(16) aliInt textureUseSetting[4] = {1, 1, 1, 1};
    alignas(16) aliInt textureIndices[k_max_bindless_resource];
  } uniformMaterial;
  Material();
  void addTexture(Renderer* renderer, std::string name, std::string path);
  void addDiffuseEnvMap(Renderer* renderer, TextureHandle handle);
  void addSpecularEnvMap(Renderer* renderer, TextureHandle handle);
  void setIblMap(Renderer* renderer, std::string path);
  void updateProgram();
  void setDiffuseTexture(TextureHandle handle) { textureMap["DiffuseTexture"].texture = handle; }
  void setNormalTexture(TextureHandle handle) { textureMap["NormalTexture"].texture = handle; }
  void setMRTexture(TextureHandle handle) {
    textureMap["MetallicRoughnessTexture"].texture = handle;
  }
  void setEmissiveTexture(TextureHandle handle) { textureMap["EmissiveTexture"].texture = handle; }
  void setAoTexture(TextureHandle handle) { textureMap["AOTexture"].texture = handle; }
  void setProgram(const std::shared_ptr<Program>& program);

 protected:
  void addLutTexture(Renderer* renderer);
  void setIblMap(Renderer* renderer, TextureHandle handle);

 public:
  Renderer*                                    renderer;
  u32                                          poolIdx;
  std::shared_ptr<Program>                     program;
  u32                                          renderOrder;
  std::unordered_map<std::string, TextureBind> textureMap;
  //render state
  bool      depthTest = true;
  bool      depthWrite = true;
  bool      useAlbedoTexture = true;
  bool      useNormalTexture = true;
  bool      useMRTexture = true;
  bool      useEmisTexture = true;
  bool      useEnvTexture = true;
  int       depthModeIdx = 0;
  int       cullModeIdx = 0;
  VkBlendOp blendOp;
};
}// namespace dg

#endif//DGRESOURCE_H