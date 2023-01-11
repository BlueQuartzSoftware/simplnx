#include "AlignSectionsList.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"

using namespace complex;

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
void AlignSectionsList::find_shifts(std::vector<int64>& xshifts, std::vector<int64>& yshifts)
{
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  SizeVec3 udims = imageGeom.getDimensions();

  int64 dims[3] = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  int64 slice = 0;
  int64 newxshift = 0, newyshift = 0;

  std::ifstream inFile;
  inFile.open(m_InputValues->InputFile);

  if(m_InputValues->DREAM3DAlignmentFile)
  {
    // These are ignored from the input file since DREAM.3D wrote the file
    int64 slice2 = 0;
    float32 xShift = 0.0f;
    float32 yShift = 0.0f;
    for(int64 iter = 1; iter < dims[2]; iter++)
    {
      inFile >> slice >> slice2 >> newxshift >> newyshift >> xShift >> yShift;
      xshifts[iter] = xshifts[iter - 1] + newxshift;
      yshifts[iter] = yshifts[iter - 1] + newyshift;
    }
  }
  else
  {
    for(int64 iter = 1; iter < dims[2]; iter++)
    {
      inFile >> slice >> newxshift >> newyshift;
      xshifts[iter] = xshifts[iter - 1] + newxshift;
      yshifts[iter] = yshifts[iter - 1] + newyshift;
    }
  }

  inFile.close();
}
