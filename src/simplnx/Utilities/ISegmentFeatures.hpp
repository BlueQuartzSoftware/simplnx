#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

#include <random>

namespace nx::core
{

class AbstractGridGeometry;

/**
 * @class ISegmentFeatures
 * @brief Pure interface for segment features algorithms. Defines the contract
 * that all segment features implementations must satisfy.
 */
class SIMPLNX_EXPORT ISegmentFeatures
{
public:
  using SeedGenerator = std::mt19937_64;

  enum class NeighborScheme : ChoicesParameter::ValueType
  {
    Face = 0,
    FaceEdgeVertex = 1
  };

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

  virtual ~ISegmentFeatures() = default;

  ISegmentFeatures(const ISegmentFeatures&) = delete;
  ISegmentFeatures(ISegmentFeatures&&) = delete;
  ISegmentFeatures& operator=(const ISegmentFeatures&) = delete;
  ISegmentFeatures& operator=(ISegmentFeatures&&) = delete;

  /**
   * @brief execute
   * @param gridGeom
   * @return
   */
  virtual Result<> execute(AbstractGridGeometry* gridGeom) = 0;

  /**
   * @brief Returns the seed for the specified values.
   * @param gnum
   * @param nextSeed
   * @return int64
   */
  virtual int64_t getSeed(int32_t gnum, int64 nextSeed) const = 0;

  /**
   * @brief Determines the grouping for the specified values.
   * @param referencePoint
   * @param neighborPoint
   * @param gnum
   * @return bool
   */
  virtual bool determineGrouping(int64_t referencePoint, int64_t neighborPoint, int32_t gnum) const = 0;

  /**
   * @brief
   * @param featureIds
   * @param totalFeatures
   */
  virtual void randomizeFeatureIds(Int32Array* featureIds, uint64 totalFeatures) = 0;

  /**
   * @brief
   * @return
   */
  virtual SeedGenerator initializeStaticVoxelSeedGenerator() const = 0;

protected:
  ISegmentFeatures() = default;
};

} // namespace nx::core
