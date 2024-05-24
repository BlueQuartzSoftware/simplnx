#include "ExtractVertexGeometryFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ExtractVertexGeometry.hpp"

#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/MoveDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

using namespace nx::core;
namespace
{

using FeatureIdsArrayType = Int32Array;
using GoodVoxelsArrayType = BoolArray;

inline constexpr int32 k_MissingGeomError = -72440;
inline constexpr int32 k_IncorrectInputArray = -72441;
inline constexpr int32 k_MissingInputArray = -72442;
inline constexpr int32 k_MissingOrIncorrectGoodVoxelsArray = -72443;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ExtractVertexGeometryFilter::name() const
{
  return FilterTraits<ExtractVertexGeometryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ExtractVertexGeometryFilter::className() const
{
  return FilterTraits<ExtractVertexGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ExtractVertexGeometryFilter::uuid() const
{
  return FilterTraits<ExtractVertexGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ExtractVertexGeometryFilter::humanName() const
{
  return "Extract Vertex Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> ExtractVertexGeometryFilter::defaultTags() const
{
  return {className(), "Core", "Geometry", "Conversion", "Image", "Generate", "Create", "Vertex"};
}

//------------------------------------------------------------------------------
Parameters ExtractVertexGeometryFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ChoicesParameter>(k_ArrayHandling_Key, "Array Handling", "[0] Move or [1] Copy input data arrays", 0,
                                                   ChoicesParameter::Choices{"Move Attribute Arrays", "Copy Attribute Arrays"}));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Specifies whether or not to use a mask array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Input Image or RectilinearGrid Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputGeometryPath_Key, "Input Geometry", "The input Image/RectilinearGrid Geometry to convert", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image, IGeometry::Type::RectGrid}));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IncludedDataArrayPaths_Key, "Included Attribute Arrays", "The arrays to copy/move to the vertex array",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               nx::core::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Output Vertex Geometry"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_VertexGeometryPath_Key, "Output Vertex Geometry", "The complete path to the vertex geometry that will be created", DataPath({"Vertex Geometry"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexAttrMatrixName_Key, "Output Vertex Attribute Matrix Name", "The name of the vertex attribute matrix that will be created",
                                                          std::string{"Vertex Data"}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SharedVertexListName_Key, "Output Shared Vertex List Name", "The name of the shared vertex list that will be created",
                                                          std::string{"SharedVertexList"}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ExtractVertexGeometryFilter::clone() const
{
  return std::make_unique<ExtractVertexGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ExtractVertexGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto pArrayHandlingValue = filterArgs.value<ChoicesParameter::ValueType>(k_ArrayHandling_Key);
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pInputGeometryPathValue = filterArgs.value<DataPath>(k_InputGeometryPath_Key);
  auto pIncludedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IncludedDataArrayPaths_Key);
  auto pVertexGeometryPathValue = filterArgs.value<DataPath>(k_VertexGeometryPath_Key);
  auto pVertexAttrMatrixNameValue = filterArgs.value<std::string>(k_VertexAttrMatrixName_Key);
  auto pSharedVertexListNameValue = filterArgs.value<std::string>(k_SharedVertexListName_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  const ExtractVertexGeometry::ArrayHandlingType arrayHandlingType = static_cast<ExtractVertexGeometry::ArrayHandlingType>(pArrayHandlingValue);

  const IGridGeometry& geometry = dataStructure.getDataRefAs<IGridGeometry>({pInputGeometryPathValue});
  SizeVec3 dims = geometry.getDimensions();
  usize geomElementCount = dims[0] * dims[1] * dims[2];

  // Create the Vertex Geometry
  auto createVertexGeometryAction = std::make_unique<CreateVertexGeometryAction>(pVertexGeometryPathValue, geomElementCount, pVertexAttrMatrixNameValue, pSharedVertexListNameValue);
  resultOutputActions.value().appendAction(std::move(createVertexGeometryAction));

  std::vector<DataPath> dataPaths = pIncludedDataArrayPathsValue;

  // Validate the GoodVoxels/Mask Array combination
  if(pUseGoodVoxelsValue)
  {
    const nx::core::IDataArray* goodVoxelsArray = dataStructure.getDataAs<IDataArray>(pMaskArrayPathValue);
    if(nullptr == goodVoxelsArray)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array is not located at path: '{}'", pMaskArrayPathValue.toString())}})};
    }

    if(goodVoxelsArray->getDataType() != DataType::boolean && goodVoxelsArray->getDataType() != DataType::uint8)
    {
      return {nonstd::make_unexpected(
          std::vector<Error>{Error{k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array at path '{}' is not of the correct type. It must be Bool or UInt8", pMaskArrayPathValue.toString())}})};
    }
    dataPaths.push_back(pMaskArrayPathValue);
  }

  // Validate the number of tuples for each of the DataArrays that are to be moved/copied
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-651, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  // Validate the number of tuples in the arrays (if any) and the number of tuples in the input grid geometry (which is also the same
  // as the output Vertex Geometry
  if(!dataPaths.empty())
  {
    const IDataArray& dataArray = dataStructure.getDataRefAs<IDataArray>(dataPaths.front());
    if(dataArray.getNumberOfTuples() != geomElementCount)
    {
      return {MakeErrorResult<OutputActions>(-2006, fmt::format("The selected DataArrays do not have the correct number of tuples. The Input Geometry ({}) has {} tuples but the "
                                                                " selected DataArrays have {} tuples.)",
                                                                pInputGeometryPathValue.toString(), geomElementCount, dataArray.getNumberOfTuples()))};
    }
  }

  // Create appropriate actions to either copy or move the data
  const DataPath vertexAttrMatrixPath = pVertexGeometryPathValue.createChildPath(pVertexAttrMatrixNameValue);
  for(const DataPath& dataPath : pIncludedDataArrayPathsValue)
  {
    const IDataArray& dataArray = dataStructure.getDataRefAs<IDataArray>(dataPath);

    if(arrayHandlingType == ExtractVertexGeometry::ArrayHandlingType::CopyArrays)
    {
      DataPath newDataPath = vertexAttrMatrixPath.createChildPath(dataPath.getTargetName());
      auto createArrayAction = std::make_unique<CreateArrayAction>(dataArray.getDataType(), dataArray.getTupleShape(), dataArray.getComponentShape(), newDataPath);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
    else if(arrayHandlingType == ExtractVertexGeometry::ArrayHandlingType::MoveArrays)
    {
      auto moveDataAction = std::make_unique<MoveDataAction>(dataPath, vertexAttrMatrixPath);
      resultOutputActions.value().appendAction(std::move(moveDataAction));
    }
  }

  //  PreflightResult preflightResult;
  //
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ExtractVertexGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  ExtractVertexGeometryInputValues inputValues;
  inputValues.ArrayHandling = filterArgs.value<ChoicesParameter::ValueType>(k_ArrayHandling_Key);
  inputValues.UseMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.InputGeometryPath = filterArgs.value<DataPath>(k_InputGeometryPath_Key);
  inputValues.IncludedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IncludedDataArrayPaths_Key);
  inputValues.VertexGeometryPath = filterArgs.value<DataPath>(k_VertexGeometryPath_Key);
  inputValues.VertexAttrMatrixName = filterArgs.value<std::string>(k_VertexAttrMatrixName_Key);
  inputValues.SharedVertexListName = filterArgs.value<std::string>(k_SharedVertexListName_Key);

  return ExtractVertexGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
