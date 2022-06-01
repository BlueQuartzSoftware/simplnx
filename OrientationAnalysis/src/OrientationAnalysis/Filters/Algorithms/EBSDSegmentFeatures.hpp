#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Utilities/SegmentFeatures.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"

#include <random>
#include <vector>

namespace complex
{

/**
 * @brief The EBSDSegmentFeaturesInputValues struct
 */
struct ORIENTATIONANALYSIS_EXPORT EBSDSegmentFeaturesInputValues
{
  float32 misorientationTolerance;
  bool useGoodVoxels;
  bool shouldRandomizeFeatureIds;
  DataPath gridGeomPath;
  DataPath quatsArrayPath;
  DataPath cellPhasesArrayPath;
  DataPath goodVoxelsArrayPath;
  DataPath crystalStructuresArrayPath;
  DataPath featureIdsArrayPath;
  DataPath cellFeatureAttributeMatrixPath;
  DataPath activeArrayPath;
};

/**
 * @brief
 */
class ORIENTATIONANALYSIS_EXPORT EBSDSegmentFeatures : public SegmentFeatures
{
public:
  using SeedGenerator = std::mt19937_64;
  using Int64Distribution = std::uniform_int_distribution<int64>;
  using FeatureIdsArrayType = Int32Array;
  using GoodVoxelsArrayType = BoolArray;

  EBSDSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EBSDSegmentFeaturesInputValues* inputValues);
  ~EBSDSegmentFeatures() noexcept;

  EBSDSegmentFeatures(const EBSDSegmentFeatures&) = delete;            // Copy Constructor Not Implemented
  EBSDSegmentFeatures(EBSDSegmentFeatures&&) = delete;                 // Move Constructor Not Implemented
  EBSDSegmentFeatures& operator=(const EBSDSegmentFeatures&) = delete; // Copy Assignment Not Implemented
  EBSDSegmentFeatures& operator=(EBSDSegmentFeatures&&) = delete;      // Move Assignment Not Implemented

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
  const EBSDSegmentFeaturesInputValues* m_InputValues = nullptr;
  Float32Array* m_QuatsArray = nullptr;
  FeatureIdsArrayType* m_CellPhases = nullptr;
  GoodVoxelsArrayType* m_GoodVoxelsArray = nullptr;
  DataArray<uint32>* m_CrystalStructures = nullptr;

  FeatureIdsArrayType* m_FeatureIdsArray = nullptr;

  std::vector<LaueOps::Pointer> m_OrientationOps;
};

} // namespace complex
