#ifndef FILTER_INTERFACE_HPP
#define FILTER_INTERFACE_HPP

#include "stdint.h"

#define FILTER_DATA_TYPE    uint16_t

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "stdint.h"

namespace Interface
{
    class IFilter
    {
        public:
            virtual ~IFilter()
            {
            }
            virtual bool insertData(FILTER_DATA_TYPE *m_pData) = 0;
            virtual bool getData(FILTER_DATA_TYPE *l_pData) = 0;
            virtual bool free() = 0;
    };
}

#ifdef __cplusplus
}
#endif

#endif
