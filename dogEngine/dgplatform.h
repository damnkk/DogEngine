#ifndef PLATFORM_H
#define PLATFORM_H
#include <cstdint>
#include <vulkan/vulkan.h>

using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using u8  = uint8_t;


#define DG_FILELINE(message)             __FILE__, "(", message


namespace ResourceUpdateType{
    enum Enum{
        Buffer,Texture,Pipeline,Sampler, DescriptorSetLayout, DescriptorSet, RenderPass, ShaderState, FrameBuffer, Count
    };
}

namespace QueueType{
    enum Enum{
        Graphics, Compute, CopyTransfer
    };

    enum Mask{
        Graphic_mask = 1<<0, Compute_mask = 1<<1, CopyTransfer_mask = 1<<2
    };

    static const char* s_value_names[] = {
        "Graphics", "compute", "CopyTransfer", "Count"
    };

}

namespace RenderPassType{
    enum Enum{
        Geometry, SwapChain,Compute
    };
}

namespace RenderPassOperation {

    enum Enum {
        DontCare, Load, Clear, Count
    }; // enum Enum
} // namespace RenderPassOperation

namespace TextureType{
    enum Enum{
        Texture1D, Texture2D, Texture3D, TextureCube, Texture1DArray, Texture2DArray, TextureCubeArray, UNDEFINED
    };

    enum Mask{
        Texture1D_Mask= 1<<0,Texture2D_Mask = 1<<1,Texture3D_Mask = 1<<2, TextureCube_Make = 1<<3, Texture1DArray_Mask = 1<<4,Texture2DArray_Mask = 1 << 5, TextureCubeArray_Mask = 1 << 6,UNDEFINED_MASK = 1<<7
    };

    static const char* s_values_names[] = {
        "Texture1D", "Texture2D", "Texture3D", "TextureCube", "Texture_1D_Array", "Texture_2D_Array", "Texture_Cube_Array"
    };

    static const char* ToString(Enum e){
        return e<Enum::UNDEFINED?"Unsupported Texture":s_values_names[e]; 
    }
}

namespace TextureFlags {
    enum Enum {
        Default, RenderTarget, Compute, Sparse, ShadingRate, Count
    };

    enum Mask {
        Default_mask = 1 << 0, RenderTarget_mask = 1 << 1, Compute_mask = 1 << 2, Sparse_mask = 1 << 3, ShadingRate_mask = 1 << 4
    };

    static const char* s_value_names[] = {
            "Default", "RenderTarget", "Compute", "Count"
    };

    static const char* ToString( Enum e ) {
        return ((u32)e < Enum::Count ? s_value_names[(int)e] : "unsupported" );
    }

} // namespace TextureFlags

namespace VertexComponentFormat {
    enum Enum {
        Float, Float2, Float3, Float4, Mat4, Byte, Byte4N, UByte, UByte4N, Short2, Short2N, Short4, Short4N, Uint, Uint2, Uint4, Count
    };

    static const char* s_value_names[] = {
        "Float", "Float2", "Float3", "Float4", "Mat4", "Byte", "Byte4N", "UByte", "UByte4N", "Short2", "Short2N", "Short4", "Short4N", "Uint", "Uint2", "Uint4", "Count"
    };

    static const char* ToString( Enum e ) {
        return ((u32)e < Enum::Count ? s_value_names[(int)e] : "unsupported" );
    }
} // namespace VertexComponentFormat



namespace ColorWriteEnabled {
    enum Enum {
        Red, Green, Blue, Alpha, All, Count
    };

    enum Mask {
        Red_mask = 1 << 0, Green_mask = 1 << 1, Blue_mask = 1 << 2, Alpha_mask = 1 << 3, All_mask = Red_mask | Green_mask | Blue_mask | Alpha_mask
    };

    static const char* s_value_names[] = {
        "Red", "Green", "Blue", "Alpha", "All", "Count"
    };

