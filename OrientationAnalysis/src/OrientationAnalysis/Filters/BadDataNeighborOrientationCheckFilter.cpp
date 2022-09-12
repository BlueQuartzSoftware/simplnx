#include "BadDataNeighborOrientationCheckFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/BadDataNeighborOrientationCheck.hpp"

using namespace complex;

namespace
{
inline constexpr int32 k_MissingGeomError = -6800;
inline constexpr int32 k_MissingInputArray = -6801;
inline constexpr int32 k_IncorrectInputArray = -6802;
inline constexpr int32 k_InvalidNumTuples = -6803;
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
  return {"#Processing", "#Cleanup"};
}

//------------------------------------------------------------------------------
Parameters BadDataNeighborOrientationCheckFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "Misorientation Tolerance (Degrees)", "", 5.0f));
  params.insert(std::make_unique<Int32Parameter>(k_NumberOfNeighbors_Key, "Required Number of Neighbors", "", 6));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::boolean}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "", DataPath({"Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath({"Ensemble Data", "CrystalStructures"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}));

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
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

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
  if(!dataStructure.validateNumberOfTuples(dataArrayPaths))
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_InvalidNumTuples, "Input arrays do not have matching tuple counts."}})};
  }

  return {};
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
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  return BadDataNeighborOrientationCheck(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
