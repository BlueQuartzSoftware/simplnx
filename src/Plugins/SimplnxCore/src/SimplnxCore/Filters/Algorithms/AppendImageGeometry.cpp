#include "AppendImageGeometry.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
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

  // Create a temporary data structure that we can use to create arrays with default values, if needed
  DataStructure tmpDataStructure;

  ParallelTaskAlgorithm taskRunner;
  for(const auto& [dataId, dataObject] : *newCellData)
  {
    if(getCancel())
    {
      return {};
    }

    const std::string name = dataObject->getName();

    auto newDataArrayPath = newCellDataPath.createChildPath(name);
    auto destDataArrayPath = destCellDataPath.createChildPath(name);
    auto* newDataArray = m_DataStructure.getDataAs<IArray>(newDataArrayPath);
    auto* destDataArray = m_DataStructure.getDataAs<IArray>(destDataArrayPath);
    //    if(destDataArray == nullptr && newDataArray == nullptr)
    //    {
    //      // One of these has to be valid, something has gone horribly wrong
    //      return MakeErrorResult(-10001,
    //                             fmt::format("There is no array at path '{}' in the given destination image geometry or at path '{}' in the given new image geometry.  Please contact the
    //                             developers.",
    //                                         destDataArrayPath.toString(), newDataArrayPath.toString()));
    //    }

    // Create default value destination data array if it doesn't exist
    if(destDataArray == nullptr)
    {
      // Use UUID as the new array's name to avoid naming clashes.  The name ultimately doesn't matter since it's in a temporary data structure and will never be publicly exposed.
      auto& dataStructure = m_InputValues->SaveAsNewGeometry ? tmpDataStructure : m_DataStructure;
      auto dataArrayName = m_InputValues->SaveAsNewGeometry ? Uuid::GenerateV4().str() : newDataArray->getName();
      auto destArrayDimsVec = destGeomDims.toContainer<std::vector<usize>>();
      std::reverse(destArrayDimsVec.begin(), destArrayDimsVec.end());
      auto result = CreateDefaultValueArrayFromArray(dataStructure, newDataArray, dataArrayName, destArrayDimsVec, m_InputValues->DefaultValue);
      if(result.invalid())
      {
        return ConvertResult(std::move(result));
      }
      destDataArray = result.value();
    }

    std::vector<const IArray*> inputDataArrays;
    std::vector<std::vector<usize>> inputTupleShapes;
    if(m_InputValues->SaveAsNewGeometry)
    {
      inputDataArrays.push_back(destDataArray);

      auto tupleShape = destGeometry.getDimensions().toContainer<std::vector<usize>>();
      std::reverse(tupleShape.begin(), tupleShape.end());
      inputTupleShapes.push_back(tupleShape);
    }

    for(const auto& inputGeometryPath : m_InputValues->InputGeometriesPaths)
    {
      const auto& inputGeometry = m_DataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
      const DataPath inputCellDataPath = inputGeometryPath.createChildPath(inputGeometry.getCellData()->getName());

      auto tupleShape = inputGeometry.getDimensions().toContainer<std::vector<usize>>();
      std::reverse(tupleShape.begin(), tupleShape.end());
      inputTupleShapes.push_back(tupleShape);

      if(m_DataStructure.getData(inputCellDataPath.createChildPath(name)) == nullptr)
      {
        results = MergeResults(
            results,
            MakeWarningVoidResult(
                -8213, fmt::format("Data object {} does not exist in the input geometry cell data attribute matrix. The resulting appended data will be initialized to the chosen default value '{}'",
                                   name, m_InputValues->DefaultValue)));

        // Use UUID as the new array's name to avoid naming clashes.  The name ultimately doesn't matter since it's in a temporary data structure and will never be publicly exposed.
        auto result = CreateDefaultValueArrayFromArray(tmpDataStructure, destDataArray, Uuid::GenerateV4().str(), tupleShape, m_InputValues->DefaultValue);
        if(result.invalid())
        {
          return ConvertResult(std::move(result));
        }
        inputDataArrays.push_back(result.value());
      }
      else
      {
        auto* inputDataArray = m_DataStructure.getDataAs<IArray>(inputCellDataPath.createChildPath(name));
        if(inputDataArray == nullptr)
        {
          continue;
        }
        inputDataArrays.push_back(inputDataArray);
      }
    }

    if(m_InputValues->SaveAsNewGeometry)
    {
      m_MessageHandler.sendInfoMessage(fmt::format("Combining data into array {}", newCellDataPath.createChildPath(name).toString()));
      auto newGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->NewGeometryPath);
      auto newDestGeomDimsVec = newGeometry.getDimensions().toContainer<std::vector<usize>>();
      std::reverse(newDestGeomDimsVec.begin(), newDestGeomDimsVec.end());
      CopyFromArray::RunParallelCombine(*newDataArray, taskRunner, inputDataArrays, inputTupleShapes, newDestGeomDimsVec, m_InputValues->Direction, m_InputValues->MirrorGeometry);
    }
    else
    {
      m_MessageHandler.sendInfoMessage(fmt::format("Appending data into array {}", newCellDataPath.createChildPath(name).toString()));
      auto originalDestGeomDimsVec = destGeomDims.toContainer<std::vector<usize>>();
      std::reverse(originalDestGeomDimsVec.begin(), originalDestGeomDimsVec.end());
      auto newDestGeomDimsVec = destGeometry.getDimensions().toContainer<std::vector<usize>>();
      std::reverse(newDestGeomDimsVec.begin(), newDestGeomDimsVec.end());
      CopyFromArray::RunParallelAppend(*destDataArray, taskRunner, inputDataArrays, inputTupleShapes, originalDestGeomDimsVec, newDestGeomDimsVec, m_InputValues->Direction,
                                       m_InputValues->MirrorGeometry);
    }
  }
  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  return results;
}
