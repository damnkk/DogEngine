#include "dgResource.h"
namespace dg{

    MaterialCreateInfo &MaterialCreateInfo::setRenderOrder(u32 renderOrder) {
        this->renderOrder = renderOrder;
        return *this;
    }

    MaterialCreateInfo &MaterialCreateInfo::setName(std::string name) {
        this->name = name;
        return *this;
    }
}