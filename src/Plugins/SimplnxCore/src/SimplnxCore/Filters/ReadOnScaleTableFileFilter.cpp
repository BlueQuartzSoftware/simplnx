#include "ReadOnScaleTableFileFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ReadOnScaleTableFile.hpp"

#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateRectGridGeometryAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/format.h>

#include <atomic>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
constexpr int32 k_InputFileDoesNotExistError = -12050;
constexpr int32 k_FileAccessError = -12051;

static std::atomic_int32_t s_InstanceId = 0;
static std::map<int32, OnScaleTableFileHeader> s_HeaderCache;

const std::array<std::string, 3> k_BoundsNames = {"X Bounds", "Y Bounds", "Z Bounds"};
} // namespace

namespace nx::core
{
ReadOnScaleTableFileFilter::ReadOnScaleTableFileFilter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  s_HeaderCache[m_InstanceId] = {};
}

ReadOnScaleTableFileFilter::~ReadOnScaleTableFileFilter() noexcept
{
  s_HeaderCache.erase(m_InstanceId);
}

std::string ReadOnScaleTableFileFilter::name() const
{
  return FilterTraits<ReadOnScaleTableFileFilter>::name.str();
}

std::string ReadOnScaleTableFileFilter::className() const
{
  return FilterTraits<ReadOnScaleTableFileFilter>::className;
}

Uuid ReadOnScaleTableFileFilter::uuid() const
{
  return FilterTraits<ReadOnScaleTableFileFilter>::uuid;
}

std::string ReadOnScaleTableFileFilter::humanName() const
{
  return "Read OnScale Table File";
}

std::vector<std::string> ReadOnScaleTableFileFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "OnScale", "PZFLEX", "Simulation"};
}

Parameters ReadOnScaleTableFileFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "The OnScale table file to import.", fs::path(""), FileSystemPathParameter::ExtensionsType{".flxtbl"},
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Fallback Origin", "The origin for each axis that is absent from the file.", VectorFloat32Parameter::ValueType{0.0F, 0.0F, 0.0F},
                                                         VectorFloat32Parameter::NamesType{"X", "Y", "Z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Fallback Spacing", "The spacing for each axis that is absent from the file.",
                                                         VectorFloat32Parameter::ValueType{1.0F, 1.0F, 1.0F}, VectorFloat32Parameter::NamesType{"X", "Y", "Z"}));

  params.insertSeparator(Parameters::Separator{"Created Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedRectGridGeometryPath_Key, "Rectilinear Grid Geometry", "The path for the created Rectilinear Grid Geometry.",
                                                             DataPath({"OnScale Volume"})));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "The name of the created cell Attribute Matrix.", "Cell Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "The name of the created material-index Data Array.", "FeatureIds"));

  params.insertSeparator(Parameters::Separator{"Created Material Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_PhaseAttributeMatrixName_Key, "Phase Attribute Matrix", "The name of the created material Attribute Matrix.", "Material Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MaterialNamesArrayName_Key, "Material Names", "The name of the created material-name StringArray.", "Material Names"));

  return params;
}

IFilter::VersionType ReadOnScaleTableFileFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer ReadOnScaleTableFileFilter::clone() const
{
  return std::make_unique<ReadOnScaleTableFileFilter>();
}

