#ifndef KALMAN_HPP
#define KALMAN_HPP

#include "Application.h"
#include "filterInterface.h"
#include "stdint.h"
#include "math.h"

namespace Filter
{
    struct KalmanInstance
    {
            double m_errEstimate;
            double m_currentEstimate;
            double m_lastestimate;
            double m_kalmanGain;

            KalmanInstance();
            float filter(int32_t value, float l_errMeasure,
                    float l_measureSpeed);
    };

    class KalmanFilter : public Interface::IFilter
    {
        public:
            static KalmanFilter* getInstance(uint32_t l_bufferLength,
                    float l_measureSpeed, float l_errMeasure);
            bool free();
            bool insertData(FILTER_DATA_TYPE *l_pData);
            bool getData(FILTER_DATA_TYPE *l_pData);
        private:
            uint32_t m_bufferLength;
            KalmanInstance **m_pDataPoint;

            double m_errMeasure;
            double m_measureSpeed;

            KalmanFilter(uint32_t l_bufferLength, float l_measureSpeed,
                    float l_errMeasure);
            ~KalmanFilter();
            static KalmanFilter *m_pInstance;
    };
}
;

#endif
