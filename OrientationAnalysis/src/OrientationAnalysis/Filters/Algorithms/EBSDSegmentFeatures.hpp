#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

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

/**
 * @brief The EBSDSegmentFeaturesInputValues struct
 */
struct ORIENTATIONANALYSIS_EXPORT EBSDSegmentFeaturesInputValues
{
  float32 pMisorientationTolerance;
  bool useGoodVoxels;
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

private:
  const EBSDSegmentFeaturesInputValues* m_InputValues = nullptr;
  FeatureIdsArrayType* m_FeatureIdsArray = nullptr;
  GoodVoxelsArrayType* m_GoodVoxelsArray = nullptr;
};

} // namespace complex
