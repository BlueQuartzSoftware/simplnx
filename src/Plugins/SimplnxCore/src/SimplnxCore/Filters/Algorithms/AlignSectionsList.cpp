#include "AlignSectionsList.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
AlignSectionsList::AlignSectionsList(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AlignSectionsListInputValues* inputValues)
: AlignSections(dataStructure, shouldCancel, mesgHandler)
, m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AlignSectionsList::~AlignSectionsList() noexcept = default;

// -----------------------------------------------------------------------------
Result<> AlignSectionsList::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }

  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("AlignSectionsList: Aligning sections from list...");

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  return execute(imageGeom.getDimensions(), m_InputValues->ImageGeometryPath);
}

// -----------------------------------------------------------------------------
Result<> AlignSectionsList::findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts)
{
  if(m_ShouldCancel)
  {
    return {};
  }
  auto alignSectionsType = m_InputValues->AlignSectionsType;

  if(alignSectionsType == static_cast<ChoicesParameter::ValueType>(to_underlying(AlignSectionsInputType::RelativeShifts)))
  {
    // Size validated in preflight
    const auto& relativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->ShiftsArrayPath)->getDataStoreRef();
    const usize numTup = relativeShiftsStore.getNumberOfTuples();
    for(usize i = 1; i < numTup; i++)
    {
      xShifts[i] = xShifts[i - 1] + relativeShiftsStore[i * 2];
      yShifts[i] = yShifts[i - 1] + relativeShiftsStore[(i * 2) + 1];
    }
  }
  else if(alignSectionsType == static_cast<ChoicesParameter::ValueType>(to_underlying(AlignSectionsInputType::CumulativeShifts)))
  {
    // Size validated in preflight
    const auto& cumulativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->ShiftsArrayPath)->getDataStoreRef();
    const usize numTup = cumulativeShiftsStore.getNumberOfTuples();
    for(usize i = 1; i < numTup; i++)
    {
      xShifts[i] = cumulativeShiftsStore[i * 2];
      yShifts[i] = cumulativeShiftsStore[(i * 2) + 1];
    }
  }

  return {};
}
