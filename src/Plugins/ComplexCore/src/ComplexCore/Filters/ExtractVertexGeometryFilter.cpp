#include "ExtractVertexGeometryFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ExtractVertexGeometry.hpp"

#include "complex/DataStructure/Geometry/IGridGeometry.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"

using namespace complex;

namespace complex
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
  return {className(), "Core", "Conversion"};
}

//------------------------------------------------------------------------------
Parameters ExtractVertexGeometryFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_ArrayHandling_Key, "Array Handling", "Move or Copy input data arrays", 0, ChoicesParameter::Choices{"Move Attribute Arrays", "Copy Attribute Arrays"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask", "Specifies whether or not to use a mask array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask", "DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_InputGeometryPath_Key, "Input Geometry", "The input Image/RectilinearGrid Geometry to convert", DataPath{},
                                                              DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::ImageGeom, BaseGroup::GroupType::RectGridGeom}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IncludedDataArrayPaths_Key, "Included Attribute Arrays", "The arrays to copy/move to the vertex array", MultiArraySelectionParameter::ValueType{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, complex::GetAllDataTypes()));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_VertexGeometryPath_Key, "Output Vertex Geometry", "The complete path to the vertex geometry that will be created",
                                                             DataPath({"[Vertex Geometry]"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexAttrMatrixName_Key, "Output Vertex Attribute Matrix Name", "The name of the vertex attribute matrix that will be created",
                                                          std::string{"VertexData"}));
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
  auto pUseMaskValue = filterArgs.value<bool>(k_UseMask_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pInputGeometryPathValue = filterArgs.value<DataPath>(k_InputGeometryPath_Key);
  auto pIncludedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IncludedDataArrayPaths_Key);
  auto pVertexGeometryPathValue = filterArgs.value<DataPath>(k_VertexGeometryPath_Key);
  auto pVertexAttrMatrixNameValue = filterArgs.value<std::string>(k_VertexAttrMatrixName_Key);
  auto pSharedVertexListNameValue = filterArgs.value<std::string>(k_SharedVertexListName_Key);

  complex::Result<OutputActions> resultOutputActions;

  ArrayHandlingType arrayHandlingType = static_cast<ArrayHandlingType>(pArrayHandlingValue);

  const IGridGeometry& geometry = dataStructure.getDataRefAs<IGridGeometry>({pInputGeometryPathValue});
  SizeVec3 dims = geometry.getDimensions();
  usize geomElementCount = dims[0] * dims[1] * dims[2];
  if(pUseMaskValue)
  {
    const BoolArray& maskArray = dataStructure.getDataRefAs<BoolArray>(pMaskArrayPathValue);
    if(maskArray.getNumberOfTuples() != geomElementCount)
    {
      return {MakeErrorResult<OutputActions>(-2003, fmt::format("{0}: The data array with path '{1}' has a tuple count of {2}, but this does not match the "
                                                                "number of tuples required by geometry '{3}' ({4})",
                                                                humanName(), pMaskArrayPathValue.toString(), maskArray.getNumberOfTuples(), geometry.getName(), geomElementCount))};
    }
  }

  std::optional<DataPath> cellAttrMatrixPathR;
  for(const DataPath& dataPath : pIncludedDataArrayPathsValue)
  {
    DataPath parentPath = dataPath.getParent();
    if(parentPath.empty())
    {
      return {MakeErrorResult<OutputActions>(-2004, fmt::format("{}: The data array with path '{}' has no parent.  It must have an AttributeMatrix as a parent.", humanName(), dataPath.toString()))};
    }
    if(dataStructure.getDataAs<AttributeMatrix>(parentPath) == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-2005, fmt::format("{}: The data array with path '{}' does not have an AttributeMatrix as a parent.", humanName(), dataPath.toString()))};
    }

    const IDataArray& dataArray = dataStructure.getDataRefAs<IDataArray>(dataPath);
    if(dataArray.getNumberOfTuples() != geomElementCount)
    {
      return {MakeErrorResult<OutputActions>(-2006, fmt::format("{}: The data array with path '{}' has a tuple count of {}, but this does not match the "
                                                                "number of vertices required by geometry with path '{}' ({})",
                                                                humanName(), dataPath.toString(), dataArray.getNumberOfTuples(), pVertexGeometryPathValue.toString(), geomElementCount))};
    }

    if(cellAttrMatrixPathR.has_value())
    {
      DataPath cellAttrMatrixPath = *cellAttrMatrixPathR;
      if(parentPath != *cellAttrMatrixPathR)
      {
        return {MakeErrorResult<OutputActions>(
            -2007, fmt::format("{}: The data array with path '{}' does not have the AttributeMatrix at path '{}' as a parent.  All data arrays must have the same AttributeMatrix parent.", humanName(),
                               dataPath.toString(), cellAttrMatrixPath.toString()))};
      }
    }
    else
    {
      cellAttrMatrixPathR = parentPath;
      {
        auto createVertexGeometryAction = std::make_unique<CreateVertexGeometryAction>(pVertexGeometryPathValue, geomElementCount, parentPath.getTargetName(), pSharedVertexListNameValue);
        resultOutputActions.value().appendAction(std::move(createVertexGeometryAction));
      }
    }

    DataPath newDataPath = pVertexGeometryPathValue.createChildPath(parentPath.getTargetName()).createChildPath(dataArray.getName());

    auto createArrayAction = std::make_unique<CreateArrayAction>(dataArray.getDataType(), dataArray.getTupleShape(), dataArray.getComponentShape(), newDataPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));

    if(arrayHandlingType == ArrayHandlingType::MoveArrays)
    {
      if(!pUseMaskValue)
      {
        auto deleteDataAction = std::make_unique<DeleteDataAction>(dataPath, DeleteDataAction::DeleteType::JustObject);
        resultOutputActions.value().appendDeferredAction(std::move(deleteDataAction));
      }
    }
  }

  PreflightResult preflightResult;

  std::vector<PreflightValue> preflightUpdatedValues;

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

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
} // namespace complex
