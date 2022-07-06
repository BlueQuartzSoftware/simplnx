#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ExtractAttributeArraysFromGeometryInputValues inputValues;

  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.XBoundsArrayPath = filterArgs.value<DataPath>(k_XBoundsArrayPath_Key);
  inputValues.YBoundsArrayPath = filterArgs.value<DataPath>(k_YBoundsArrayPath_Key);
  inputValues.ZBoundsArrayPath = filterArgs.value<DataPath>(k_ZBoundsArrayPath_Key);
  inputValues.SharedVertexListArrayPath0 = filterArgs.value<DataPath>(k_SharedVertexListArrayPath0_Key);
  inputValues.SharedVertexListArrayPath1 = filterArgs.value<DataPath>(k_SharedVertexListArrayPath1_Key);
  inputValues.SharedEdgeListArrayPath = filterArgs.value<DataPath>(k_SharedEdgeListArrayPath_Key);
  inputValues.SharedVertexListArrayPath2 = filterArgs.value<DataPath>(k_SharedVertexListArrayPath2_Key);
  inputValues.SharedTriListArrayPath = filterArgs.value<DataPath>(k_SharedTriListArrayPath_Key);
  inputValues.SharedVertexListArrayPath3 = filterArgs.value<DataPath>(k_SharedVertexListArrayPath3_Key);
  inputValues.SharedQuadListArrayPath = filterArgs.value<DataPath>(k_SharedQuadListArrayPath_Key);
  inputValues.SharedVertexListArrayPath4 = filterArgs.value<DataPath>(k_SharedVertexListArrayPath4_Key);
  inputValues.SharedTetListArrayPath = filterArgs.value<DataPath>(k_SharedTetListArrayPath_Key);
  inputValues.SharedVertexListArrayPath5 = filterArgs.value<DataPath>(k_SharedVertexListArrayPath5_Key);
  inputValues.SharedHexListArrayPath = filterArgs.value<DataPath>(k_SharedHexListArrayPath_Key);

  return ExtractAttributeArraysFromGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT ExtractAttributeArraysFromGeometryInputValues
{
  DataPath DataContainerName;
  DataPath XBoundsArrayPath;
  DataPath YBoundsArrayPath;
  DataPath ZBoundsArrayPath;
  DataPath SharedVertexListArrayPath0;
  DataPath SharedVertexListArrayPath1;
  DataPath SharedEdgeListArrayPath;
  DataPath SharedVertexListArrayPath2;
  DataPath SharedTriListArrayPath;
  DataPath SharedVertexListArrayPath3;
  DataPath SharedQuadListArrayPath;
  DataPath SharedVertexListArrayPath4;
  DataPath SharedTetListArrayPath;
  DataPath SharedVertexListArrayPath5;
  DataPath SharedHexListArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT ExtractAttributeArraysFromGeometry
{
public:
  ExtractAttributeArraysFromGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     ExtractAttributeArraysFromGeometryInputValues* inputValues);
  ~ExtractAttributeArraysFromGeometry() noexcept;

  ExtractAttributeArraysFromGeometry(const ExtractAttributeArraysFromGeometry&) = delete;
  ExtractAttributeArraysFromGeometry(ExtractAttributeArraysFromGeometry&&) noexcept = delete;
  ExtractAttributeArraysFromGeometry& operator=(const ExtractAttributeArraysFromGeometry&) = delete;
  ExtractAttributeArraysFromGeometry& operator=(ExtractAttributeArraysFromGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExtractAttributeArraysFromGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
