#include "FillBadData.hpp"

#include "FillBadDataBFS.hpp"
#include "FillBadDataCCL.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <vector>

using namespace nx::core;

// -----------------------------------------------------------------------------
FillBadData::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const FillBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------

Result<> FillBadData::operator()()
{
  std::vector<const IArray*> targets;
  const auto append = [&targets](const IArray* array) {
    if(array != nullptr)
    {
      targets.push_back(array);
    }
  };
  append(m_DataStructure.getDataAs<IDataArray>(m_InputValues->feature_ids_array_path));
  if(m_InputValues->store_as_new_phase)
  {
    append(m_DataStructure.getDataAs<IDataArray>(m_InputValues->cell_phases_array_path));
  }
  const auto* imageGeomPtr = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->input_image_geometry);
  if(imageGeomPtr != nullptr && imageGeomPtr->getCellData() != nullptr)
  {
    for(const auto& [name, object] : *imageGeomPtr->getCellData())
    {
      append(dynamic_cast<const IDataArray*>(object.get()));
    }
  }
  return DispatchAlgorithm<FillBadDataBFS, FillBadDataCCL>(AlgorithmArrayTargets(std::move(targets)), m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
