#pragma once

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindTriangleGeomNeighborsInputValues inputValues;

  inputValues.FaceLabelsArrayPath = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  inputValues.FeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  inputValues.NumNeighborsArrayName = filterArgs.value<DataPath>(k_NumNeighborsArrayName_Key);
  inputValues.NeighborListArrayName = filterArgs.value<DataPath>(k_NeighborListArrayName_Key);

  return FindTriangleGeomNeighbors(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SURFACEMESHING_EXPORT FindTriangleGeomNeighborsInputValues
{
  DataPath FaceLabelsArrayPath;
  DataPath FeatureAttributeMatrixPath;
  DataPath NumNeighborsArrayName;
  DataPath NeighborListArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SURFACEMESHING_EXPORT FindTriangleGeomNeighbors
{
public:
  FindTriangleGeomNeighbors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindTriangleGeomNeighborsInputValues* inputValues);
  ~FindTriangleGeomNeighbors() noexcept;

  FindTriangleGeomNeighbors(const FindTriangleGeomNeighbors&) = delete;
  FindTriangleGeomNeighbors(FindTriangleGeomNeighbors&&) noexcept = delete;
  FindTriangleGeomNeighbors& operator=(const FindTriangleGeomNeighbors&) = delete;
  FindTriangleGeomNeighbors& operator=(FindTriangleGeomNeighbors&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindTriangleGeomNeighborsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
