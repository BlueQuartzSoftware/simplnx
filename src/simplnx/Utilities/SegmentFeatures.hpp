#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <random>
#include <vector>

namespace nx::core
{

class IGridGeometry;
template <typename T>
class AbstractDataStore;

namespace segment_features
{
inline constexpr StringLiteral k_6NeighborString = "Face Neighbors";
inline constexpr StringLiteral k_26NeighborString = "All Connected Neighbors";

inline const ChoicesParameter::Choices k_OperationChoices = {k_6NeighborString, k_26NeighborString};

inline constexpr ChoicesParameter::ValueType k_6NeighborIndex = 0ULL;
inline constexpr ChoicesParameter::ValueType k_26NeighborIndex = 1ULL;
} // namespace segment_features

class SIMPLNX_EXPORT SegmentFeatures
{

public:
  using SeedGenerator = std::mt19937_64;

  SegmentFeatures(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);

  virtual ~SegmentFeatures();

  SegmentFeatures(const SegmentFeatures&) = delete;            // Copy Constructor Not Implemented
  SegmentFeatures(SegmentFeatures&&) = delete;                 // Move Constructor Not Implemented
  SegmentFeatures& operator=(const SegmentFeatures&) = delete; // Copy Assignment Not Implemented
  SegmentFeatures& operator=(SegmentFeatures&&) = delete;      // Move Assignment Not Implemented

  enum class NeighborScheme : ChoicesParameter::ValueType
  {
    Face = 0,
    FaceEdgeVertex = 1
  };

  /**
   * @brief Original DFS-based segmentation (in-core optimized).
   * @param gridGeom
   * @return
   */
  Result<> execute(IGridGeometry* gridGeom);

  /**
   * @brief Chunk-sequential CCL-based segmentation optimized for out-of-core.
   * Subclasses must override isValidVoxel() and areNeighborsSimilar() to use this code path.
   * @param gridGeom
   * @param featureIdsStore
   * @return
   */
  Result<> executeCCL(IGridGeometry* gridGeom, AbstractDataStore<int32>& featureIdsStore);

  /**
   * @brief Returns the seed for the specified values.
   * @param gnum
   * @param nextSeed
   * @return int64
   */
  virtual int64_t getSeed(int32_t gnum, int64 nextSeed) const;

  /**
   * @brief Determines the grouping for the specified values.
   * @param referencePoint
   * @param neighborPoint
   * @param gnum
   * @return bool
   */
  virtual bool determineGrouping(int64_t referencePoint, int64_t neighborPoint, int32_t gnum) const;

  /**
   * @brief
   * @param featureIds
   * @param totalFeatures
   */
  void randomizeFeatureIds(Int32Array* featureIds, uint64 totalFeatures);

  /**
   * @brief
   * @return
   */
  virtual SeedGenerator initializeStaticVoxelSeedGenerator() const;

  /* from http://www.newty.de/fpt/functor.html */
  /**
   * @brief The CompareFunctor class serves as a functor superclass for specific implementations
   * of performing scalar comparisons
   */
  class CompareFunctor
  {
  public:
    virtual ~CompareFunctor() = default;

    virtual bool operator()(int64 index, int64 neighIndex, int32 gnum) // call using () operator
    {
      return false;
    }

    /**
     * @brief Pure data comparison without featureId assignment.
     * Used by the CCL algorithm which handles label assignment separately.
     * @param index First voxel index
     * @param neighIndex Second voxel index
     * @return true if the two voxels should be in the same feature
     */
    virtual bool compare(int64 index, int64 neighIndex)
    {
      return false;
    }
  };

  /**
   * @brief Can this voxel be a feature member? (mask + phase check, NO featureId check)
   * Default returns true (all voxels are valid).
   * @param point Linear voxel index
   * @return true if this voxel can participate in segmentation
   */
  virtual bool isValidVoxel(int64 point) const;

  /**
   * @brief Should these two adjacent voxels be in the same feature? (data comparison only)
   * Default returns false (no voxels are similar).
   * @param point1 First voxel index
   * @param point2 Second voxel index
   * @return true if the two voxels should be grouped together
   */
  virtual bool areNeighborsSimilar(int64 point1, int64 point2) const;

protected:
  DataStructure& m_DataStructure;
  bool m_IsPeriodic = false;
  const std::atomic_bool& m_ShouldCancel;
  MessageHelper m_MessageHelper;
  int32 m_FoundFeatures = 0;
  NeighborScheme m_NeighborScheme = NeighborScheme::Face;
};

} // namespace nx::core
