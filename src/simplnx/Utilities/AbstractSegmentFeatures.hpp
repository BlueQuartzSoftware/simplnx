#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/DataStructure/AbstractDataArray.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/ISegmentFeatures.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <random>
#include <vector>

namespace nx::core
{

class AbstractGridGeometry;

namespace segment_features
{
inline constexpr StringLiteral k_6NeighborString = "Face Neighbors";
inline constexpr StringLiteral k_26NeighborString = "All Connected Neighbors";

inline const ChoicesParameter::Choices k_OperationChoices = {k_6NeighborString, k_26NeighborString};

inline constexpr ChoicesParameter::ValueType k_6NeighborIndex = 0ULL;
inline constexpr ChoicesParameter::ValueType k_26NeighborIndex = 1ULL;
} // namespace segment_features

class SIMPLNX_EXPORT AbstractSegmentFeatures : public ISegmentFeatures
{

public:
  AbstractSegmentFeatures(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);

  ~AbstractSegmentFeatures() override;

  AbstractSegmentFeatures(const AbstractSegmentFeatures&) = delete;            // Copy Constructor Not Implemented
  AbstractSegmentFeatures(AbstractSegmentFeatures&&) = delete;                 // Move Constructor Not Implemented
  AbstractSegmentFeatures& operator=(const AbstractSegmentFeatures&) = delete; // Copy Assignment Not Implemented
  AbstractSegmentFeatures& operator=(AbstractSegmentFeatures&&) = delete;      // Move Assignment Not Implemented

  /**
   * @brief execute
   * @param gridGeom
   * @return
   */
  Result<> execute(AbstractGridGeometry* gridGeom) override;

  /**
   * @brief Returns the seed for the specified values.
   * @param data
   * @param args
   * @param gnum
   * @param nextSeed
   * @return int64
   */
  int64_t getSeed(int32_t gnum, int64 nextSeed) const override;

  /**
   * @brief Determines the grouping for the specified values.
   * @param data
   * @param args
   * @param referencePoint
   * @param neighborPoint
   * @param gnum
   * @return bool
   */
  bool determineGrouping(int64_t referencePoint, int64_t neighborPoint, int32_t gnum) const override;

  /**
   * @brief
   * @param featureIds
   * @param totalFeatures
   * @param distribution
   */
  void randomizeFeatureIds(Int32Array* featureIds, uint64 totalFeatures) override;

  /**
   * @brief
   * @return
   */
  SeedGenerator initializeStaticVoxelSeedGenerator() const override;

protected:
  DataStructure& m_DataStructure;
  bool m_IsPeriodic = false;
  const std::atomic_bool& m_ShouldCancel;
  MessageHelper m_MessageHelper;
  int32 m_FoundFeatures = 0;
  NeighborScheme m_NeighborScheme = NeighborScheme::Face;
};

} // namespace nx::core
