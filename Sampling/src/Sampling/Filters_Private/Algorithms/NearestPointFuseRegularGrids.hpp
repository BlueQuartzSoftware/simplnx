#pragma once

#include "Sampling/Sampling_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  NearestPointFuseRegularGridsInputValues inputValues;

  inputValues.ReferenceCellAttributeMatrixPath = filterArgs.value<DataPath>(k_ReferenceCellAttributeMatrixPath_Key);
  inputValues.SamplingCellAttributeMatrixPath = filterArgs.value<DataPath>(k_SamplingCellAttributeMatrixPath_Key);

  return NearestPointFuseRegularGrids(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SAMPLING_EXPORT NearestPointFuseRegularGridsInputValues
{
  DataPath ReferenceCellAttributeMatrixPath;
  DataPath SamplingCellAttributeMatrixPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SAMPLING_EXPORT NearestPointFuseRegularGrids
{
public:
  NearestPointFuseRegularGrids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, NearestPointFuseRegularGridsInputValues* inputValues);
  ~NearestPointFuseRegularGrids() noexcept;

  NearestPointFuseRegularGrids(const NearestPointFuseRegularGrids&) = delete;
  NearestPointFuseRegularGrids(NearestPointFuseRegularGrids&&) noexcept = delete;
  NearestPointFuseRegularGrids& operator=(const NearestPointFuseRegularGrids&) = delete;
  NearestPointFuseRegularGrids& operator=(NearestPointFuseRegularGrids&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const NearestPointFuseRegularGridsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
