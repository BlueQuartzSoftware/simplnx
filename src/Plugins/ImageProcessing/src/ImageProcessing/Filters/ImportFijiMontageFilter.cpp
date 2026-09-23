#include "ImportFijiMontageFilter.hpp"

#include "ImageProcessing/Filters/Algorithms/ReadImage.hpp"
#include "ImageProcessing/utils/FijiMontageUtilities.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateDataGroupAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOFactory.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ColorToGrayScale.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
// Resolves a single tile's geometry + array DataPaths given the montage naming options.
struct TilePaths
{
  DataPath geomPath;
  DataPath arrayPath;
};
TilePaths MakeTilePaths(const fiji::FijiTile& tile, bool parentGroup, const std::string& groupName, const std::string& cellAmName, const std::string& arrayName)
{
  const DataPath geomPath = parentGroup ? DataPath({groupName, tile.imageName}) : DataPath({tile.imageName});
  return {geomPath, geomPath.createChildPath(cellAmName).createChildPath(arrayName)};
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ImportFijiMontageFilter::name() const
{
  return FilterTraits<ImportFijiMontageFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportFijiMontageFilter::className() const
{
  return FilterTraits<ImportFijiMontageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ImportFijiMontageFilter::uuid() const
{
  return FilterTraits<ImportFijiMontageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportFijiMontageFilter::humanName() const
{
  return "Read Fiji Montage";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportFijiMontageFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import", "Fiji", "Montage", "Image"};
}

//------------------------------------------------------------------------------
Parameters ImportFijiMontageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Fiji Configuration File",
                                                          "The Fiji TileConfiguration[.registered].txt file, located alongside the tile images it references.", fs::path(""),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Unit", "The length unit that will be set into every created image geometry.",
                                                   to_underlying(IGeometry::LengthUnit::Micrometer), IGeometry::GetAllLengthUnitStrings()));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeOrigin_Key, "Change Origin", "Rebase the montage so its minimum corner sits at a user-defined origin.", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "The new origin of the montage's minimum corner.", std::vector<float32>(3), std::vector<std::string>{"X", "Y", "Z"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ConvertToGrayScale_Key, "Convert To GrayScale", "Convert each imported RGB tile to a single-channel grayscale image.", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ColorWeights_Key, "Color Weighting", "The luminosity weights used for the grayscale conversion.",
                                                         std::vector<float32>{0.2125f, 0.7154f, 0.0721f}, std::vector<std::string>{"R", "G", "B"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeDataType_Key, "Set Image Data Type", "Cast every imported tile array to a chosen data type.", false));
  params.insert(std::make_unique<ChoicesParameter>(k_ImageDataType_Key, "Output Data Type", "Numeric type of the created image arrays.", 0ULL,
                                                   ChoicesParameter::Choices{"uint8", "uint16", "uint32"})); // Sequence Dependent DO NOT REORDER
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_ParentDataGroup_Key, "Parent Imported Images Under a DataGroup", "Create one parent DataGroup that holds all imported tile geometries.", true));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<StringParameter>(k_DataGroupName_Key, "Name of Created DataGroup", "Name of the parent DataGroup.", "Zen DataGroup"));
  params.insert(std::make_unique<StringParameter>(k_DataContainerPath_Key, "Image Geometry Prefix", "Prefix prepended to each tile's file-stem to name its Image Geometry.", "Mosaic-"));
  params.insert(std::make_unique<StringParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "Name of the created cell Attribute Matrix in each tile geometry.", "Tile Data"));
  params.insert(std::make_unique<StringParameter>(k_ImageDataArrayName_Key, "Image DataArray Name", "Name of the created image data array in each tile geometry.", "Image"));

  params.linkParameters(k_ChangeDataType_Key, k_ImageDataType_Key, true);
  params.linkParameters(k_ChangeOrigin_Key, k_Origin_Key, true);
  params.linkParameters(k_ConvertToGrayScale_Key, k_ColorWeights_Key, true);
  params.linkParameters(k_ParentDataGroup_Key, k_DataGroupName_Key, true);
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ImportFijiMontageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportFijiMontageFilter::clone() const
{
  return std::make_unique<ImportFijiMontageFilter>();
}

