#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT FindFeatureClusteringInputValues
{
  int32 NumberOfBins;
  int32 PhaseNumber;
  bool RemoveBiasedFeatures;
  DataPath EquivalentDiametersArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CentroidsArrayPath;
  DataPath BiasedFeaturesArrayPath;
  DataPath CellEnsembleAttributeMatrixName;
  DataPath ClusteringListArrayName;
  DataPath RDFArrayName;
  DataPath MaxMinArrayName;
};

/**
 * @class FindFeatureClustering
 * @brief This filter determines the radial distribution function (RDF), as a histogram, of a given set of Features.
 */

class COMPLEXCORE_EXPORT FindFeatureClustering
{
public:
  FindFeatureClustering(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindFeatureClusteringInputValues* inputValues);
  ~FindFeatureClustering() noexcept;

  FindFeatureClustering(const FindFeatureClustering&) = delete;
  FindFeatureClustering(FindFeatureClustering&&) noexcept = delete;
  FindFeatureClustering& operator=(const FindFeatureClustering&) = delete;
  FindFeatureClustering& operator=(FindFeatureClustering&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindFeatureClusteringInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
