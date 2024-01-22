#ifndef GAMEVIEWER_H
#define GAMEVIEWER_H
#include "ComponentViewer.h"
namespace dg{
    class GameViewer:public ComponentViewer{
        public:
        GameViewer();
        GameViewer(std::string name);
        void OnGUI() override;
    };
}
#endif //GAMEVIEWER_H