#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT FindFeatureClusteringInputValues
{
  DataPath ImageGeometryPath;
  int32 NumberOfBins;
  int32 PhaseNumber;
  bool RemoveBiasedFeatures;
  uint64 SeedValue;
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

class SIMPLNXCORE_EXPORT FindFeatureClustering
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

} // namespace nx::core
