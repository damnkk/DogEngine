#include "Renderer.h"
#include "CommandBuffer.h"
#include "ModelLoader.h"
#include "dgGeometry.h"
#include "ktx.h"
#include "ktxvulkan.h"
#include "unordered_set"
#include "dgShaderCompiler.hpp"

namespace dg {

#define RDOC_CATCH_START renderer->getContext()->m_renderDoc_api->StartFrameCapture(NULL, NULL);
#define RDOC_CATCH_END renderer->getContext()->m_renderDoc_api->EndFrameCapture(NULL, NULL);

    const std::string TextureResource::k_type = "texture_resource";
    const std::string BufferResource::k_type = "buffer_resource";
    const std::string SamplerResource::k_type = "sampler_resource";

    u32 CalcuMipMap(const int &width, const int &height) {
        return std::floor(std::log2(std::max(width, height))) + 1;
    }

    void ResourceCache::destroy(Renderer *renderer) {
        for (auto it = textures.begin(); it != textures.end(); ++it) {
            if (!it->second) {
                DG_WARN("An invalid TextureResource pointer");
                return;
            }
            renderer->getContext()->DestroyTexture(it->second->handle);
            renderer->getTextures().release(it->second);
        }
        textures.clear();

        for (auto it = samplers.begin(); it != samplers.end(); ++it) {
            if (!it->second) {
                DG_WARN("An invalid SamplerResource pointer");
                return;
            }
            renderer->getContext()->DestroySampler(it->second->handle);
            renderer->getSamplers().release(it->second);
        }
        samplers.clear();

        for (auto it = buffers.begin(); it != buffers.end(); ++it) {
            if (!it->second) {
                DG_WARN("An invalid bufferResource pointer");
                return;
            }
            renderer->getContext()->DestroyBuffer(it->second->handle);
            renderer->getBuffers().release(it->second);
        }
        buffers.clear();

        for (auto it = materials.begin(); it != materials.end(); ++it) {
            if (!it->second) {
                DG_WARN("An invalid Material pointer");
                return;
            }
            renderer->getMaterials().release(it->second);
        }
        materials.clear();
    }

    TextureResource* ResourceCache::queryTexture(const std::string &name) {
        auto find = this->textures.find(name);
        if(find != this->textures.end()){
            return find->second;
        }
        return nullptr;
    }

    SamplerResource* ResourceCache::querySampler(const std::string &name) {
        auto find = this->samplers.find(name);
        if(find != this->samplers.end()){
            return find->second;
        }
        return nullptr;
    }

    BufferResource* ResourceCache::QueryBuffer(const std::string &name){
        auto find = this->buffers.find(name);
        if(find != this->buffers.end()){
            return find->second;
        }
        return nullptr;
    }

    Material* ResourceCache::QueryMaterial(const std::string &name){
        auto find = this->materials.find(name);
        if(find != this->materials.end()){
            return find->second;
        }
        return nullptr;
    }

