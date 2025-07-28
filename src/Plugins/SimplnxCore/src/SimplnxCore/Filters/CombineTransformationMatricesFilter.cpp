#include "CombineTransformationMatricesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/CombineTransformationMatrices.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <random>

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string CombineTransformationMatricesFilter::name() const
{
  return FilterTraits<CombineTransformationMatricesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CombineTransformationMatricesFilter::className() const
{
  return FilterTraits<CombineTransformationMatricesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CombineTransformationMatricesFilter::uuid() const
{
  return FilterTraits<CombineTransformationMatricesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CombineTransformationMatricesFilter::humanName() const
{
  return "Combine Transformation Matrices";
}

//------------------------------------------------------------------------------
std::vector<std::string> CombineTransformationMatricesFilter::defaultTags() const
{
  return {className(), "Matrix", "Calculate", "Multiplication"};
}

//------------------------------------------------------------------------------
Parameters CombineTransformationMatricesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_InputArrays_Key, "Input Matrices", "The list of Attribute Arrays that represent Square Matrices of all the same dimensions",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               MultiArraySelectionParameter::AllowedDataTypes{}, MultiArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Parameters"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputArray_Key, "Output Array", "The output array that contains the output from the operations.", DataPath({""})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CombineTransformationMatricesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CombineTransformationMatricesFilter::clone() const
{
  return std::make_unique<CombineTransformationMatricesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CombineTransformationMatricesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                            const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto inputArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_InputArrays_Key);
  auto outputArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_OutputArray_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(inputArrayPaths.empty())
  {
    return MakePreflightErrorResult(to_underlying(CombineTransformationMatrices::ErrorCodes::EmptyInputArrays), "No input arrays have been selected.  Please select at least 2 input arrays.");
  }

  if(inputArrayPaths.size() == 1)
  {
    return MakePreflightErrorResult(to_underlying(CombineTransformationMatrices::ErrorCodes::OneInputArray), "Only one input array has been selected.  Please select at least 2 input arrays.");
  }

  // Check for unequal array types, data types, and component dimensions
  std::vector<usize> cDims;
  IArray::ArrayType arrayType;
  std::string arrayTypeName;
  usize numTuples = 0;
  nx::core::DataType arrayDataType;
  for(usize i = 0; i < inputArrayPaths.size(); ++i)
  {
    const auto& inputDataArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPaths[i]);

    for(usize j = i + 1; j < inputArrayPaths.size(); ++j)
    {
      const auto& inputDataArray2 = dataStructure.getDataRefAs<IDataArray>(inputArrayPaths[j]);

      if(inputDataArray.getDataType() != inputDataArray2.getDataType())
      {
        return MakePreflightErrorResult(to_underlying(CombineTransformationMatrices::ErrorCodes::TypeNameMismatch),
                                        fmt::format("Input array '{}' has array type '{}', but input array '{}' has array type '{}'.  The array types must match.", inputArrayPaths[i].toString(),
                                                    inputDataArray.getTypeName(), inputArrayPaths[j].toString(), inputDataArray2.getTypeName()));
      }

      if(inputDataArray.getNumberOfComponents() != 1)

      {
        return MakePreflightErrorResult(to_underlying(CombineTransformationMatrices::ErrorCodes::ComponentShapeMismatch),
                                        fmt::format("Input array '{}' has component shape '{}'. Input arrays must only have a single component.", inputArrayPaths[i].toString(),
                                                    fmt::join(inputDataArray.getComponentShape(), ","), inputArrayPaths[j].toString()));
      }

      cDims = inputDataArray.getComponentShape();
      arrayType = inputDataArray.getArrayType();
      arrayTypeName = inputDataArray.getTypeName();
      arrayDataType = inputDataArray.getDataType();
    }

    auto tupleShape = inputDataArray.getTupleShape();
    numTuples = std::accumulate(tupleShape.begin(), tupleShape.end(), static_cast<usize>(1), std::multiplies<>());
  }

  // create the destination array for the calculated results
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, IArray::ShapeType{numTuples}, IArray::ShapeType{1}, outputArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CombineTransformationMatricesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  CombineTransformationMatricesInputValues inputValues;
  inputValues.SelectedPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_InputArrays_Key);
  inputValues.OutputPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_OutputArray_Key);

  return CombineTransformationMatrices(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
