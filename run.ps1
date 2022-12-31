cd .\shaders
.\compile.bat
cd ..
cd .\build
cmake --build .
cd ..
./build/Debug/vulkan_learning.exe