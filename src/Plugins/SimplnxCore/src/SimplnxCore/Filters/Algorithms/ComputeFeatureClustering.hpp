#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeFeatureClusteringInputValues
{
  DataPath ImageGeometryPath;
  int32 NumberOfBins;
  int32 PhaseNumber;
  bool RemoveBiasedFeatures;
  uint64 SeedValue;
  DataPath FeaturePhasesArrayPath;
  DataPath CentroidsArrayPath;
  DataPath BiasedFeaturesArrayPath;
  DataPath CellEnsembleAttributeMatrixName;
  DataPath ClusteringListArrayName;
  DataPath RDFArrayName;
  DataPath MaxMinArrayName;
};

/**
 * @class ComputeFeatureClustering
 * @brief This filter determines the radial distribution function (RDF), as a histogram, of a given set of Features.
 */

class SIMPLNXCORE_EXPORT ComputeFeatureClustering
{
public:
  ComputeFeatureClustering(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureClusteringInputValues* inputValues);
  ~ComputeFeatureClustering() noexcept;

  ComputeFeatureClustering(const ComputeFeatureClustering&) = delete;
  ComputeFeatureClustering(ComputeFeatureClustering&&) noexcept = delete;
  ComputeFeatureClustering& operator=(const ComputeFeatureClustering&) = delete;
  ComputeFeatureClustering& operator=(ComputeFeatureClustering&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureClusteringInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
