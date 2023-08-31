#include "GenerateIPFColorsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/GenerateIPFColors.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;
namespace
{

using FeatureIdsArrayType = Int32Array;
using GoodVoxelsArrayType = BoolArray;

inline constexpr int32 k_MissingGeomError = -71440;
inline constexpr int32 k_IncorrectInputArray = -71441;
inline constexpr int32 k_MissingInputArray = -71442;
inline constexpr int32 k_MissingOrIncorrectGoodVoxelsArray = -71443;
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateIPFColorsFilter::name() const
{
  return FilterTraits<GenerateIPFColorsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateIPFColorsFilter::className() const
{
  return FilterTraits<GenerateIPFColorsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateIPFColorsFilter::uuid() const
{
  return FilterTraits<GenerateIPFColorsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateIPFColorsFilter::humanName() const
{
  return "Generate IPF Colors";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateIPFColorsFilter::defaultTags() const
{
  return {className(), "Processing", "Crystallography"};
}

//------------------------------------------------------------------------------
Parameters GenerateIPFColorsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ReferenceDir_Key, "Reference Direction", "The reference axis with respect to compute the IPF colors", std::vector<float32>{0.0F, 0.0F, 1.0F},
                                                         std::vector<std::string>(3)));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseGoodVoxels_Key, "Use Mask Array", "Whether to assign a black color to 'bad' Elements", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsPath_Key, "Mask", "Path to the data array used to define Elements as good or bad.", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseGoodVoxels_Key, k_GoodVoxelsPath_Key, true);

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "Three angles defining the orientation of the Element in Bunge convention (Z-X-Z)",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each cell belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Required Input Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_CellIPFColorsArrayName_Key, "IPF Colors", "The name of the array containing the RGB colors encoded as unsigned chars for each Element", "IPFColors"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateIPFColorsFilter::clone() const
{
  return std::make_unique<GenerateIPFColorsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateIPFColorsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{

  auto pReferenceDirValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ReferenceDir_Key);
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pCellIPFColorsArrayNameValue = pCellEulerAnglesArrayPathValue.getParent().createChildPath(filterArgs.value<std::string>(k_CellIPFColorsArrayName_Key));

  // Validate the Crystal Structures array
  const UInt32Array& crystalStructures = dataStructure.getDataRefAs<UInt32Array>(pCrystalStructuresArrayPathValue);
  if(crystalStructures.getNumberOfComponents() != 1)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Crystal Structures Input Array must be a 1 component Int32 array"}})};
  }

  std::vector<DataPath> dataPaths;

  // Validate the Eulers array
  const Float32Array& quats = dataStructure.getDataRefAs<Float32Array>(pCellEulerAnglesArrayPathValue);
  if(quats.getNumberOfComponents() != 3)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Euler Angles Input Array must be a 3 component Float32 array"}})};
  }
  dataPaths.push_back(pCellEulerAnglesArrayPathValue);

  // Validate the Phases array
  const Int32Array& phases = dataStructure.getDataRefAs<Int32Array>(pCellPhasesArrayPathValue);
  if(phases.getNumberOfComponents() != 1)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Phases Input Array must be a 1 component Int32 array"}})};
  }
  dataPaths.push_back(pCellPhasesArrayPathValue);

  // Validate the GoodVoxels/Mask Array combination
  DataPath goodVoxelsPath;
  if(pUseGoodVoxelsValue)
  {
    goodVoxelsPath = filterArgs.value<DataPath>(k_GoodVoxelsPath_Key);

    const complex::IDataArray* goodVoxelsArray = dataStructure.getDataAs<IDataArray>(goodVoxelsPath);
    if(nullptr == goodVoxelsArray)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array is not located at path: '{}'", goodVoxelsPath.toString())}})};
    }

    if(goodVoxelsArray->getDataType() != DataType::boolean && goodVoxelsArray->getDataType() != DataType::uint8)
    {
      return {nonstd::make_unexpected(
          std::vector<Error>{Error{k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array at path '{}' is not of the correct type. It must be Bool or UInt8", goodVoxelsPath.toString())}})};
    }
    dataPaths.push_back(goodVoxelsPath);
  }

  auto numTupleCheckResult = dataStructure.validateNumberOfTuples(dataPaths);
  if(!numTupleCheckResult.first)
  {
    return {MakeErrorResult<OutputActions>(-651, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", numTupleCheckResult.second))};
  }

  // Get the number of tuples
  auto* eulersArray = dataStructure.getDataAs<Float32Array>(pCellEulerAnglesArrayPathValue);

  // Create output DataStructure Items
  auto createIpfColorsAction = std::make_unique<CreateArrayAction>(DataType::uint8, eulersArray->getIDataStore()->getTupleShape(), std::vector<usize>{3}, pCellIPFColorsArrayNameValue);

  OutputActions actions;
  actions.appendAction(std::move(createIpfColorsAction));

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> GenerateIPFColorsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  GenerateIPFColorsInputValues inputValues;

  inputValues.referenceDirection = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ReferenceDir_Key);
  inputValues.useGoodVoxels = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  inputValues.cellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.cellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.goodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsPath_Key);
  inputValues.crystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.cellIpfColorsArrayPath = inputValues.cellEulerAnglesArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_CellIPFColorsArrayName_Key));

  // Let the Algorithm instance do the work
  return GenerateIPFColors(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
