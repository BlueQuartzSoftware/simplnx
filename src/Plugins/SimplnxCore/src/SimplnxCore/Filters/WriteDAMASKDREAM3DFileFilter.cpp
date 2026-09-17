#include "WriteDAMASKDREAM3DFileFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/WriteDAMASKDREAM3DFile.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataPathSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

#include <cmath>
#include <filesystem>
#include <optional>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
std::optional<float64> UnitScaleToMeters(IGeometry::LengthUnit unit)
{
  using LengthUnit = IGeometry::LengthUnit;
  switch(unit)
  {
  case LengthUnit::Yoctometer:
    return 1.0e-24;
  case LengthUnit::Zeptometer:
    return 1.0e-21;
  case LengthUnit::Attometer:
    return 1.0e-18;
  case LengthUnit::Femtometer:
    return 1.0e-15;
  case LengthUnit::Picometer:
    return 1.0e-12;
  case LengthUnit::Nanometer:
    return 1.0e-9;
  case LengthUnit::Micrometer:
    return 1.0e-6;
  case LengthUnit::Millimeter:
    return 1.0e-3;
  case LengthUnit::Centimeter:
    return 1.0e-2;
  case LengthUnit::Decimeter:
    return 1.0e-1;
  case LengthUnit::Meter:
    return 1.0;
  case LengthUnit::Decameter:
    return 1.0e1;
  case LengthUnit::Hectometer:
    return 1.0e2;
  case LengthUnit::Kilometer:
    return 1.0e3;
  case LengthUnit::Megameter:
    return 1.0e6;
  case LengthUnit::Gigameter:
    return 1.0e9;
  case LengthUnit::Terameter:
    return 1.0e12;
  case LengthUnit::Petameter:
    return 1.0e15;
  case LengthUnit::Exameter:
    return 1.0e18;
  case LengthUnit::Zettameter:
    return 1.0e21;
  case LengthUnit::Yottameter:
    return 1.0e24;
  case LengthUnit::Angstrom:
    return 1.0e-10;
  case LengthUnit::Mil:
    return 2.54e-5;
  case LengthUnit::Inch:
    return 2.54e-2;
  case LengthUnit::Foot:
    return 3.048e-1;
  case LengthUnit::Mile:
    return 1609.344;
  case LengthUnit::Fathom:
    return 1.8288;
  case LengthUnit::Unspecified:
  case LengthUnit::Unknown:
    return std::nullopt;
  }
  return std::nullopt;
}

Result<float64> ResolveScaleToMeters(IGeometry::LengthUnit unit, float64 userScale)
{
  if(const auto unitScale = UnitScaleToMeters(unit); unitScale.has_value())
  {
    return {*unitScale};
  }
  if(!std::isfinite(userScale) || userScale <= 0.0)
  {
    return MakeErrorResult<float64>(-12104, fmt::format("The selected Image Geometry unit is '{}', so Scale to Meters must be finite and greater than zero. The provided value is {}.",
                                                        IGeometry::LengthUnitToString(unit), userScale));
  }
  return {userScale};
}

Result<> ValidateCellTupleCount(const DataStructure& dataStructure, const DataPath& arrayPath, const DataPath& geometryPath, usize cellCount)
{
  const auto& array = dataStructure.getDataRefAs<IDataArray>(arrayPath);
  if(array.getNumberOfTuples() != cellCount)
  {
    return MakeErrorResult(-12105, fmt::format("Array '{}' has {} tuples, but Image Geometry '{}' has {} cells. Select an array with one tuple per cell.", arrayPath.toString(),
                                               array.getNumberOfTuples(), geometryPath.toString(), cellCount));
  }
  return {};
}
} // namespace

std::string WriteDAMASKDREAM3DFileFilter::name() const
{
  return FilterTraits<WriteDAMASKDREAM3DFileFilter>::name.str();
}

std::string WriteDAMASKDREAM3DFileFilter::className() const
{
  return FilterTraits<WriteDAMASKDREAM3DFileFilter>::className;
}

Uuid WriteDAMASKDREAM3DFileFilter::uuid() const
{
  return FilterTraits<WriteDAMASKDREAM3DFileFilter>::uuid;
}

std::string WriteDAMASKDREAM3DFileFilter::humanName() const
{
  return "Write DAMASK Import File";
}

std::vector<std::string> WriteDAMASKDREAM3DFileFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "DAMASK", "DREAM3D", "HDF5"};
}

