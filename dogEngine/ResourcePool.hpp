#ifndef RESOURCEPOOL_H
#define RESOURCEPOOL_H
#include "dgpch.h"
#include "dgAssert.h"

namespace dg{
    static const u32                    INVALID_INDEX = 0xffffffff;
    template<typename T>
    struct ResourcePool{
        void init(u32 poolsize);
        
        u32                         obtainResource();
        
        void                        releaseResource(u32 handle);
        void                        releaseAllResource();

        T*                          accessResource(u32 handle);
        const T*                    accessResource(u32 handle) const;

        const u32                   resourceSize(){ return sizeof(T);}

        std::vector<T>              m_objectArray;
        std::vector<u32>            m_freeIndices;
        u32                         m_freeHeadIndice = 0;
        u32                         m_usedNum = 0;
        u32                         m_poolSize = 16;

    };

    template<typename T>
    void  ResourcePool<T>::init( u32 poolsize){
        m_poolSize = poolsize;
        m_objectArray.resize(m_poolSize);
        m_freeIndices.resize(m_poolSize);
        for(u32 i = 0;i<m_freeIndices.size();++i){
            m_freeIndices[i] = i;
        }
    }

    template<typename T>
    u32  ResourcePool<T>::obtainResource(){
        if(m_freeHeadIndice<m_poolSize){
            u32 availableIndice = m_freeIndices[m_freeHeadIndice++];
            ++m_usedNum;
            return availableIndice;
        }
        DGASSERT(false);
        return INVALID_INDEX;
    }

    template<typename T>
    void  ResourcePool<T>::releaseResource(u32 handle){
        if(handle>=m_objectArray.size()) 
        {
            DG_WARN("Trying to release an invalid resource, check again");
            return;
        }
        m_freeIndices[--m_freeHeadIndice] = handle;
        --m_usedNum;
        return;
    }

    template<typename T>
    void ResourcePool<T>::releaseAllResource(){
        m_freeHeadIndice = 0;
        m_usedNum = 0;
        for(u32 i = 0;i<m_freeIndices.size();++i){
            m_freeIndices[i] = i;
        }
    }

    template<typename T>
    T* ResourcePool<T>::accessResource(u32 handle){
        if(handle>=m_objectArray.size()) 
        {
            DGASSERT(false);
            return nullptr;
        }
        return &m_objectArray[handle];
    }    

    template<typename T>
    const T* ResourcePool<T>::accessResource(u32 handle) const{
        if(handle>=m_objectArray.size()) 
        {
            DGASSERT(false);
            return nullptr;
        }
        return &m_objectArray[handle];
    }
}



#endif //RESOURCEPOOL_H