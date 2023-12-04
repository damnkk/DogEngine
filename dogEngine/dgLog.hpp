#ifndef LOG_H
#define LOG_H
#include "spdlog/spdlog.h"

namespace dg{
    
    #define DG_INFO(format,...)              spdlog::info(format,##__VA_ARGS__);             
    #define DG_WARN(format,...)              spdlog::warn(format,##__VA_ARGS__);
    #define DG_ERROR(format,...)             spdlog::error(format,##__VA_ARGS__); //exit(-1);
}


#endif  //LOG_H