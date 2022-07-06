#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ExtractVertexGeometryInputValues inputValues;

  inputValues.ArrayHandling = filterArgs.value<ChoicesParameter::ValueType>(k_ArrayHandling_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.SelectedDataContainerName = filterArgs.value<DataPath>(k_SelectedDataContainerName_Key);
  inputValues.IncludedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IncludedDataArrayPaths_Key);
  inputValues.VertexDataContainerName = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);

  return ExtractVertexGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT ExtractVertexGeometryInputValues
{
  ChoicesParameter::ValueType ArrayHandling;
  bool UseMask;
  DataPath MaskArrayPath;
  DataPath SelectedDataContainerName;
  MultiArraySelectionParameter::ValueType IncludedDataArrayPaths;
  DataPath VertexDataContainerName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT ExtractVertexGeometry
{
public:
  ExtractVertexGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExtractVertexGeometryInputValues* inputValues);
  ~ExtractVertexGeometry() noexcept;

  ExtractVertexGeometry(const ExtractVertexGeometry&) = delete;
  ExtractVertexGeometry(ExtractVertexGeometry&&) noexcept = delete;
  ExtractVertexGeometry& operator=(const ExtractVertexGeometry&) = delete;
  ExtractVertexGeometry& operator=(ExtractVertexGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExtractVertexGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
