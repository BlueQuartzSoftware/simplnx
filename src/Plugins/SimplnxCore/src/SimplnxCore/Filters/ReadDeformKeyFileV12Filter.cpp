#include "ReadDeformKeyFileV12Filter.hpp"

#include "Algorithms/ReadDeformKeyFileV12.hpp"

#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry2DAction.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
std::atomic_int32_t s_InstanceId = 0;
std::map<int32, FileCache> s_HeaderCache;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
ReadDeformKeyFileV12Filter::ReadDeformKeyFileV12Filter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  s_HeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
ReadDeformKeyFileV12Filter::~ReadDeformKeyFileV12Filter() noexcept
{
  s_HeaderCache.erase(m_InstanceId);
}

//------------------------------------------------------------------------------
std::string ReadDeformKeyFileV12Filter::name() const
{
  return FilterTraits<ReadDeformKeyFileV12Filter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadDeformKeyFileV12Filter::className() const
{
  return FilterTraits<ReadDeformKeyFileV12Filter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadDeformKeyFileV12Filter::uuid() const
{
  return FilterTraits<ReadDeformKeyFileV12Filter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadDeformKeyFileV12Filter::humanName() const
{
  return "Read Deform Key File (v12)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadDeformKeyFileV12Filter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ReadDeformKeyFileV12Filter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFilePath_Key, "Input File", "File path that points to the imported file", fs::path(""), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::InputFile));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_QuadGeomPath_Key, "Quad Geometry", "The created Quad Geometry from  imported from file", DataPath({"Quad Geometry"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexAMName_Key, "Vertex Data Name", "The name of the created Attribute Matrix for vertex data", "Vertex Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAMName_Key, "Cell Data Name", "The name of the created Attribute Matrix for cell data", "Cell Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadDeformKeyFileV12Filter::clone() const
{
  return std::make_unique<ReadDeformKeyFileV12Filter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadDeformKeyFileV12Filter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto pInputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFilePath_Key);

  auto pQuadGeomPathValue = filterArgs.value<DataPath>(k_QuadGeomPath_Key);
  auto pVertexAMNameValue = filterArgs.value<std::string>(k_VertexAMName_Key);
  auto pCellAMNameValue = filterArgs.value<std::string>(k_CellAMName_Key);

  // Declare the preflightResult variable
  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  DataPath vertexDataPath = pQuadGeomPathValue.createChildPath(pVertexAMNameValue);
  DataPath cellDataPath = pQuadGeomPathValue.createChildPath(pCellAMNameValue);

  // Read from the file if the input file has changed or the input file's time stamp is out of date.
  if(pInputFilePathValue != s_HeaderCache[m_InstanceId].inputFile || s_HeaderCache[m_InstanceId].timeStamp < fs::last_write_time(pInputFilePathValue))
  {
    ReadDeformKeyFileV12InputValues inputValues;
    inputValues.InputFilePath = pInputFilePathValue;

    inputValues.QuadGeomPath = pQuadGeomPathValue;
    inputValues.CellAMPath = cellDataPath;
    inputValues.VertexAMPath = vertexDataPath;

    // Read from the file
    DataStructure throwaway = DataStructure();
    ReadDeformKeyFileV12 algorithm(throwaway, messageHandler, shouldCancel, &inputValues);
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
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  for(const DataArrayMetadata& daMetadata : s_HeaderCache[m_InstanceId].dataArrays)
  {
    if(daMetadata.path.getParent() == vertexDataPath)
    {
      if(s_HeaderCache[m_InstanceId].vertexAttrMatTupleCount != daMetadata.tupleCount)
      {
        return MakePreflightErrorResult(-2020, fmt::format("{}'s tuple dims ({}) do not match, the parent {} Attribute Matrix's tuple count({})", daMetadata.path.getTargetName(),
                                                           daMetadata.tupleCount, pVertexAMNameValue, s_HeaderCache[m_InstanceId].vertexAttrMatTupleCount));
      }
    }
    else if(daMetadata.path.getParent() == cellDataPath)
    {
      if(s_HeaderCache[m_InstanceId].cellAttrMatTupleCount != daMetadata.tupleCount)
      {
        return MakePreflightErrorResult(-2021, fmt::format("{}'s tuple dims ({}) do not match, the parent {} Attribute Matrix's tuple count({})", daMetadata.path.getTargetName(),
                                                           daMetadata.tupleCount, pCellAMNameValue, s_HeaderCache[m_InstanceId].cellAttrMatTupleCount));
      }
    }
    else
    {
      return MakePreflightErrorResult(-2022, fmt::format("Unable to determine the type for cached data array \"{}\".  The type must be either vertex or cell.", daMetadata.path.getTargetName()));
    }

    {
      auto createAction = std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{daMetadata.tupleCount}, std::vector<usize>{daMetadata.componentCount}, daMetadata.path);
      resultOutputActions.value().appendAction(std::move(createAction));
    }
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadDeformKeyFileV12Filter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  ReadDeformKeyFileV12InputValues inputValues;

  inputValues.InputFilePath = s_HeaderCache[m_InstanceId].inputFile;
  inputValues.QuadGeomPath = filterArgs.value<DataPath>(k_QuadGeomPath_Key);
  inputValues.CellAMPath = filterArgs.value<DataPath>(k_QuadGeomPath_Key).createChildPath(filterArgs.value<std::string>(k_CellAMName_Key));
  inputValues.VertexAMPath = filterArgs.value<DataPath>(k_QuadGeomPath_Key).createChildPath(filterArgs.value<std::string>(k_VertexAMName_Key));

  return ReadDeformKeyFileV12(dataStructure, messageHandler, shouldCancel, &inputValues)(true);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_DEFORMInputFileKey = "DEFORMInputFile";
constexpr StringLiteral k_VerboseOutputKey = "VerboseOutput";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_VertexAttributeMatrixNameKey = "VertexAttributeMatrixName";
constexpr StringLiteral k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
} // namespace SIMPL
} // namespace

Result<Arguments> ReadDeformKeyFileV12Filter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadDeformKeyFileV12Filter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_DEFORMInputFileKey, k_InputFilePath_Key));
  // Verbose Output parameter is not applicable in NX
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToDataPathFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_QuadGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_VertexAttributeMatrixNameKey, k_VertexAMName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixNameKey, k_CellAMName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
