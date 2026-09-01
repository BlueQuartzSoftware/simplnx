#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <vector>

namespace nx::core
{

class IGridGeometry;
template <typename T>
class AbstractDataStore;

/**
 * @namespace segment_features
 * @brief Defines neighbor-scheme parameter labels and indexes.
 */
namespace segment_features
{
inline constexpr StringLiteral k_6NeighborString = "Face Neighbors";
inline constexpr StringLiteral k_26NeighborString = "All Connected Neighbors";

inline const ChoicesParameter::Choices k_OperationChoices = {k_6NeighborString, k_26NeighborString};

inline constexpr ChoicesParameter::ValueType k_6NeighborIndex = 0ULL;
inline constexpr ChoicesParameter::ValueType k_26NeighborIndex = 1ULL;
} // namespace segment_features

/**
 * @class SegmentFeatures
 * @brief Base class for grid segmentation algorithms that share a scanline
 * connected-component-labeling engine.
 *
 * Subclasses provide voxel validity, neighbor similarity, and optional slice
 * preloading. The forward scan keeps two label slices in RAM. OOC input stores
 * equivalence and final-label state in temporary record stores.
 */
class SIMPLNX_EXPORT SegmentFeatures
{

public:
  /**
   * @brief Creates a shared connected-component-labeling engine.
   * @param dataStructure Provides subclass input and output arrays.
   * @param shouldCancel Stops before later slices or resolution chunks when true.
   * @param mesgHandler Receives phase and periodic-boundary messages.
   */
  SegmentFeatures(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);

  /**
   * @brief Destroys the non-owning segmentation engine.
   */
  virtual ~SegmentFeatures();

  SegmentFeatures(const SegmentFeatures&) = delete;
  SegmentFeatures(SegmentFeatures&&) = delete;
  SegmentFeatures& operator=(const SegmentFeatures&) = delete;
  SegmentFeatures& operator=(SegmentFeatures&&) = delete;

  /**
   * @enum NeighborScheme
   * @brief Selects face-only or complete 26-neighbor connectivity.
   */
  enum class NeighborScheme : ChoicesParameter::ValueType
  {
    Face = 0,          ///< Uses six face neighbors.
    FaceEdgeVertex = 1 ///< Uses all face, edge, and vertex neighbors.
  };

  /**
   * @brief Segments the grid into features using connected-component labeling.
   *
   * Voxels use Z-Y-X order and one rolling label-slice pair. External scratch
   * stores worst-case equivalence and final-label state for genuine OOC input.
   *
   * @param gridGeom Provides dimensions and coordinate topology.
   * @param featureIdsStore Receives provisional and final Feature IDs.
   * @param usesOutOfCoreInput Requires external scratch when true. False permits
   * resident fallback for forced-path tests.
   * @return Scratch, subclass, or Feature-ID I/O error, or success after cancellation.
   *
   * Cancellation can retain provisional or partially resolved Feature IDs.
   */
  Result<> executeCCL(IGridGeometry* gridGeom, AbstractDataStore<int32>& featureIdsStore, bool usesOutOfCoreInput = false);

  /**
   * @brief Applies a random permutation to positive feature IDs after segmentation.
   * @param featureIds Provides and receives output labels. Feature ID 0 remains background.
   * @param totalFeatures Specifies generated positive features.
   *
   * This method does not check cancellation or return DataStore errors.
   */
  void randomizeFeatureIds(Int32Array* featureIds, uint64 totalFeatures);

  /**
   * @class CompareFunctor
   * @brief Defines a type-independent neighbor-comparison interface.
   */
  class CompareFunctor
  {
  public:
    /**
     * @brief Destroys the comparison interface.
     */
    virtual ~CompareFunctor() = default;

    /**
     * @brief Tests whether two voxels belong to the same feature.
     * @param index Specifies the first voxel.
     * @param neighIndex Specifies its neighbor.
     * @return False by default.
     */
    virtual bool compare(int64 index, int64 neighIndex)
    {
      return false;
    }
  };

  /**
   * @brief Tests whether one voxel can belong to a feature.
   * @param point Specifies the flat voxel index.
   * @return True by default.
   */
  virtual bool isValidVoxel(int64 point) const;

  /**
   * @brief Tests whether two adjacent voxels belong to the same feature.
   * @param point1 Specifies the first flat voxel index.
   * @param point2 Specifies the second flat voxel index.
   * @return False by default.
   */
  virtual bool areNeighborsSimilar(int64 point1, int64 point2) const;

  /**
   * @brief Prepares subclass data for one Z slice.
   *
   * Subclasses can load bounded input buffers before neighbor comparisons. The
   * default implementation performs no work.
   * @param iz Specifies the current Z index.
   * @param dimX Specifies X cells.
   * @param dimY Specifies Y cells.
   * @param dimZ Specifies Z cells.
   * @return Subclass preparation error, or success.
   */
  virtual Result<> prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ);

protected:
  DataStructure& m_DataStructure;
  bool m_IsPeriodic = false;
  const std::atomic_bool& m_ShouldCancel;
  MessageHelper m_MessageHelper;
  int32 m_FoundFeatures = 0;
  NeighborScheme m_NeighborScheme = NeighborScheme::Face;
};

} // namespace nx::core
