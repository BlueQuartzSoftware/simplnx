#pragma once

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindTriangleGeomCentroidsInputValues inputValues;

  inputValues.FaceLabelsArrayPath = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  inputValues.FeatureAttributeMatrixName = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  inputValues.CentroidsArrayName = filterArgs.value<DataPath>(k_CentroidsArrayName_Key);

  return FindTriangleGeomCentroids(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SURFACEMESHING_EXPORT FindTriangleGeomCentroidsInputValues
{
  DataPath FaceLabelsArrayPath;
  DataPath FeatureAttributeMatrixName;
  DataPath CentroidsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SURFACEMESHING_EXPORT FindTriangleGeomCentroids
{
public:
  FindTriangleGeomCentroids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindTriangleGeomCentroidsInputValues* inputValues);
  ~FindTriangleGeomCentroids() noexcept;

  FindTriangleGeomCentroids(const FindTriangleGeomCentroids&) = delete;
  FindTriangleGeomCentroids(FindTriangleGeomCentroids&&) noexcept = delete;
  FindTriangleGeomCentroids& operator=(const FindTriangleGeomCentroids&) = delete;
  FindTriangleGeomCentroids& operator=(FindTriangleGeomCentroids&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindTriangleGeomCentroidsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
