#ifndef DGSCENEGRAPH_H
#define DGSCENEGRAPH_H
#include "dgpch.h"
#include "glm/common.hpp"
#include "glm/gtc/matrix_transform.hpp"
namespace dg {
struct Hierarchy {
  Hierarchy() {}
  Hierarchy(int pParent, int pFirstChild, int pNextSibling, int pLastSibling, int pLevel) : parent(pParent), firstChild(pFirstChild), nextSibling(pNextSibling), lastSibling(pLastSibling), level(pLevel) {}
  int parent = -1;
  int firstChild = -1;
  int nextSibling = -1;
  int lastSibling = -1;
  int level = 0;
};

struct SceneGraph {
  int  addNode(int parent, int level);
  int  addNode(int parent, int level, std::string nodeName);
  void collectNodesToDelete(int node, std::vector<u32>& nodes);
  // resource loader may execute scene again, after delecting the node of scene
  void        deleteSceneNodes(const std::vector<u32>& nodesToDelete);
  void        recalculateAllTransforms();
  std::string getNodeName(int node) const;

 public:
  const glm::mat4& getGlobalTransformsFromIdx(u32 idx) { return m_globalTransforms[idx]; }

  std::vector<std::vector<int>> m_changedAtThisFrame = std::vector<std::vector<int>>(32);
  std::vector<Hierarchy>        m_nodeHierarchy;
  std::vector<glm::mat4>        m_localTransforms;
  std::vector<glm::mat4>        m_globalTransforms;
  std::vector<glm::vec3>        m_rotateRecord;
  std::unordered_map<u32, u32>  m_meshMap;
  std::unordered_map<u32, u32>  m_materialMap;
  std::unordered_map<u32, u32>  m_boundingBoxMap;
  std::unordered_map<u32, u32>  m_nameForNodeMap;
  std::vector<std::string>      m_nodeNames;

  int maxLevel = -1;
};

//if any item in v is founded in selection array, the item will be remove to end of the v;
template<class T, class Index = u32>
void eraseSelected(std::vector<T>& v, const std::vector<Index>& selection) {
  v.resize(std::distance(v.begin(), std::stable_partition(v.begin(), v.end(), [&selection, &v](const T& item) {
                           return !std::binary_search(selection.begin(), selection.end(), static_cast<Index>(static_cast<const T*>(&item) - &v[0]));
                         })));
}
}// namespace dg

#endif//DGSCENEGRAPH_H
