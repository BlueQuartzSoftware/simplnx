#include "AppendImageGeometryZSlice.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
AppendImageGeometryZSlice::AppendImageGeometryZSlice(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                     AppendImageGeometryZSliceInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AppendImageGeometryZSlice::~AppendImageGeometryZSlice() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AppendImageGeometryZSlice::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AppendImageGeometryZSlice::operator()()
{
  Result<> results = {};

  const auto& inputGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputGeometryPath);
  auto& destGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DestinationGeometryPath);
  AttributeMatrix* destCellData = destGeometry.getCellData();
  const DataPath destCellDataPath = m_InputValues->DestinationGeometryPath.createChildPath(destCellData->getName());
  const DataPath inputCellDataPath = m_InputValues->InputGeometryPath.createChildPath(inputGeometry.getCellData()->getName());
  DataPath newCellDataPath = destCellDataPath;
  AttributeMatrix* newCellData = destCellData;
  SizeVec3 inputGeomDims = inputGeometry.getDimensions();
  SizeVec3 destGeomDims = destGeometry.getDimensions();

  if(m_InputValues->SaveAsNewGeometry)
  {
    newCellData = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->NewGeometryPath).getCellData();
    newCellDataPath = m_InputValues->NewGeometryPath.createChildPath(newCellData->getName());
  }
  else
  {
    // We are only appending in the Z direction
    destGeomDims[2] = destGeomDims[2] + inputGeomDims[2];
    destGeometry.setDimensions(destGeomDims);
    const std::vector<size_t> newDims = {destGeomDims[2], destGeomDims[1], destGeomDims[0]};
    destCellData->resizeTuples(newDims);
  }
  const usize tupleOffset = destGeometry.getNumberOfCells();

  ParallelTaskAlgorithm taskRunner;
  for(const auto& [dataId, dataObject] : *newCellData)
  {
    if(getCancel())
    {
      return {};
    }

    const std::string name = dataObject->getName();
    if(m_DataStructure.getData(inputCellDataPath.createChildPath(name)) == nullptr)
    {
      results = MergeResults(
          results,
          MakeWarningVoidResult(
              -8213,
              fmt::format("Data object {} does not exist in the input geometry cell data attribute matrix. Cannot append data so the resulting data object will likely contain invalid data!", name)));
      continue;
    }

    auto* inputDataArray = m_DataStructure.getDataAs<IArray>(inputCellDataPath.createChildPath(name));
    auto* destDataArray = m_DataStructure.getDataAs<IArray>(destCellDataPath.createChildPath(name));
    auto* newDataArray = m_DataStructure.getDataAs<IArray>(newCellDataPath.createChildPath(name));
    if(destDataArray == nullptr || newDataArray == nullptr || inputDataArray == nullptr)
    {
      continue;
    }

    if(m_InputValues->SaveAsNewGeometry)
    {
      m_MessageHandler(fmt::format("Combining data into array {}", newCellDataPath.createChildPath(name).toString()));
      CopyFromArray::RunParallelCombine(*destDataArray, taskRunner, *inputDataArray, *newDataArray);
    }
    else
    {
      m_MessageHandler(fmt::format("Appending Data Array {}", inputCellDataPath.createChildPath(name).toString()));
      CopyFromArray::RunParallelAppend(*destDataArray, taskRunner, *inputDataArray, tupleOffset);
    }
  }
  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  return results;
}
