#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT FindTriangleGeomCentroidsInputValues
{
  DataPath TriangleGeometryPath;
  DataPath FaceLabelsArrayPath;
  DataPath FeatureAttributeMatrixPath;
  DataPath CentroidsArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT FindTriangleGeomCentroids
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

} // namespace nx::core