IFilter::PreflightResult ReadOnScaleTableFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const auto inputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  std::error_code fileError;
  if(!fs::exists(inputFile, fileError))
  {
    return MakePreflightErrorResult(k_InputFileDoesNotExistError, fmt::format("The input OnScale table file '{}' does not exist.", inputFile.string()));
  }

  const auto writeTime = fs::last_write_time(inputFile, fileError);
  if(fileError)
  {
    return MakePreflightErrorResult(k_FileAccessError, fmt::format("The modification time for input OnScale table file '{}' could not be read: {}", inputFile.string(), fileError.message()));
  }

  auto& cachedHeader = s_HeaderCache[m_InstanceId];
  if(cachedHeader.InputFile != inputFile || cachedHeader.TimeStamp != writeTime)
  {
    ReadOnScaleTableFileInputValues inputValues;
    inputValues.InputFile = inputFile;
    DataStructure throwaway;
    ReadOnScaleTableFile algorithm(throwaway, messageHandler, shouldCancel, &inputValues);
    auto headerResult = algorithm.readHeader();
    if(headerResult.invalid())
    {
      return {ConvertInvalidResult<OutputActions>(std::move(headerResult))};
    }
    cachedHeader = std::move(headerResult.value());
    cachedHeader.TimeStamp = writeTime;
  }

  const auto geometryPath = filterArgs.value<DataPath>(k_CreatedRectGridGeometryPath_Key);
  const auto cellDataName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  const auto featureIdsName = filterArgs.value<std::string>(k_FeatureIdsArrayName_Key);
  const auto phaseDataName = filterArgs.value<std::string>(k_PhaseAttributeMatrixName_Key);
  const auto materialNamesName = filterArgs.value<std::string>(k_MaterialNamesArrayName_Key);
  const ShapeType cellShape = {cachedHeader.BoundsCounts[2] - 1, cachedHeader.BoundsCounts[1] - 1, cachedHeader.BoundsCounts[0] - 1};

  Result<OutputActions> resultOutputActions;
  resultOutputActions.value().appendAction(std::make_unique<CreateRectGridGeometryAction>(geometryPath, cachedHeader.BoundsCounts[0], cachedHeader.BoundsCounts[1], cachedHeader.BoundsCounts[2],
                                                                                          cellDataName, k_BoundsNames[0], k_BoundsNames[1], k_BoundsNames[2]));
  resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::int32, cellShape, ShapeType{1}, geometryPath.createChildPath(cellDataName).createChildPath(featureIdsName)));
  const DataPath phaseDataPath = geometryPath.createChildPath(phaseDataName);
  resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(phaseDataPath, ShapeType{cachedHeader.NameCount}));
  resultOutputActions.value().appendAction(std::make_unique<CreateStringArrayAction>(ShapeType{cachedHeader.NameCount}, phaseDataPath.createChildPath(materialNamesName), ""));

  std::vector<PreflightValue> updatedValues;
  updatedValues.push_back({"Rect Grid Geometry Info",
                           fmt::format("Bounds: X={}, Y={}, Z={}\nDimensions: X={}, Y={}, Z={}\nMaterial names: {}", cachedHeader.BoundsCounts[0], cachedHeader.BoundsCounts[1],
                                       cachedHeader.BoundsCounts[2], cachedHeader.BoundsCounts[0] - 1, cachedHeader.BoundsCounts[1] - 1, cachedHeader.BoundsCounts[2] - 1, cachedHeader.NameCount)});
  return {std::move(resultOutputActions), std::move(updatedValues)};
}

Result<> ReadOnScaleTableFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ReadOnScaleTableFileInputValues inputValues;
  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.FallbackOrigin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.FallbackSpacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.RectGridGeometryPath = filterArgs.value<DataPath>(k_CreatedRectGridGeometryPath_Key);
  inputValues.CellAttributeMatrixPath = inputValues.RectGridGeometryPath.createChildPath(filterArgs.value<std::string>(k_CellAttributeMatrixName_Key));
  inputValues.FeatureIdsArrayPath = inputValues.CellAttributeMatrixPath.createChildPath(filterArgs.value<std::string>(k_FeatureIdsArrayName_Key));
  inputValues.PhaseAttributeMatrixPath = inputValues.RectGridGeometryPath.createChildPath(filterArgs.value<std::string>(k_PhaseAttributeMatrixName_Key));
  inputValues.MaterialNamesArrayPath = inputValues.PhaseAttributeMatrixPath.createChildPath(filterArgs.value<std::string>(k_MaterialNamesArrayName_Key));
  inputValues.Header = s_HeaderCache[m_InstanceId];

  return ReadOnScaleTableFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputFileKey = "InputFile";
constexpr StringLiteral k_OriginKey = "Origin";
constexpr StringLiteral k_SpacingKey = "Spacing";
constexpr StringLiteral k_VolumeDataContainerNameKey = "VolumeDataContainerName";
constexpr StringLiteral k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteral k_FeatureIdsArrayNameKey = "FeatureIdsArrayName";
constexpr StringLiteral k_PhaseAttributeMatrixNameKey = "PhaseAttributeMatrixName";
constexpr StringLiteral k_MaterialNameArrayNameKey = "MaterialNameArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ReadOnScaleTableFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadOnScaleTableFileFilter().getDefaultArguments();
  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_InputFileKey, k_InputFile_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_OriginKey, k_Origin_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_SpacingKey, k_Spacing_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DCPathBuilderFilterParameterConverter>(args, json, SIMPL::k_VolumeDataContainerNameKey, k_CreatedRectGridGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixNameKey, k_CellAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayNameKey, k_FeatureIdsArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_PhaseAttributeMatrixNameKey, k_PhaseAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_MaterialNameArrayNameKey, k_MaterialNamesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));
  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
