#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.

*/

namespace complex
{

struct CORE_EXPORT FindTriangleGeomCentroidsInputValues
{
  DataPath TriangleGeometryPath;
  DataPath FaceLabelsArrayPath;
  DataPath FeatureAttributeMatrixPath;
  DataPath CentroidsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT FindTriangleGeomCentroids
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
