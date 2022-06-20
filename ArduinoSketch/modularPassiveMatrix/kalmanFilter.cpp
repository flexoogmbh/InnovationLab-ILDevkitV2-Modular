#include "kalmanFilter.h"
#include "algorithm"


using namespace Filter;

KalmanInstance::KalmanInstance()
{
    m_errEstimate = 40;
    m_currentEstimate = 0.0;
    m_lastestimate = 0.0;
    m_kalmanGain = 0.0;
}

float KalmanInstance::filter(int32_t l_value, float l_errMeasure,
        float l_measureSpeed)
{
    m_kalmanGain = m_errEstimate / (m_errEstimate + l_errMeasure);
    m_currentEstimate = m_lastestimate + m_kalmanGain * (l_value - m_lastestimate);
    m_errEstimate = (1.0 - m_kalmanGain) * m_errEstimate
            + fabs(m_lastestimate - m_currentEstimate) * l_measureSpeed;
    m_lastestimate = m_currentEstimate;
    return m_currentEstimate;
}

KalmanFilter* KalmanFilter::getInstance(uint32_t l_bufferLength,
        float l_measureSpeed, float l_errMeasure)
{
    if (m_pInstance == nullptr) {

        m_pInstance = new KalmanFilter(l_bufferLength,
                l_measureSpeed, l_errMeasure);
        
        if (m_pInstance == nullptr) {
            return nullptr;
        }
        
    }

    return m_pInstance;
}

bool KalmanFilter::free()
{
    //delete this;
    m_pInstance->~KalmanFilter();
    m_pInstance = nullptr;
    return true;
}

bool KalmanFilter::insertData(FILTER_DATA_TYPE *l_pData)
{
    if (m_pDataPoint != nullptr) {
        for (uint32_t i = 0; i < m_bufferLength; i++) {
            m_pDataPoint[i]->filter(l_pData[i], m_errMeasure, m_measureSpeed);
        }
        return true;
    }else {
        //LOG_MESSAGE_ERROR("Data buffer is NULL");
        return false;
    }
}

bool KalmanFilter::getData(FILTER_DATA_TYPE *l_pData)
{
    if (m_pDataPoint != nullptr) {
        for (uint32_t i = 0; i < m_bufferLength; i++) {
            l_pData[i] = (int16_t) round(m_pDataPoint[i]->m_currentEstimate);
        }
        return true;
    }else {
        //LOG_MESSAGE_ERROR("Data buffer is NULL");
        return false;
    }
}

KalmanFilter::KalmanFilter(uint32_t l_bufferLength, float l_measureSpeed,
        float l_errMeasure)
{
    bool l_result = true;

    m_bufferLength = l_bufferLength;
    m_measureSpeed = l_measureSpeed;
    m_errMeasure = l_errMeasure;

    m_pDataPoint = new KalmanInstance*[l_bufferLength];

    if (m_pDataPoint == nullptr) {
        l_result = false;
        goto exit;
    }else {
        for (uint16_t i = 0; i < l_bufferLength; i++) {
            m_pDataPoint[i] = nullptr;
        }
    }

    for (uint16_t i = 0; i < m_bufferLength; i++) {
        m_pDataPoint[i] = new KalmanInstance;

        if (m_pDataPoint[i] == nullptr) {
            l_result = false;
            goto exit;
        }
        m_pDataPoint[i] = new (m_pDataPoint[i]) KalmanInstance();
    }

    exit : if (!l_result) {
        //clean up if bad allocation
        if (m_pDataPoint != nullptr) {
            for (uint16_t i = 0; i < m_bufferLength; i++) {
                if (m_pDataPoint[i] != nullptr) {
                    delete m_pDataPoint[i];
                    m_pDataPoint[i] = nullptr;
                }
            }
            delete [] m_pDataPoint;
            m_pDataPoint = nullptr;
        }

    }
}

KalmanFilter::~KalmanFilter()
{
    if (m_pDataPoint != nullptr) {
        for (uint16_t i = 0; i < m_bufferLength; i++) {
            if (m_pDataPoint[i] != nullptr) {
                delete m_pDataPoint[i];
                m_pDataPoint[i] = nullptr;
            }
        }
        delete [] m_pDataPoint;
        m_pDataPoint = nullptr;
    }
}

KalmanFilter *KalmanFilter::m_pInstance = nullptr;
