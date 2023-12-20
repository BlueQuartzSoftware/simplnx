#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT FindBiasedFeaturesInputValues
{
  bool CalcByPhase;
  DataPath ImageGeometryPath;
  DataPath CentroidsArrayPath;
  DataPath SurfaceFeaturesArrayPath;
  DataPath PhasesArrayPath;
  DataPath BiasedFeaturesArrayName;
};

/**
 * @class FindBiasedFeatures
 * @brief This filter determines which Features are biased by the outer surfaces of the sample.
 */

class SIMPLNXCORE_EXPORT FindBiasedFeatures
{
public:
  FindBiasedFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindBiasedFeaturesInputValues* inputValues);
  ~FindBiasedFeatures() noexcept;

  FindBiasedFeatures(const FindBiasedFeatures&) = delete;
  FindBiasedFeatures(FindBiasedFeatures&&) noexcept = delete;
  FindBiasedFeatures& operator=(const FindBiasedFeatures&) = delete;
  FindBiasedFeatures& operator=(FindBiasedFeatures&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

protected:
  void findBoundingBoxFeatures();
  void findBoundingBoxFeatures2D();

private:
  DataStructure& m_DataStructure;
  const FindBiasedFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
