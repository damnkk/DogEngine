#include "MaterialViewer.h"
#include "Renderer.h"
#include "dgResource.h"
namespace dg {
MaterialViewer::MaterialViewer() {
}

MaterialViewer::MaterialViewer(std::string viewerName) {
  m_name = viewerName;
  //纯测试代码，下面这行必删
  //m_material = new Material();
}

void MaterialViewer::init() {
  m_material = m_renderer->getGltfLoader()->getRenderObject()[0].m_material;
}

void MaterialViewer::OnGUI() {
  init();
  ImGui::Begin("Material");
  if (!m_material) {
    ImGui::Text("No material been selected");
    ImGui::End();
    return;
  } else {
    const char *Textures[] = {
        "none"};
    ImGui::SeparatorText("Material Property");

    ImGui::TextColored({217.0 / 255.0, 128.0 / 255.0, 250.0 / 255.0, 1.0f}, "BaseColor");
    ImGui::Separator();
    ImGui::PushItemWidth(-1);
    ImGui::Text("use albedo texture");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 12.5);
    ImGui::Checkbox("##use albedo texture", &m_material->useAlbedoTexture);
    if (!m_material->useAlbedoTexture) ImGui::BeginDisabled(true);
    ImGui::Text("albedo texture");
    ImGui::SameLine();
    int albedoIndex = 0;
    ImGui::SameLine(dgUI::itemTab);
    ImGui::Combo("##albedo texture", &albedoIndex, Textures, IM_ARRAYSIZE(Textures), IM_ARRAYSIZE(Textures));
    if (!m_material->useAlbedoTexture) ImGui::EndDisabled();
    m_material->uniformMaterial.textureUseSetting[0].idx = m_material->useAlbedoTexture ? 1 : -1;

    ImGui::TextColored({217.0 / 255.0, 128.0 / 255.0, 250.0 / 255.0, 1.0f}, "Normal");
    ImGui::Separator();
    ImGui::Text("use normal texture");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 12.5);
    ImGui::Checkbox("##use normal texture", &m_material->useNormalTexture);
    if (!m_material->useNormalTexture) ImGui::BeginDisabled(true);
    ImGui::Text("normal texture");
    ImGui::SameLine(dgUI::itemTab);
    int normalIndex = 0;
    ImGui::Combo("##normal texture", &normalIndex, Textures, IM_ARRAYSIZE(Textures), IM_ARRAYSIZE(Textures));
    if (!m_material->useNormalTexture) ImGui::EndDisabled();
    m_material->uniformMaterial.textureUseSetting[1].idx = m_material->useNormalTexture ? 1 : -1;

    ImGui::Text("normal intensity");
    ImGui::SameLine(dgUI::itemTab);
    ImGui::SliderFloat("##normal intensity", &m_material->uniformMaterial.intensity.x, 0.0f, 10.0f, "%0.3f", ImGuiSliderFlags_AlwaysClamp);

