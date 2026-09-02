#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include "simplnx/simplnx_export.hpp"
#include <mutex>

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

  virtual void generatePoints(std::vector<Point3Df>& points) = 0;

  /**
   * @brief Returns the message handler so the type-dispatch functor can send status text.
   * @return
   */
  const IFilter::MessageHandler& getMessageHandler() const;

  /**
   * @brief Sets the progress denominator and label for the sampling pass.
   * @param maxProgress Total points across every feature
   * @param label Describes the work being done
   */
  void resetProgress(usize maxProgress, std::string label);

  /**
   * @brief Thread-safe progress update. Safe to call from the per-feature workers.
   * @param counter Points completed since the previous call
   */
  void sendThreadSafeProgressMessage(usize counter);

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  mutable std::mutex m_ProgressMessage_Mutex;
  ThrottledMessageHandler m_Throttle;
};
} // namespace nx::core
