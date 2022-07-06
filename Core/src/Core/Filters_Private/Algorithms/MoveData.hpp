#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  MoveDataInputValues inputValues;

  inputValues.WhatToMove = filterArgs.value<ChoicesParameter::ValueType>(k_WhatToMove_Key);
  inputValues.AttributeMatrixSource = filterArgs.value<DataPath>(k_AttributeMatrixSource_Key);
  inputValues.DataContainerDestination = filterArgs.value<DataPath>(k_DataContainerDestination_Key);
  inputValues.DataArraySource = filterArgs.value<DataPath>(k_DataArraySource_Key);
  inputValues.AttributeMatrixDestination = filterArgs.value<DataPath>(k_AttributeMatrixDestination_Key);

  return MoveData(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT MoveDataInputValues
{
  ChoicesParameter::ValueType WhatToMove;
  DataPath AttributeMatrixSource;
  DataPath DataContainerDestination;
  DataPath DataArraySource;
  DataPath AttributeMatrixDestination;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT MoveData
{
public:
  MoveData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MoveDataInputValues* inputValues);
  ~MoveData() noexcept;

  MoveData(const MoveData&) = delete;
  MoveData(MoveData&&) noexcept = delete;
  MoveData& operator=(const MoveData&) = delete;
  MoveData& operator=(MoveData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MoveDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
