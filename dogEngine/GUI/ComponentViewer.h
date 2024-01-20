#ifndef COMPONENTVIEWER_H
#define COMPONENTVIEWER_H
#include "dgpch.h"
#include "dgLog.hpp"
namespace dg{

class ComponentViewer{
public:
    ComponentViewer();
    ComponentViewer(std::string conponentName);
    virtual ~ComponentViewer();
    virtual void OnGUI(){
        DG_ERROR("You are trying to innovate an unoverloaded function")
        exit(-1);};
private:
    std::string m_name;
};
}
#endif //COMPONENTVIEWER_H