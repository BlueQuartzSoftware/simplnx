#include "WriteStlFileFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteStlFile.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string WriteStlFileFilter::name() const
{
  return FilterTraits<WriteStlFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string WriteStlFileFilter::className() const
{
  return FilterTraits<WriteStlFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid WriteStlFileFilter::uuid() const
{
  return FilterTraits<WriteStlFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string WriteStlFileFilter::humanName() const
{
  return "Write STL Files from Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> WriteStlFileFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "Triangles", "SurfaceMesh"};
}

//------------------------------------------------------------------------------
Parameters WriteStlFileFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_GroupingType_Key, "File Grouping Type", "How to partition the stl files", to_underlying(GroupingType::Features),
                                                                    ChoicesParameter::Choices{"Features", "Phases and Features", "Single File", "Part Index"})); // sequence dependent DO NOT REORDER
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputStlDirectory_Key, "Output STL Directory", "Directory to dump the STL file(s) to", fs::path(),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(std::make_unique<StringParameter>(k_OutputStlPrefix_Key, "Output STL File Prefix",
                                                  "The prefix name of created files (other values will be appended later - including the .stl extension)", "Triangle"));

  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputStlFile_Key, "Output STL File", "STL File to dump the Triangle Geometry to", fs::path(),
                                                          FileSystemPathParameter::ExtensionsType{".stl"}, FileSystemPathParameter::PathType::OutputFile, false));

  params.insertSeparator(Parameters::Separator{"Input Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeomPath_Key, "Selected Triangle Geometry", "The geometry to print", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Triangle}));

  params.insertSeparator(Parameters::Separator{"Input Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsPath_Key, "Face labels", "The triangle feature ids array to order/index files by", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesPath_Key, "Face Phases", "The face phases array (the phase on each side of each triangle) to further order/index files by",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_PartNumberPath_Key, "Part Numbers", "The Part Numbers to order/index files by", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  // link params -- GroupingType enum is stored in the algorithm header [WriteStlFile.hpp]
  //------------ Group by Features -------------
  params.linkParameters(k_GroupingType_Key, k_OutputStlDirectory_Key, to_underlying(GroupingType::Features));
  params.linkParameters(k_GroupingType_Key, k_OutputStlPrefix_Key, to_underlying(GroupingType::Features));
  params.linkParameters(k_GroupingType_Key, k_FeatureIdsPath_Key, to_underlying(GroupingType::Features));

  //------- Group by Features and Phases -------
  params.linkParameters(k_GroupingType_Key, k_OutputStlDirectory_Key, to_underlying(GroupingType::FeaturesAndPhases));
  params.linkParameters(k_GroupingType_Key, k_OutputStlPrefix_Key, to_underlying(GroupingType::FeaturesAndPhases));
  params.linkParameters(k_GroupingType_Key, k_FeatureIdsPath_Key, to_underlying(GroupingType::FeaturesAndPhases));
  params.linkParameters(k_GroupingType_Key, k_FeaturePhasesPath_Key, to_underlying(GroupingType::FeaturesAndPhases));

  //--------------- Single File ----------------
  params.linkParameters(k_GroupingType_Key, k_OutputStlFile_Key, to_underlying(GroupingType::SingleFile));

  //--------------- Part Number ----------------
  params.linkParameters(k_GroupingType_Key, k_OutputStlDirectory_Key, to_underlying(GroupingType::PartNumber));
  params.linkParameters(k_GroupingType_Key, k_OutputStlPrefix_Key, to_underlying(GroupingType::PartNumber));
  params.linkParameters(k_GroupingType_Key, k_PartNumberPath_Key, to_underlying(GroupingType::PartNumber));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType WriteStlFileFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType WriteStlFileFilter::clone() const
{
  return std::make_unique<WriteStlFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WriteStlFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                           const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pGroupingTypeValue = static_cast<GroupingType>(filterArgs.value<ChoicesParameter::ValueType>(k_GroupingType_Key));
  auto pOutputStlDirectoryValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputStlDirectory_Key);
  auto pTriangleGeomPathValue = filterArgs.value<DataPath>(k_TriangleGeomPath_Key);
  auto pFeatureIdsPathValue = filterArgs.value<DataPath>(k_FeatureIdsPath_Key);
  auto pFeaturePhasesPathValue = filterArgs.value<DataPath>(k_FeaturePhasesPath_Key);
  auto pPartNumberPathValue = filterArgs.value<DataPath>(k_PartNumberPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  const auto* triangleGeomPtr = dataStructure.getDataAs<TriangleGeom>(pTriangleGeomPathValue);
  if(triangleGeomPtr == nullptr)
  {
    return MakePreflightErrorResult(-27870, fmt::format("Triangle Geometry doesn't exist at: {}", pTriangleGeomPathValue.toString()));
  }

  if(triangleGeomPtr->getNumberOfFaces() > std::numeric_limits<int32>::max())
  {
    if(pGroupingTypeValue == GroupingType::SingleFile)
    {
      std::string ss =
          fmt::format("The number of triangles is {}, but the STL specification only supports triangle counts up to {}. Since single file was selected this means overflow files WILL be generated.",
                      triangleGeomPtr->getNumberOfFaces(), std::numeric_limits<int32>::max());
      resultOutputActions.warnings().emplace_back(Warning{.code = -27871, .message = ss});
    }
    else
    {
      std::string ss = fmt::format("The number of triangles is {}, but the STL specification only supports triangle counts up to {}. This MAY cause some groups to generate overflow files.",
                                   triangleGeomPtr->getNumberOfFaces(), std::numeric_limits<int32>::max());
      resultOutputActions.warnings().emplace_back(Warning{.code = -27875, .message = ss});
    }
  }

  if(pGroupingTypeValue == GroupingType::Features || pGroupingTypeValue == GroupingType::FeaturesAndPhases)
  {
    const auto* featureIdsPtr = dataStructure.getDataAs<Int32Array>(pFeatureIdsPathValue);
    if(featureIdsPtr == nullptr)
    {
      return MakePreflightErrorResult(-27873, fmt::format("Feature Ids Array doesn't exist at: {}", pFeatureIdsPathValue.toString()));
    }
    if(featureIdsPtr->getNumberOfTuples() != triangleGeomPtr->getNumberOfFaces())
    {
      return MakePreflightErrorResult(-27888, fmt::format("Array '{}' has {} tuples but Triangle Geometry '{}' has {} faces; it must have one tuple per face.", pFeatureIdsPathValue.toString(),
                                                          featureIdsPtr->getNumberOfTuples(), pTriangleGeomPathValue.toString(), triangleGeomPtr->getNumberOfFaces()));
    }
  }

  if(pGroupingTypeValue == GroupingType::FeaturesAndPhases)
  {
    const auto* featurePhasesPtr = dataStructure.getDataAs<Int32Array>(pFeaturePhasesPathValue);
    if(featurePhasesPtr == nullptr)
    {
      return MakePreflightErrorResult(-27872, fmt::format("Face Phases Array doesn't exist at: {}", pFeaturePhasesPathValue.toString()));
    }
    if(featurePhasesPtr->getNumberOfTuples() != triangleGeomPtr->getNumberOfFaces())
    {
      return MakePreflightErrorResult(-27888, fmt::format("Array '{}' has {} tuples but Triangle Geometry '{}' has {} faces; it must have one tuple per face.", pFeaturePhasesPathValue.toString(),
                                                          featurePhasesPtr->getNumberOfTuples(), pTriangleGeomPathValue.toString(), triangleGeomPtr->getNumberOfFaces()));
    }
  }

  if(pGroupingTypeValue == GroupingType::PartNumber)
  {
    const auto* partNumbersPtr = dataStructure.getDataAs<Int32Array>(pPartNumberPathValue);
    if(partNumbersPtr == nullptr)
    {
      return MakePreflightErrorResult(-27874, fmt::format("Part Number Array doesn't exist at: {}", pPartNumberPathValue.toString()));
    }
    if(partNumbersPtr->getNumberOfTuples() != triangleGeomPtr->getNumberOfFaces())
    {
      return MakePreflightErrorResult(-27888, fmt::format("Array '{}' has {} tuples but Triangle Geometry '{}' has {} faces; it must have one tuple per face.", pPartNumberPathValue.toString(),
                                                          partNumbersPtr->getNumberOfTuples(), pTriangleGeomPathValue.toString(), triangleGeomPtr->getNumberOfFaces()));
    }
  }

  // Return both the resultOutputActions via std::move()
  return {.outputActions = std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> WriteStlFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  WriteStlFileInputValues inputValues;

  inputValues.GroupingType = filterArgs.value<ChoicesParameter::ValueType>(k_GroupingType_Key);
  inputValues.OutputStlFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputStlFile_Key);
  inputValues.OutputStlDirectory = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputStlDirectory_Key);
  inputValues.OutputStlPrefix = filterArgs.value<StringParameter::ValueType>(k_OutputStlPrefix_Key);
  inputValues.FeatureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsPath_Key);
  inputValues.FeaturePhasesPath = filterArgs.value<DataPath>(k_FeaturePhasesPath_Key);
  inputValues.TriangleGeomPath = filterArgs.value<DataPath>(k_TriangleGeomPath_Key);
  inputValues.PartNumberPath = filterArgs.value<DataPath>(k_PartNumberPath_Key);

  return WriteStlFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_OutputStlDirectoryKey = "OutputStlDirectory";
constexpr StringLiteralType k_OutputStlPrefixKey = "OutputStlPrefix";
constexpr StringLiteralType k_SurfaceMeshFaceLabelsArrayPathKey = "SurfaceMeshFaceLabelsArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> WriteStlFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WriteStlFileFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::OutputFileFilterParameterConverter>(args, json, simpl::k_OutputStlDirectoryKey, k_OutputStlDirectory_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_OutputStlPrefixKey, k_OutputStlPrefix_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_SurfaceMeshFaceLabelsArrayPathKey, k_TriangleGeomPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SurfaceMeshFaceLabelsArrayPathKey, k_FeatureIdsPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
