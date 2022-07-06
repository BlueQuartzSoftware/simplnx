#pragma once

#include "Sampling/Sampling_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AppendImageGeometryZSliceInputValues inputValues;

  inputValues.InputAttributeMatrix = filterArgs.value<DataPath>(k_InputAttributeMatrix_Key);
  inputValues.DestinationAttributeMatrix = filterArgs.value<DataPath>(k_DestinationAttributeMatrix_Key);
  inputValues.CheckResolution = filterArgs.value<bool>(k_CheckResolution_Key);

  return AppendImageGeometryZSlice(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SAMPLING_EXPORT AppendImageGeometryZSliceInputValues
{
  DataPath InputAttributeMatrix;
  DataPath DestinationAttributeMatrix;
  bool CheckResolution;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SAMPLING_EXPORT AppendImageGeometryZSlice
{
public:
  AppendImageGeometryZSlice(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AppendImageGeometryZSliceInputValues* inputValues);
  ~AppendImageGeometryZSlice() noexcept;

  AppendImageGeometryZSlice(const AppendImageGeometryZSlice&) = delete;
  AppendImageGeometryZSlice(AppendImageGeometryZSlice&&) noexcept = delete;
  AppendImageGeometryZSlice& operator=(const AppendImageGeometryZSlice&) = delete;
  AppendImageGeometryZSlice& operator=(AppendImageGeometryZSlice&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AppendImageGeometryZSliceInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
