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

struct SIMPLNXCORE_EXPORT FindTriangleGeomSizesInputValues
{
  DataPath TriangleGeometryPath;
  DataPath FaceLabelsArrayPath;
  DataPath FeatureAttributeMatrixPath;
  DataPath VolumesArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT FindTriangleGeomSizes
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

} // namespace nx::core
