#ifndef MATERIALVIEWER_H
#define MATERIALVIEWER_H
#include "ComponentViewer.h"
namespace dg{
    class MaterialViewer:public ComponentViewer{
        MaterialViewer();
        MaterialViewer(std::string name);
        void OnGUI() override;
    };
}
#endif MATERIALVIEWER_H