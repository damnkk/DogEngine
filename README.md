# Vulkan_learn
A dumb ass's vulkan learning project

The whole thing in this shit is based on the code provided by [vulkan-tutorial](https://vulkan-tutorial.com/Introduction).
I added some thing new like **vulkan memory allocator**, **complex model loading**, **Bling_Phong lighting**, **fog simulation** etc. However this renderer is still a trash, cuz I am new to C++ and vulkan. Maybe I will perfect it in the future after I know more about vulkan, or I totally forget it. That's all,bye bye.
![color_visible](asset/color_visible.png)
![normal_visible](asset/normal_visible.png)

简单梳理Vulkan渲染流程，加深理解，在这个分支中，我使用了vulkan.hpp头文件，它对vulkan对象有了更好的封装。
步骤1:Vulkan万物的起始点都是这个instance，而instance的创建需要ApplicationInfo,获取需要的Instance扩展(**注意和Device扩展相区分**),也是在这里确定是否开启验证层。
步骤2:在Instance之后，需要创建物理设备，物理设备的选择在之前的教程中写的及其复杂，需要想好你需要的特性，然后遍历所有的设备找出符合条件的，实际上在简单场景下并不需要这样,因为功能最强的显卡一定在最前面，所以像pytorch一样直接选择设备0即可。之后我们获取物理设备的队列族属性，找到符合要求的（一般是同时满足Compute和Graphic的队列）队列，并得到它的队列族索引。之后用queueCreateInfo从物理设备上创建逻辑设备。
步骤3:前两部是有顺序的，是我们整个Vulkan程序的基础。这一步开始很多东西并没有先后顺序，因为他们都是从逻辑设备中创建出来的。