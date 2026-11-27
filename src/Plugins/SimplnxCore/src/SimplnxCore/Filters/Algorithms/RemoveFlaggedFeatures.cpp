#include "RemoveFlaggedFeatures.hpp"

#include "RemoveFlaggedFeaturesDirect.hpp"
#include "RemoveFlaggedFeaturesScanline.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

using namespace nx::core;

// Neighbor filling repeatedly reads Feature IDs and companion arrays. The
// scanline path avoids random chunk access by using sequential Z-slice transfers.

RemoveFlaggedFeatures::RemoveFlaggedFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             RemoveFlaggedFeaturesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

const std::atomic_bool& RemoveFlaggedFeatures::getCancel()
{
  return m_ShouldCancel;
}

Result<> RemoveFlaggedFeatures::operator()()
{
  std::vector<const IArray*> targets;
  const auto append = [&targets](const IDataArray* array) {
    if(array != nullptr)
    {
      targets.push_back(array);
    }
  };

  append(m_DataStructure.getDataAs<IDataArray>(m_InputValues->feature_ids_array_path));
  if(static_cast<Functionality>(m_InputValues->extract_features) != Functionality::Extract && m_InputValues->fill_removed_features)
  {
    for(const auto& array : GenerateDataArrayList(m_DataStructure, m_InputValues->feature_ids_array_path, m_InputValues->ignored_data_array_paths))
    {
      append(array.get());
    }
  }
  return DispatchAlgorithm<RemoveFlaggedFeaturesDirect, RemoveFlaggedFeaturesScanline>(AlgorithmArrayTargets(std::move(targets)), m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
