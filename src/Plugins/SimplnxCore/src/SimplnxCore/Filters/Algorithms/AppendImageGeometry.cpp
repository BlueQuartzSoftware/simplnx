#include "AppendImageGeometry.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

AppendImageGeometry::AppendImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AppendImageGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

const std::atomic_bool& AppendImageGeometry::getCancel()
{
  return m_ShouldCancel;
}

Result<> AppendImageGeometry::operator()()
{
  Result<> results = {};

  auto& destGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->destination_geometry_path);
  AttributeMatrix* destCellDataPtr = destGeometry.getCellData();
  const DataPath destCellDataPath = m_InputValues->destination_geometry_path.createChildPath(destCellDataPtr->getName());
  DataPath newCellDataPath = destCellDataPath;
  AttributeMatrix* newCellDataPtr = destCellDataPtr;
  SizeVec3Type destGeomDims = destGeometry.getDimensions();

  if(m_InputValues->save_as_new_geometry)
  {
    newCellDataPtr = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->new_geometry_path).getCellData();
    newCellDataPath = m_InputValues->new_geometry_path.createChildPath(newCellDataPtr->getName());
  }
  else
  {
    auto newDestGeomDims = destGeomDims;
    auto dim = to_underlying(m_InputValues->direction);
    for(const auto& inputGeometryPath : m_InputValues->input_geometries_paths)
    {
      const auto& inputGeometry = m_DataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
      SizeVec3Type inputGeomDims = inputGeometry.getDimensions();
      newDestGeomDims[dim] = newDestGeomDims[dim] + inputGeomDims[dim];
    }
    destGeometry.setDimensions(newDestGeomDims);
    const std::vector<size_t> newDims = {newDestGeomDims[2], newDestGeomDims[1], newDestGeomDims[0]};
    Result<> resizeResult = destCellDataPtr->resizeTuples(newDims);
    if(resizeResult.invalid())
    {
      return resizeResult;
    }
  }

  // Temporary default-filled arrays supply data that a source geometry lacks.
  DataStructure tmpDataStructure;

  // Declared before the task runner so the runner's destructor joins every worker while this holder is still alive.
  copy_from_array::ParallelTaskResult taskResult;
  ParallelTaskAlgorithm taskRunner;
  for(const auto& [dataId, dataObject] : *newCellDataPtr)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const std::string name = dataObject->getName();

    auto newDataArrayPath = newCellDataPath.createChildPath(name);
    auto destDataArrayPath = destCellDataPath.createChildPath(name);
    auto* newDataArrayPtr = m_DataStructure.getDataAs<IArray>(newDataArrayPath);
    auto* destDataArrayPtr = m_DataStructure.getDataAs<IArray>(destDataArrayPath);
    // A new output array can require a default-filled destination prefix.
    if(destDataArrayPtr == nullptr)
    {
      // A UUID prevents name collisions for arrays that remain in the temporary structure.
      auto& dataStructure = m_InputValues->save_as_new_geometry ? tmpDataStructure : m_DataStructure;
      auto dataArrayName = m_InputValues->save_as_new_geometry ? Uuid::GenerateV4().str() : newDataArrayPtr->getName();
      auto destArrayDimsVec = destGeomDims.toContainer<std::vector<usize>>();
      std::reverse(destArrayDimsVec.begin(), destArrayDimsVec.end());
      auto result = CreateDefaultValueArrayFromArray(dataStructure, newDataArrayPtr, dataArrayName, destArrayDimsVec, m_InputValues->default_value);
      if(result.invalid())
      {
        return ConvertResult(std::move(result));
      }
      destDataArrayPtr = result.value();
    }

    std::vector<const IArray*> inputDataArrays;
    std::vector<std::vector<usize>> inputTupleShapes;
    if(m_InputValues->save_as_new_geometry)
    {
      inputDataArrays.push_back(destDataArrayPtr);

      auto tupleShape = destGeometry.getDimensions().toContainer<std::vector<usize>>();
      std::reverse(tupleShape.begin(), tupleShape.end());
      inputTupleShapes.push_back(tupleShape);
    }

    for(const auto& inputGeometryPath : m_InputValues->input_geometries_paths)
    {
      const auto& inputGeometry = m_DataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
      const DataPath inputCellDataPath = inputGeometryPath.createChildPath(inputGeometry.getCellData()->getName());

      auto tupleShape = inputGeometry.getDimensions().toContainer<std::vector<usize>>();
      std::reverse(tupleShape.begin(), tupleShape.end());
      inputTupleShapes.push_back(tupleShape);

      if(m_DataStructure.getData(inputCellDataPath.createChildPath(name)) == nullptr)
      {
        results.warnings().push_back(
            {-8213, fmt::format("Data object {} does not exist in the input geometry cell data attribute matrix. The resulting appended data will be initialized to the chosen default value '{}'",
                                name, m_InputValues->default_value)});

        // A UUID prevents name collisions in the temporary structure.
        auto result = CreateDefaultValueArrayFromArray(tmpDataStructure, destDataArrayPtr, Uuid::GenerateV4().str(), tupleShape, m_InputValues->default_value);
        if(result.invalid())
        {
          return ConvertResult(std::move(result));
        }
        inputDataArrays.push_back(result.value());
      }
      else
      {
        auto* inputDataArrayPtr = m_DataStructure.getDataAs<IArray>(inputCellDataPath.createChildPath(name));
        if(inputDataArrayPtr == nullptr)
        {
          continue;
        }
        inputDataArrays.push_back(inputDataArrayPtr);
      }
    }

    if(m_InputValues->save_as_new_geometry)
    {
      m_MessageHandler.sendInfoMessage(fmt::format("Combining data into array {}", newCellDataPath.createChildPath(name).toString()));
      auto newGeometry = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->new_geometry_path);
      auto newDestGeomDimsVec = newGeometry.getDimensions().toContainer<std::vector<usize>>();
      std::reverse(newDestGeomDimsVec.begin(), newDestGeomDimsVec.end());
      copy_from_array::RunParallelCombine(*newDataArrayPtr, taskRunner, taskResult, inputDataArrays, inputTupleShapes, newDestGeomDimsVec, m_InputValues->direction, m_InputValues->mirror_geometry);
    }
    else
    {
      m_MessageHandler.sendInfoMessage(fmt::format("Appending data into array {}", newCellDataPath.createChildPath(name).toString()));
      auto originalDestGeomDimsVec = destGeomDims.toContainer<std::vector<usize>>();
      std::reverse(originalDestGeomDimsVec.begin(), originalDestGeomDimsVec.end());
      auto newDestGeomDimsVec = destGeometry.getDimensions().toContainer<std::vector<usize>>();
      std::reverse(newDestGeomDimsVec.begin(), newDestGeomDimsVec.end());
      copy_from_array::RunParallelAppend(*destDataArrayPtr, taskRunner, taskResult, inputDataArrays, inputTupleShapes, originalDestGeomDimsVec, newDestGeomDimsVec, m_InputValues->direction,
                                         m_InputValues->mirror_geometry);
    }
  }
  taskRunner.wait();
  Result<> taskExecutionResult = taskResult.takeResult();
  return MergeResults(std::move(results), std::move(taskExecutionResult));
}
