#pragma once

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"
#include "complex/Parameters/VectorParameter.hpp"

namespace complex
{
struct COMPLEX_EXPORT SampleSurfaceMeshInputValues
{
  DataPath TriangleGeometryPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  VectorUInt64Parameter::ValueType Dimensions;
  VectorFloat32Parameter::ValueType Spacing;
  VectorFloat32Parameter::ValueType Origin;
  VectorFloat32Parameter::ValueType Uncertainty;
  DataPath DataContainerName;
  DataPath CellAttributeMatrixName;
  DataPath FeatureIdsArrayName;
};

class COMPLEX_EXPORT SampleSurfaceMesh
{
public:
  SampleSurfaceMesh(DataStructure& data, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
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
  Result<> execute();

  void updateProgress(const std::string& progMessage);
  void sendThreadSafeProgressMessage(usize featureId, size_t numCompleted, size_t totalFeatures);

protected:
  virtual void generatePoints(VertexGeom& vertexGeom) = 0;

private:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Thread safe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  usize m_TotalElements = 0;
  usize m_ProgressCounter = 0;
  usize m_LastProgressInt = 0;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
};
} // namespace complex
