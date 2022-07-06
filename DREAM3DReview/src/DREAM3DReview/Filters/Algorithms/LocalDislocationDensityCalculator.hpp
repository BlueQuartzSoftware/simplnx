#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  LocalDislocationDensityCalculatorInputValues inputValues;

  inputValues.CellSize = filterArgs.value<VectorFloat32Parameter::ValueType>(k_CellSize_Key);
  inputValues.EdgeDataContainerName = filterArgs.value<DataPath>(k_EdgeDataContainerName_Key);
  inputValues.BurgersVectorsArrayPath = filterArgs.value<DataPath>(k_BurgersVectorsArrayPath_Key);
  inputValues.SlipPlaneNormalsArrayPath = filterArgs.value<DataPath>(k_SlipPlaneNormalsArrayPath_Key);
  inputValues.OutputDataContainerName = filterArgs.value<DataPath>(k_OutputDataContainerName_Key);
  inputValues.OutputAttributeMatrixName = filterArgs.value<DataPath>(k_OutputAttributeMatrixName_Key);
  inputValues.OutputArrayName = filterArgs.value<DataPath>(k_OutputArrayName_Key);
  inputValues.DominantSystemArrayName = filterArgs.value<DataPath>(k_DominantSystemArrayName_Key);

  return LocalDislocationDensityCalculator(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT LocalDislocationDensityCalculatorInputValues
{
  VectorFloat32Parameter::ValueType CellSize;
  DataPath EdgeDataContainerName;
  DataPath BurgersVectorsArrayPath;
  DataPath SlipPlaneNormalsArrayPath;
  DataPath OutputDataContainerName;
  DataPath OutputAttributeMatrixName;
  DataPath OutputArrayName;
  DataPath DominantSystemArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT LocalDislocationDensityCalculator
{
public:
  LocalDislocationDensityCalculator(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                    LocalDislocationDensityCalculatorInputValues* inputValues);
  ~LocalDislocationDensityCalculator() noexcept;

  LocalDislocationDensityCalculator(const LocalDislocationDensityCalculator&) = delete;
  LocalDislocationDensityCalculator(LocalDislocationDensityCalculator&&) noexcept = delete;
  LocalDislocationDensityCalculator& operator=(const LocalDislocationDensityCalculator&) = delete;
  LocalDislocationDensityCalculator& operator=(LocalDislocationDensityCalculator&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const LocalDislocationDensityCalculatorInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
