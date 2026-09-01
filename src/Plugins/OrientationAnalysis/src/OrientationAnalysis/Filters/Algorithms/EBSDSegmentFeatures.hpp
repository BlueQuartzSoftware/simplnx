#pragma once

#include <array>

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/SegmentFeatures.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <vector>

namespace nx::core
{

/**
 * @struct EBSDSegmentFeaturesInputValues
 * @brief Identifies EBSD segmentation inputs.
 *
 * MisorientationTolerance is in radians. ActiveArrayPath reserves tuple zero
 * for the background feature.
 */
struct ORIENTATIONANALYSIS_EXPORT EBSDSegmentFeaturesInputValues
{
  float32 MisorientationTolerance = 0.0f;
  bool UseMask = false;
  bool RandomizeFeatureIds = false;
  SegmentFeatures::NeighborScheme NeighborScheme{};
  DataPath ImageGeometryPath;
  DataPath QuatsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath MaskArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellFeatureAttributeMatrixPath;
  DataPath ActiveArrayPath;
  bool IsPeriodic = false;
};

/**
 * @class EBSDSegmentFeatures
 * @brief Segments EBSD cells by crystallographic misorientation.
 *
 * Same-phase cells join when their symmetry-reduced misorientation is below the
 * tolerance. SegmentFeatures processes one slice at a time. Two local slices
 * and a local crystal-structure cache prevent per-voxel OOC access.
 */
class ORIENTATIONANALYSIS_EXPORT EBSDSegmentFeatures : public SegmentFeatures
{
public:
  /**
   * @brief Defines the feature ID array type.
   */
  using FeatureIdsArrayType = Int32Array;

  /**
   * @brief Initializes EBSD segmentation.
   * @param dataStructure Provides selected arrays and the geometry.
   * @param mesgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies segmentation settings.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  EBSDSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EBSDSegmentFeaturesInputValues* inputValues);
  /**
   * @brief Destroys the EBSD segmentation executor.
   */
  ~EBSDSegmentFeatures() noexcept override;

  EBSDSegmentFeatures(const EBSDSegmentFeatures&) = delete;
  EBSDSegmentFeatures(EBSDSegmentFeatures&&) = delete;
  EBSDSegmentFeatures& operator=(const EBSDSegmentFeatures&) = delete;
  EBSDSegmentFeatures& operator=(EBSDSegmentFeatures&&) = delete;

  /**
   * @brief Executes EBSD connected-component labeling.
   * @return Success, or a mask or bulk-I/O error.
   */
  Result<> operator()();

protected:
  /**
   * @brief Checks whether a voxel can seed or join a feature.
   * @param point Identifies the voxel.
   * @return True if the mask permits the voxel and its phase is positive.
   */
  bool isValidVoxel(int64 point) const override;

  /**
   * @brief Checks misorientation for neighboring voxels.
   * @param point1 Identifies the source voxel.
   * @param point2 Identifies the neighbor voxel.
   * @return True if the cells share a phase and meet the tolerance.
   */
  bool areNeighborsSimilar(int64 point1, int64 point2) const override;

  /**
   * @brief Loads a Z slice into a two-slot LRU buffer.
   * @param iz Identifies the slice, or a negative value to disable buffering.
   * @param dimX Specifies the X dimension.
   * @param dimY Specifies the Y dimension.
   * @param dimZ Specifies the Z dimension.
   * @return Success, or a source bulk-I/O error.
   */
  Result<> prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ) override;

private:
  const EBSDSegmentFeaturesInputValues* m_InputValues = nullptr;
  Float32Array* m_QuatsArray = nullptr;
  FeatureIdsArrayType* m_CellPhases = nullptr;
  std::unique_ptr<MaskCompareUtilities::MaskCompare> m_GoodVoxelsArray = nullptr;
  DataArray<uint32>* m_CrystalStructures = nullptr;

  FeatureIdsArrayType* m_FeatureIdsArray = nullptr;

  std::vector<ebsdlib::LaueOps::Pointer> m_OrientationOps;

  /**
   * @brief Allocates two local X-Y slice buffers.
   * @param dimX Specifies the X dimension.
   * @param dimY Specifies the Y dimension.
   * @return Success, or a crystal-structure bulk-I/O error.
   */
  Result<> allocateSliceBuffers(int64 dimX, int64 dimY);

  /**
   * @brief Releases local slice buffers.
   */
  void deallocateSliceBuffers();

  std::vector<float32> m_QuatBuffer;
  std::vector<int32> m_PhaseBuffer;
  std::vector<uint8> m_MaskBuffer;
  std::vector<uint32> m_CrystalStructuresCache;
  int64 m_BufSliceSize = 0;
  std::array<int64, 2> m_BufferedSliceZ = {-1, -1};
  std::array<uint64, 2> m_BufferUseSequence = {0, 0};
  uint64 m_NextBufferUseSequence = 1;
  bool m_UseSliceBuffers = false;
};

} // namespace nx::core
