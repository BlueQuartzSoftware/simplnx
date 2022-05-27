#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Utilities/SegmentFeatures.hpp"

#include <random>
#include <vector>

namespace complex
{

struct COMPLEXCORE_EXPORT ScalarSegmentFeaturesInputValues
{
  DataPath pInputDataPath;
  int pScalarTolerance = 0;
  bool pShouldRandomizeFeatureIds = false;
  DataPath pActiveArrayPath;
  DataPath pFeatureIdsPath;
  bool pUseGoodVoxels = false;
  DataPath pGoodVoxelsPath;
  DataPath pGridGeomPath;
};

/**
 * @brief The ScalarSegmentFeatures class
 */
class COMPLEXCORE_EXPORT ScalarSegmentFeatures : public SegmentFeatures
{
public:
  using SeedGenerator = std::mt19937_64;
  using Int64Distribution = std::uniform_int_distribution<int64>;
  using FeatureIdsArrayType = Int32Array;
  using GoodVoxelsArrayType = BoolArray;

  ScalarSegmentFeatures(DataStructure& data, ScalarSegmentFeaturesInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~ScalarSegmentFeatures() noexcept;

  ScalarSegmentFeatures(const ScalarSegmentFeatures&) = delete;
  ScalarSegmentFeatures(ScalarSegmentFeatures&&) noexcept = delete;
  ScalarSegmentFeatures& operator=(const ScalarSegmentFeatures&) = delete;
  ScalarSegmentFeatures& operator=(ScalarSegmentFeatures&&) noexcept = delete;

  Result<> operator()();

protected:
  /**
   * @brief
   * @param data
   * @param args
   * @param gnum
   * @param nextSeed
   * @return int64
   */
  int64_t getSeed(int32 gnum, int64 nextSeed) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param referencepoint
   * @param neighborpoint
   * @param gnum
   * @return bool
   */
  bool determineGrouping(int64 referencePoint, int64 neighborPoint, int32 gnum) const override;

private:
  const ScalarSegmentFeaturesInputValues* m_InputValues = nullptr;
  FeatureIdsArrayType* m_FeatureIdsArray = nullptr;
  GoodVoxelsArrayType* m_GoodVoxelsArray = nullptr;
  std::shared_ptr<SegmentFeatures::CompareFunctor> m_CompareFunctor;
};
} // namespace complex
