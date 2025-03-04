#include "AlignSectionsList.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

using namespace nx::core;

namespace
{
void CopyShifts(const UInt64AbstractDataStore& positioningStore, std::vector<int64_t>& xShifts, std::vector<int64_t>& yShifts)
{
  const usize size = positioningStore.getNumberOfTuples();
  for(usize i = 1; i < size; i++)
  {
    xShifts[i] = xShifts[i - 1] + positioningStore[i * 2];
    yShifts[i] = yShifts[i - 1] + positioningStore[(i * 2) + 1];
  }
}
} // namespace

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
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  Result<> result = execute(imageGeom.getDimensions());
  if(result.invalid())
  {
    return result;
  }
  if(m_Result.invalid())
  {
    return m_Result;
  }
  return {};
}

// -----------------------------------------------------------------------------
std::vector<DataPath> AlignSectionsList::getSelectedDataPaths() const
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const auto& cellAttributeMatrix = imageGeom.getCellData();
  std::optional<std::vector<DataPath>> selectedCellArrays = GetAllChildDataPaths(m_DataStructure, m_InputValues->ImageGeometryPath.createChildPath(cellAttributeMatrix->getName()));
  if(selectedCellArrays.has_value())
  {
    return selectedCellArrays.value();
  }
  return {};
}

// -----------------------------------------------------------------------------
Result<> AlignSectionsList::findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts)
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  SizeVec3 udims = imageGeom.getDimensions();
  auto zDim = static_cast<int64>(udims[2]);

  if(m_InputValues->UseFile)
  {
    return readUserShiftsFile(m_InputValues->InputFile, zDim, xShifts, yShifts);
  }

  // Size validated in preflight
  const auto& positoningStore = m_DataStructure.getDataAs<UInt64Array>(m_InputValues->PositioningArrayPath)->getDataStoreRef();
  CopyShifts(positoningStore, xShifts, yShifts);

  return {};
}
