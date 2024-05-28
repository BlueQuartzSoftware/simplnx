#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeBiasedFeaturesInputValues
{
  bool CalcByPhase;
  DataPath ImageGeometryPath;
  DataPath CentroidsArrayPath;
  DataPath SurfaceFeaturesArrayPath;
  DataPath PhasesArrayPath;
  DataPath BiasedFeaturesArrayName;
};

/**
 * @class ComputeBiasedFeatures
 * @brief This filter determines which Features are biased by the outer surfaces of the sample.
 */

class SIMPLNXCORE_EXPORT ComputeBiasedFeatures
{
public:
  ComputeBiasedFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeBiasedFeaturesInputValues* inputValues);
  ~ComputeBiasedFeatures() noexcept;

  ComputeBiasedFeatures(const ComputeBiasedFeatures&) = delete;
  ComputeBiasedFeatures(ComputeBiasedFeatures&&) noexcept = delete;
  ComputeBiasedFeatures& operator=(const ComputeBiasedFeatures&) = delete;
  ComputeBiasedFeatures& operator=(ComputeBiasedFeatures&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

protected:
  Result<> findBoundingBoxFeatures();
  Result<> findBoundingBoxFeatures2D();

private:
  DataStructure& m_DataStructure;
  const ComputeBiasedFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
