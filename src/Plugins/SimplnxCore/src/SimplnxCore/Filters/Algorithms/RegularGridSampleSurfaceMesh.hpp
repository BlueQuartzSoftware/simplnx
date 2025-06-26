#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/SampleSurfaceMesh.hpp"

#include <set>

namespace nx::core
{
struct SIMPLNXCORE_EXPORT RegularGridSampleSurfaceMeshInputValues
{
  VectorUInt64Parameter::ValueType Dimensions;
  VectorFloat32Parameter::ValueType Spacing;
  VectorFloat32Parameter::ValueType Origin;
  DataPath TriangleGeometryPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath ImageGeometryOutputPath;
  DataPath FeatureIdsArrayPath;
  DataPath SurfaceMeshPartIdsArrayPath;
};

/**
 * @class ConditionalSetValueFilter

 */
class SIMPLNXCORE_EXPORT RegularGridSampleSurfaceMesh : public SampleSurfaceMesh
{
public:
  RegularGridSampleSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RegularGridSampleSurfaceMeshInputValues* inputValues);
  ~RegularGridSampleSurfaceMesh() noexcept override;

  RegularGridSampleSurfaceMesh(const RegularGridSampleSurfaceMesh&) = delete;
  RegularGridSampleSurfaceMesh(RegularGridSampleSurfaceMesh&&) noexcept = delete;
  RegularGridSampleSurfaceMesh& operator=(const RegularGridSampleSurfaceMesh&) = delete;
  RegularGridSampleSurfaceMesh& operator=(RegularGridSampleSurfaceMesh&&) noexcept = delete;

  Result<> operator()();

  void sendThreadSafeUpdate(const std::vector<int32>& rasterBuffer, usize offset);

protected:
  void generatePoints(std::vector<Point3Df>& points) override;
  void processSlice(int32 m_CurrentSliceId, usize m_ImageGeomIdx, const std::set<int32>& uniquePartIds);

private:
  DataStructure& m_DataStructure;
  const RegularGridSampleSurfaceMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Thread safe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  usize m_LayerCompleted = 0;
  usize m_TotalSlices = 0;
  // usize m_LastProgressInt = 0;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
  Int32Array* m_FeatureIdsDataStorePtr = nullptr;
};
} // namespace nx::core
