#ifndef MOV_AVERAGE_HPP
#define MOV_AVERAGE_HPP

#include "filterInterface.h"
#include "stdint.h"
#include "math.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace Filter
{
    class MovAverage : public Interface::IFilter
    {
        public:
            static MovAverage* getInstance(uint32_t l_bufferLength,
                    uint16_t windowSize);
            bool free();
            bool insertData(FILTER_DATA_TYPE *m_pData);
            bool getData(FILTER_DATA_TYPE *l_pData);
        private:
            uint16_t m_bufferCounter;
            uint32_t m_bufferLength;
            uint16_t m_windowSize;
            FILTER_DATA_TYPE **m_pData;

            MovAverage(uint32_t l_bufferLength, uint16_t l_windowSize);
            ~MovAverage();
            static MovAverage *m_pInstance;
    };
}
;

#ifdef __cplusplus
}
#endif

#endif
