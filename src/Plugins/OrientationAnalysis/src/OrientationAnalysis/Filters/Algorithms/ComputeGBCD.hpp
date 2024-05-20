#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SizeGBCD
{
  SizeGBCD(usize faceChunkSize, usize numMisoReps, float32 gbcdRes);

  void initializeBinsWithValue(int32 value);

  std::vector<float32> m_GbcdDeltas;
  std::vector<float32> m_GbcdLimits;
  std::vector<int32> m_GbcdSizes;
  std::vector<int32> m_GbcdBins;
  std::vector<bool> m_GbcdHemiCheck;

  usize m_FaceChunkSize;
  usize m_NumMisoReps;
};

struct ORIENTATIONANALYSIS_EXPORT ComputeGBCDInputValues
{
  float32 GBCDRes;
  DataPath TriangleGeometryPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath SurfaceMeshFaceNormalsArrayPath;
  DataPath SurfaceMeshFaceAreasArrayPath;
  DataPath FeatureEulerAnglesArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath FaceEnsembleAttributeMatrixName;
  DataPath GBCDArrayName;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeGBCD
{
public:
  ComputeGBCD(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeGBCDInputValues* inputValues);
  ~ComputeGBCD() noexcept;

  ComputeGBCD(const ComputeGBCD&) = delete;
  ComputeGBCD(ComputeGBCD&&) noexcept = delete;
  ComputeGBCD& operator=(const ComputeGBCD&) = delete;
  ComputeGBCD& operator=(ComputeGBCD&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeGBCDInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
