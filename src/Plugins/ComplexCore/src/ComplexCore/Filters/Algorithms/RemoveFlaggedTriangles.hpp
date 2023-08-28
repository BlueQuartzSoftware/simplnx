#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RemoveFlaggedTrianglesInputValues inputValues;

  inputValues.TriangleGeometry = filterArgs.value<DataPath>(k_TriangleGeometry_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.RegionIDsArrayPath = filterArgs.value<DataPath>(k_RegionIDsArrayPath_Key);
  inputValues.ReducedTriangleGeometry = filterArgs.value<StringParameter::ValueType>(k_ReducedTriangleGeometry_Key);

  return RemoveFlaggedTriangles(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct COMPLEXCORE_EXPORT RemoveFlaggedTrianglesInputValues
{
  DataPath TriangleGeometry;
  DataPath MaskArrayPath;
  DataPath RegionIDsArrayPath;
  StringParameter::ValueType ReducedTriangleGeometry;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT RemoveFlaggedTriangles
{
public:
  RemoveFlaggedTriangles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RemoveFlaggedTrianglesInputValues* inputValues);
  ~RemoveFlaggedTriangles() noexcept;

  RemoveFlaggedTriangles(const RemoveFlaggedTriangles&) = delete;
  RemoveFlaggedTriangles(RemoveFlaggedTriangles&&) noexcept = delete;
  RemoveFlaggedTriangles& operator=(const RemoveFlaggedTriangles&) = delete;
  RemoveFlaggedTriangles& operator=(RemoveFlaggedTriangles&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RemoveFlaggedTrianglesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
