#pragma once

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindTriangleGeomSizesInputValues inputValues;

  inputValues.FaceLabelsArrayPath = filterArgs.value<DataPath>(k_FaceLabelsArrayPath_Key);
  inputValues.FeatureAttributeMatrixName = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);
  inputValues.VolumesArrayName = filterArgs.value<DataPath>(k_VolumesArrayName_Key);

  return FindTriangleGeomSizes(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SURFACEMESHING_EXPORT FindTriangleGeomSizesInputValues
{
  DataPath FaceLabelsArrayPath;
  DataPath FeatureAttributeMatrixName;
  DataPath VolumesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SURFACEMESHING_EXPORT FindTriangleGeomSizes
{
public:
  FindTriangleGeomSizes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindTriangleGeomSizesInputValues* inputValues);
  ~FindTriangleGeomSizes() noexcept;

  FindTriangleGeomSizes(const FindTriangleGeomSizes&) = delete;
  FindTriangleGeomSizes(FindTriangleGeomSizes&&) noexcept = delete;
  FindTriangleGeomSizes& operator=(const FindTriangleGeomSizes&) = delete;
  FindTriangleGeomSizes& operator=(FindTriangleGeomSizes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindTriangleGeomSizesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
