#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  MoveMultiDataInputValues inputValues;

  inputValues.WhatToMove = filterArgs.value<ChoicesParameter::ValueType>(k_WhatToMove_Key);
  inputValues.AttributeMatrixSources = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_AttributeMatrixSources_Key);
  inputValues.DataContainerDestination = filterArgs.value<DataPath>(k_DataContainerDestination_Key);
  inputValues.DataArraySources = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_DataArraySources_Key);
  inputValues.AttributeMatrixDestination = filterArgs.value<DataPath>(k_AttributeMatrixDestination_Key);

  return MoveMultiData(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT MoveMultiDataInputValues
{
  ChoicesParameter::ValueType WhatToMove;
  <<<NOT_IMPLEMENTED>>> AttributeMatrixSources;
  /*[x]*/ DataPath DataContainerDestination;
  MultiArraySelectionParameter::ValueType DataArraySources;
  DataPath AttributeMatrixDestination;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT MoveMultiData
{
public:
  MoveMultiData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MoveMultiDataInputValues* inputValues);
  ~MoveMultiData() noexcept;

  MoveMultiData(const MoveMultiData&) = delete;
  MoveMultiData(MoveMultiData&&) noexcept = delete;
  MoveMultiData& operator=(const MoveMultiData&) = delete;
  MoveMultiData& operator=(MoveMultiData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MoveMultiDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