Parameters WriteDAMASKDREAM3DFileFilter::parameters() const
{
  Parameters params;
  const ArraySelectionParameter::AllowedTypes floatTypes = {DataType::float32, DataType::float64};
  const ArraySelectionParameter::AllowedTypes integerTypes = {DataType::int8, DataType::int16, DataType::int32, DataType::int64, DataType::uint8, DataType::uint16, DataType::uint32, DataType::uint64};

  params.insertSeparator(Parameters::Separator{"Representation"});
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_Representation_Key, "Material Representation", "Write cell-level or segmented feature-level data.", 0, ChoicesParameter::Choices{"Pointwise", "Grainwise"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_WritePhaseNames_Key, "Write Phase Names", "Write the selected phase names with DAMASK's canonical name.", false));

  params.insertSeparator(Parameters::Separator{"Input Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Input Image Geometry", "The Image Geometry to export.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<Float64Parameter>(k_ScaleToMeters_Key, "Scale to Meters", "The scale for Unknown or Unspecified geometry units.", 1.0));

  params.insertSeparator(Parameters::Separator{"Pointwise Input"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Cell Euler Angles", "The Bunge Euler angles in radians for each cell.", DataPath{}, floatTypes,
                                                          ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "The phase index for each cell.", DataPath{}, integerTypes,
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.linkParameters(k_Representation_Key, k_CellEulerAnglesArrayPath_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_Representation_Key, k_CellPhasesArrayPath_Key, std::make_any<ChoicesParameter::ValueType>(0));

  params.insertSeparator(Parameters::Separator{"Grainwise Input"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "The segmented feature index for each cell.", DataPath{}, integerTypes,
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureEulerAnglesArrayPath_Key, "Feature Euler Angles", "The Bunge Euler angles in radians for each feature.", DataPath{}, floatTypes,
                                                          ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Feature Phases", "The phase index for each feature.", DataPath{}, integerTypes,
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.linkParameters(k_Representation_Key, k_FeatureIdsArrayPath_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_Representation_Key, k_FeatureEulerAnglesArrayPath_Key, std::make_any<ChoicesParameter::ValueType>(1));
  params.linkParameters(k_Representation_Key, k_FeaturePhasesArrayPath_Key, std::make_any<ChoicesParameter::ValueType>(1));

  params.insertSeparator(Parameters::Separator{"Optional Phase Names"});
  params.insert(std::make_unique<DataPathSelectionParameter>(k_PhaseNamesArrayPath_Key, "Phase Names", "The ensemble-level StringArray that contains phase names.", DataPath{}));
  params.linkParameters(k_WritePhaseNames_Key, k_PhaseNamesArrayPath_Key, true);

  params.insertSeparator(Parameters::Separator{"Output"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputFile_Key, "Output DREAM3D File", "The canonical DREAM3D file for DAMASK's importer.", fs::path("damask.dream3d"),
                                                          FileSystemPathParameter::ExtensionsType{".dream3d"}, FileSystemPathParameter::PathType::OutputFile, false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseCompression_Key, "Use HDF5 Compression", "Apply gzip compression to numeric datasets.", true));
  params.insert(std::make_unique<Int32Parameter>(k_CompressionLevel_Key, "Compression Level (1-9)", "The gzip compression level.", 5));
  params.linkParameters(k_UseCompression_Key, k_CompressionLevel_Key, true);

  return params;
}

IFilter::VersionType WriteDAMASKDREAM3DFileFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer WriteDAMASKDREAM3DFileFilter::clone() const
{
  return std::make_unique<WriteDAMASKDREAM3DFileFilter>();
}

IFilter::PreflightResult WriteDAMASKDREAM3DFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const auto outputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  if(outputFile.empty())
  {
    return MakePreflightErrorResult(-12100, "The Output DREAM3D File path is empty. Select a .dream3d output file.");
  }

  const bool useCompression = filterArgs.value<bool>(k_UseCompression_Key);
  const int32 compressionLevel = filterArgs.value<int32>(k_CompressionLevel_Key);
  if(useCompression && (compressionLevel < 1 || compressionLevel > 9))
  {
    return MakePreflightErrorResult(-12101, fmt::format("Compression Level ({}) must be from 1 through 9 when HDF5 compression is enabled.", compressionLevel));
  }

  const auto representation = filterArgs.value<ChoicesParameter::ValueType>(k_Representation_Key);
  if(representation > 1)
  {
    return MakePreflightErrorResult(-12102, fmt::format("Material Representation index ({}) is invalid. Valid choices are [0: Pointwise, 1: Grainwise].", representation));
  }

  const DataPath geometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(geometryPath);
  auto scaleResult = ResolveScaleToMeters(imageGeom.getUnits(), filterArgs.value<float64>(k_ScaleToMeters_Key));
  if(scaleResult.invalid())
  {
    return {ConvertInvalidResult<OutputActions>(std::move(scaleResult))};
  }

  Result<> validationResult;
  if(representation == 0)
  {
    const DataPath eulerPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
    const DataPath phasesPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
    validationResult = ValidateCellTupleCount(dataStructure, eulerPath, geometryPath, imageGeom.getNumberOfCells());
    if(validationResult.valid())
    {
      validationResult = ValidateCellTupleCount(dataStructure, phasesPath, geometryPath, imageGeom.getNumberOfCells());
    }
  }
  else
  {
    const DataPath featureIdsPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
    const DataPath featureEulerPath = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
    const DataPath featurePhasesPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
    validationResult = ValidateCellTupleCount(dataStructure, featureIdsPath, geometryPath, imageGeom.getNumberOfCells());
    if(validationResult.valid())
    {
      const auto& featureEuler = dataStructure.getDataRefAs<IDataArray>(featureEulerPath);
      const auto& featurePhases = dataStructure.getDataRefAs<IDataArray>(featurePhasesPath);
      if(featureEuler.getNumberOfTuples() != featurePhases.getNumberOfTuples())
      {
        validationResult =
            MakeErrorResult(-12103, fmt::format("Feature Euler Angles array '{}' has {} tuples, but Feature Phases array '{}' has {} tuples. Select feature arrays with equal tuple counts.",
                                                featureEulerPath.toString(), featureEuler.getNumberOfTuples(), featurePhasesPath.toString(), featurePhases.getNumberOfTuples()));
      }
    }
  }
  if(validationResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(validationResult), {})};
  }

  if(filterArgs.value<bool>(k_WritePhaseNames_Key))
  {
    const DataPath phaseNamesPath = filterArgs.value<DataPath>(k_PhaseNamesArrayPath_Key);
    if(dataStructure.getDataAs<StringArray>(phaseNamesPath) == nullptr)
    {
      return MakePreflightErrorResult(-12106, fmt::format("The selected Phase Names object '{}' is not a StringArray. Select an ensemble-level StringArray.", phaseNamesPath.toString()));
    }
  }

  const float32 scale = static_cast<float32>(scaleResult.value());
  const FloatVec3 spacing = imageGeom.getSpacing() * scale;
  const FloatVec3 origin = imageGeom.getOrigin() * scale;
  std::vector<PreflightValue> updatedValues;
  updatedValues.push_back(
      {"Canonical Output", representation == 0 ? "DataContainer/CellData/{EulerAngles, Phases}" : "DataContainer/CellData/FeatureIds and DataContainer/CellFeatureData/{EulerAngles, Phases}"});
  updatedValues.push_back({"Output Geometry in Meters", GeometryHelpers::Description::GenerateGeometryInfo(imageGeom.getDimensions(), spacing, origin, IGeometry::LengthUnit::Meter)});
  return {Result<OutputActions>{}, std::move(updatedValues)};
}

Result<> WriteDAMASKDREAM3DFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const DataPath geometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(geometryPath);
  auto scaleResult = ResolveScaleToMeters(imageGeom.getUnits(), filterArgs.value<float64>(k_ScaleToMeters_Key));
  if(scaleResult.invalid())
  {
    return ConvertResult(std::move(scaleResult));
  }

  WriteDAMASKDREAM3DFileInputValues inputValues;
  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.UseCompression = filterArgs.value<bool>(k_UseCompression_Key);
  inputValues.CompressionLevel = filterArgs.value<int32>(k_CompressionLevel_Key);
  inputValues.Representation = filterArgs.value<ChoicesParameter::ValueType>(k_Representation_Key);
  inputValues.WritePhaseNames = filterArgs.value<bool>(k_WritePhaseNames_Key);
  inputValues.ScaleToMeters = scaleResult.value();
  inputValues.ImageGeometryPath = geometryPath;
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.FeatureEulerAnglesArrayPath = filterArgs.value<DataPath>(k_FeatureEulerAnglesArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.PhaseNamesArrayPath = filterArgs.value<DataPath>(k_PhaseNamesArrayPath_Key);
  return WriteDAMASKDREAM3DFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
