#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <random>
#include <vector>

namespace nx::core
{

class IGridGeometry;

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
   * @brief execute
   * @param gridGeom
   * @return
   */
  Result<> execute(IGridGeometry* gridGeom);

  /**
   * @brief Returns the seed for the specified values.
   * @param data
   * @param args
   * @param gnum
   * @param nextSeed
   * @return int64
   */
  virtual int64_t getSeed(int32_t gnum, int64 nextSeed) const;

  /**
   * @brief Determines the grouping for the specified values.
   * @param data
   * @param args
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
   * @param distribution
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
  };

protected:
  DataStructure& m_DataStructure;
  bool m_IsPeriodic = false;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  int32 m_FoundFeatures = 0;
  NeighborScheme m_NeighborScheme = NeighborScheme::Face;
};

} // namespace nx::core
