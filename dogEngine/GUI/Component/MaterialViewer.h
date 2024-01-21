#ifndef MATERIALVIEWER_H
#define MATERIALVIEWER_H
#include "ComponentViewer.h"
namespace dg {
    struct Material;
    class MaterialViewer : public ComponentViewer {
    public:
        MaterialViewer();
        MaterialViewer(std::string name);
        //纯测试函数
        void init();
        void OnGUI() override;
        Material *m_material = nullptr;

    private:
    };
}// namespace dg
#endif//MATERIALVIEWER_H