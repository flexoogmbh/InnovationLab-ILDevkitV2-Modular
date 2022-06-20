#ifndef MEDIAN_HPP
#define MEDIAN_HPP

#include "filterInterface.h"
#include "stdint.h"
#include "math.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace Filter
{
    class MedianFilter : public Interface::IFilter
    {
        public:
            static MedianFilter* getInstance(uint32_t l_bufferLength,
                    uint16_t windowSize);
            bool free();
            bool insertData(FILTER_DATA_TYPE *m_pData);
            bool getData(FILTER_DATA_TYPE *l_pData);
        private:
            uint32_t m_bufferLength;
            uint16_t m_windowSize;
            FILTER_DATA_TYPE **m_pData;

            MedianFilter(uint32_t l_bufferLength, uint16_t l_windowSize);
            ~MedianFilter();
            static MedianFilter *m_pInstance;
    };
}

#ifdef __cplusplus
}
#endif

#endif
