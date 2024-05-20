#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeTriangleGeomSizesInputValues
{
  DataPath TriangleGeometryPath;
  DataPath FaceLabelsArrayPath;
  DataPath FeatureAttributeMatrixPath;
  DataPath VolumesArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ComputeTriangleGeomSizes
{
public:
  ComputeTriangleGeomSizes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeTriangleGeomSizesInputValues* inputValues);
  ~ComputeTriangleGeomSizes() noexcept;

  ComputeTriangleGeomSizes(const ComputeTriangleGeomSizes&) = delete;
  ComputeTriangleGeomSizes(ComputeTriangleGeomSizes&&) noexcept = delete;
  ComputeTriangleGeomSizes& operator=(const ComputeTriangleGeomSizes&) = delete;
  ComputeTriangleGeomSizes& operator=(ComputeTriangleGeomSizes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeTriangleGeomSizesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
