#include "movAverageWeighted.h"
#include "algorithm"
#include "math.h"

using namespace Filter;

MovAverageWeighted* MovAverageWeighted::getInstance(uint32_t l_bufferLength,
        uint16_t l_windowSize)
{
    if (m_pInstance == nullptr) {
        //m_pInstance = new MovAverageWeighted(l_bufferLength, l_windowSize);

        m_pInstance = new MovAverageWeighted(l_bufferLength,
                l_windowSize);

        if (m_pInstance == nullptr) {
            return nullptr;
        }
    }

    return m_pInstance;
}

bool MovAverageWeighted::free()
{
    //delete this;
    m_pInstance->~MovAverageWeighted();
    m_pInstance = nullptr;
    return true;
}

bool MovAverageWeighted::insertData(FILTER_DATA_TYPE *l_pData)
{
    if (m_pData != nullptr) {
        for (uint32_t i = 0; i < m_bufferLength; i++) {
            for (int16_t j = m_windowSize - 1; j > 0; j--) {
                m_pData[i][j] = m_pData[i][j - 1];
            }
            m_pData[i][0] = l_pData[i];
        }
        return true;
    }else {
        //LOG_MESSAGE_ERROR("Data buffer is NULL");
        return false;
    }
}

bool MovAverageWeighted::getData(FILTER_DATA_TYPE *l_pData)
{
    m_indexSumm = 0;

    //Get factorial of window size
    for (uint16_t i = 1; i < (m_windowSize + 1); i++) {
        m_indexSumm += i;
    }

    if (m_pData != nullptr) {
        for (uint32_t i = 0; i < m_bufferLength; i++) {
            m_result = 0;
            for (uint16_t j = 0; j < m_windowSize; j++) {
                m_result += m_pData[i][j] * ((float)(m_windowSize - j) / m_indexSumm);
            }
            l_pData[i] = (FILTER_DATA_TYPE) round(m_result);
        }
        return true;
    }else {
        //LOG_MESSAGE_ERROR("Error. Data buffer is NULL");
        return false;
    }
}

MovAverageWeighted::MovAverageWeighted(uint32_t l_bufferLength,
        uint16_t l_windowSize)
{
    bool l_result = true;

    m_windowSize = l_windowSize;
    m_bufferLength = l_bufferLength;

    m_pData = new float*[m_bufferLength];

    if (m_pData == nullptr) {
        l_result = false;
        goto exit;
    }else {
        for (uint16_t i = 0; i < l_windowSize; i++) {
            m_pData[i] = nullptr;
        }
    }

    for (uint16_t i = 0; i < m_bufferLength; i++) {
        m_pData[i] = new float[m_windowSize];
        if (m_pData[i] == nullptr) {
            l_result = false;
            goto exit;
        }
    }

    exit : if (!l_result) {
        //LOG_MESSAGE_ERROR("Bad memory allocation");

        //clean up if bad allocation
        if (m_pData != nullptr) {
            for (uint16_t i = 0; i < m_bufferLength; i++) {
                if (m_pData[i] != nullptr) {
                    delete[] m_pData[i];
                    m_pData[i] = nullptr;
                }
            }
            delete[] m_pData;
            m_pData = nullptr;
        }

    }else {
        //LOG_MESSAGE_OK("MovAverageWeighted::Allocating of %d bytes",
        //        (m_bufferLength * m_windowSize * sizeof(float)));
    }
}

MovAverageWeighted::~MovAverageWeighted()
{
    //LOG_MESSAGE("MovAverageWeighted::Free of %d bytes", (m_bufferLength * sizeof(float)));

    if (m_pData != nullptr) {
        for (uint16_t i = 0; i < m_bufferLength; i++) {
            if (m_pData[i] != nullptr) {
                delete[] m_pData[i];
                m_pData[i] = nullptr;
            }
        }
        delete[] m_pData;
        m_pData = nullptr;
    }
}

MovAverageWeighted *MovAverageWeighted::m_pInstance = nullptr;

