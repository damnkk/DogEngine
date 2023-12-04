#ifndef RENDERER_H
#define RENDERER_H
#include "GUI/gui.h"
#include "DeviceContext.h"

namespace dg{

void keycallback(GLFWwindow *window);

struct objLoader;
struct gltfLoader;
struct Renderer{
public:

    void                                        init(std::shared_ptr<DeviceContext> context);
    void                                        destroy();

    void                                        newFrame();
    void                                        present();
    void                                        drawScene();
    void                                        drawUI();
    void                                        loadFromPath(const std::string& path);
    void                                        executeScene();
    CommandBuffer*                              getCommandBuffer();

private:
    std::shared_ptr<DeviceContext>              m_context;
    std::shared_ptr<objLoader>                  m_objLoader;
    std::shared_ptr<gltfLoader>                 m_gltfLoader;

};

}
#endif //RENDERER_H