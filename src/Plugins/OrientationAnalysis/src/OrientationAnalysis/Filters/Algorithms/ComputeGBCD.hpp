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

/**
 * @brief Input values for the ComputeGBCD algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeGBCDInputValues
{
  float32 GBCDRes;                          ///< GBCD resolution in degrees
  DataPath TriangleGeometryPath;            ///< TriangleGeom containing the surface mesh
  DataPath SurfaceMeshFaceLabelsArrayPath;  ///< Face-level Int32 labels (2 components: feature1, feature2)
  DataPath SurfaceMeshFaceNormalsArrayPath; ///< Face-level Float64 normals (3 components)
  DataPath SurfaceMeshFaceAreasArrayPath;   ///< Face-level Float64 areas
  DataPath FeatureEulerAnglesArrayPath;     ///< Feature-level Float32 Euler angles (3 components)
  DataPath FeaturePhasesArrayPath;          ///< Feature-level Int32 phase index per feature
  DataPath CrystalStructuresArrayPath;      ///< Ensemble-level UInt32 crystal structure Laue classes
  DataPath FaceEnsembleAttributeMatrixName; ///< Ensemble-level AttributeMatrix for output
  DataPath GBCDArrayName;                   ///< Output: Ensemble-level Float64 GBCD histogram
};

/**
 * @class ComputeGBCD
 * @brief Computes the five-dimensional Grain Boundary Character Distribution
 *        (GBCD) for a triangle surface mesh.
 *
 * The GBCD represents the relative area of grain boundary for a given
 * misorientation and boundary normal. Triangles are processed in chunks,
 * and for each triangle the misorientation bin and boundary normal bin are
 * determined. The triangle's area is accumulated into the appropriate GBCD
 * bin. The final distribution is normalized to multiples of random distribution
 * (MRD).
 *
 * ## OOC Optimization
 *
 * Several levels of caching eliminate per-element OOC overhead:
 *   - Feature-level arrays (Euler angles, phases) are cached entirely in
 *     local vectors via `copyIntoBuffer()` -- these are small (O(features)).
 *   - Ensemble-level crystal structures are cached locally (tiny).
 *   - Triangle-level arrays (labels, normals, areas) are read in chunks of
 *     50000 triangles via `copyIntoBuffer()` before each parallel pass.
 *   - The GBCD output histogram is accumulated in a local vector and written
 *     back via `copyFromBuffer()` after normalization.
 *   - The parallel `CalculateGBCDImpl` worker receives raw pointers into
 *     the local buffers, achieving zero virtual dispatch in the hot loop.
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

  /**
   * @brief Executes the GBCD computation with locally cached data and chunked I/O.
   * @return Result<> with any errors encountered during execution.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeGBCDInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
