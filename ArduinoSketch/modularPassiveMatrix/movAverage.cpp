#include "movAverage.h"

using namespace Filter;

MovAverage* MovAverage::getInstance(uint32_t l_bufferLength, uint16_t l_windowSize)
{
    if (m_pInstance == nullptr) {
        m_pInstance = new MovAverage(l_bufferLength, l_windowSize);
    }

    return m_pInstance;
}

bool MovAverage::free()
{
    delete this;
    m_pInstance = nullptr;
    return true;
}

bool MovAverage::insertData(FILTER_DATA_TYPE *l_pData)
{
    bool l_result = true;

    for (uint32_t i = 0; i < m_bufferLength; i++) {
        m_pData[m_bufferCounter][i] = l_pData[i];
    }
    ++m_bufferCounter;
    if (m_bufferCounter >= m_windowSize) {
        m_bufferCounter = 0;
    }

    exit : return l_result;
}

bool MovAverage::getData(FILTER_DATA_TYPE *l_pData)
{
    bool l_result = true;
    int32_t l_point = 0;


    for (uint32_t i = 0; i < m_bufferLength; i++) {
        for (uint16_t j = 0; j < m_windowSize; j++) {
            l_point += m_pData[j][i];
        }
        l_pData[i] = l_point / m_windowSize;
        l_point = 0;
    }

    exit : if (!l_result) {
        //zeroing data buffer for indicates that something is wrong
        for (uint32_t i = 0; i < m_bufferLength; i++) {
            l_pData[i] = 0;
        }
    }
    return l_result;
}

MovAverage::MovAverage(uint32_t l_bufferLength, uint16_t l_windowSize)
{
    bool l_result = true;
    m_bufferCounter = 0;

    m_windowSize = l_windowSize;
    m_bufferLength = l_bufferLength;

    m_pData = new FILTER_DATA_TYPE*[l_windowSize];

    if (m_pData == nullptr) {
        l_result = false;
        goto exit;
    }else {
        for (uint16_t i = 0; i < l_windowSize; i++) {
            m_pData[i] = nullptr;
        }
    }

    for (uint16_t i = 0; i < m_windowSize; i++) {
        m_pData[i] = new FILTER_DATA_TYPE[m_bufferLength];
        if (m_pData[i] == nullptr) {
            l_result = false;
            goto exit;
        }
    }

    exit : if (!l_result) {

        //clean up if bad allocation
        if (m_pData != nullptr) {
            for (uint16_t i = 0; i < m_windowSize; i++) {
                if (m_pData[i] != nullptr) {
                    delete[] m_pData[i];
                    m_pData[i] = nullptr;
                }
            }
            delete m_pData;
            m_pData = nullptr;
        }

    }
}

MovAverage::~MovAverage()
{
    if (m_pData != nullptr) {
        for (uint16_t i = 0; i < m_windowSize; i++) {
            if (m_pData[i] != nullptr) {
                delete[] m_pData[i];
                m_pData[i] = nullptr;
            }
        }
        delete m_pData;
        m_pData = nullptr;
    }
}

MovAverage *MovAverage::m_pInstance = nullptr;