    ImGui::TextColored({217.0 / 255.0, 128.0 / 255.0, 250.0 / 255.0, 1.0f}, "MRAO");
    ImGui::Separator();
    ImGui::Text("use MRAO texture");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 12.5);
    ImGui::Checkbox("##use MRAO texture", &m_material->useMRTexture);
    if (!m_material->useMRTexture) ImGui::BeginDisabled(true);
    ImGui::Text("mrao Texture");
    ImGui::SameLine(dgUI::itemTab);
    int mrTextureIndex = 0;
    ImGui::Combo("##mrTexture", &mrTextureIndex, Textures, IM_ARRAYSIZE(Textures), IM_ARRAYSIZE(Textures));
    if (!m_material->useMRTexture) ImGui::EndDisabled();
    m_material->uniformMaterial.textureUseSetting[2].idx = m_material->useMRTexture ? 1 : -1;

    ImGui::Text("metallic");
    ImGui::SameLine(dgUI::itemTab);
    ImGui::SliderFloat("##metallic", &m_material->uniformMaterial.mrFactor.x, 0.0f, 1.0f, "%0.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::Text("roughness");
    ImGui::SameLine(dgUI::itemTab);
    ImGui::SliderFloat("##roughness", &m_material->uniformMaterial.mrFactor.y, 0.0f, 1.0f, "%0.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::Text("ao intensity");
    ImGui::SameLine(dgUI::itemTab);
    ImGui::SliderFloat("##ao intensity", &m_material->uniformMaterial.mrFactor.z, 0.0f, 1.0f, "%0.3f", ImGuiSliderFlags_AlwaysClamp);

    ImGui::TextColored({217.0 / 255.0, 128.0 / 255.0, 250.0 / 255.0, 1.0f}, "Emissive");
    ImGui::Separator();
    ImGui::Text("use emis texture");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 12.5);
    ImGui::Checkbox("##use emis texture", &m_material->useEmisTexture);
    if (!m_material->useEmisTexture) ImGui::BeginDisabled(true);
    ImGui::Text("emis texture");
    ImGui::SameLine(dgUI::itemTab);
    int emissiveTextureIdx = 0;
    ImGui::Combo("##emissive texture", &emissiveTextureIdx, Textures, IM_ARRAYSIZE(Textures), IM_ARRAYSIZE(Textures));
    if (!m_material->useEmisTexture) ImGui::EndDisabled();

    m_material->uniformMaterial.textureUseSetting[3].idx = m_material->useEmisTexture ? 1 : -1;

    ImGui::Text("emis Color");
    ImGui::SameLine(dgUI::itemTab);
    ImGui::ColorEdit3("##emis Color", glm::value_ptr(m_material->uniformMaterial.emissiveFactor));
    ImGui::Text("emis intensity");
    ImGui::SameLine(dgUI::itemTab);
    ImGui::SliderFloat("##emis intensity", &m_material->uniformMaterial.intensity.y, 0.0f, 1.0f, "%0.3f", ImGuiSliderFlags_AlwaysClamp);

    ImGui::TextColored({217.0 / 255.0, 128.0 / 255.0, 250.0 / 255.0, 1.0f}, "Env Control");
    ImGui::Separator();
    ImGui::Text("env rotate");
    ImGui::SameLine(dgUI::itemTab);
    ImGui::SliderFloat("##env rotate", &m_material->uniformMaterial.envFactor.x, 0.0f, 10.0f, "%0.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::Text("env exposure");
    ImGui::SameLine(dgUI::itemTab);
    ImGui::SliderFloat("##env exposure", &m_material->uniformMaterial.envFactor.y, 0.0f, 10.0f, "%0.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::Text("env gamma");
    ImGui::SameLine(dgUI::itemTab);
    ImGui::SliderFloat("##env gamma", &m_material->uniformMaterial.envFactor.z, 0.0f, 5.0f, "%0.3f", ImGuiSliderFlags_AlwaysClamp);

    ImGui::SeparatorText("Render State");
    HelpMarker("Render state is about to render pipeline reconstruction, switching rendering states frequently will affect rendering efficiency.");
    ImGui::Text("Depth Test");
    ImGui::SameLine(dgUI::itemTab);
    bool depthTest = m_material->depthTest;
    ImGui::Checkbox("##Depth Test", &depthTest);
    ImGui::Text("Depth Write");
    ImGui::SameLine(dgUI::itemTab);
    bool depthWrite = m_material->depthWrite;
    ImGui::Checkbox("##Depth Write", &depthWrite);
    ImGui::Text("Depth Func");
    ImGui::SameLine(dgUI::itemTab);
    const char *DepthFunc[] = {
        "always", "smaller", "bigger", "equal"};
    int depthModeIdx = m_material->depthModeIdx;
    ImGui::Combo("##Depth Func", &depthModeIdx, DepthFunc, IM_ARRAYSIZE(DepthFunc), IM_ARRAYSIZE(DepthFunc));

    ImGui::Text("Face Cull");
    ImGui::SameLine(dgUI::itemTab);
    const char *FaceCullFunc[] = {
        "BackFace", "FrontFace", "BackAndFront"};
    int cullModeIdx = m_material->cullModeIdx;
    ImGui::Combo("##Face Cull", &cullModeIdx, FaceCullFunc, IM_ARRAYSIZE(FaceCullFunc), IM_ARRAYSIZE(FaceCullFunc));
    if (ImGui::Button("Save")) {
      std::cout << "Save" << std::endl;
      m_material->depthTest = depthTest;
      m_material->depthWrite = depthWrite;
      m_material->depthModeIdx = depthModeIdx;
      m_material->cullModeIdx = cullModeIdx;
      m_material->updateProgram();
    }
    ImGui::PopItemWidth();
    ImGui::End();
  }
}
}// namespace dg