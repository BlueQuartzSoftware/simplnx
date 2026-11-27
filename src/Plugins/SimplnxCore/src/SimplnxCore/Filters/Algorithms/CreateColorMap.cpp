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

const std::atomic_bool& CreateColorMap::getCancel()
{
  return m_ShouldCancel;
}

Result<> CreateColorMap::operator()()
{
  const auto* selectedArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->selected_data_array_path);
  const auto* colorArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->rgb_array_path);
  const auto* maskArrayPtr = m_InputValues->use_mask ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->mask_array_path) : nullptr;

  // Input, output, and mask storage each determine whether bulk I/O is required.
  return DispatchAlgorithm<CreateColorMapDirect, CreateColorMapScanline>({selectedArrayPtr, colorArrayPtr, maskArrayPtr}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
