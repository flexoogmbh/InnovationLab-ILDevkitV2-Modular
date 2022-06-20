#ifndef BYPASS_HPP
#define BYPASS_HPP

#include "Application.h"
#include "filterInterface.h"
#include "stdint.h"
#include "math.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace Filter
{
    class ByPass : public Interface::IFilter
    {
        public:
            static ByPass* getInstance();
            bool free();
            bool insertData(FILTER_DATA_TYPE *m_pData);
            bool getData(FILTER_DATA_TYPE *l_pData);
        private:
            FILTER_DATA_TYPE *m_pData;

            ByPass();
            ~ByPass();
            static ByPass *m_pInstance;
    };
};

#ifdef __cplusplus
}
#endif

#endif
