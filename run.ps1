cd .\shaders
.\compile.bat
cd ..
cmake --build build -j25
./build/Debug/vulkan_learning.exe