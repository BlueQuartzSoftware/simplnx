#include "NeighborOrientationCorrelationFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/NeighborOrientationCorrelation.hpp"

using namespace complex;

namespace
{
inline constexpr int32 k_MissingGeomError = -580090;
inline constexpr int32 k_MissingInputArray = -580091;
inline constexpr int32 k_IncorrectInputArray = -580092;
inline constexpr int32 k_InvalidNumTuples = -580093;
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string NeighborOrientationCorrelationFilter::name() const
{
  return FilterTraits<NeighborOrientationCorrelationFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string NeighborOrientationCorrelationFilter::className() const
{
  return FilterTraits<NeighborOrientationCorrelationFilter>::className;
}

//------------------------------------------------------------------------------
Uuid NeighborOrientationCorrelationFilter::uuid() const
{
  return FilterTraits<NeighborOrientationCorrelationFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string NeighborOrientationCorrelationFilter::humanName() const
{
  return "Neighbor Orientation Correlation";
}

//------------------------------------------------------------------------------
std::vector<std::string> NeighborOrientationCorrelationFilter::defaultTags() const
{
  return {"#Processing", "#Cleanup"};
}

//------------------------------------------------------------------------------
Parameters NeighborOrientationCorrelationFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<Float32Parameter>(k_MinConfidence_Key, "Minimum Confidence Index", "", 0.1f));
  params.insert(std::make_unique<Float32Parameter>(k_MisorientationTolerance_Key, "Misorientation Tolerance (Degrees)", "", 5.0f));
  params.insert(std::make_unique<Int32Parameter>(k_Level_Key, "Cleanup Level", "", 6));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_ConfidenceIndexArrayPath_Key, "Confidence Index", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "", DataPath({"Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::float32}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath({"Ensemble Data", "CrystalStructures"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "", MultiArraySelectionParameter::ValueType{},
                                                               ArraySelectionParameter::AllowedTypes{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer NeighborOrientationCorrelationFilter::clone() const
{
  return std::make_unique<NeighborOrientationCorrelationFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult NeighborOrientationCorrelationFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                             const std::atomic_bool& shouldCancel) const
{
  auto pImageGeomPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pMinConfidenceValue = filterArgs.value<float32>(k_MinConfidence_Key);
  auto pMisorientationToleranceValue = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  auto pLevelValue = filterArgs.value<int32>(k_Level_Key);
  auto pConfidenceIndexArrayPathValue = filterArgs.value<DataPath>(k_ConfidenceIndexArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(pImageGeomPathValue);
  if(imageGeomPtr == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingGeomError, fmt::format("Could not find input image geometry at path '{}'", pImageGeomPathValue.toString())}})};
  }

  std::vector<DataPath> dataArrayPaths;

  // Validate the confidence index array
  auto* confIndexPtr = dataStructure.getDataAs<IDataArray>(pConfidenceIndexArrayPathValue);
  if(nullptr == confIndexPtr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingInputArray, fmt::format("Could not find confidence index array at path '{}'", pConfidenceIndexArrayPathValue.toString())}})};
  }
  auto* confIndexFloatPtr = dataStructure.getDataAs<Float32Array>(pConfidenceIndexArrayPathValue);
  if(nullptr == confIndexFloatPtr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{
        Error{k_IncorrectInputArray, fmt::format("Confidence index array at path '{}' is not of the correct type. It must be Float32.", pConfidenceIndexArrayPathValue.toString())}})};
  }
  if(confIndexFloatPtr->getNumberOfComponents() != 1)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Confidence index input array must be a 1 component Float32 array"}})};
  }
  dataArrayPaths.push_back(pConfidenceIndexArrayPathValue);

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
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Cell phases input array must be a 1 component Int32 array"}})};
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
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Crystal structures input array must be a 1 component UInt32 array"}})};
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
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Quaternion input array must be a 4 component Float32 array"}})};
  }
  dataArrayPaths.push_back(pQuatsArrayPathValue);

  // collect the rest of the geometry's arrays that aren't ignored to check the tuple count
  DataPath parentPath = pConfidenceIndexArrayPathValue.getParent();
  const auto& parent = dataStructure.getDataRefAs<BaseGroup>(parentPath);
  auto childArrays = parent.findAllChildrenOfType<IDataArray>();
  for(const auto childArray : childArrays)
  {
    bool ignore = false;
    DataPath childArrayPath;
    for(const auto& childDataPath : childArray->getDataPaths())
    {
      if(parentPath == childDataPath.getParent())
      {
        childArrayPath = childDataPath;
      }
    }
    for(const auto& ignoredPath : pIgnoredDataArrayPathsValue)
    {
      if(childArrayPath == ignoredPath)
      {
        ignore = true;
        break;
      }
    }
    if(!ignore)
    {
      dataArrayPaths.push_back(childArrayPath);
    }
  }

  // validate the number of tuples
  if(!dataStructure.validateNumberOfTuples(dataArrayPaths))
  {
    return {nonstd::make_unexpected(std::vector<Error>{
        Error{k_InvalidNumTuples, "Geometry arrays do not have matching tuple counts.  Input arrays as well as all arrays not on the ignore list must have matching tuple counts."}})};
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> NeighborOrientationCorrelationFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                           const std::atomic_bool& shouldCancel) const
{
  NeighborOrientationCorrelationInputValues inputValues;

  inputValues.ImageGeomPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.MinConfidence = filterArgs.value<float32>(k_MinConfidence_Key);
  inputValues.MisorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  inputValues.Level = filterArgs.value<int32>(k_Level_Key);
  inputValues.ConfidenceIndexArrayPath = filterArgs.value<DataPath>(k_ConfidenceIndexArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  return NeighborOrientationCorrelation(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
