#ifndef GAMEVIEWER_H
#define GAMEVIEWER_H
#include "ComponentViewer.h"
namespace dg {
class GameViewer : public ComponentViewer {
 public:
  GameViewer();
  GameViewer(std::string name);
  void   OnGUI() override;
  ImVec2 m_oldWindowSize;
};
}// namespace dg
#endif//GAMEVIEWER_H