    static const char* ToString( Enum e ) {
        return ((u32)e < Enum::Count ? s_value_names[(int)e] : "unsupported" );
    }
} // namespace ColorWriteEnabled

namespace VertexInputRate {
    enum Enum {
        PerVertex, PerInstance, Count
    };

    enum Mask {
        PerVertex_mask = 1 << 0, PerInstance_mask = 1 << 1, Count_mask = 1 << 2
    };

    static const char* s_value_names[] = {
        "PerVertex", "PerInstance", "Count"
    };

    static const char* ToString( Enum e ) {
        return ((u32)e < Enum::Count ? s_value_names[(int)e] : "unsupported" );
    }
} // namespace VertexInputRate


namespace FillMode {
    enum Enum {
        Wireframe, Solid, Point, Count
    };

    enum Mask {
        Wireframe_mask = 1 << 0, Solid_mask = 1 << 1, Point_mask = 1 << 2, Count_mask = 1 << 3
    };

    static const char* s_value_names[] = {
        "Wireframe", "Solid", "Point", "Count"
    };

    static const char* ToString( Enum e ) {
        return ((u32)e < Enum::Count ? s_value_names[(int)e] : "unsupported" );
    }
} // namespace FillMode

namespace TextureFormat {

    inline bool                     is_depth_stencil( VkFormat value ) {
        return value == VK_FORMAT_D16_UNORM_S8_UINT || value == VK_FORMAT_D24_UNORM_S8_UINT || value == VK_FORMAT_D32_SFLOAT_S8_UINT;
    }
    inline bool                     is_depth_only( VkFormat value ) {
        return value >= VK_FORMAT_D16_UNORM && value < VK_FORMAT_D32_SFLOAT;
    }
    inline bool                     is_stencil_only( VkFormat value ) {
        return value == VK_FORMAT_S8_UINT;
    }

    inline bool                     has_depth( VkFormat value ) {
        return (value >= VK_FORMAT_D16_UNORM && value < VK_FORMAT_S8_UINT ) || (value >= VK_FORMAT_D16_UNORM_S8_UINT && value <= VK_FORMAT_D32_SFLOAT_S8_UINT);
    }
    inline bool                     has_stencil( VkFormat value ) {
        return value >= VK_FORMAT_S8_UINT && value <= VK_FORMAT_D32_SFLOAT_S8_UINT;
    }
    inline bool                     has_depth_or_stencil( VkFormat value ) {
        return value >= VK_FORMAT_D16_UNORM && value <= VK_FORMAT_D32_SFLOAT_S8_UINT;
    }

} // namespace TextureFormat

namespace PipelineStage {

    enum Enum {
        DrawIndirect = 0, VertexInput = 1, VertexShader = 2, FragmentShader = 3, RenderTarget = 4, ComputeShader = 5, Transfer = 6
    };

    enum Mask {
        DrawIndirect_mask = 1 << 0, VertexInput_mask = 1 << 1, VertexShader_mask = 1 << 2, FragmentShader_mask = 1 << 3, RenderTarget_mask = 1 << 4, ComputeShader_mask = 1 << 5, Transfer_mask = 1 << 6
    };

} // namespace PipelineStage

namespace TopologyType {
    enum Enum {
        Unknown, Point, Line, Triangle, Patch, Count
    };

    enum Mask {
        Unknown_mask = 1 << 0, Point_mask = 1 << 1, Line_mask = 1 << 2, Triangle_mask = 1 << 3, Patch_mask = 1 << 4, Count_mask = 1 << 5
    };

    static const char* s_value_names[] = {
        "Unknown", "Point", "Line", "Triangle", "Patch", "Count"
    };

    static const char* ToString( Enum e ) {
        return ((u32)e < Enum::Count ? s_value_names[(int)e] : "unsupported" );
    }
} // namespace TopologyType



#endif //PLATFORM_H