//------------------------------------------------------------------------------
Result<std::vector<fiji::FijiTile>> ImportFijiMontageFilter::loadTiles(const fs::path& inputFile, const std::string& prefix, bool needMetadata) const
{
  // The cache is keyed on the config file's identity + last-write-time. Anything derived from the
  // live parameters (names, origin rebasing, output type) is recomputed by the callers on every
  // run, so the cache holds ONLY the file-I/O-bound results (the parse + the per-tile metadata).
  std::error_code ec;
  const fs::file_time_type currentTimeStamp = fs::last_write_time(inputFile, ec);
  const bool cacheHit = m_Cache.tilesValid && !ec && m_Cache.inputFile == inputFile && m_Cache.timeStamp == currentTimeStamp;

  if(!cacheHit)
  {
    auto parseResult = fiji::ParseTileConfiguration(inputFile);
    if(parseResult.invalid())
    {
      m_Cache = MetadataCache{}; // never serve a half-populated cache after a failed refresh
      return parseResult;        // propagate the parser's own error (-35900/-35901/-35903)
    }
    m_Cache.rawTiles = std::move(parseResult.value());
    m_Cache.metadata.clear();
    m_Cache.metadataValid = false;
    m_Cache.inputFile = inputFile;
    m_Cache.timeStamp = currentTimeStamp;
    m_Cache.tilesValid = true;
  }

  if(needMetadata && !m_Cache.metadataValid)
  {
    std::vector<ImageMetadata> metadata;
    metadata.reserve(m_Cache.rawTiles.size());
    for(const auto& tile : m_Cache.rawTiles)
    {
      auto ioResult = CreateImageIO(tile.filePath);
      if(ioResult.invalid())
      {
        return ConvertResultTo<std::vector<fiji::FijiTile>>(ConvertResult(std::move(ioResult)), {}); // propagate the ImageIO factory's error (unknown/unsupported format)
      }
      auto metadataResult = ioResult.value()->readMetadata(tile.filePath);
      if(metadataResult.invalid())
      {
        return ConvertResultTo<std::vector<fiji::FijiTile>>(ConvertResult(std::move(metadataResult)), {}); // propagate the reader's metadata error
      }
      metadata.push_back(std::move(metadataResult.value()));
    }
    m_Cache.metadata = std::move(metadata);
    m_Cache.metadataValid = true;
  }

  std::vector<fiji::FijiTile> tiles = m_Cache.rawTiles; // copy: names (and, in preflight, origins) are per-run
  fiji::AssignTileNames(tiles, prefix);
  return {std::move(tiles)};
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportFijiMontageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto inputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto parentDataGroup = filterArgs.value<bool>(k_ParentDataGroup_Key);
  auto dataGroupName = filterArgs.value<StringParameter::ValueType>(k_DataGroupName_Key);
  auto changeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto originValues = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto convertToGrayScale = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  auto imagePrefix = filterArgs.value<StringParameter::ValueType>(k_DataContainerPath_Key);
  auto cellAmName = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto imageArrayName = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);
  auto changeDataType = filterArgs.value<bool>(k_ChangeDataType_Key);
  auto dataTypeChoice = filterArgs.value<ChoicesParameter::ValueType>(k_ImageDataType_Key);
  auto lengthUnit = static_cast<IGeometry::LengthUnit>(filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key));

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(!fs::exists(inputFile))
  {
    return MakePreflightErrorResult(-35920, fmt::format("The Fiji TileConfiguration file does not exist: '{}'", inputFile.string()));
  }

  // Guard the hand-editable data-type index before ChoiceToImageDataType (which throws on an
  // out-of-range choice) can turn a bad pipeline JSON into an unhandled exception.
  if(changeDataType && dataTypeChoice > 2)
  {
    return MakePreflightErrorResult(-35924, fmt::format("'Set Image Data Type' is enabled but the Output Data Type index {} is out of range (valid: 0=uint8, 1=uint16, 2=uint32).", dataTypeChoice));
  }

  // loadTiles memoizes the config parse + per-tile metadata read (see MetadataCache); needMetadata=true
  // because preflight needs each tile's dims/type/components to declare the geometry and array.
  auto tilesResult = loadTiles(inputFile, imagePrefix, /*needMetadata=*/true);
  if(tilesResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(ConvertResult(std::move(tilesResult)), {})};
  }
  std::vector<fiji::FijiTile> tiles = std::move(tilesResult.value());

  // Rebase origins here (a preflight-only concern: origins are baked into the geometry below, and
  // execute never reads them). Guard the Origin vector against a short hand-edited JSON value.
  if(changeOrigin)
  {
    if(originValues.size() < 3)
    {
      return MakePreflightErrorResult(-35925, fmt::format("'Change Origin' is enabled but the Origin vector has {} value(s); exactly 3 (X, Y, Z) are required.", originValues.size()));
    }
    fiji::RebaseOrigins(tiles, FloatVec3{originValues[0], originValues[1], originValues[2]});
  }

  // Invariant guaranteed by loadTiles(needMetadata=true); a defensive check keeps the index-based
  // pairing below memory-safe even if that ever regresses.
  if(m_Cache.metadata.size() != tiles.size())
  {
    return MakePreflightErrorResult(-35926, fmt::format("Internal error: cached metadata count ({}) does not match tile count ({}).", m_Cache.metadata.size(), tiles.size()));
  }

  if(parentDataGroup)
  {
    resultOutputActions.value().appendAction(std::make_unique<CreateDataGroupAction>(DataPath({dataGroupName})));
  }

  const DataType destType = changeDataType ? ChoiceToImageDataType(dataTypeChoice) : DataType::uint8;

  for(usize i = 0; i < tiles.size(); i++)
  {
    const fiji::FijiTile& tile = tiles[i];
    const ImageMetadata& md = m_Cache.metadata[i];

    const TilePaths paths = MakeTilePaths(tile, parentDataGroup, dataGroupName, cellAmName, imageArrayName);

    const std::vector<usize> geomDims = {md.width, md.height, 1};       // X, Y, Z
    const std::vector<usize> arrayTupleDims = {1, md.height, md.width}; // Z, Y, X
    const std::vector<usize> componentDims = {md.numComponents};
    const std::vector<float32> originVec = {tile.origin[0], tile.origin[1], tile.origin[2]};
    const std::vector<float32> spacingVec = {1.0f, 1.0f, 1.0f};

    resultOutputActions.value().appendAction(std::make_unique<CreateImageGeometryAction>(paths.geomPath, geomDims, originVec, spacingVec, cellAmName, lengthUnit));

    const DataType arrayType = changeDataType ? destType : md.dataType;
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(arrayType, arrayTupleDims, componentDims, paths.arrayPath));
  }

  if(convertToGrayScale)
  {
    preflightUpdatedValues.push_back(
        {"GrayScale", fmt::format("Due to execution order, grayscale conversion runs during execute; every '{}' array will be converted to single-channel grayscale.", imageArrayName)});
  }

  preflightUpdatedValues.push_back({"Import Information", fmt::format("Imported Image Count: {}", tiles.size())});
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImportFijiMontageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto inputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto parentDataGroup = filterArgs.value<bool>(k_ParentDataGroup_Key);
  auto dataGroupName = filterArgs.value<StringParameter::ValueType>(k_DataGroupName_Key);
  auto convertToGrayScale = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  auto colorWeights = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  auto imagePrefix = filterArgs.value<StringParameter::ValueType>(k_DataContainerPath_Key);
  auto cellAmName = filterArgs.value<StringParameter::ValueType>(k_CellAttributeMatrixName_Key);
  auto imageArrayName = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);
  auto changeDataType = filterArgs.value<bool>(k_ChangeDataType_Key);
  auto dataTypeChoice = filterArgs.value<ChoicesParameter::ValueType>(k_ImageDataType_Key);

  // Same guard as preflight: reject a hand-edited out-of-range index before ChoiceToImageDataType throws.
  if(changeDataType && dataTypeChoice > 2)
  {
    return MakeErrorResult(-35942, fmt::format("'Set Image Data Type' is enabled but the Output Data Type index {} is out of range (valid: 0=uint8, 1=uint16, 2=uint32).", dataTypeChoice));
  }

  // Re-derive the same tile list preflight produced (framework re-runs preflight before execute, so the
  // geometries + arrays already exist; here we only fill pixels + optionally grayscale). needMetadata=false:
  // execute needs only the tile names + file paths — origins were baked into the geometry at preflight — so
  // it neither rebases origins nor re-reads metadata (a warm cache from preflight makes this a no-op).
  auto tilesResult = loadTiles(inputFile, imagePrefix, /*needMetadata=*/false);
  if(tilesResult.invalid())
  {
    return ConvertResult(std::move(tilesResult));
  }
  const std::vector<fiji::FijiTile> tiles = std::move(tilesResult.value());

  Result<> result;
  usize successfulTileCount = 0;
  for(const auto& tile : tiles)
  {
    if(shouldCancel)
    {
      return {};
    }
    messageHandler(IFilter::Message::Type::Info, fmt::format("Importing {}", tile.filePath.filename().string()));

    const TilePaths paths = MakeTilePaths(tile, parentDataGroup, dataGroupName, cellAmName, imageArrayName);

    ReadImageInputValues iv;
    iv.inputFilePath = tile.filePath;
    iv.imageGeometryPath = paths.geomPath;
    iv.imageDataArrayPath = paths.arrayPath;
    iv.cellDataName = cellAmName;
    iv.changeOrigin = false; // origin already set by CreateImageGeometryAction; do not let ReadImage move it
    iv.changeSpacing = false;
    iv.changeDataType = changeDataType;
    iv.imageDataType = changeDataType ? ChoiceToImageDataType(dataTypeChoice) : DataType::uint8;
    iv.croppingOptions.type = CropGeometryParameter::CropValues::TypeEnum::NoCropping;

    auto readResult = ReadImage(dataStructure, messageHandler, shouldCancel, iv)();
    if(readResult.invalid())
    {
      for(const auto& error : readResult.errors())
      {
        messageHandler(IFilter::Message::Type::Warning, fmt::format("|-- Error reading tile ({}): code {} - {}", tile.filePath.filename().string(), error.code, error.message));
      }
      result.warnings().emplace_back(Warning{-35940, fmt::format("Tile '{}' could not be read and was skipped.", tile.filePath.filename().string())});
      continue; // mirror legacy: try to continue with the remaining tiles
    }
    ++successfulTileCount; // the tile's pixel data was read; grayscale (below) is a further transform, not a read

    if(convertToGrayScale)
    {
      if(dataStructure.getDataRefAs<IDataArray>(paths.arrayPath).getDataType() != DataType::uint8)
      {
        result.warnings().emplace_back(Warning{-35932, fmt::format("Tile array '{}' is not uint8; grayscale conversion skipped.", paths.arrayPath.getTargetName())});
        continue;
      }

      ConvertColorToGrayScaleInputValues grayscaleInputValues;
      grayscaleInputValues.ConversionAlgorithm = 0;
      grayscaleInputValues.ColorWeights = colorWeights;
      grayscaleInputValues.ColorChannel = 0;
      grayscaleInputValues.InputDataArrayPaths = {paths.arrayPath};
      grayscaleInputValues.OutputArrayPrefix = "gray";

      auto grayscalePreflight = PreflightColorToGrayScale(dataStructure, grayscaleInputValues);
      if(grayscalePreflight.outputActions.invalid())
      {
        return ConvertResult(std::move(grayscalePreflight.outputActions));
      }
      Result<> actionResult = grayscalePreflight.outputActions.value().applyAll(dataStructure, IDataAction::Mode::Execute);
      if(actionResult.invalid())
      {
        return actionResult;
      }

      Result<> grayResult = ConvertColorToGrayScaleArrays(dataStructure, grayscaleInputValues, messageHandler, shouldCancel);
      if(grayResult.invalid())
      {
        return grayResult;
      }

      // Delete the color array, then rename gray<name> -> <name> so the tile keeps its declared array name.
      const DataObject::IdType colorId = dataStructure.getDataRefAs<IDataArray>(paths.arrayPath).getId();
      dataStructure.removeData(colorId);
      auto& gray = dataStructure.getDataRefAs<IDataArray>(paths.arrayPath.replaceName("gray" + imageArrayName));
      if(!gray.canRename(imageArrayName))
      {
        return MakeErrorResult(-35933, fmt::format("Unable to rename the grayscale array back to '{}'.", imageArrayName));
      }
      gray.rename(imageArrayName);
    }
  }

  // A total wipe-out (every tile failed to read) must fail loudly rather than reporting success with
  // zero-initialized arrays. Partial success (some tiles read) still returns valid, keeping the
  // per-tile -35940 warnings, to match the legacy filter's best-effort behavior.
  if(!tiles.empty() && successfulTileCount == 0)
  {
    return MakeErrorResult(-35941,
                           fmt::format("None of the {} montage tile(s) referenced by '{}' could be read; no image data was imported. See the per-tile warnings for the underlying reader errors.",
                                       tiles.size(), inputFile.string()));
  }

  return result;
}
} // namespace nx::core
