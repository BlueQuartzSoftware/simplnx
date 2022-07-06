#pragma once

#include "Sampling/Sampling_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  WarpRegularGridInputValues inputValues;

  inputValues.PolyOrder = filterArgs.value<ChoicesParameter::ValueType>(k_PolyOrder_Key);
  inputValues.SecondOrderACoeff = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SecondOrderACoeff_Key);
  inputValues.SecondOrderBCoeff = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_SecondOrderBCoeff_Key);
  inputValues.ThirdOrderACoeff = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ThirdOrderACoeff_Key);
  inputValues.ThirdOrderBCoeff = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ThirdOrderBCoeff_Key);
  inputValues.FourthOrderACoeff = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_FourthOrderACoeff_Key);
  inputValues.FourthOrderBCoeff = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_FourthOrderBCoeff_Key);
  inputValues.SaveAsNewDataContainer = filterArgs.value<bool>(k_SaveAsNewDataContainer_Key);
  inputValues.NewDataContainerName = filterArgs.value<DataPath>(k_NewDataContainerName_Key);
  inputValues.CellAttributeMatrixPath = filterArgs.value<DataPath>(k_CellAttributeMatrixPath_Key);

  return WarpRegularGrid(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SAMPLING_EXPORT WarpRegularGridInputValues
{
  ChoicesParameter::ValueType PolyOrder;
  <<<NOT_IMPLEMENTED>>> SecondOrderACoeff;
  /*[x]*/<<<NOT_IMPLEMENTED>>> SecondOrderBCoeff;
  /*[x]*/<<<NOT_IMPLEMENTED>>> ThirdOrderACoeff;
  /*[x]*/<<<NOT_IMPLEMENTED>>> ThirdOrderBCoeff;
  /*[x]*/<<<NOT_IMPLEMENTED>>> FourthOrderACoeff;
  /*[x]*/<<<NOT_IMPLEMENTED>>> FourthOrderBCoeff;
  /*[x]*/ bool SaveAsNewDataContainer;
  DataPath NewDataContainerName;
  DataPath CellAttributeMatrixPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SAMPLING_EXPORT WarpRegularGrid
{
public:
  WarpRegularGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WarpRegularGridInputValues* inputValues);
  ~WarpRegularGrid() noexcept;

  WarpRegularGrid(const WarpRegularGrid&) = delete;
  WarpRegularGrid(WarpRegularGrid&&) noexcept = delete;
  WarpRegularGrid& operator=(const WarpRegularGrid&) = delete;
  WarpRegularGrid& operator=(WarpRegularGrid&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WarpRegularGridInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
