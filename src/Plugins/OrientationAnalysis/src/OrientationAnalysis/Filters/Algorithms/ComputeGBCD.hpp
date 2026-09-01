#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct SizeGBCD
 * @brief Stores GBCD bin dimensions and triangle scratch data.
 *
 * Scratch storage scales with one triangle chunk and its symmetry
 * representations.
 */
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
 * @struct ComputeGBCDInputValues
 * @brief Identifies GBCD inputs.
 *
 * GBCDRes is in degrees.
 */
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
 * @class ComputeGBCD
 * @brief Computes a five-dimensional grain boundary distribution.
 *
 * Triangle areas accumulate by misorientation and boundary-normal bins. The
 * final histogram uses multiples of random distribution. Feature and ensemble
 * data stays local. Triangle data uses 50,000-face chunks so the worker uses
 * local raw buffers instead of OOC element access.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeGBCD
{
public:
  /**
   * @brief Initializes GBCD computation.
   * @param dataStructure Provides selected arrays.
   * @param mesgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays and GBCD resolution.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeGBCD(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeGBCDInputValues* inputValues);
  /**
   * @brief Destroys the GBCD executor.
   */
  ~ComputeGBCD() noexcept;

  ComputeGBCD(const ComputeGBCD&) = delete;
  ComputeGBCD(ComputeGBCD&&) noexcept = delete;
  ComputeGBCD& operator=(const ComputeGBCD&) = delete;
  ComputeGBCD& operator=(ComputeGBCD&&) noexcept = delete;

  /**
   * @brief Computes the GBCD histogram.
   * @return Success.
   *
   * Cancellation returns success with completed chunks preserved. Current bulk-
   * I/O Result values are not inspected.
   */
  Result<> operator()();

  /**
   * @brief Returns the retained cancellation flag.
   * @return Reference to the cancellation flag supplied at construction.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeGBCDInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
