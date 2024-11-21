#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeTriangleGeomVolumesInputValues
{
  DataPath TriangleGeometryPath;
  DataPath FaceLabelsArrayPath;
  DataPath FeatureAttributeMatrixPath;
  DataPath VolumesArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeTriangleGeomVolumes
{
public:
  ComputeTriangleGeomVolumes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeTriangleGeomVolumesInputValues* inputValues);
  ~ComputeTriangleGeomVolumes() noexcept;

  ComputeTriangleGeomVolumes(const ComputeTriangleGeomVolumes&) = delete;
  ComputeTriangleGeomVolumes(ComputeTriangleGeomVolumes&&) noexcept = delete;
  ComputeTriangleGeomVolumes& operator=(const ComputeTriangleGeomVolumes&) = delete;
  ComputeTriangleGeomVolumes& operator=(ComputeTriangleGeomVolumes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeTriangleGeomVolumesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
