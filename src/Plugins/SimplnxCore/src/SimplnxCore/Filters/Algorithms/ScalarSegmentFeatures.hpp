#pragma once

#include <array>

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/SegmentFeatures.hpp"

#include <random>
#include <vector>

namespace nx::core
{

/**
 * @struct ScalarSegmentFeaturesInputValues
 * @brief Stores tolerance, connectivity, mask, periodic, and output selections.
 */
struct SIMPLNXCORE_EXPORT ScalarSegmentFeaturesInputValues
{
  int ScalarTolerance = 0;
  bool UseMask;
  bool RandomizeFeatureIds;
  bool IsPeriodic = false;
  SegmentFeatures::NeighborScheme NeighborScheme;
  DataPath ImageGeometryPath;
  DataPath InputDataPath;
  DataPath MaskArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellFeatureAttributeMatrixPath;
  DataPath ActiveArrayPath;
};

/**
 * @class ScalarSegmentFeatures
 * @brief Labels connected grid cells whose scalar difference is within a tolerance.
 *
 * Face or 26-neighbor connectivity can wrap at periodic boundaries. A selected
 * Bool or UInt8 mask excludes cells from labels. The connected-component
 * labeling (CCL) engine writes provisional Feature IDs before it resolves dense
 * final IDs. Cancellation or an error can therefore leave provisional or partly
 * resolved output. Feature data is resized only after CCL completes.
 *
 * Two LRU slots retain scalar and mask Z slices. Scalar values widen to float64
 * so one comparison path serves all input types. Distinct int64 or uint64 values
 * above exact float64 precision can compare as equal. Buffered Boolean values use
 * numeric difference, so a tolerance of one or more joins false and true. The
 * direct fallback instead compares Boolean values for exact equality. A
 * negative tolerance prevents every finite buffered neighbor match.
 *
 * Input slice buffers use 18 bytes per XY cell. The base engine also keeps label
 * slices. Resident equivalence and final-label tables can grow with provisional
 * feature count. A genuine out-of-core participating store moves that
 * worst-case state to temporary records behind bounded page caches. Storage
 * overrides can force either policy.
 *
 * Optional ID randomization uses bounded Feature ID transfers after all feature
 * output is initialized. It has no cancellation check and discards bulk-I/O
 * results. The permutation improves visual contrast but does not change feature
 * membership.
 */
class SIMPLNXCORE_EXPORT ScalarSegmentFeatures : public SegmentFeatures
{
public:
  /**
   * @brief Defines the per-cell Feature ID array type.
   */
  using FeatureIdsArrayType = Int32Array;
  /**
   * @brief Defines the Boolean mask array type.
   */
  using GoodVoxelsArrayType = BoolArray;

  /**
   * @brief Initializes the scalar-segmentation algorithm.
   * @param dataStructure Contains scalar, mask, geometry, and output data.
   * @param inputValues Selects segmentation and output behavior.
   * @param shouldCancel Signals cancellation between CCL slices and phases.
   * @param mesgHandler Receives connected-component progress messages.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ScalarSegmentFeatures(DataStructure& dataStructure, ScalarSegmentFeaturesInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  /**
   * @brief Destroys the scalar-segmentation algorithm.
   */
  ~ScalarSegmentFeatures() noexcept override;

  ScalarSegmentFeatures(const ScalarSegmentFeatures&) = delete;
  ScalarSegmentFeatures(ScalarSegmentFeatures&&) noexcept = delete;
  ScalarSegmentFeatures& operator=(const ScalarSegmentFeatures&) = delete;
  ScalarSegmentFeatures& operator=(ScalarSegmentFeatures&&) noexcept = delete;

  /**
   * @brief Labels features and initializes feature-level output.
   * @return Mask, scalar-I/O, CCL storage, or no-feature result.
   * @pre InputDataPath is a scalar array that matches the grid cell dimensions.
   * @pre MaskArrayPath is scalar Bool or UInt8 when UseMask is true.
   *
   * Cancellation returns success without rollback. After CCL, the method resizes
   * feature output, marks positive features active, and reserves feature zero.
   * These postprocessing steps do not inspect cancellation.
   */
  Result<> operator()();

protected:
  /**
   * @brief Tests whether one cell can receive a feature label.
   * @param point Flat cell index.
   * @return True when the mask permits the cell.
   *
   * Active buffering treats a request for an unloaded slice as invalid instead
   * of reading the store. Inactive buffering uses the mask comparator directly.
   */
  bool isValidVoxel(int64 point) const override;

  /**
   * @brief Tests whether two cells can share one feature.
   * @param point1 Flat index of the labeled cell.
   * @param point2 Flat index of its neighbor.
   * @return True when point2 is valid and the scalar difference is within tolerance.
   *
   * Active buffering compares widened float64 values and rejects unloaded
   * slices. Inactive buffering uses the native typed comparator. Signed native
   * subtraction requires a representable difference.
   */
  bool areNeighborsSimilar(int64 point1, int64 point2) const override;

  /**
   * @brief Loads one scalar and mask Z slice into two-slot LRU buffers.
   * @param iz Z-slice index, or a negative value to disable buffering.
   * @param dimX Grid X dimension.
   * @param dimY Grid Y dimension.
   * @param dimZ Grid Z dimension.
   * @return Scalar or mask bulk-read result.
   * @pre A nonnegative iz is less than dimZ.
   *
   * LRU replacement retains ordinary adjacent slices and explicit periodic
   * boundary pairs without rereading a resident slice.
   */
  Result<> prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ) override;

private:
  /**
   * @brief Allocates two scalar and mask XY-slice slots.
   * @param dimX Grid X dimension.
   * @param dimY Grid Y dimension.
   * @pre dimX times dimY fits int64 and usize.
   */
  void allocateSliceBuffers(int64 dimX, int64 dimY);

  /**
   * @brief Releases the slice buffers and resets buffering state.
   */
  void deallocateSliceBuffers();

  const ScalarSegmentFeaturesInputValues* m_InputValues = nullptr;
  FeatureIdsArrayType* m_FeatureIdsArray = nullptr;
  GoodVoxelsArrayType* m_GoodVoxelsArray = nullptr;
  std::shared_ptr<SegmentFeatures::CompareFunctor> m_CompareFunctor;
  std::unique_ptr<MaskCompareUtilities::MaskCompare> m_GoodVoxels = nullptr;
  IDataArray* m_InputDataArray = nullptr;

  std::vector<float64> m_ScalarBuffer;
  std::vector<uint8> m_MaskBuffer;
  int64 m_BufSliceSize = 0;
  std::array<int64, 2> m_BufferedSliceZ = {-1, -1};
  std::array<uint64, 2> m_BufferUseSequence = {0, 0};
  uint64 m_NextBufferUseSequence = 1;
  bool m_UseSliceBuffers = false;
};
} // namespace nx::core
