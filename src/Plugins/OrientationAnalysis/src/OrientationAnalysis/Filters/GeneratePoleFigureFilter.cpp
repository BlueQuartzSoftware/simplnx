#include "GeneratePoleFigureFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/GeneratePoleFigure.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string GeneratePoleFigureFilter::name() const
{
  return FilterTraits<GeneratePoleFigureFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string GeneratePoleFigureFilter::className() const
{
  return FilterTraits<GeneratePoleFigureFilter>::className;
}

//------------------------------------------------------------------------------
Uuid GeneratePoleFigureFilter::uuid() const
{
  return FilterTraits<GeneratePoleFigureFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string GeneratePoleFigureFilter::humanName() const
{
  return "Export Pole Figure Images";
}

//------------------------------------------------------------------------------
std::vector<std::string> GeneratePoleFigureFilter::defaultTags() const
{
  return {"IO", "Output", "Write", "Export"};
}

//------------------------------------------------------------------------------
Parameters GeneratePoleFigureFilter::parameters() const
{
  Parameters params;

  /**
   * Please separate the parameters into groups generally of the following:
   *
   * params.insertSeparator(Parameters::Separator{"Input Parameters"});
   * params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
   * params.insertSeparator(Parameters::Separator{"Required Input Feature Data"});
   * params.insertSeparator(Parameters::Separator{"Created Cell Data"});
   * params.insertSeparator(Parameters::Separator{"Created Cell Feature Data"});
   *
   * .. or create appropriate separators as needed. The UI in COMPLEX no longer
   * does this for the developer by using catgories as in SIMPL
   */

  // Create the parameter descriptors that are needed for this filter

  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_GenerationAlgorithm_Key, "Pole Figure Type", "", 0, ChoicesParameter::Choices{"Color Intensity", "Discrete"}));

  params.insert(std::make_unique<Int32Parameter>(k_LambertSize_Key, "Lambert Image Size (Pixels)", "", 64));
  params.insert(std::make_unique<Int32Parameter>(k_NumColors_Key, "Number of Colors", "", 32));
  params.insert(std::make_unique<Int32Parameter>(k_ImageSize_Key, "Image Size (Square Pixels)", "", 512));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseGoodVoxels_Key, "Use Mask Array", "Whether to assign a black color to 'bad' Elements", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsPath_Key, "Mask", "Path to the data array used to define Elements as good or bad.", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "Three angles defining the orientation of the Element in Bunge convention (Z-X-Z)",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Required Input Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaterialNameArrayPath_Key, "Material Name", "", DataPath{},
                                                          complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));

  params.insertSeparator(Parameters::Separator{"Created Output Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerName_Key, "Created Image Geometry", "The complete path to the Geometry being created.", DataPath({"Pole Figures"})));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Created Cell Attribute Matrix", "The Attribute Matrix where the scan data is stored.", ImageGeom::k_CellDataName));

  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "<001>|<0001>", "Cubic|Hexagonal Default Array Name", "<001>|<0001>"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "<011>|<10-10>", "Cubic|Hexagonal Default Array Name", "<011>|<10-10>"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "<111>|<2-1-10>", "Cubic|Hexagonal Default Array Name", "<111>|<2-1-10>"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseGoodVoxels_Key, k_GoodVoxelsPath_Key, true);

  params.linkParameters(k_GenerationAlgorithm_Key, k_LambertSize_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_GenerationAlgorithm_Key, k_NumColors_Key, std::make_any<ChoicesParameter::ValueType>(0));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GeneratePoleFigureFilter::clone() const
{
  return std::make_unique<GeneratePoleFigureFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GeneratePoleFigureFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{

  auto pOutputArrayPrefixValue = filterArgs.value<StringParameter::ValueType>(k_DataArrayPrefix_Key);
  auto pGenerationAlgorithmValue = filterArgs.value<ChoicesParameter::ValueType>(k_GenerationAlgorithm_Key);
  auto pLambertSizeValue = filterArgs.value<int32>(k_LambertSize_Key);
  auto pNumColorsValue = filterArgs.value<int32>(k_NumColors_Key);

  auto pImageSizeValue = filterArgs.value<int32>(k_ImageSize_Key);
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pMaterialNameArrayPathValue = filterArgs.value<DataPath>(k_MaterialNameArrayPath_Key);
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_DataContainerName_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // Scope this next section due to the use of std::move().
  {
    CreateImageGeometryAction::DimensionType imageGeomDims = {static_cast<size_t>(pImageSizeValue), static_cast<size_t>(pImageSizeValue), static_cast<size_t>(1)};
    std::vector<size_t> tupleDims = {imageGeomDims[2], imageGeomDims[1], imageGeomDims[0]};
    // We need to center the Image Geometry at 0,0,0 and have bounds be from -1 to 1 in X and Y directions.
    CreateImageGeometryAction::SpacingType spacing = {2.0f / pImageSizeValue, 2.0f / pImageSizeValue, 1.0f};
    CreateImageGeometryAction::OriginType origin = {-(pImageSizeValue / 2.0f), -(pImageSizeValue / 2.0f), 0.0f};

    // Define a custom class that generates the changes to the DataStructure.
    auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pImageGeometryPath, CreateImageGeometryAction::DimensionType({imageGeomDims[0], imageGeomDims[1], imageGeomDims[2]}),
                                                                                 origin, spacing, pCellAttributeMatrixNameValue);
    // Assign the createImageGeometryAction to the Result<OutputActions>::actions vector via a push_back
    resultOutputActions.value().actions.push_back(std::move(createImageGeometryAction));
  }
  // Create the Cell Attribute Matrix and the output arrays
  {
    DataPath cellAttributeMatrixPath = pImageGeometryPath.createChildPath(pCellAttributeMatrixNameValue);

    //    DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(name);
    //    auto action = std::make_unique<CreateArrayAction>(complex::DataType::int32, tupleDims, cDims, dataArrayPath);
    //    resultOutputActions.value().actions.push_back(std::move(action));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GeneratePoleFigureFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{

  GeneratePoleFigureInputValues inputValues;

  //  inputValues.pOutputArrayPrefixValue = filterArgs.value<StringParameter::ValueType>(k_DataArrayPrefix_Key);
  //  inputValues.pGenerationAlgorithmValue = filterArgs.value<ChoicesParameter::ValueType>(k_GenerationAlgorithm_Key);
  //  inputValues.pLambertSizeValue = filterArgs.value<int32>(k_LambertSize_Key);
  //  inputValues.pNumColorsValue = filterArgs.value<int32>(k_NumColors_Key);
  //
  //  inputValues.pImageSizeValue = filterArgs.value<int32>(k_ImageSize_Key);
  //  inputValues.pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  //  inputValues.pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  //  inputValues.pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  //  inputValues.pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsPath_Key);
  //  inputValues.pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  //  inputValues.pMaterialNameArrayPathValue = filterArgs.value<DataPath>(k_MaterialNameArrayPath_Key);
  //  inputValues.pImageGeometryPath = filterArgs.value<DataPath>(k_DataContainerName_Key);
  //  inputValues.pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);

  return GeneratePoleFigure(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
