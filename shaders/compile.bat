%VULKAN_SDK%\Bin\glslangValidator.exe --target-env vulkan1.2 vspbr.vert -o pbrvert.spv
%VULKAN_SDK%\Bin\glslangValidator.exe --target-env vulkan1.2 fspbr.frag -o pbrfrag.spv
%VULKAN_SDK%\Bin\glslangValidator.exe --target-env vulkan1.2 vsskybox.vert -o skyvert.spv
%VULKAN_SDK%\Bin\glslangValidator.exe --target-env vulkan1.2 fsskybox.frag -o skyfrag.spv
%VULKAN_SDK%\Bin\glslangValidator.exe --target-env vulkan1.2 lut.vert -o lutvert.spv
%VULKAN_SDK%\Bin\glslangValidator.exe --target-env vulkan1.2 lut.frag -o lutfrag.spv
%VULKAN_SDK%\Bin\glslangValidator.exe --target-env vulkan1.2 irra.frag -o irrafrag.spv
%VULKAN_SDK%\Bin\glslangValidator.exe --target-env vulkan1.2 prefilter.frag -o prefilterfrag.spv
