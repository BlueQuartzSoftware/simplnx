#include "BadDataNeighborOrientationCheckFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/BadDataNeighborOrientationCheck.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

#include "complex/Utilities/SIMPLConversion.hpp"

#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

using namespace complex;

namespace
{
inline constexpr int32 k_MissingGeomError = -6800;
inline constexpr int32 k_MissingInputArray = -6801;
inline constexpr int32 k_IncorrectInputArray = -6802;
inline constexpr int32 k_InvalidNumTuples = -6803;
inline constexpr int32 k_InconsistentTupleCount = -6809;
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string BadDataNeighborOrientationCheckFilter::name() const
{
  return FilterTraits<BadDataNeighborOrientationCheckFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string BadDataNeighborOrientationCheckFilter::className() const
{
  return FilterTraits<BadDataNeighborOrientationCheckFilter>::className;
}

//------------------------------------------------------------------------------
Uuid BadDataNeighborOrientationCheckFilter::uuid() const
{
  return FilterTraits<BadDataNeighborOrientationCheckFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string BadDataNeighborOrientationCheckFilter::humanName() const
{
  return "Neighbor Orientation Comparison (Bad Data)";
}

//------------------------------------------------------------------------------
std::vector<std::string> BadDataNeighborOrientationCheckFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup"};
}

//------------------------------------------------------------------------------
Parameters BadDataNeighborOrientationCheckFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "Misorientation Tolerance (Degrees)", "Angular tolerance used to compare with neighboring Cells", 5.0f));
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfNeighbors_Key, "Required Number of Neighbors",
                                                 "Minimum number of neighbor Cells that must have orientations within above tolerance to allow Cell to be changed", 6));
  params.insert(
      std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "The target geometry", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "Specifies the orientation of the Cell in quaternion representation", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "Used to define Cells as good or bad", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath({"Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each phase",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer BadDataNeighborOrientationCheckFilter::clone() const
{
  return std::make_unique<BadDataNeighborOrientationCheckFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult BadDataNeighborOrientationCheckFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                              const std::atomic_bool& shouldCancel) const
{
  auto pMisorientationToleranceValue = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  auto pNumberOfNeighborsValue = filterArgs.value<int32>(k_NumberOfNeighbors_Key);
  auto pImageGeomPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<DataPath> dataArrayPaths;

  auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(pImageGeomPathValue);
  if(imageGeomPtr == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingGeomError, fmt::format("Could not find input image geometry at path '{}'", pImageGeomPathValue.toString())}})};
  }

  // Validate the mask array
  auto* goodVoxelsPtr = dataStructure.getDataAs<IDataArray>(pGoodVoxelsArrayPathValue);
  if(nullptr == goodVoxelsPtr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingInputArray, fmt::format("Could not find mask array at path '{}'", pGoodVoxelsArrayPathValue.toString())}})};
  }
  auto* goodVoxelsBoolPtr = dataStructure.getDataAs<BoolArray>(pGoodVoxelsArrayPathValue);
  if(nullptr == goodVoxelsBoolPtr)
  {
    return {nonstd::make_unexpected(
        std::vector<Error>{Error{k_IncorrectInputArray, fmt::format("Mask array at path '{}' is not of the correct type. It must be Bool.", pGoodVoxelsArrayPathValue.toString())}})};
  }
  if(goodVoxelsBoolPtr->getNumberOfComponents() != 1)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Mask Input Array must be a 1 component Bool array"}})};
  }
  dataArrayPaths.push_back(pGoodVoxelsArrayPathValue);

  // validate the cell phases array
  auto* cellPhasesPtr = dataStructure.getDataAs<IDataArray>(pCellPhasesArrayPathValue);
  if(nullptr == cellPhasesPtr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingInputArray, fmt::format("Could not find cell phases array at path '{}'", pCellPhasesArrayPathValue.toString())}})};
  }
  auto* cellPhasesInt32Ptr = dataStructure.getDataAs<Int32Array>(pCellPhasesArrayPathValue);
  if(nullptr == cellPhasesInt32Ptr)
  {
    return {nonstd::make_unexpected(
        std::vector<Error>{Error{k_IncorrectInputArray, fmt::format("Cell phases array at path '{}' is not of the correct type. It must be Int32.", pCellPhasesArrayPathValue.toString())}})};
  }
  if(cellPhasesInt32Ptr->getNumberOfComponents() != 1)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Cell phases Input Array must be a 1 component Int32 array"}})};
  }
  dataArrayPaths.push_back(pCellPhasesArrayPathValue);

  // validate the crystal structures array
  auto* crystalStructuresPtr = dataStructure.getDataAs<IDataArray>(pCrystalStructuresArrayPathValue);
  if(nullptr == crystalStructuresPtr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingInputArray, fmt::format("Could not find crystal structures array at path '{}'", pCrystalStructuresArrayPathValue.toString())}})};
  }
  auto* crystalStructuresUInt32Ptr = dataStructure.getDataAs<UInt32Array>(pCrystalStructuresArrayPathValue);
  if(nullptr == crystalStructuresUInt32Ptr)
  {
    return {nonstd::make_unexpected(
        std::vector<Error>{Error{k_IncorrectInputArray, fmt::format("Crystal structures array at path '{}' is not of the correct type. It must be UInt32.", pCellPhasesArrayPathValue.toString())}})};
  }
  if(crystalStructuresUInt32Ptr->getNumberOfComponents() != 1)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Crystal structures Input Array must be a 1 component UInt32 array"}})};
  }

  // validate the quaternions array
  auto* quatsPtr = dataStructure.getDataAs<IDataArray>(pQuatsArrayPathValue);
  if(nullptr == quatsPtr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingInputArray, fmt::format("Could not find quaternions array at path '{}'", pQuatsArrayPathValue.toString())}})};
  }
  auto* quatsFloat32Ptr = dataStructure.getDataAs<Float32Array>(pQuatsArrayPathValue);
  if(nullptr == quatsFloat32Ptr)
  {
    return {nonstd::make_unexpected(
        std::vector<Error>{Error{k_IncorrectInputArray, fmt::format("Quaternions array at path '{}' is not of the correct type. It must be Float32.", pCellPhasesArrayPathValue.toString())}})};
  }
  if(quatsFloat32Ptr->getNumberOfComponents() != 4)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Quaternion Input Array must be a 4 component Float32 array"}})};
  }
  dataArrayPaths.push_back(pQuatsArrayPathValue);

  // validate the number of tuples
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrayPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(k_InconsistentTupleCount,
                                           fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  resultOutputActions.value().modifiedActions.emplace_back(
      DataObjectModification{pGoodVoxelsArrayPathValue, DataObjectModification::ModifiedType::Modified, dataStructure.getData(pGoodVoxelsArrayPathValue)->getDataObjectType()});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> BadDataNeighborOrientationCheckFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  BadDataNeighborOrientationCheckInputValues inputValues;

  inputValues.MisorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  inputValues.NumberOfNeighbors = filterArgs.value<int32>(k_NumberOfNeighbors_Key);
  inputValues.ImageGeomPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  return BadDataNeighborOrientationCheck(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MisorientationToleranceKey = "MisorientationTolerance";
constexpr StringLiteral k_NumberOfNeighborsKey = "NumberOfNeighbors";
constexpr StringLiteral k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteral k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> BadDataNeighborOrientationCheckFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = BadDataNeighborOrientationCheckFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_MisorientationToleranceKey, k_MisorientationTolerance_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_NumberOfNeighborsKey, k_NumberOfNeighbors_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_QuatsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace complex
