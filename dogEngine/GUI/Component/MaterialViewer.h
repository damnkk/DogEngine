#ifndef MATERIALVIEWER_H
#define MATERIALVIEWER_H
#include "ComponentViewer.h"
namespace dg {
struct Material;
class MaterialViewer : public ComponentViewer {
 public:
  MaterialViewer();
  MaterialViewer(std::string name);
  static void setMaterialPtr(Material* ptr);
  //纯测试函数
  void init();
  void OnGUI() override;

 private:
  static Material* m_material;
};
}// namespace dg
#endif//MATERIALVIEWER_H