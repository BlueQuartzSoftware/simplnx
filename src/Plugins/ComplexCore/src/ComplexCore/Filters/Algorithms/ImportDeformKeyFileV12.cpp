#include "ImportDeformKeyFileV12.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ImportDeformKeyFileV12::ImportDeformKeyFileV12(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportDeformKeyFileV12InputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ImportDeformKeyFileV12::~ImportDeformKeyFileV12() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ImportDeformKeyFileV12::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ImportDeformKeyFileV12::operator()()
{
  return {};
}
