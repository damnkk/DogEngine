#include "SceneHierachyViewer.h"
#include "MaterialViewer.h"
#include "ModelLoader.h"
#include "Renderer.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/quaternion.hpp"

namespace dg {

int selectNode = -1;
int renderSceneTree(const SceneGraph& scene, int node) {
  int         selected = -1;
  std::string name = scene.getNodeName(node);
  std::string label = name.empty() ? (std::string("Node") + std::to_string(node)) : name;
  const int   flags = (scene.m_nodeHierarchy[node].firstChild < 0)
        ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet
        : 0;
  const bool  opened = ImGui::TreeNodeEx(&scene.m_nodeHierarchy[node], flags, "%s", label.c_str());
  ImGui::PushID(node);
  if (ImGui::IsItemClicked(0)) {
    selected = node;
    selectNode = node;
  }

  if (opened) {
    for (int ch = scene.m_nodeHierarchy[node].firstChild; ch != -1;
         ch = scene.m_nodeHierarchy[ch].nextSibling) {
      int subNode = renderSceneTree(scene, ch);
      if (subNode > -1) selected = subNode;
    }
    ImGui::TreePop();
  }
  ImGui::PopID();
  return selected;
}

void SceneHierachyViewer::OnGUI() {
  if (!m_sceneGraph) m_sceneGraph = m_renderer->getResourceLoader()->getSceneGraph();
  ImGui::Begin("Scene Hierachy");
  ImGui::BeginChild("Graph", ImVec2(ImGui::GetWindowWidth() - 15, ImGui::GetWindowHeight() - 200),
                    ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY
                        | ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_Border);
  int idx = renderSceneTree(*(m_sceneGraph), 0);
  if (m_sceneGraph->m_materialMap.contains(selectNode)) {
    MaterialViewer::setMaterialPtr(m_renderer->getResourceLoader()->getMaterialWidthIdx(
        m_sceneGraph->m_materialMap.at(selectNode)));
  } else {
    MaterialViewer::setMaterialPtr(nullptr);
  }
  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()
      && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive()) {
    MaterialViewer::setMaterialPtr(nullptr);
    selectNode = -1;
  }
  ImGui::EndChild();
  ImGui::Separator();
  if (selectNode > -1) {
    ImGui::Text("Node Name");
    ImGui::SameLine(100.0);
    ImGui::TextColored(
        {217.0 / 255.0, 128.0 / 255.0, 250.0 / 255.0, 1.0f}, "%s",
        m_renderer->getResourceLoader()->getSceneGraph()->getNodeName(selectNode).c_str());
    glm::mat4& matrix = m_sceneGraph->m_localTransforms[selectNode];
    glm::vec3  pos = glm::vec3(matrix[3]);
    glm::vec3  oldPos = pos;
    glm::vec3  scale =
        glm::vec3(glm::length(glm::vec3(matrix[0])), glm::length(glm::vec3(matrix[1])),
                  glm::length(glm::vec3(matrix[2])));
    glm::vec3 oldScale = scale;

    //rotate used here saves the last rotate component value, to avoid state reconstruction causing
    //float number pricision lost
    glm::vec3& rotate = m_sceneGraph->m_rotateRecord[selectNode];
    glm::vec3  oldRotate = rotate;

    ImGui::Text("Position");
    ImGui::SameLine(100.0);
    ImGui::DragFloat3("##Position", (float*) &pos.x, 0.1, -1000000000, 1000000000);
    ImGui::Text("Rotate");
    ImGui::SameLine(100.0);
    ImGui::DragFloat3("##Rotate", (float*) &rotate, 0.001);
    //rotate.y = glm::abs(rotate.y) > 89.0f ? (rotate.y > 0.0f ? 89.0f : -89.0f) : rotate.y;
    glm::mat4 rotateMatrix = glm::mat4(1.0f);

    glm::quat quatx = glm::angleAxis(rotate.x, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat quaty = glm::angleAxis(rotate.y, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat quatz = glm::angleAxis(rotate.z, glm::vec3(0.0f, 0.0f, 1.0f));
    // if (rotate.y > 1.5f) {
    //   DG_INFO("test");
    // }

    rotateMatrix = glm::rotate(rotateMatrix, glm::radians(rotate.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotateMatrix = glm::rotate(rotateMatrix, glm::radians(rotate.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotateMatrix = glm::rotate(rotateMatrix, glm::radians(rotate.z), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::quat rotatQuat = quatz * quaty * quatx;
    ImGui::Text("Scale");
    ImGui::SameLine(100.0);
    ImGui::DragFloat3("##Scale", (float*) &scale, 0.1f, 0.00001, 1000000000);
    matrix = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rotatQuat)
        * glm::scale(glm::mat4(1.0f), scale);
    if (oldPos != pos || oldScale != scale || oldRotate != rotate) {
      m_sceneGraph->markAsChanged(selectNode);
    }

  } else {
    ImGui::Text("No scene node been selected");
  }

  ImGui::End();
}

}// namespace dg