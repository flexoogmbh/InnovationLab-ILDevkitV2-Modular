#include "byPass.h"

using namespace Filter;

ByPass* ByPass::getInstance()
{
    if (m_pInstance == nullptr) {
        m_pInstance = new ByPass();
    }

    return m_pInstance;
}

bool ByPass::free()
{

    delete this;
    m_pInstance = nullptr;
    return true;
}

bool ByPass::insertData(FILTER_DATA_TYPE *l_pData)
{
    m_pData = l_pData;
    return true;
}

bool ByPass::getData(FILTER_DATA_TYPE *l_pData)
{
    l_pData = m_pData;
    return true;
}

ByPass::ByPass()
{
    m_pData = nullptr;
    

}

ByPass::~ByPass()
{
    
}

ByPass *ByPass::m_pInstance = nullptr;
