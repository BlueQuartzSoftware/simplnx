#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT SurfaceNetsInputValues
{
  bool ApplySmoothing;
  bool RepairTriangleWinding;
  int32 SmoothingIterations;
  float32 MaxDistanceFromVoxel;
  float32 RelaxationFactor;

  DataPath GridGeomDataPath;
  DataPath FeatureIdsArrayPath;
  MultiArraySelectionParameter::ValueType SelectedCellDataArrayPaths;
  MultiArraySelectionParameter::ValueType SelectedFeatureDataArrayPaths;
  DataPath TriangleGeometryPath;
  DataPath VertexGroupDataPath;
  DataPath NodeTypesDataPath;
  DataPath FaceGroupDataPath;
  DataPath FaceLabelsDataPath;
  MultiArraySelectionParameter::ValueType CreatedDataArrayPaths;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT SurfaceNetsDirect
{
public:
  SurfaceNetsDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SurfaceNetsInputValues* inputValues);
  ~SurfaceNetsDirect() noexcept;

  SurfaceNetsDirect(const SurfaceNetsDirect&) = delete;
  SurfaceNetsDirect(SurfaceNetsDirect&&) noexcept = delete;
  SurfaceNetsDirect& operator=(const SurfaceNetsDirect&) = delete;
  SurfaceNetsDirect& operator=(SurfaceNetsDirect&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SurfaceNetsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
