#include "ConvertOrientationsToVertexGeometryFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ConvertOrientationsToVertexGeometry.hpp"

#include <EbsdLib/Core/EbsdDataArray.hpp>
#include <EbsdLib/OrientationMath/OrientationConverter.hpp>

#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Actions/CopyDataObjectAction.hpp"
#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Filter/Actions/MoveDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace fs = std::filesystem;
using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ConvertOrientationsToVertexGeometryFilter::name() const
{
  return FilterTraits<ConvertOrientationsToVertexGeometryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertOrientationsToVertexGeometryFilter::className() const
{
  return FilterTraits<ConvertOrientationsToVertexGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertOrientationsToVertexGeometryFilter::uuid() const
{
  return FilterTraits<ConvertOrientationsToVertexGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertOrientationsToVertexGeometryFilter::humanName() const
{
  return "Convert Orientations To Rodrigues Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertOrientationsToVertexGeometryFilter::defaultTags() const
{
  return {className(), "rodrigues", "orientation", "quaternion", "quat", "stereographic", "euler", "axis", "angle", "homochoric", "cubochoric", "Vertex", "Fundamental Zone"};
}

//------------------------------------------------------------------------------
Parameters ConvertOrientationsToVertexGeometryFilter::parameters() const
{
  Parameters params;

  using OrientationConverterType = ebsdlib::OrientationConverter<EbsdDataArray<float32>, float32>;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ChoicesParameter>(k_InputType_Key, "Input Orientation Type", "Specifies the incoming orientation representation enumeration index", 0,
                                                   OrientationConverterType::GetOrientationTypeStrings<ChoicesParameter::Choices>()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputOrientationArrayPath_Key, "Input Orientations", "The complete path to the incoming orientation representation data array", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32, DataType::float64}, ArraySelectionParameter::AllowedComponentShapes{{3}, {4}, {9}}));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CopyVertexPaths_Key, "Copy Vertex Arrays",
                                                               "Paths to vertex-related DataArrays that should be copied to the output rodrigues vertex geometry", std::vector<DataPath>{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, MultiArraySelectionParameter::AllowedDataTypes{}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ConvertToFundamentalZone_Key, "Convert To Fundamental Zone", "Convert the orientations to the fundamental zone.", false));

  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Input Phases", "The phases of the data. The data should be the indices into the Crystal Structures Data Array.",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Vertex Geometry"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_VertexGeometryPath_Key, "Output Vertex Geometry", "The complete path to the vertex geometry that will be created", DataPath({"Vertex Geometry"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexAttrMatrixName_Key, "Output Vertex Attribute Matrix Name", "The name of the vertex attribute matrix that will be created",
                                                          VertexGeom::k_VertexAttributeMatrixName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SharedVertexListName_Key, "Output Shared Vertex List Name", "The name of the shared vertex list that will be created",
                                                          VertexGeom::k_SharedVertexListName));

  params.linkParameters(k_ConvertToFundamentalZone_Key, k_CellPhasesArrayPath_Key, true);
  params.linkParameters(k_ConvertToFundamentalZone_Key, k_CrystalStructuresArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ConvertOrientationsToVertexGeometryFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertOrientationsToVertexGeometryFilter::clone() const
{
  return std::make_unique<ConvertOrientationsToVertexGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertOrientationsToVertexGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto inputRepType = static_cast<ebsdlib::orientations::Type>(filterArgs.value<ChoicesParameter::ValueType>(k_InputType_Key));
  auto inputOrientationsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_InputOrientationArrayPath_Key);
  auto vertexPathsToCopy = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_CopyVertexPaths_Key);
  auto convertToFundamentalZone = filterArgs.value<BoolParameter::ValueType>(k_ConvertToFundamentalZone_Key);
  auto cellPhasesArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_CellPhasesArrayPath_Key);
  auto crystalStructuresArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_CrystalStructuresArrayPath_Key);
  auto outputVertexGeometryPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_VertexGeometryPath_Key);
  auto outputVertexAttrMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_VertexAttrMatrixName_Key);
  auto outputSharedVertexListName = filterArgs.value<DataObjectNameParameter::ValueType>(k_SharedVertexListName_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  if(static_cast<int>(inputRepType) < 0 || inputRepType >= ebsdlib::orientations::Type::Unknown)
  {
    return {MakeErrorResult<OutputActions>(-1001, fmt::format("Input Representation Type must be a value from 0 to 6. '{}'", fmt::underlying(inputRepType)))};
  }

  auto& inputOrientationsArray = dataStructure.getDataRefAs<Float32Array>(inputOrientationsArrayPath);

  std::vector<usize> inputCompShape = inputOrientationsArray.getComponentShape();
  if(inputCompShape.size() > 1)
  {
    return {MakeErrorResult<OutputActions>(-1002, fmt::format("Input Component Shape has multiple dimensions. It can only have 1 dimension. '{}'", inputCompShape.size()))};
  }

  using OrientationConverterType = ebsdlib::OrientationConverter<EbsdDataArray<float32>, float32>;
  auto representationNames = OrientationConverterType::GetOrientationTypeStrings<std::vector<std::string>>();
  auto representationElementCount = OrientationConverterType::GetComponentCounts<std::vector<usize>>();
  if(inputCompShape[0] != representationElementCount[static_cast<usize>(inputRepType)])
  {
    std::stringstream message;
    message << "Number of components for input array is not correct for input representation type. " << representationNames[static_cast<usize>(inputRepType)] << " should have "
            << representationElementCount[static_cast<usize>(inputRepType)] << " components but the selected input array has " << inputCompShape[0];
    return {MakeErrorResult<OutputActions>(-1003, message.str())};
  }

  if(convertToFundamentalZone)
  {
    const auto& phaseData = dataStructure.getDataRefAs<Int32Array>(cellPhasesArrayPath);
    if(phaseData.getNumberOfTuples() != inputOrientationsArray.getNumberOfTuples())
    {
      return {MakeErrorResult<OutputActions>(
          -49001, fmt::format("Orientation and Phase Arrays must have the same number of tuples. '{} != {}'", inputOrientationsArray.getNumberOfTuples(), phaseData.getNumberOfTuples()))};
    }
  }

  // Create the Vertex Geometry
  auto createVertexGeometryAction =
      std::make_unique<CreateVertexGeometryAction>(outputVertexGeometryPath, inputOrientationsArray.getNumberOfTuples(), outputVertexAttrMatrixName, outputSharedVertexListName);
  resultOutputActions.value().appendAction(std::move(createVertexGeometryAction));
  DataPath vertexAttrMatrixPath = outputVertexGeometryPath.createChildPath(outputVertexAttrMatrixName);
  for(const auto& vertexPathToCopy : vertexPathsToCopy)
  {
    auto& vertexDataArray = dataStructure.getDataRefAs<IDataArray>(vertexPathToCopy);
    DataType type = vertexDataArray.getDataType();
    DataPath copyPath = vertexAttrMatrixPath.createChildPath(vertexDataArray.getName());
    auto numTuples = vertexDataArray.getNumberOfTuples();
    auto components = vertexDataArray.getNumberOfComponents();
    const std::string dataStoreFormat = vertexDataArray.getDataFormat();

    if(numTuples != inputOrientationsArray.getNumberOfTuples())
    {
      return {MakeErrorResult<OutputActions>(-1004, fmt::format("Array at path {} only has {} tuples, but it MUST have {} tuples to be copied into output vertex attribute matrix at path {}!",
                                                                vertexPathToCopy.toString(), numTuples, inputOrientationsArray.getNumberOfTuples(), vertexAttrMatrixPath.toString()))};
    }

    auto action = std::make_unique<CopyDataObjectAction>(vertexPathToCopy, copyPath, std::vector<DataPath>{});
    resultOutputActions.value().appendAction(std::move(action));

    //    auto moveDataAction = std::make_unique<MoveDataAction>(vertexPathToCopy, vertexAttrMatrixPath);
    //    resultOutputActions.value().appendAction(std::move(moveDataAction));

    //    auto action = std::make_unique<CreateArrayAction>(type, std::vector<usize>{numTuples}, std::vector<usize>{components}, copyPath, dataStoreFormat);
    //    resultOutputActions.value().appendAction(std::move(action));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ConvertOrientationsToVertexGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ConvertOrientationsToVertexGeometryInputValues inputValues;

  inputValues.InputOrientationType = static_cast<ebsdlib::orientations::Type>(filterArgs.value<ChoicesParameter::ValueType>(k_InputType_Key));
  inputValues.InputOrientationArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_InputOrientationArrayPath_Key);
  inputValues.CopyVertexArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_CopyVertexPaths_Key);
  inputValues.ConvertToFundamentalZone = filterArgs.value<BoolParameter::ValueType>(k_ConvertToFundamentalZone_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_CellPhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_CrystalStructuresArrayPath_Key);
  inputValues.OutputVertexGeometryPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_VertexGeometryPath_Key);
  inputValues.OutputVertexAttrMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_VertexAttrMatrixName_Key);
  inputValues.OutputSharedVertexListName = filterArgs.value<DataObjectNameParameter::ValueType>(k_SharedVertexListName_Key);

  return ConvertOrientationsToVertexGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
