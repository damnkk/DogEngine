#include "dgSceneGraph.h"

int untitledNodeCount = 0;

namespace dg {
int SceneGraph::addNode(int parent, int level) {
  int newNode = (int) m_nodeHierarchy.size();
  m_nodeHierarchy.emplace_back(-1, -1, -1, -1, 0);
  {
    m_globalTransforms.push_back(glm::mat4(1.0f));
    m_localTransforms.push_back(glm::mat4(1.0f));
    m_rotateRecord.push_back(glm::vec3(0.0f));
  }
  Hierarchy& hi = m_nodeHierarchy.back();
  if (parent > -1) {
    int s = m_nodeHierarchy[parent].firstChild;
    hi.parent = parent;
    if (s == -1) {
      m_nodeHierarchy[parent].firstChild = newNode;
      m_nodeHierarchy[newNode].lastSibling = newNode;
    } else {
      int dest = m_nodeHierarchy[s].lastSibling;
      //after node delection, the first child node's lastSibling may be lost,for this situation,
      //this branch can find the last sibling node at runtime
      if (dest <= -1) {
        for (dest = s; m_nodeHierarchy[dest].nextSibling > -1;
             dest = m_nodeHierarchy[dest].nextSibling)
          ;
      }
      m_nodeHierarchy[dest].nextSibling = newNode;
      m_nodeHierarchy[s].lastSibling = newNode;
    }
  }
  hi.level = level;

  //set node name
  u32 nameID = m_nodeNames.size();
  m_nodeNames.push_back(std::string("untitledNode-") + std::to_string(untitledNodeCount));
  m_nameForNodeMap[newNode] = nameID;
  return newNode;
}

int SceneGraph::addNode(int parent, int level, std::string nodeName) {
  int newNode = (int) m_nodeHierarchy.size();
  maxLevel = std::max(maxLevel, level);
  m_nodeHierarchy.emplace_back(-1, -1, -1, -1, 0);
  {
    m_globalTransforms.push_back(glm::mat4(1.0f));
    m_localTransforms.push_back(glm::mat4(1.0f));
    m_rotateRecord.push_back(glm::vec3(0.0f));
  }
  Hierarchy& hi = m_nodeHierarchy.back();
  if (parent > -1) {
    int s = m_nodeHierarchy[parent].firstChild;
    hi.parent = parent;
    if (s == -1) {
      m_nodeHierarchy[parent].firstChild = newNode;
      m_nodeHierarchy[newNode].lastSibling = newNode;
    } else {
      int dest = m_nodeHierarchy[s].lastSibling;
      //after node delection, the first child node's lastSibling may be lost,for this situation,
      //this branch can find the last sibling node at runtime
      if (dest <= -1) {
        for (dest = s; m_nodeHierarchy[dest].nextSibling > -1;
             dest = m_nodeHierarchy[dest].nextSibling)
          ;
      }
      m_nodeHierarchy[dest].nextSibling = newNode;
      m_nodeHierarchy[s].lastSibling = newNode;
    }
  }
  hi.level = level;

  //set node name
  u32 nameID = m_nodeNames.size();
  m_nodeNames.push_back(nodeName);
  m_nameForNodeMap[newNode] = nameID;
  return newNode;
}

void addUniqueIdx(std::vector<u32>& v, int index) {
  if (!std::binary_search(v.begin(), v.end(), index)) { v.push_back(index); }
}

void SceneGraph::collectNodesToDelete(int node, std::vector<u32>& nodes) {
  for (int i = m_nodeHierarchy[node].firstChild; i != -1; i = m_nodeHierarchy[i].nextSibling) {
    addUniqueIdx(nodes, i);
    collectNodesToDelete(i, nodes);
  }
}

int findLastNonDeletedItem(const SceneGraph& scene, const std::vector<int>& newIndices, int node) {
  if (node == -1) return -1;
  return (newIndices[node] == -1)
      ? findLastNonDeletedItem(scene, newIndices, scene.m_nodeHierarchy[node].nextSibling)
      : newIndices[node];
}
void shiftMapIndices(std::unordered_map<u32, u32>& items, std::vector<int>& indices) {
  std::unordered_map<u32, u32> newItems;
  for (const auto& m : items) {
    if (indices[m.first] != -1) newItems[indices[m.first]] = m.second;
  }
  items = newItems;
}

void SceneGraph::deleteSceneNodes(const std::vector<u32>& nodesToDelete) {
  auto indicesToDelete = nodesToDelete;
  int  preDelteSize = indicesToDelete.size();
  //collect recursivelly all the child node of i
  for (int i = 0; i < preDelteSize; ++i) {
    collectNodesToDelete(indicesToDelete[i], indicesToDelete);
  }
  std::sort(indicesToDelete.begin(), indicesToDelete.end());
  //recording array
  std::vector<u32> nodes(m_nodeHierarchy.size());
  std::iota(nodes.begin(), nodes.end(), 0);
  auto oldSize = nodes.size();
  //erase collected items
  eraseSelected(nodes, indicesToDelete);
  std::vector<int> newIndices(oldSize, -1);
  //the deleted nodes' indices of newindices will be set to -1
  for (u32 i = 0; i < nodes.size(); ++i) { newIndices[nodes[i]] = i; }
  auto nodeMover = [this, &newIndices](Hierarchy& h) {
    Hierarchy archy;
    archy.parent = (h.parent != -1) ? newIndices[h.parent] : -1;
    archy.firstChild = findLastNonDeletedItem(*this, newIndices, h.firstChild);
    archy.nextSibling = findLastNonDeletedItem(*this, newIndices, h.nextSibling);
    //if the lastSibling node is delection selected, the archy.lastSibling will be set -1
    archy.lastSibling = findLastNonDeletedItem(*this, newIndices, h.lastSibling);
    archy.level = h.level;
    return archy;
  };
  std::transform(m_nodeHierarchy.begin(), m_nodeHierarchy.end(), m_nodeHierarchy.begin(),
                 nodeMover);
  eraseSelected(m_nodeHierarchy, indicesToDelete);
  eraseSelected(m_localTransforms, indicesToDelete);
  eraseSelected(m_globalTransforms, indicesToDelete);
  eraseSelected(m_rotateRecord, indicesToDelete);

  shiftMapIndices(m_meshMap, newIndices);
  shiftMapIndices(m_materialMap, newIndices);
  shiftMapIndices(m_nameForNodeMap, newIndices);
}

void SceneGraph::markAsChanged(int node) {
  int level = m_nodeHierarchy[node].level;
  m_changedAtThisFrame[level].push_back(node);
  for (int s = m_nodeHierarchy[node].firstChild; s != -1; s = m_nodeHierarchy[s].nextSibling) {
    markAsChanged(s);
  }
}

void recalculateGlobalTransforms(SceneGraph& scene) {}

void SceneGraph::recalculateAllTransforms() {
  if (!m_changedAtThisFrame[0].empty()) {
    int c = m_changedAtThisFrame[0][0];
    m_globalTransforms[c] = m_localTransforms[c];
    m_changedAtThisFrame[0].clear();
  }

  for (int i = 1; i <= maxLevel; ++i) {
    for (const int& c : m_changedAtThisFrame[i]) {
      int p = m_nodeHierarchy[c].parent;
      m_globalTransforms[c] = m_globalTransforms[p] * m_localTransforms[c];
    }
    m_changedAtThisFrame[i].clear();
  }
}

std::string SceneGraph::getNodeName(int node) const {
  int strID = m_nameForNodeMap.contains(node) ? m_nameForNodeMap.at(node) : -1;
  return (strID > -1) ? m_nodeNames[strID] : std::string();
}
}// namespace dg