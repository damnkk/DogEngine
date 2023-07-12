# Vulkan_learn
A dumb ass's vulkan learning project

The whole thing in this shit is based on the code provided by [vulkan-tutorial](https://vulkan-tutorial.com/Introduction).
I added some thing new like **vulkan memory allocator**, **complex model loading**, **Bling_Phong lighting**, **fog simulation** etc. However this renderer is still a trash, cuz I am new to C++ and vulkan. Maybe I will perfect it in the future after I know more about vulkan, or I totally forget it. That's all,bye bye.
![color_visible](asset/color_visible.png)
![normal_visible](asset/normal_visible.png)

简单梳理Vulkan渲染流程，加深理解，在这个分支中，我使用了vulkan.hpp头文件，它对vulkan对象有了更好的封装。
**步骤1**:Vulkan万物的起始点都是这个instance，而instance的创建需要ApplicationInfo,获取需要的Instance扩展(**注意和Device扩展相区分**),也是在这里确定是否开启验证层。其实主要的难点就在于这个开启验证层，还要开启回调函数输出错误。首先在扩展上，除了从glfw库里面获取的glfw需要的扩展之外，还需要带上一个VK_EXT_DEBUG_UTILS_EXTENSION_NAME。之后在enableLayers的地方开启你需要的验证层这个不难，之后，回调函数的配置比较麻烦，你需要先重载两个函数，以便用这两个函数可以创建和销毁Debug Messenger，这里不细说。之后回调函数
**步骤2**:在Instance之后，需要创建物理设备，物理设备的选择在之前的教程中写的及其复杂，需要想好你需要的特性，然后遍历所有的设备找出符合条件的，实际上在简单场景下并不需要这样,因为功能最强的显卡一定在最前面，所以像pytorch一样直接选择设备0即可。之后我们获取物理设备的队列族属性，找到符合要求的（一般是同时满足Compute和Graphic的队列）队列，并得到它的队列族索引（这里遇到了一点问题，因为显示功能是扩展，无法简单的用queueFlagsBit去获取，需要专门调用一个函数去检验支持）。之后用queueCreateInfo从物理设备上创建逻辑设备。
**步骤3**:前两步是有顺序的，是我们整个Vulkan程序的基础。这一步开始很多东西并没有先后顺序，因为他们都是从逻辑设备中创建出来的。到这一步我们会创建一些和显示有关的东西。首先是surface，直接从glfw里面给出来就行。之后是交换链，交换链是一个很复杂的东西，不过这个不像管线一样，这个只要建立一个就行了。其他没有什什么特别卡的地方，swapChainInfo需要什么，你就创建什么就行。但是注意的是，swapchain的很多东西都会被其他结构复用，所以不能临时创建在函数当中，例如Format，swapchainImage, swapchainImageView, swapExtent, swapChainrameBuffers