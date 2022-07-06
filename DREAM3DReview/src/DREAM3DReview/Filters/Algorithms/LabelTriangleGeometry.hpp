#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  LabelTriangleGeometryInputValues inputValues;

  inputValues.CADDataContainerName = filterArgs.value<DataPath>(k_CADDataContainerName_Key);
  inputValues.TriangleAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_TriangleAttributeMatrixName_Key);
  inputValues.RegionIdArrayName = filterArgs.value<StringParameter::ValueType>(k_RegionIdArrayName_Key);

  return LabelTriangleGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT LabelTriangleGeometryInputValues
{
  DataPath CADDataContainerName;
  StringParameter::ValueType TriangleAttributeMatrixName;
  StringParameter::ValueType RegionIdArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT LabelTriangleGeometry
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
