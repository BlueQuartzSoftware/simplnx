#include "InterpolateValuesToUnstructuredGridFilter.hpp"

#include "complex/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"

#include "ComplexCore/Filters/Algorithms/InterpolateValuesToUnstructuredGrid.hpp"

#include <string>

using namespace complex;

namespace complex
{

//------------------------------------------------------------------------------
std::string InterpolateValuesToUnstructuredGridFilter::name() const
{
  return FilterTraits<InterpolateValuesToUnstructuredGridFilter>::name;
}

//------------------------------------------------------------------------------
std::string InterpolateValuesToUnstructuredGridFilter::className() const
{
  return FilterTraits<InterpolateValuesToUnstructuredGridFilter>::className;
}

//------------------------------------------------------------------------------
Uuid InterpolateValuesToUnstructuredGridFilter::uuid() const
{
  return FilterTraits<InterpolateValuesToUnstructuredGridFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string InterpolateValuesToUnstructuredGridFilter::humanName() const
{
  return "Interpolate Values To Unstructured Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> InterpolateValuesToUnstructuredGridFilter::defaultTags() const
{
  return {className(), "Vertex", "Edge", "Triangle", "Quad", "Tetrahedral", "Hexahedral"};
}

//------------------------------------------------------------------------------
Parameters InterpolateValuesToUnstructuredGridFilter::parameters() const
{
  GeometrySelectionParameter::AllowedTypes geomTypes = {IGeometry::Type::Vertex, IGeometry::Type::Edge,        IGeometry::Type::Triangle,
                                                        IGeometry::Type::Quad,   IGeometry::Type::Tetrahedral, IGeometry::Type::Hexahedral};

  Parameters params;

  params.insertSeparator(Parameters::Separator{"Required Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SourceGeometry_Key, "Node-Based Geometry To Interpolate", "DataPath to node-based geometry to interpolate", DataPath(), geomTypes));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_DestinationGeometry_Key, "Interpolated Node-Based Geometry", "DataPath to node-based interpolated geometry", DataPath(), geomTypes));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_InterpolatedArrayPaths_Key, "Attribute Arrays to Interpolate", "DataPaths to interpolate", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseExistingAttrMatrix_Key, "Use Existing Attribute Matrix", "Use an existing attribute matrix to store the interpolated arrays.", false));
  params.insert(
      std::make_unique<AttributeMatrixSelectionParameter>(k_ExistingAttrMatrix_Key, "Vertex Attribute Matrix", "Vertex attribute matrix to store the interpolated data", DataPath({"VertexData"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CreatedAttrMatrix_Key, "Created Vertex Attribute Matrix", "DataPath to created AttributeMatrix for interpolated data", "VertexData"));

  params.linkParameters(k_UseExistingAttrMatrix_Key, k_CreatedAttrMatrix_Key, false);
  params.linkParameters(k_UseExistingAttrMatrix_Key, k_ExistingAttrMatrix_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer InterpolateValuesToUnstructuredGridFilter::clone() const
{
  return std::make_unique<InterpolateValuesToUnstructuredGridFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult InterpolateValuesToUnstructuredGridFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                  const std::atomic_bool& shouldCancel) const
{
  auto srcGeomPath = filterArgs.value<DataPath>(k_SourceGeometry_Key);
  auto destGeomPath = filterArgs.value<DataPath>(k_DestinationGeometry_Key);
  auto inputDataPaths = filterArgs.value<std::vector<DataPath>>(k_InterpolatedArrayPaths_Key);
  auto useExistingAttrMatrix = filterArgs.value<BoolParameter::ValueType>(k_UseExistingAttrMatrix_Key);
  auto existingAttrMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_ExistingAttrMatrix_Key);
  auto createdAttrMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CreatedAttrMatrix_Key);

  OutputActions actions;

  if(inputDataPaths.empty())
  {
    return {MakeErrorResult<OutputActions>(-3000, "There are no attribute arrays to interpolate.  Please select at least one attribute array to interpolate.")};
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(inputDataPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-3002, fmt::format("The following DataArrays all must have equal numbers of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  auto* destGeometry = dataStructure.getDataAs<INodeGeometry0D>(destGeomPath);

  DataPath interpolatedAttrMatrixPath;
  if(useExistingAttrMatrix)
  {
    interpolatedAttrMatrixPath = existingAttrMatrixPath;
  }
  else
  {
    interpolatedAttrMatrixPath = destGeomPath.createChildPath(createdAttrMatrixName);
    auto createAttrMatrixAction = std::make_unique<CreateAttributeMatrixAction>(interpolatedAttrMatrixPath, std::vector<usize>{destGeometry->getNumberOfVertices()});
    actions.appendAction(std::move(createAttrMatrixAction));
  }

  for(const auto& inputDataPath : inputDataPaths)
  {
    auto* inputArray = dataStructure.getDataAs<IDataArray>(inputDataPath);
    DataPath outputDataPath = interpolatedAttrMatrixPath.createChildPath(inputDataPath.getTargetName());

    auto createArrayAction = std::make_unique<CreateArrayAction>(inputArray->getDataType(), std::vector<usize>{destGeometry->getNumberOfVertices()}, std::vector<usize>{1}, outputDataPath);
    actions.appendAction(std::move(createArrayAction));
  }

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> InterpolateValuesToUnstructuredGridFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  InterpolateValuesToUnstructuredGridInputValues inputValues;

  inputValues.SourceGeomPath = args.value<DataPath>(k_SourceGeometry_Key);
  inputValues.DestinationGeomPath = args.value<DataPath>(k_DestinationGeometry_Key);
  inputValues.InputDataPaths = args.value<std::vector<DataPath>>(k_InterpolatedArrayPaths_Key);
  inputValues.UseExistingAttrMatrix = args.value<BoolParameter::ValueType>(k_UseExistingAttrMatrix_Key);
  inputValues.ExistingAttrMatrixPath = args.value<AttributeMatrixSelectionParameter::ValueType>(k_ExistingAttrMatrix_Key);
  inputValues.CreatedAttrMatrixName = args.value<DataObjectNameParameter::ValueType>(k_CreatedAttrMatrix_Key);

  return InterpolateValuesToUnstructuredGrid(data, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
