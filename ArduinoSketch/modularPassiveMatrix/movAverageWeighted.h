#ifndef MOV_AVERAGE_WEIGHTED_HPP
#define MOV_AVERAGE_WEIGHTED_HPP

#include "filterInterface.h"
#include "stdint.h"
#include "math.h"

namespace Filter
{
    class MovAverageWeighted : public Interface::IFilter
    {
        public:
            static MovAverageWeighted* getInstance(uint32_t l_bufferLength,
                    uint16_t l_windowSize);
            bool free();
            bool insertData(FILTER_DATA_TYPE *m_pData);
            bool getData(FILTER_DATA_TYPE *l_pData);
        private:
            uint32_t m_bufferLength;
            float **m_pData;
            uint16_t m_windowSize;
            uint16_t m_indexSumm;
            float m_result;

            MovAverageWeighted(uint32_t l_bufferLength, uint16_t l_windowSize);
            ~MovAverageWeighted();
            static MovAverageWeighted *m_pInstance;
    };
};

#endif
