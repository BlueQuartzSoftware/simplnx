#include "AppendImageGeometry.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
AppendImageGeometry::AppendImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AppendImageGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AppendImageGeometry::~AppendImageGeometry() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AppendImageGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> AppendImageGeometry::operator()()
{
  Result<> results = {};

  auto& destGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DestinationGeometryPath);
  AttributeMatrix* destCellData = destGeometry.getCellData();
  const DataPath destCellDataPath = m_InputValues->DestinationGeometryPath.createChildPath(destCellData->getName());
  DataPath newCellDataPath = destCellDataPath;
  AttributeMatrix* newCellData = destCellData;
  SizeVec3 destGeomDims = destGeometry.getDimensions();

  if(m_InputValues->SaveAsNewGeometry)
  {
    newCellData = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->NewGeometryPath).getCellData();
    newCellDataPath = m_InputValues->NewGeometryPath.createChildPath(newCellData->getName());
  }
  else
  {
    auto newDestGeomDims = destGeomDims;
    auto dim = to_underlying(m_InputValues->Direction);
    for(const auto& inputGeometryPath : m_InputValues->InputGeometriesPaths)
    {
      const auto& inputGeometry = m_DataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
      SizeVec3 inputGeomDims = inputGeometry.getDimensions();
      newDestGeomDims[dim] = newDestGeomDims[dim] + inputGeomDims[dim];
    }
    destGeometry.setDimensions(newDestGeomDims);
    const std::vector<size_t> newDims = {newDestGeomDims[2], newDestGeomDims[1], newDestGeomDims[0]};
    destCellData->resizeTuples(newDims);
  }

  ParallelTaskAlgorithm taskRunner;
  for(const auto& [dataId, dataObject] : *newCellData)
  {
    if(getCancel())
    {
      return {};
    }

    const std::string name = dataObject->getName();

    auto* newDataArray = m_DataStructure.getDataAs<IArray>(newCellDataPath.createChildPath(name));
    auto* destDataArray = m_DataStructure.getDataAs<IArray>(destCellDataPath.createChildPath(name));
    if(destDataArray == nullptr || newDataArray == nullptr)
    {
      continue;
    }

    std::vector<const IArray*> inputDataArrays;
    if(m_InputValues->SaveAsNewGeometry)
    {
      inputDataArrays.push_back(destDataArray);
    }

    for(const auto& inputGeometryPath : m_InputValues->InputGeometriesPaths)
    {
      const auto& inputGeometry = m_DataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
      const DataPath inputCellDataPath = inputGeometryPath.createChildPath(inputGeometry.getCellData()->getName());

      if(m_DataStructure.getData(inputCellDataPath.createChildPath(name)) == nullptr)
      {
        results = MergeResults(
            results,
            MakeWarningVoidResult(
                -8213, fmt::format("Data object {} does not exist in the input geometry cell data attribute matrix. Cannot append data so the resulting data object will likely contain invalid data!",
                                   name)));
        continue;
      }

      auto* inputDataArray = m_DataStructure.getDataAs<IArray>(inputCellDataPath.createChildPath(name));
      if(inputDataArray == nullptr)
      {
        continue;
      }
      inputDataArrays.push_back(inputDataArray);
    }

    if(m_InputValues->SaveAsNewGeometry)
    {
      m_MessageHandler(fmt::format("Combining data into array {}", newCellDataPath.createChildPath(name).toString()));
      CopyFromArray::RunParallelCombine(*newDataArray, taskRunner, inputDataArrays, m_InputValues->Direction);
    }
    else
    {
      m_MessageHandler(fmt::format("Appending data into array {}", newCellDataPath.createChildPath(name).toString()));
      auto destGeomDimsVec = destGeomDims.toContainer<std::vector<usize>>();
      std::reverse(destGeomDimsVec.begin(), destGeomDimsVec.end());
      CopyFromArray::RunParallelAppend(*destDataArray, taskRunner, inputDataArrays, destGeomDimsVec, m_InputValues->Direction);
    }
  }
  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  return results;
}
