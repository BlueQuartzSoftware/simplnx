#include "CreateColorMap.hpp"

#include "CreateColorMapDirect.hpp"
#include "CreateColorMapScanline.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

CreateColorMap::CreateColorMap(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, CreateColorMapInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

CreateColorMap::~CreateColorMap() noexcept = default;

const std::atomic_bool& CreateColorMap::getCancel()
{
  return m_ShouldCancel;
}

Result<> CreateColorMap::operator()()
{
  const auto* selectedArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->SelectedDataArrayPath);
  const auto* colorArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->RgbArrayPath);
  const auto* maskArray = m_InputValues->UseMask ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath) : nullptr;

  // Input, output, and mask storage each determine whether bulk I/O is required.
  return DispatchAlgorithm<CreateColorMapDirect, CreateColorMapScanline>({selectedArray, colorArray, maskArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
