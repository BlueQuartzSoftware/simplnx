#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
struct SIMPLNX_EXPORT SampleSurfaceMeshInputValues
{
  DataPath TriangleGeometryPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath FeatureIdsArrayPath; // Make sure it's been initialized with zeroes
};

class SIMPLNX_EXPORT SampleSurfaceMesh
{
public:
  SampleSurfaceMesh(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  virtual ~SampleSurfaceMesh() noexcept;

  SampleSurfaceMesh(const SampleSurfaceMesh&) = delete;            // Copy Constructor Not Implemented
  SampleSurfaceMesh(SampleSurfaceMesh&&) = delete;                 // Move Constructor Not Implemented
  SampleSurfaceMesh& operator=(const SampleSurfaceMesh&) = delete; // Copy Assignment Not Implemented
  SampleSurfaceMesh& operator=(SampleSurfaceMesh&&) = delete;      // Move Assignment Not Implemented

  /**
   * @brief execute
   * @param gridGeom
   * @return
   */
  Result<> execute(SampleSurfaceMeshInputValues& inputValues);

  void updateProgress(const std::string& progMessage);
  void sendThreadSafeProgressMessage(usize featureId, size_t numCompleted, size_t totalFeatures);

protected:
  virtual void generatePoints(std::vector<Point3Df>& points) = 0;

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Thread safe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  usize m_ProgressCounter = 0;
  usize m_LastProgressInt = 0;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
};
} // namespace nx::core
