#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT PointSampleEdgeGeometryInputValues
{
  float32 ScanVectorSamplingRes;
  DataPath ScanVectorGeometryPath;
  DataPath SampledVertexGeometryPath;
  bool CalculateCumulativeSampleDistance;
  std::string CumulativeSampleDistanceArrayName;
  std::string EdgeIdsArrayName;
  DataPath pVertexGroupDataPath;
  MultiArraySelectionParameter::ValueType pSelectedDataArrayPaths;
  MultiArraySelectionParameter::ValueType pCreatedDataArrayPaths;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT PointSampleEdgeGeometry
{
public:
  PointSampleEdgeGeometry(DataStructure& dataStructure, PointSampleEdgeGeometryInputValues* inputValues, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel);
  ~PointSampleEdgeGeometry() noexcept;

  PointSampleEdgeGeometry(const PointSampleEdgeGeometry&) = delete;
  PointSampleEdgeGeometry(PointSampleEdgeGeometry&&) noexcept = delete;
  PointSampleEdgeGeometry& operator=(const PointSampleEdgeGeometry&) = delete;
  PointSampleEdgeGeometry& operator=(PointSampleEdgeGeometry&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const PointSampleEdgeGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
