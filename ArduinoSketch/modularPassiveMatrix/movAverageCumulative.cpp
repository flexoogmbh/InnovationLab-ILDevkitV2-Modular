#include "movAverageCumulative.h"
#include "algorithm"

using namespace Filter;

MovAverageCumulative* MovAverageCumulative::getInstance(uint32_t l_bufferLength,
        float l_filtrationCoef)
{
    if (m_pInstance == nullptr) {
        m_pInstance = new MovAverageCumulative(l_bufferLength,
                l_filtrationCoef);
    }

    return m_pInstance;
}

bool MovAverageCumulative::free()
{
    m_pInstance->~MovAverageCumulative();
    m_pInstance = nullptr;
    return true;
}

bool MovAverageCumulative::insertData(FILTER_DATA_TYPE *l_pData)
{
    if (m_pData != nullptr) {
        for (uint32_t i = 0; i < m_bufferLength; i++) {
            m_pData[i] = m_pData[i] * (1 - m_filtrationCoef)
                    + ((float) l_pData[i] * m_filtrationCoef);
        }
        return true;
    }
}

bool MovAverageCumulative::getData(FILTER_DATA_TYPE *l_pData)
{
    if (m_pData != nullptr) {
        for (uint32_t i = 0; i < m_bufferLength; i++) {
            l_pData[i] = (int16_t) round(m_pData[i]);
        }
        return true;
    }
}

MovAverageCumulative::MovAverageCumulative(uint32_t l_bufferLength,
        float l_filtrationCoef)
{
    m_bufferLength = l_bufferLength;
    m_filtrationCoef = l_filtrationCoef;
    m_pData = new float[m_bufferLength];
}

MovAverageCumulative::~MovAverageCumulative()
{
    delete[] m_pData;
}

MovAverageCumulative *MovAverageCumulative::m_pInstance = nullptr;
