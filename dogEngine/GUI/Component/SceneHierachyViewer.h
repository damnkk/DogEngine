#ifndef SCENEHIERACHY_H
#define SCENEHIERACHY_H
#include "ComponentViewer.h"

namespace dg {
struct SceneGraph;

class SceneHierachyViewer : public ComponentViewer {
 public:
  SceneHierachyViewer() {}
  SceneHierachyViewer(std::string viewName) { m_name = viewName; }

  void OnGUI() override;

 private:
  std::shared_ptr<SceneGraph> m_sceneGraph;
};

}// namespace dg

#endif