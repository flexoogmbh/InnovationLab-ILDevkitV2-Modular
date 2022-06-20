#include "median.h"
#include "algorithm"

using namespace Filter;

MedianFilter* MedianFilter::getInstance(uint32_t l_bufferLength, uint16_t l_windowSize)
{
    if (m_pInstance == nullptr) {
        m_pInstance = new MedianFilter(l_bufferLength, l_windowSize);
    }

    return m_pInstance;
}

bool MedianFilter::free()
{
    delete this;
    m_pInstance = nullptr;
    return true;
}

bool MedianFilter::insertData(FILTER_DATA_TYPE *l_pData)
{
    volatile bool l_result = true;

    if (m_windowSize == 0) {
        //LOG_MESSAGE_ERROR("MedianFilter::Error. Window size is zero");
        return false;
    }

    //Create temp buffer
    FILTER_DATA_TYPE *l_buffer = new FILTER_DATA_TYPE[m_windowSize];

    //check pointers if all is ok
    if ((l_buffer == nullptr) || (m_pData == nullptr)) {
        l_result = false;
        goto exit;
    }else {
        for (uint16_t i = 0; i < m_windowSize; i++) {
            if (m_pData[i] == nullptr) {
                l_result = false;
                goto exit;
            }
        }
    }

    //put data in to first array and sort data
    for (uint32_t i = 0; i < m_bufferLength; i++) {
        if (l_pData[i] < m_pData[m_windowSize / 2][i]) {
            m_pData[m_windowSize - 1][i] = l_pData[i];
        }else if (l_pData[i] > m_pData[m_windowSize / 2][i]) {
            m_pData[0][i] = l_pData[i];
        }else {
            continue;
        }

        for (uint32_t j = 0; j < m_windowSize; j++) {
            l_buffer[j] = m_pData[j][i];
        }

        std::sort(&l_buffer[0], &l_buffer[m_windowSize]);

        for (uint32_t j = 0; j < m_windowSize; j++) {
            m_pData[j][i] = l_buffer[j];
        }
    }

    exit : delete[] l_buffer;
    if (l_result == false) {
        //LOG_MESSAGE_ERROR("MedianFilter::Error. Data buffer is NULL");
        return false;
    }else {
        return true;
    }
}

bool MedianFilter::getData(FILTER_DATA_TYPE *l_pData)
{
    bool l_result = true;

    //check external buffer
    if ((l_pData == nullptr) || (m_pData == nullptr)) {
        return false;
    }


    for (uint16_t j = 0; j < m_windowSize; j++) {
        if (m_pData[j] == nullptr) {
            return false;
        }
    }

    for (uint32_t i = 0; i < m_bufferLength; i++) {
        l_pData[i] = m_pData[m_windowSize / 2][i];
    }

    exit : if (!l_result) {
        //zeroing data buffer for indicates that something is wrong
        for (uint32_t i = 0; i < m_bufferLength; i++) {
            l_pData[i] = 0;
        }
    }
    return l_result;
}

MedianFilter::MedianFilter(uint32_t l_bufferLength, uint16_t l_windowSize)
{
    bool l_result = true;

    m_windowSize = l_windowSize;
    m_bufferLength = l_bufferLength;

    m_pData = new FILTER_DATA_TYPE*[l_windowSize];

    if (m_pData == nullptr) {
        goto exit;
    }

    for (uint16_t i = 0; i < l_windowSize; i++) {
        m_pData[i] = nullptr;
    }

    for (uint16_t i = 0; i < m_windowSize; i++) {
        m_pData[i] = new FILTER_DATA_TYPE[m_bufferLength];
        if (m_pData[i] == nullptr) { 
            goto exit;
        }

        for (uint16_t j = 0; j < m_bufferLength; j++) {
            m_pData[i][j] = 0;
        }
    }

    exit : if (!l_result) {
        //LOG_MESSAGE_ERROR("MedianFilter::Bad memory allocation");

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

    }else {
        //LOG_MESSAGE_OK("MedianFilter::Allocating of %d bytes", (m_bufferLength * 2 * m_windowSize));
    }

}

MedianFilter::~MedianFilter()
{
    //LOG_MESSAGE("MedianFilter::Free of %d bytes", (m_bufferLength * 2 * m_windowSize));

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

MedianFilter *MedianFilter::m_pInstance = nullptr;