    void Renderer::makeDefaultMaterial() {
        MaterialCreateInfo matCI{};
        matCI.setName("defaultMat");
        m_defaultMaterial = createMaterial(matCI);
        DescriptorSetLayoutCreateInfo descInfo = m_context->m_defaultSetLayoutCI;
        descInfo.addBinding({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1, "MaterialUniformBuffer"});
        descInfo.setName("defaultDescLayout");
        DescriptorSetLayoutHandle descLayout = m_context->createDescriptorSetLayout(descInfo);
        PipelineCreateInfo pipelineInfo{};
//        BlendStateCreation blendState{};
//        blendState.add_blend_state();
//        blendState.m_blendStates[0].m_blendEnabled = true;
//        blendState.m_blendStates[0].m_separateBlend = false;
//        pipelineInfo.m_BlendState =blendState;
        pipelineInfo.m_renderPassHandle = m_context->m_swapChainPass;
        pipelineInfo.m_depthStencil.setDepth(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineInfo.m_vertexInput.Attrib = Vertex::getAttributeDescriptions();
        pipelineInfo.m_vertexInput.Binding = Vertex::getBindingDescription();
        pipelineInfo.m_shaderState.reset();
        auto vsShader = ShaderCompiler::compileShader("./shaders/vspbr.vert");
        auto fsShader = ShaderCompiler::compileShader("./shaders/fspbr.frag");
        pipelineInfo.m_shaderState.addStage(vsShader.spvData.data(), vsShader.spvData.size()*sizeof(unsigned int), VK_SHADER_STAGE_VERTEX_BIT);
        pipelineInfo.m_shaderState.addStage(fsShader.spvData.data(), fsShader.spvData.size()*sizeof(unsigned int), VK_SHADER_STAGE_FRAGMENT_BIT);
        pipelineInfo.m_shaderState.setName("defaultPipeline");
        pipelineInfo.name = pipelineInfo.m_shaderState.name;
        pipelineInfo.addDescriptorSetlayout(descLayout);
        pipelineInfo.addDescriptorSetlayout(m_context->m_bindlessDescriptorSetLayout);
        m_defaultMaterial->setProgram(createProgram("defaultPbrProgram", {pipelineInfo}));
        m_defaultMaterial->setIblMap(this, "./models/skybox/graveyard_pathways_2k.hdr");
    }

    void Renderer::init(std::shared_ptr<DeviceContext> context) {
        m_context = context;
        GUI::getInstance().init(context);
        ShaderCompiler::init();
        m_buffers.init(4096);
        m_textures.init(512);
        m_samplers.init(512);
        m_materials.init(512);
        m_objLoader = std::make_shared<objLoader>(this);
        m_gltfLoader = std::make_shared<gltfLoader>(this);

        makeDefaultMaterial();
    }

    void Renderer::destroy() {
        vkDeviceWaitIdle(m_context->m_logicDevice);
        GUI::getInstance().Destroy();
        ShaderCompiler::destroy();
        m_resourceCache.destroy(this);
        m_gltfLoader->destroy();
        m_objLoader->destroy();
        m_textures.destroy();
        m_buffers.destroy();
        m_materials.destroy();
        m_samplers.destroy();
        destroySkyBox();
        m_context->Destroy();
    }

    TextureResource *Renderer::createTexture(TextureCreateInfo &textureInfo) {
        if (textureInfo.name.empty()) {
            DG_ERROR("Sampled texture object must have an unique name");
            exit(-1);
        }
        auto findRes = m_resourceCache.textures.find(textureInfo.name);
        if (m_resourceCache.textures.find(textureInfo.name) != m_resourceCache.textures.end()) {
            DG_INFO("There is a Texture with the same name:{} in resourceCache, renderer will reuse it", textureInfo.name);
            return findRes->second;
        }
        textureInfo.setMipmapLevel(std::min(textureInfo.m_mipmapLevel, CalcuMipMap(textureInfo.m_textureExtent.width, textureInfo.m_textureExtent.height)));
        auto textureRes = m_textures.obtain();
        textureRes->name = textureInfo.name;
        textureRes->handle = m_context->createTexture(textureInfo);
        if(textureInfo.m_mipmapLevel>1){
            SamplerCreateInfo sampleInfo{};
            sampleInfo.set_min_mag_mip(VK_FILTER_LINEAR,VK_FILTER_LINEAR,VK_SAMPLER_MIPMAP_MODE_LINEAR).set_LodMinMaxBias(0,(float)textureInfo.m_mipmapLevel,0)
                    .set_address_uvw(VK_SAMPLER_ADDRESS_MODE_REPEAT,VK_SAMPLER_ADDRESS_MODE_REPEAT,VK_SAMPLER_ADDRESS_MODE_REPEAT).set_name(textureInfo.name.c_str());
            auto* sampleResource = this->createSampler(sampleInfo);
            Texture* tex = m_context->accessTexture(textureRes->handle);
            tex->setSampler(this->m_context,sampleResource->handle);
        }
        m_resourceCache.textures[textureInfo.name] = textureRes;
        return textureRes;
    }

    BufferResource *Renderer::createBuffer(BufferCreateInfo &bufferInfo) {
        if (bufferInfo.name.empty()) {
            DG_ERROR("Buffer created by renderer must have an unique name");
            exit(-1);
        }
        auto find = m_resourceCache.buffers.find(bufferInfo.name);
        if (find != m_resourceCache.buffers.end()) {
            DG_WARN("You are trying to create a buffer which has a same name{} with an existing buffer, through renderer",bufferInfo.name);
            return find->second;
        }
        auto bufferRes = m_buffers.obtain();
        bufferRes->name = bufferInfo.name;
        bufferRes->handle = m_context->createBuffer(bufferInfo);
        m_resourceCache.buffers[bufferInfo.name] = bufferRes;
        return bufferRes;
    }

    SamplerResource *Renderer::createSampler(SamplerCreateInfo &samplerInfo) {
        if (samplerInfo.name.empty()) {
            DG_ERROR("Sampler created by renderer must have an unique name")
            exit(-1);
        }
        auto findRes = m_resourceCache.samplers.find(samplerInfo.name);
        if (findRes != m_resourceCache.samplers.end()) {
            DG_INFO("There is a Sampler with the same name:{} in resourceCache, renderer will reuse it", samplerInfo.name);
            return findRes->second;
        }
        auto samplerRes = m_samplers.obtain();
        samplerRes->name = samplerInfo.name;
        samplerRes->handle = m_context->createSampler(samplerInfo);
        m_resourceCache.samplers[samplerInfo.name] = samplerRes;
        return samplerRes;
    }

    Material *Renderer::createMaterial(MaterialCreateInfo &matInfo) {
        if (matInfo.name.empty()) {
            DG_ERROR("Material created by renderer must have an unique name");
            exit(-1);
        }
        auto findRes = m_resourceCache.materials.find(matInfo.name);
        if (findRes != m_resourceCache.materials.end()) {
            DG_INFO("There is a material with the same name in resourceCache, renderer will reuse it")
            return findRes->second;
        }
        auto material = m_materials.obtain();
        material->name = matInfo.name;
        material->renderOrder = matInfo.renderOrder;
        material->renderer = this;
        m_resourceCache.materials[material->name] = material;
        return material;
    }

    std::shared_ptr<Program> Renderer::createProgram(const std::string &name, ProgramPassCreateInfo progPassCI) {
        std::shared_ptr<Program> pg = std::make_shared<Program>(m_context, name);
        u32 numPasses = 1;
        pg->passes.resize(numPasses);
        for (int i = 0; i < numPasses; ++i) {
            ProgramPass &pass = pg->passes[i];
            pass.pipeline = m_context->createPipeline(progPassCI.pipelineInfo);
            for (auto &layoutHandle: progPassCI.pipelineInfo.m_descLayout) {
                pass.descriptorSetLayout.emplace_back(layoutHandle);
            }
        }
        return pg;
    }


    void Renderer::destroyBuffer(BufferResource *bufRes) {
        if (!bufRes) {
            DG_WARN("An invalid bufferResource pointer");
            return;
        }
        auto it = m_resourceCache.buffers.find(bufRes->name);
        if (it != m_resourceCache.buffers.end()) {
            m_resourceCache.buffers.erase(it);
        }
        m_context->DestroyBuffer(bufRes->handle);
        m_buffers.release(bufRes);
    }

    void Renderer::destroyTexture(TextureResource *texRes) {
        if (!texRes) {
            DG_WARN("An invalid TextureResource pointer");
            return;
        }
        auto it = m_resourceCache.textures.find(texRes->name);
        if (it != m_resourceCache.textures.end()) {
            m_resourceCache.textures.erase(it);
        }
        m_context->DestroyTexture(texRes->handle);
        m_textures.release(texRes);
    }

    void Renderer::destroySampler(SamplerResource *samplerRes) {
        if (!samplerRes) {
            DG_WARN("An invalid SamplerResource pointer");
            return;
        }
        auto it = m_resourceCache.samplers.find(samplerRes->name);
        if (it != m_resourceCache.samplers.end()) {
            m_resourceCache.samplers.erase(it);
        }
        m_context->DestroySampler(samplerRes->handle);
        m_samplers.release(samplerRes);
    }

    void Renderer::destroyMaterial(Material *material) {
        if (!material) {
            DG_WARN("An invalid Material pointer");
            return;
        }
        auto it = m_resourceCache.materials.find(material->name);
        if (it != m_resourceCache.materials.end()) {
            m_resourceCache.materials.erase(it);
        }
        m_materials.release(material);
    }


    void Renderer::newFrame() {
        m_context->newFrame();
        GUI::getInstance().newGUIFrame();
    }

    void Renderer::present() {
        m_context->present();
    }

    void Renderer::loadFromPath(const std::string &path) {
        if (path.empty()) {
            DG_WARN("Invalid resource path");
            return;
        }
        auto found = path.find_last_of(".");
        std::string fileExtension = path.substr(found + 1);
        if (fileExtension == "obj") {
            m_objLoader->loadFromPath(path);
        } else if (fileExtension == "gltf") {
            m_gltfLoader->loadFromPath(path);
        }
    }

    void Renderer::executeScene() {
        if (have_skybox) executeSkyBox();
        if (m_objLoader->haveContent()) {
            m_objLoader->Execute(m_objLoader->getSceneRoot());
            m_renderObjects.insert(m_renderObjects.end(), m_objLoader->getRenderObject().begin(), m_objLoader->getRenderObject().end());
        }
        if (m_gltfLoader->haveContent()) {
            m_gltfLoader->Execute(m_gltfLoader->getSceneRoot());
            m_renderObjects.insert(m_renderObjects.end(), m_gltfLoader->getRenderObject().begin(), m_gltfLoader->getRenderObject().end());
        }
    }

    void Renderer::drawScene() {
        CommandBuffer *cmd = m_context->getCommandBuffer(QueueType::Enum::Graphics, true);
        for (int i = 0; i < m_renderObjects.size(); ++i) {
            auto &currRenderObject = m_renderObjects[i];
            cmd->bindPass(currRenderObject.m_renderPass);
            cmd->bindPipeline(currRenderObject.m_material->program->passes[0].pipeline);
            Rect2DInt scissor;
            scissor = {0, 0, (u16) m_context->m_swapChainWidth, (u16) m_context->m_swapChainHeight};
            cmd->setScissor(&scissor);
            ViewPort viewPort;
            viewPort.max_depth = 1.0f;
            viewPort.min_depth = 0.0f;
            viewPort.rect.width = scissor.width;
            viewPort.rect.height = scissor.height;

            cmd->setViewport(&viewPort);
            cmd->bindVertexBuffer(currRenderObject.m_vertexBuffer, 0, 0);
            cmd->bindIndexBuffer(currRenderObject.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdSetDepthTestEnable(cmd->m_commandBuffer, VK_TRUE);
            Buffer *globalUniformBuffer = m_context->accessBuffer(currRenderObject.m_GlobalUniform);
            if (!globalUniformBuffer) {
                DG_WARN("Invalid uniform Buffer");
                continue;
            }
            void *data;
            vmaMapMemory(m_context->m_vma, globalUniformBuffer->m_allocation, &data);
            UniformData udata{};
            size_t test = sizeof(UniformData);
            udata.cameraPos = m_context->m_camera->pos;
            udata.cameraDirectory = m_context->m_camera->direction;
            static auto startTime = std::chrono::high_resolution_clock::now();
            auto endTime = std::chrono::high_resolution_clock::now();

            udata.projectMatrix = m_context->m_camera->getProjectMatrix();
            udata.viewMatrix = m_context->m_camera->getViewMatrix();
            memcpy(data, &udata, sizeof(UniformData));
            vmaUnmapMemory(m_context->m_vma, globalUniformBuffer->m_allocation);
            Material::UniformMaterial um{};
            um.baseColorFactor = currRenderObject.m_material->uniformMaterial.baseColorFactor;
            um.emissiveFactor = currRenderObject.m_material->uniformMaterial.emissiveFactor;
            um.modelMatrix = currRenderObject.m_modelMatrix;
            um.mrFactor = currRenderObject.m_material->uniformMaterial.mrFactor;
            um.tueFactor = currRenderObject.m_material->uniformMaterial.tueFactor;
            for (auto &[first, second]: currRenderObject.m_material->textureMap) {
                um.textureIndices[second.bindIdx] = {(int) second.texture.index};
            }
            Buffer *materialBuffer = m_context->accessBuffer(currRenderObject.m_MaterialUniform);
            vmaMapMemory(m_context->m_vma, materialBuffer->m_allocation, &data);
            memcpy(data, &um, sizeof(Material::UniformMaterial));
            vmaUnmapMemory(m_context->m_vma, materialBuffer->m_allocation);
            cmd->bindDescriptorSet(currRenderObject.m_descriptors, 0, nullptr, 0);
            cmd->bindDescriptorSet({m_context->m_bindlessDescriptorSet}, 1, nullptr, 0);
            //cmd->draw(TopologyType::Enum::Triangle, 0, currRenderObject.m_vertexCount, 0, 0);
            cmd->drawIndexed(TopologyType::Enum::Triangle, currRenderObject.m_indexCount, 1, 0, 0, 0);
        }
        //cmd->endpass();
    }

    void Renderer::setDefaultMaterial(Material *mat) {
        if (!mat) {
            DG_WARN("You are implementing a invalid material as current material, check again");
            return;
        }
        m_defaultMaterial = mat;
    }

    Material *Renderer::getDefaultMaterial() {
        return m_defaultMaterial;
    }

    void Renderer::drawUI() {
        //之后可以在这里补充一些其他的界面逻辑
        //这里UI绘制和主场景绘制的commandbuffer共用,效果不好,回头单独给GUI创建一个commandbufferpool,分开录制。
        GUI::getInstance().OnGUI();
        GUI::getInstance().endGUIFrame();
        auto cmd = m_context->getCommandBuffer(QueueType::Enum::Graphics, false);
        auto drawData = GUI::getInstance().GetDrawData();
        //VkRenderPassBeginInfo passBeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        //passBeginInfo.
        ImGui_ImplVulkan_RenderDrawData(drawData, cmd->m_commandBuffer);
        m_context->queueCommandBuffer(cmd);
    }

    TextureHandle Renderer::upLoadTextureToGPU(std::string &texPath, TextureCreateInfo &texInfo) {
        if (texPath.empty()) return {k_invalid_index};
        auto found = texPath.find_last_of('.');
        if (texPath.substr(found + 1) == "ktx") {
            ktxTexture *ktxTexture;
            ktxResult res = ktxTexture_CreateFromNamedFile(texPath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
            DGASSERT(res == KTX_SUCCESS);
            TextureCreateInfo texCI{};
            texCI.setData(ktxTexture).setName("skyTexture").setFormat(VK_FORMAT_R16G16B16A16_SFLOAT).setFlag(TextureFlags::Mask::Default_mask).setMipmapLevel(ktxTexture->numLevels).setExtent({ktxTexture->baseWidth, ktxTexture->baseHeight, 1}).setBindLess(true);
            texCI.m_imageType = TextureType::Enum::TextureCube;
            TextureHandle handle = createTexture(texCI)->handle;
            ktxTexture_Destroy(ktxTexture);
            ktxTexture = nullptr;
            return handle;
        }
        u32 foundHead = texPath.find_last_of("/\\");
        u32 foundEnd = texPath.find_last_not_of(".");
        std::string texName = texPath.substr(foundHead + 1, foundEnd);
        auto* resource = this->m_resourceCache.queryTexture(texName);
        if(resource){
            return resource->handle;
        }
        int imgWidth, imgHeight, imgChannel;
        stbi_uc *ptr = stbi_load(texPath.c_str(), &imgWidth, &imgHeight, &imgChannel, 4);
        texInfo.setName(texName.c_str()).setData(ptr).setExtent({(u32) imgWidth, (u32) imgHeight, 1}).setMipmapLevel(std::min(texInfo.m_mipmapLevel, CalcuMipMap(imgWidth, imgHeight)));
        TextureHandle texHandle = createTexture(texInfo)->handle;
        if (texInfo.m_mipmapLevel > 1) {
            SamplerCreateInfo samplerCI{};
            samplerCI.set_name(texName.c_str()).set_min_mag_mip(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR).set_LodMinMaxBias(0.0f, (float) (texInfo.m_mipmapLevel), 0.0f);
            auto *samplerPtr = createSampler(samplerCI);
            Texture *tex = this->m_context->accessTexture(texHandle);
            tex->setSampler(this->m_context, samplerPtr->handle);
        }
        delete ptr;
        ptr = nullptr;
        return texHandle;
    }

    void Renderer::addImageBarrier(VkCommandBuffer cmdBuffer, Texture *texture, ResourceState oldState, ResourceState newState, u32 baseMipLevel, u32 mipCount, bool isDepth) {
        this->m_context->addImageBarrier(cmdBuffer, texture, oldState, newState, baseMipLevel, mipCount, isDepth);
    }

    void Renderer::executeSkyBox() {
        if (m_skyTexture.index == k_invalid_index) {
            DG_INFO("You have not set skybox in this renderer");
            return;
        }
        //create render object
        m_renderObjects.emplace_back();
        RenderObject &skybox = m_renderObjects.back();
        //render data prepare
        skybox.m_renderPass = m_context->m_swapChainPass;
        skybox.m_vertexBuffer = upLoadBufferToGPU(cubeVertexData, "skyMesh");
        skybox.m_indexBuffer = upLoadBufferToGPU(cubeIndexData, "skyMesh");
        skybox.m_vertexCount = cubeVertexData.size();
        skybox.m_indexCount = cubeIndexData.size();
        skybox.m_GlobalUniform = m_context->m_viewProjectUniformBuffer;
        BufferCreateInfo bcinfo{};
        bcinfo.reset().setName("skyBoxMaterialUniformBuffer").setDeviceOnly(false).setUsageSize(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Material::UniformMaterial));
        skybox.m_MaterialUniform = createBuffer(bcinfo)->handle;
        //material stuff
        DescriptorSetLayoutCreateInfo skyLayoutInfo{};
        skyLayoutInfo.reset().addBinding({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, "globalUniform"}).addBinding({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1, "MaterialUniform"});
        skyLayoutInfo.setName("skyDescLayout");
        PipelineCreateInfo pipelineInfo;
        pipelineInfo.m_renderPassHandle = m_context->m_swapChainPass;
        pipelineInfo.m_depthStencil.setDepth(false, false, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineInfo.m_vertexInput.Attrib = Vertex::getAttributeDescriptions();
        pipelineInfo.m_vertexInput.Binding = Vertex::getBindingDescription();
        pipelineInfo.m_shaderState.reset();
        RasterizationCreation resCI{};
        resCI.m_cullMode = VK_CULL_MODE_FRONT_BIT;
        pipelineInfo.m_rasterization = resCI;
        auto vsShader = ShaderCompiler::compileShader("./shaders/vsskybox.vert");
        auto fsShader = ShaderCompiler::compileShader("./shaders/fsskybox.frag");
        pipelineInfo.m_shaderState.addStage(vsShader.spvData.data(), vsShader.spvData.size()*sizeof(unsigned int), VK_SHADER_STAGE_VERTEX_BIT);
        pipelineInfo.m_shaderState.addStage(fsShader.spvData.data(), fsShader.spvData.size()*sizeof(unsigned int), VK_SHADER_STAGE_FRAGMENT_BIT);
        pipelineInfo.m_shaderState.setName("skyPipeline");
        pipelineInfo.addDescriptorSetlayout(m_context->createDescriptorSetLayout(skyLayoutInfo));
        pipelineInfo.addDescriptorSetlayout(m_context->m_bindlessDescriptorSetLayout);
        MaterialCreateInfo matInfo{};
        matInfo.setName("skyMaterial");
        skybox.m_material = createMaterial(matInfo);
        std::shared_ptr<Program> skyProgram = createProgram("skyPipe", {pipelineInfo});
        skybox.m_material->setProgram(skyProgram);
        skybox.m_material->setDiffuseTexture(m_skyTexture);
        DescriptorSetCreateInfo descInfo{};
        descInfo.reset().setName("skyDesc").setLayout(skybox.m_material->program->passes[0].descriptorSetLayout[0]).buffer(skybox.m_GlobalUniform, 0).buffer(skybox.m_MaterialUniform, 1);
        skybox.m_descriptors.push_back(m_context->createDescriptorSet(descInfo));
    }

    void Renderer::addSkyBox(std::string skyTexPath) {
        TextureCreateInfo texInfo{};
        texInfo.setFlag(TextureFlags::Mask::Default_mask).setFormat(VK_FORMAT_R8G8B8A8_SRGB).setMipmapLevel(k_max_mipmap_level).setBindLess(true);
        m_skyTexture = upLoadTextureToGPU(skyTexPath, texInfo);
        have_skybox = true;
    }

    void Renderer::destroySkyBox() {
        auto &rj = m_renderObjects.front();
        for (auto &i: rj.m_descriptors) {
            m_context->DestroyDescriptorSet(i);
        }
    }

    Program::~Program() {
        for (auto &passe: passes) {
            context->DestroyPipeline(passe.pipeline);
        }
    }

    Program::Program(std::shared_ptr<DeviceContext> context, std::string name) {
        this->context = context;
        this->name = name;
    }

    Material::Material() : textureMap({{"DiffuseTexture", {k_invalid_index, 0}}, {"MetallicRoughnessTexture", {k_invalid_index, 1}}, {"AOTexture", {k_invalid_index, 2}}, {"NormalTexture", {k_invalid_index, 3}}, {"EmissiveTexture", {k_invalid_index, 4}}}) {
    }

    void Material::addTexture(Renderer *renderer, std::string name, std::string path) {
        TextureCreateInfo texInfo{};
        texInfo.setFormat(VK_FORMAT_R8G8B8A8_UNORM).setFlag(TextureFlags::Mask::Default_mask).setBindLess(true).setMipmapLevel(1);
        TextureHandle handle = renderer->upLoadTextureToGPU(path, texInfo);
        textureMap[name] = {handle, (u32) textureMap.size()};
    }

    void generateLUTTexture(Renderer *renderer, TextureHandle LUTTexture) {
        RenderPassCreateInfo lutRenderPassCI{};
        lutRenderPassCI.setName("LutRenderPass").addRenderTexture(LUTTexture).setOperations(RenderPassOperation::Enum::Clear, RenderPassOperation::Enum::Clear, RenderPassOperation::Enum::Clear);
        RenderPassHandle lutRenderPass = renderer->getContext()->createRenderPass(lutRenderPassCI);
        FrameBufferCreateInfo lutFrameBuffer{};
        lutFrameBuffer.reset().addRenderTarget(LUTTexture).setRenderPass(lutRenderPass).setExtent({512, 512});
        FrameBufferHandle lutFBO = renderer->getContext()->createFrameBuffer(lutFrameBuffer);

        PipelineCreateInfo pipelineInfo;
        pipelineInfo.m_renderPassHandle = lutRenderPass;
        pipelineInfo.m_depthStencil.setDepth(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineInfo.m_rasterization.m_cullMode = VK_CULL_MODE_NONE;
        pipelineInfo.m_shaderState.reset();
        auto vsShader = ShaderCompiler::compileShader("./shaders/lut.vert");
        auto fsShader = ShaderCompiler::compileShader("./shaders/lut.frag");
        pipelineInfo.m_shaderState.addStage(vsShader.spvData.data(), vsShader.spvData.size()*sizeof(unsigned int), VK_SHADER_STAGE_VERTEX_BIT);
        pipelineInfo.m_shaderState.addStage(fsShader.spvData.data(), fsShader.spvData.size()*sizeof(unsigned int), VK_SHADER_STAGE_FRAGMENT_BIT);
        pipelineInfo.m_shaderState.setName("lutPipeline");
        pipelineInfo.addDescriptorSetlayout(renderer->getContext()->m_bindlessDescriptorSetLayout);
        PipelineHandle lutPipeline = renderer->getContext()->createPipeline(pipelineInfo);
        CommandBuffer *cmd = renderer->getContext()->getInstantCommandBuffer();
        VkCommandBufferBeginInfo cmdBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd->m_commandBuffer, &cmdBeginInfo);
        cmd->bindPass(lutRenderPass);

        cmd->bindPipeline(lutPipeline);
        Rect2DInt scissor;
        scissor = {0, 0, 512, 512};
        cmd->setScissor(&scissor);
        ViewPort viewport;
        viewport.max_depth = 1.0f;
        viewport.min_depth = 0.0f;
        viewport.rect.width = scissor.width;
        viewport.rect.height = scissor.height;
        cmd->setViewport(&viewport);
        cmd->setDepthStencilState(VK_TRUE);
        cmd->draw(TopologyType::Triangle, 0, 3, 0, 1);
        cmd->endpass();
        cmd->flush(renderer->getContext()->m_graphicsQueue);
        renderer->getContext()->DestroyPipeline(lutPipeline);
        renderer->getContext()->DestroyFrameBuffer(lutFBO, false);
        renderer->getContext()->DestroyRenderPass(lutRenderPass);
        cmd->destroy();
    }

    void Material::addLutTexture(Renderer *renderer) {
        auto LUTTextureFind = renderer->getResourceCache().textures.find("LUTTexture");
        if (LUTTextureFind != renderer->getResourceCache().textures.end()) {
            if(textureMap.find("LUTTexture")!=textureMap.end()){
                textureMap["LUTTexture"].texture = LUTTextureFind->second->handle;
                return;
            }
            textureMap["LUTTexture"] = {LUTTextureFind->second->handle, (u32) this->textureMap.size()};
            return;
        }
        TextureCreateInfo lutCI{};
        lutCI.setExtent({512, 512, 1}).setName("LUTTexture").setBindLess(true).setFlag(TextureFlags::Mask::Default_mask | TextureFlags::Mask::RenderTarget_mask).setTextureType(TextureType::Enum::Texture2D).setFormat(VK_FORMAT_R16G16_SFLOAT);
        TextureHandle LutTex = renderer->createTexture(lutCI)->handle;
        generateLUTTexture(renderer, LutTex);
        textureMap["LUTTexture"] = {LutTex, (u32) textureMap.size()};
    }

    void generateDiffuseEnvTexture(Renderer *renderer, TextureHandle handle, TextureHandle envMap) {
        Material::aliInt um = {(int) envMap.index};
        BufferCreateInfo bufferCI{};
        bufferCI.reset().setName("ENVMapIdx").setUsageSize(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Material::aliInt)).setDeviceOnly(false).setData(&um);
        BufferHandle bufHandle = renderer->getContext()->createBuffer(bufferCI);

        DescriptorSetLayoutCreateInfo descLayoutInfo{};
        descLayoutInfo.reset().setName("ENVMapIdx").addBinding({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, "Idx"});
        DescriptorSetLayoutHandle descLayoutHandle = renderer->getContext()->createDescriptorSetLayout(descLayoutInfo);

        DescriptorSetCreateInfo descCI{};
        descCI.reset().buffer(bufHandle, 0).setName("ENVMapIdx").setLayout(descLayoutHandle);
        DescriptorSetHandle descHandle = renderer->getContext()->createDescriptorSet(descCI);

        RenderPassCreateInfo irraRenderPassCI{};
        irraRenderPassCI.setName("DiffRenderPass").addRenderTexture(handle).setOperations(RenderPassOperation::Enum::Clear, RenderPassOperation::Enum::Clear, RenderPassOperation::Enum::Clear);
        RenderPassHandle irraRenderPass = renderer->getContext()->createRenderPass(irraRenderPassCI);
        Texture *irraDTexture = renderer->getContext()->accessTexture(handle);
        FrameBufferCreateInfo diffFrameBufferCI{};
        diffFrameBufferCI.reset().addRenderTarget(handle).setRenderPass(irraRenderPass).setExtent({irraDTexture->m_extent.width, irraDTexture->m_extent.height});
        FrameBufferHandle irraFBO = renderer->getContext()->createFrameBuffer(diffFrameBufferCI);
        //RDOC_CATCH_START
        PipelineCreateInfo pipelineInfo{};
        pipelineInfo.m_renderPassHandle = irraRenderPass;
        pipelineInfo.m_depthStencil.setDepth(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineInfo.m_rasterization.m_cullMode = VK_CULL_MODE_NONE;
        pipelineInfo.m_shaderState.reset();
        auto vsShader = ShaderCompiler::compileShader("./shaders/lut.vert");
        auto fsShader = ShaderCompiler::compileShader("./shaders/irra.frag");
        pipelineInfo.m_shaderState.addStage(vsShader.spvData.data(), vsShader.spvData.size()*sizeof(unsigned int), VK_SHADER_STAGE_VERTEX_BIT);
        pipelineInfo.m_shaderState.addStage(fsShader.spvData.data(), fsShader.spvData.size()*sizeof(unsigned int), VK_SHADER_STAGE_FRAGMENT_BIT);
        pipelineInfo.m_shaderState.setName("irraPipeline");
        pipelineInfo.addDescriptorSetlayout(descLayoutHandle);
        pipelineInfo.addDescriptorSetlayout(renderer->getContext()->m_bindlessDescriptorSetLayout);
        PipelineHandle irraPipeline = renderer->getContext()->createPipeline(pipelineInfo);
        CommandBuffer *cmd = renderer->getContext()->getInstantCommandBuffer();

        VkCommandBufferBeginInfo cmdBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        vkBeginCommandBuffer(cmd->m_commandBuffer, &cmdBeginInfo);
        cmd->bindPass(irraRenderPass);
        cmd->bindPipeline(irraPipeline);
        Rect2DInt scissor;
        scissor = {0, 0, (u16) irraDTexture->m_extent.width, (u16) irraDTexture->m_extent.height};
        cmd->setScissor(&scissor);
        ViewPort viewport;
        viewport.max_depth = 1.0f;
        viewport.min_depth = 0.0f;
        viewport.rect.width = scissor.width;
        viewport.rect.height = scissor.height;
        cmd->setViewport(&viewport);
        cmd->setDepthStencilState(VK_FALSE);
        cmd->bindDescriptorSet({descHandle}, 0, nullptr, 0);
        cmd->bindDescriptorSet({renderer->getContext()->m_bindlessDescriptorSet}, 1, nullptr, 0);
        cmd->draw(TopologyType::Enum::Triangle, 0, 3, 0, 1);
        cmd->endpass();
        cmd->flush(renderer->getContext()->m_graphicsQueue);
        renderer->getContext()->DestroyPipeline(irraPipeline);
        renderer->getContext()->DestroyFrameBuffer(irraFBO, false);
        renderer->getContext()->DestroyRenderPass(irraRenderPass);
        renderer->getContext()->DestroyBuffer(bufHandle);
        renderer->getContext()->DestroyDescriptorSet(descHandle);
        cmd->destroy();
    }

    void Material::addDiffuseEnvMap(Renderer *renderer, TextureHandle HDRTexture) {
        Texture *HDR = renderer->getContext()->accessTexture(HDRTexture);
        std::string name = "irradianceTex";
        name +=HDR->m_name;
        auto Find = renderer->getResourceCache().textures.find(name);
        if (Find != renderer->getResourceCache().textures.end()) {
            if(textureMap.find("DiffuseEnvMap")!=textureMap.end()){
                textureMap["DiffuseEnvMap"].texture = Find->second->handle;
                return;
            }
            textureMap["DiffuseEnvMap"] = {Find->second->handle, (u32)textureMap.size()};
            return;
        }
        TextureCreateInfo DiffTexCI{};
        DiffTexCI.setBindLess(true).setName(name.c_str()).setExtent({HDR->m_extent.width / 2, HDR->m_extent.height / 2, 1}).setFormat(VK_FORMAT_R8G8B8A8_SRGB).setFlag(TextureFlags::Mask::Default_mask | TextureFlags::Mask::RenderTarget_mask).setMipmapLevel(1).setTextureType(TextureType::Enum::Texture2D);
        TextureHandle handle = renderer->createTexture(DiffTexCI)->handle;
        generateDiffuseEnvTexture(renderer, handle, HDRTexture);
        textureMap["DiffuseEnvMap"] = {handle, (u32) textureMap.size()};
    };

    void generateSpecularEnvTexture(Renderer *renderer, TextureHandle handle, TextureHandle envMap) {
        struct PushBlock {
            int EnvIdx;
            float roughness;
        } pushBlock;

        Texture *specTexture = renderer->getContext()->accessTexture(handle);
        TextureCreateInfo fboTex{};
        fboTex.setName("fbo").setMipmapLevel(1).setBindLess(true).setFormat(VK_FORMAT_R8G8B8A8_SRGB).setExtent(specTexture->m_extent).setFlag(TextureFlags::Mask::Default_mask | TextureFlags::Mask::RenderTarget_mask);
        TextureHandle fboHandle = renderer->getContext()->createTexture(fboTex);
        Texture *fboTexture = renderer->getContext()->accessTexture(fboHandle);

        RenderPassCreateInfo specRenderPassCI{};
        specRenderPassCI.setName("specRenderPass").addRenderTexture(fboHandle).setOperations(RenderPassOperation::Enum::Clear, RenderPassOperation::Enum::Clear, RenderPassOperation::Enum::Clear).setFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        RenderPassHandle specRenderPass = renderer->getContext()->createRenderPass(specRenderPassCI);

        FrameBufferCreateInfo specFrameBufferCI{};
        specFrameBufferCI.reset().addRenderTarget(fboHandle).setRenderPass(specRenderPass).setExtent({specTexture->m_extent.width, specTexture->m_extent.height});
        FrameBufferHandle specFBO = renderer->getContext()->createFrameBuffer(specFrameBufferCI);

        VkPushConstantRange push{};
        push.size = sizeof(PushBlock);
        push.offset = 0;
        push.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        PipelineCreateInfo pipelineInfo{};
        pipelineInfo.m_renderPassHandle = specRenderPass;
        pipelineInfo.m_depthStencil.setDepth(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineInfo.m_rasterization.m_cullMode = VK_CULL_MODE_NONE;
        pipelineInfo.m_shaderState.reset();
        auto vsShader = ShaderCompiler::compileShader("./shaders/lut.vert");
        auto fsShader = ShaderCompiler::compileShader("./shaders/prefilter.frag");
        pipelineInfo.m_shaderState.addStage(vsShader.spvData.data(), vsShader.spvData.size()*sizeof(unsigned int), VK_SHADER_STAGE_VERTEX_BIT);
        pipelineInfo.m_shaderState.addStage(fsShader.spvData.data(), fsShader.spvData.size()*sizeof(unsigned int), VK_SHADER_STAGE_FRAGMENT_BIT);
        pipelineInfo.m_shaderState.setName("irraPipeline");
        pipelineInfo.addDescriptorSetlayout(renderer->getContext()->m_bindlessDescriptorSetLayout);
        pipelineInfo.addPushConstants(push);
        PipelineHandle specPipeline = renderer->getContext()->createPipeline(pipelineInfo);
        CommandBuffer *cmd = renderer->getContext()->getInstantCommandBuffer();
        //RDOC_CATCH_START
        VkCommandBufferBeginInfo cmdBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd->m_commandBuffer, &cmdBeginInfo);
        renderer->getContext()->addImageBarrier(cmd->m_commandBuffer, fboTexture, ResourceState::RESOURCE_STATE_UNDEFINED, ResourceState::RESOURCE_STATE_RENDER_TARGET, 0, 1, false);
        renderer->getContext()->addImageBarrier(cmd->m_commandBuffer, specTexture, ResourceState::RESOURCE_STATE_UNDEFINED, ResourceState::RESOURCE_STATE_COPY_DEST, 0, specTexture->m_mipLevel, false);
        for (u32 i = 0; i < specTexture->m_mipLevel; ++i) {
            pushBlock.roughness = (float) i / (float) (specTexture->m_mipLevel - 1);
            pushBlock.EnvIdx = envMap.index;
            cmd->bindPass(specRenderPass);
            cmd->bindPipeline(specPipeline);
            Rect2DInt scissor;
            scissor = {0, 0, (u16) specTexture->m_extent.width, (u16) specTexture->m_extent.height};
            cmd->setScissor(&scissor);
            ViewPort viewport;
            viewport.max_depth = 1.0f;
            viewport.min_depth = 0.0f;
            viewport.rect.width = static_cast<float>(scissor.width * std::pow(0.5f, i));
            viewport.rect.height = static_cast<float>(scissor.height * std::pow(0.5f, i));
            cmd->setViewport(&viewport);
            cmd->setDepthStencilState(VK_FALSE);
            cmd->bindPushConstants<PushBlock>(VK_SHADER_STAGE_FRAGMENT_BIT, &pushBlock);
            cmd->bindDescriptorSet({renderer->getContext()->m_bindlessDescriptorSet}, 0, nullptr, 0);
            cmd->draw(TopologyType::Enum::Triangle, 0, 3, 0, 1);
            cmd->endpass();
            renderer->addImageBarrier(cmd->m_commandBuffer, fboTexture, ResourceState::RESOURCE_STATE_RENDER_TARGET, ResourceState ::RESOURCE_STATE_COPY_SOURCE, 0, 1, false);
            VkImageCopy copyRegion = {};
            copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.srcSubresource.baseArrayLayer = 0;
            copyRegion.srcSubresource.mipLevel = 0;
            copyRegion.srcOffset = {0, 0, 0};

            copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.dstSubresource.baseArrayLayer = 0;
            copyRegion.dstSubresource.mipLevel = i;
            copyRegion.dstSubresource.layerCount = 1;
            copyRegion.dstOffset = {0, 0, 0};

            copyRegion.extent.width = static_cast<u32>(viewport.rect.width);
            copyRegion.extent.height = static_cast<u32>(viewport.rect.height);
            copyRegion.extent.depth = 1;
            if (copyRegion.extent.width != 0 && copyRegion.extent.height != 0) {
                vkCmdCopyImage(cmd->m_commandBuffer, fboTexture->m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, specTexture->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
            }
            renderer->addImageBarrier(cmd->m_commandBuffer, fboTexture, ResourceState::RESOURCE_STATE_COPY_SOURCE, ResourceState::RESOURCE_STATE_RENDER_TARGET, 0, 1, false);
        }
        renderer->addImageBarrier(cmd->m_commandBuffer, specTexture, ResourceState::RESOURCE_STATE_COPY_DEST, ResourceState::RESOURCE_STATE_SHADER_RESOURCE, 0, specTexture->m_mipLevel, false);
        cmd->flush(renderer->getContext()->m_graphicsQueue);
        //RDOC_CATCH_END
        renderer->getContext()->DestroyPipeline(specPipeline);
        renderer->getContext()->DestroyFrameBuffer(specFBO, true);
        renderer->getContext()->DestroyRenderPass(specRenderPass);
        cmd->destroy();
    }

    void Material::addSpecularEnvMap(Renderer *renderer, TextureHandle HDRTexture) {
        Texture *HDR = renderer->getContext()->accessTexture(HDRTexture);
        std::string name = "SpecularTex";
        name +=HDR->m_name;
        auto Find = renderer->getResourceCache().textures.find(name);
        if (Find != renderer->getResourceCache().textures.end()) {
            if(textureMap.find("SpecularEnvMap")!=textureMap.end()){
                textureMap["SpecularEnvMap"].texture = Find->second->handle;
            }
            textureMap["SpecularEnvMap"] = {Find->second->handle, (u32)textureMap.size()};
            return;
        }
        TextureCreateInfo SpecTexCI{};
        SpecTexCI.setBindLess(true).setName(name.c_str()).setExtent({HDR->m_extent.width, HDR->m_extent.height, 1}).setFormat(VK_FORMAT_R8G8B8A8_SRGB).setFlag(TextureFlags::Mask::Default_mask).setMipmapLevel(k_max_mipmap_level).setTextureType(TextureType::Enum::Texture2D);
        TextureHandle handle = renderer->createTexture(SpecTexCI)->handle;
        generateSpecularEnvTexture(renderer, handle, HDRTexture);
        textureMap["SpecularEnvMap"] = {handle, (u32) textureMap.size()};
    }

    void Material::setIblMap(Renderer *renderer, std::string path) {
        TextureCreateInfo texInfo{};
        texInfo.setFlag(TextureFlags::Mask::Default_mask).setFormat(VK_FORMAT_R8G8B8A8_SRGB).setMipmapLevel(6).setBindLess(true);
        TextureHandle hdrTex = this->renderer->upLoadTextureToGPU(path, texInfo);
        Texture *hdr = renderer->getContext()->accessTexture(hdrTex);
        //renderer->getContext()->m_descriptorSetUpdateQueue.pop_back();
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.descriptorCount = 1;
        write.dstArrayElement = hdrTex.index;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.dstSet = renderer->getContext()->m_VulkanBindlessDescriptorSet;
        write.dstBinding = k_bindless_sampled_texture_bind_index;
        write.pImageInfo = &hdr->m_imageInfo;
        vkUpdateDescriptorSets(renderer->getContext()->m_logicDevice, 1, &write, 0, nullptr);
        addLutTexture(renderer);
        addDiffuseEnvMap(this->renderer, hdrTex);
        addSpecularEnvMap(this->renderer, hdrTex);
        bool isSameWithSkyTex = (hdrTex.index == renderer->getSkyTexture().index);
        if (!isSameWithSkyTex) {
            auto hdrResource = renderer->getResourceCache().textures[hdr->m_name];
            renderer->destroyTexture(hdrResource);
        }
    }

    void Material::setIblMap(Renderer *renderer, TextureHandle handle) {
        Texture *hdr = renderer->getContext()->accessTexture(handle);
        renderer->getContext()->m_descriptorSetUpdateQueue.pop_back();

        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.descriptorCount = 1;
        write.dstArrayElement = handle.index;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.dstSet = renderer->getContext()->m_VulkanBindlessDescriptorSet;
        write.dstBinding = k_bindless_sampled_texture_bind_index;
        write.pImageInfo = &hdr->m_imageInfo;
        vkUpdateDescriptorSets(renderer->getContext()->m_logicDevice, 1, &write, 0, nullptr);
        addDiffuseEnvMap(this->renderer, handle);
        bool isSameWithSkyTex = (handle.index == renderer->getSkyTexture().index);
        if (!isSameWithSkyTex) {
            auto hdrResource = renderer->getResourceCache().textures[hdr->m_name];
            renderer->destroyTexture(hdrResource);
        }
    }

    void Material::setProgram(const std::shared_ptr<Program> &program) {
        Material::program = program;
    };

}// namespace dg