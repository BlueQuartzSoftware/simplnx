#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT SharedFeatureFaceInputValues
{
  DataPath TriangleGeometryPath;

  DataPath FaceLabelsArrayPath;
  DataPath FeatureFaceIdsArrayPath;
  DataPath GrainBoundaryAttributeMatrixPath;
  DataPath FeatureFaceLabelsArrayPath;
  DataPath FeatureFaceNumTrianglesArrayPath;
  bool ShouldRandomizeFeatureIds;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT SharedFeatureFace
{
public:
  SharedFeatureFace(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SharedFeatureFaceInputValues* inputValues);
  ~SharedFeatureFace() noexcept;

  SharedFeatureFace(const SharedFeatureFace&) = delete;
  SharedFeatureFace(SharedFeatureFace&&) noexcept = delete;
  SharedFeatureFace& operator=(const SharedFeatureFace&) = delete;
  SharedFeatureFace& operator=(SharedFeatureFace&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SharedFeatureFaceInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
