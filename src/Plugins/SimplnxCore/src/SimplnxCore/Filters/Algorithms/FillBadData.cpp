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
FillBadData::~FillBadData() noexcept = default;

Result<> FillBadData::operator()()
{
  std::vector<const IArray*> targets;
  const auto append = [&targets](const IArray* array) {
    if(array != nullptr)
    {
      targets.push_back(array);
    }
  };
  append(m_DataStructure.getDataAs<IDataArray>(m_InputValues->featureIdsArrayPath));
  if(m_InputValues->storeAsNewPhase)
  {
    append(m_DataStructure.getDataAs<IDataArray>(m_InputValues->cellPhasesArrayPath));
  }
  const auto* imageGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->inputImageGeometry);
  if(imageGeom != nullptr && imageGeom->getCellData() != nullptr)
  {
    for(const auto& [name, object] : *imageGeom->getCellData())
    {
      append(dynamic_cast<const IDataArray*>(object.get()));
    }
  }
  return DispatchAlgorithm<FillBadDataBFS, FillBadDataCCL>(AlgorithmArrayTargets(std::move(targets)), m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
