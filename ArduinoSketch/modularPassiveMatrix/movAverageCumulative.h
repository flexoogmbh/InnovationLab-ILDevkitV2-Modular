#ifndef MOV_AVERAGE_CUMULATIVE_HPP
#define MOV_AVERAGE_CUMULATIVE_HPP

#include "Application.h"
#include "filterInterface.h"
#include "stdint.h"
#include "math.h"

namespace Filter
{
    class MovAverageCumulative : public Interface::IFilter
    {
        public:
            static MovAverageCumulative* getInstance(uint32_t l_bufferLength,
                    float l_filtrationCoef);
            bool free();
            bool insertData(FILTER_DATA_TYPE *m_pData);
            bool getData(FILTER_DATA_TYPE *l_pData);
        private:
            uint32_t m_bufferLength;
            float *m_pData;
            float m_filtrationCoef;

            MovAverageCumulative(uint32_t l_bufferLength, float l_filtrationCoef);
            ~MovAverageCumulative();
            static MovAverageCumulative *m_pInstance;
    };
};


#endif
