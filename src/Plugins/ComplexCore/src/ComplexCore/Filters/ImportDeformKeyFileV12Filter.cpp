#include "ImportDeformKeyFileV12Filter.hpp"

#include "Algorithms/ImportDeformKeyFileV12.hpp"

#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Filter/Actions/CreateGeometry2DAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;

namespace
{
std::atomic_int32_t s_InstanceId = 0;
std::map<int32, FileCache> s_HeaderCache;
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
ImportDeformKeyFileV12Filter::ImportDeformKeyFileV12Filter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  s_HeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
ImportDeformKeyFileV12Filter::~ImportDeformKeyFileV12Filter() noexcept
{
  s_HeaderCache.erase(m_InstanceId);
}

//------------------------------------------------------------------------------
std::string ImportDeformKeyFileV12Filter::name() const
{
  return FilterTraits<ImportDeformKeyFileV12Filter>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportDeformKeyFileV12Filter::className() const
{
  return FilterTraits<ImportDeformKeyFileV12Filter>::className;
}

//------------------------------------------------------------------------------
Uuid ImportDeformKeyFileV12Filter::uuid() const
{
  return FilterTraits<ImportDeformKeyFileV12Filter>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportDeformKeyFileV12Filter::humanName() const
{
  return "Import Deform Key File (v12)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportDeformKeyFileV12Filter::defaultTags() const
{
  return {"IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ImportDeformKeyFileV12Filter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFilePath_Key, "Input File", "File path that points to the imported file", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::InputFile));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_QuadGeomPath_Key, "Quad Geometry", "", DataPath{}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexAMName_Key, "Vertex Data Name", "The name of the created Attribute Matrix for vertex data", "Vertex Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAMName_Key, "Cell Data Name", "The name of the created Attribute Matrix for cell data", "Cell Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportDeformKeyFileV12Filter::clone() const
{
  return std::make_unique<ImportDeformKeyFileV12Filter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportDeformKeyFileV12Filter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{
  auto pInputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFilePath_Key);

  auto pQuadGeomPathValue = filterArgs.value<DataPath>(k_QuadGeomPath_Key);
  auto pVertexAMNameValue = filterArgs.value<std::string>(k_VertexAMName_Key);
  auto pCellAMNameValue = filterArgs.value<std::string>(k_CellAMName_Key);

  // Declare the preflightResult variable
  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  DataPath vertexDataPath = pQuadGeomPathValue.createChildPath(pVertexAMNameValue);
  DataPath cellDataPath = pQuadGeomPathValue.createChildPath(pCellAMNameValue);

  // Read from the file if the input file has changed or the input file's time stamp is out of date.
  if(pInputFilePathValue != s_HeaderCache[m_InstanceId].inputFile || s_HeaderCache[m_InstanceId].timeStamp < fs::last_write_time(pInputFilePathValue))
  {
    ImportDeformKeyFileV12InputValues inputValues;
    inputValues.InputFilePath = pInputFilePathValue;

    inputValues.QuadGeomPath = pQuadGeomPathValue;
    inputValues.CellAMPath = cellDataPath;
    inputValues.VertexAMPath = vertexDataPath;

    // Read from the file
    DataStructure throwaway{};
    ImportDeformKeyFileV12 algorithm(throwaway, messageHandler, shouldCancel, &inputValues);
    algorithm.operator()(false);
    // Cache the results from algorithm run
    s_HeaderCache[m_InstanceId] = algorithm.getCache();

    // Update the cached variables
    s_HeaderCache[m_InstanceId].inputFile = pInputFilePathValue;
    s_HeaderCache[m_InstanceId].timeStamp = fs::last_write_time(pInputFilePathValue);
  }

  // Read from the cache and create objects
  {
    auto createAction = std::make_unique<CreateQuadGeometryAction>(pQuadGeomPathValue, s_HeaderCache[m_InstanceId].cellAttrMatTupleCount, s_HeaderCache[m_InstanceId].vertexAttrMatTupleCount,
                                                                   vertexDataPath.getTargetName(), cellDataPath.getTargetName(), "SharedVertices", "SharedFaces");
    resultOutputActions.value().actions.push_back(std::move(createAction));
  }

  for(const DataArrayMetadata& daMetadata : s_HeaderCache[m_InstanceId].dataArrays)
  {
    DataPath myPath = {};
    if(daMetadata.type == DataArrayType::VERTEX)
    {
      if(s_HeaderCache[m_InstanceId].vertexAttrMatTupleCount != daMetadata.tupleCount)
      {
        return MakePreflightErrorResult(-2020, fmt::format("{}'s tuple dims ({}) do not match, the parent {} Attribute Matrix's tuple count({})", daMetadata.name, daMetadata.tupleCount,
                                                           pVertexAMNameValue, s_HeaderCache[m_InstanceId].vertexAttrMatTupleCount));
      }
      myPath = vertexDataPath.createChildPath(daMetadata.name);
    }
    else if(daMetadata.type == DataArrayType::CELL)
    {
      if(s_HeaderCache[m_InstanceId].cellAttrMatTupleCount != daMetadata.tupleCount)
      {
        return MakePreflightErrorResult(-2021, fmt::format("{}'s tuple dims ({}) do not match, the parent {} Attribute Matrix's tuple count({})", daMetadata.name, daMetadata.tupleCount,
                                                           pCellAMNameValue, s_HeaderCache[m_InstanceId].cellAttrMatTupleCount));
      }
      myPath = cellDataPath.createChildPath(daMetadata.name);
    }
    else
    {
      return MakePreflightErrorResult(-2022, fmt::format("Unable to determine the type for cached data array \"{}\".  The type must be either vertex or cell.", daMetadata.name));
    }

    {
      auto createAction = std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{daMetadata.tupleCount}, std::vector<usize>{daMetadata.componentCount}, myPath);
      resultOutputActions.value().actions.push_back(std::move(createAction));
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImportDeformKeyFileV12Filter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  ImportDeformKeyFileV12InputValues inputValues;

  inputValues.InputFilePath = s_HeaderCache[m_InstanceId].inputFile;

  return ImportDeformKeyFileV12(dataStructure, messageHandler, shouldCancel, &inputValues)(true);
}
} // namespace complex
