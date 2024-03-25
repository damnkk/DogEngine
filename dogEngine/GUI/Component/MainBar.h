#ifndef MAINBAR_H
#define MAINBAR_H
#include "ComponentViewer.h"
namespace dg {
class MainBar : public ComponentViewer {
 public:
  MainBar();
  MainBar(std::string name);
  void OnGUI() override;
};
}// namespace dg

#endif//MAINBAR_H
