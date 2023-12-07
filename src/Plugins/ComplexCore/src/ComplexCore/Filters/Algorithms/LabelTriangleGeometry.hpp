#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"


/**
* This is example code to put in the Execute Method of the filter.
  LabelTriangleGeometryInputValues inputValues;

  inputValues.CADDataContainerPath = filterArgs.value<DataPath>(k_CADDataContainerPath_Key);
  inputValues.RegionIdArrayPath = filterArgs.value<DataPath>(k_RegionIdArrayPath_Key);
  inputValues.TriangleAttributeMatrixName = filterArgs.value<DataPath>(k_TriangleAttributeMatrixName_Key);
  inputValues.NumTrianglesArrayName = filterArgs.value<DataPath>(k_NumTrianglesArrayName_Key);

  return LabelTriangleGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct COMPLEXCORE_EXPORT LabelTriangleGeometryInputValues
{
  DataPath CADDataContainerPath;
  DataPath RegionIdArrayPath;
  DataPath TriangleAttributeMatrixName;
  DataPath NumTrianglesArrayName;

};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT LabelTriangleGeometry
{
public:
  LabelTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, LabelTriangleGeometryInputValues* inputValues);
  ~LabelTriangleGeometry() noexcept;

  LabelTriangleGeometry(const LabelTriangleGeometry&) = delete;
  LabelTriangleGeometry(LabelTriangleGeometry&&) noexcept = delete;
  LabelTriangleGeometry& operator=(const LabelTriangleGeometry&) = delete;
  LabelTriangleGeometry& operator=(LabelTriangleGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const LabelTriangleGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
