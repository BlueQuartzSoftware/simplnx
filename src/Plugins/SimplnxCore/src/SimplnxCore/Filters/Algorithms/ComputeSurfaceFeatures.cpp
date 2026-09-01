#include "ComputeSurfaceFeatures.hpp"

#include "ComputeSurfaceFeaturesDirect.hpp"
#include "ComputeSurfaceFeaturesScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

using namespace nx::core;

ComputeSurfaceFeatures::ComputeSurfaceFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ComputeSurfaceFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeSurfaceFeatures::~ComputeSurfaceFeatures() noexcept = default;

Result<> ComputeSurfaceFeatures::operator()()
{
  // Cell-scale face-neighbor reads determine whether the Scanline path is needed.
  auto* featureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath);
  return DispatchAlgorithm<ComputeSurfaceFeaturesDirect, ComputeSurfaceFeaturesScanline>({featureIdsArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
