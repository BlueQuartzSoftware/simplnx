#include "ResampleRectGridToImageGeomFilter.hpp"

#include "ComplexCore/Filters/Algorithms/ResampleRectGridToImageGeom.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/DataStructure/INeighborList.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Filter/Actions/CreateNeighborListAction.hpp"
#include "complex/Filter/Actions/CreateStringArrayAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ResampleRectGridToImageGeomFilter::name() const
{
  return FilterTraits<ResampleRectGridToImageGeomFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ResampleRectGridToImageGeomFilter::className() const
{
  return FilterTraits<ResampleRectGridToImageGeomFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ResampleRectGridToImageGeomFilter::uuid() const
{
  return FilterTraits<ResampleRectGridToImageGeomFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ResampleRectGridToImageGeomFilter::humanName() const
{
  return "Resample Rectilinear Grid to Image Geom";
}

//------------------------------------------------------------------------------
std::vector<std::string> ResampleRectGridToImageGeomFilter::defaultTags() const
{
  return {"Unsupported", "Sampling"};
}

//------------------------------------------------------------------------------
Parameters ResampleRectGridToImageGeomFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Geometry Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_RectilinearGridPath_Key, "Input Rectilinear Grid", "Path to the RectGrid Geometry to be re-sampled", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::RectGrid}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(
      k_SelectedDataArrayPaths_Key, "Attribute Arrays to Copy", "Rectilinear Grid Cell Data to possibly copy", MultiArraySelectionParameter::ValueType{},
      MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray, IArray::ArrayType::StringArray, IArray::ArrayType::NeighborListArray}, GetAllDataTypes()));
  params.insertSeparator(Parameters::Separator{"Output Image Geometry Parameters"});
  params.insert(std::make_unique<VectorInt32Parameter>(k_Dimensions_Key, "Dimensions (Voxels)", "The image geometry voxel dimensions in which to re-sample the rectilinear grid geometry",
                                                       std::vector<int32>{128, 128, 128}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeometryPath_Key, "Created Image Geometry", "Path to the created Image Geometry", DataPath({"Image Geometry"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ImageGeomCellAttributeMatrix_Key, "Cell Attribute Matrix", "The name of the cell data Attribute Matrix created with the Image Geometry",
                                                          ImageGeom::k_CellDataName));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ResampleRectGridToImageGeomFilter::clone() const
{
  return std::make_unique<ResampleRectGridToImageGeomFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ResampleRectGridToImageGeomFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                          const std::atomic_bool& shouldCancel) const
{
  auto pRectilinearGridPathValue = filterArgs.value<DataPath>(k_RectilinearGridPath_Key);
  auto pSelectedDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pDimensionsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  auto pImageGeometryPathValue = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pImageGeomCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_ImageGeomCellAttributeMatrix_Key);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // check for valid inputs
  if(pSelectedDataArrayPathsValue.empty())
  {
    return MakePreflightErrorResult(-7360, "At least one attribute array must be selected");
  }

  if(!dataStructure.validateNumberOfTuples(pSelectedDataArrayPathsValue))
  {
    return MakePreflightErrorResult(-7361, "One or more of the selected attribute arrays have mismatching numbers of tuples. All selected attribute arrays must come from the same attribute matrix.");
  }

  if(pDimensionsValue[0] < 0)
  {
    return MakePreflightErrorResult(-7362, fmt::format("The x dimension value ({}) for the created image geometry must be positive", pDimensionsValue[0]));
  }
  if(pDimensionsValue[1] < 0)
  {
    return MakePreflightErrorResult(-7363, fmt::format("The y dimension value ({}) for the created image geometry must be positive", pDimensionsValue[1]));
  }
  if(pDimensionsValue[2] < 0)
  {
    return MakePreflightErrorResult(-7364, fmt::format("The z dimension value ({}) for the created image geometry must be positive", pDimensionsValue[2]));
  }

  const std::vector<usize> dims = {static_cast<usize>(pDimensionsValue[0]), static_cast<usize>(pDimensionsValue[1]), static_cast<usize>(pDimensionsValue[2])};

  // Create the re-sampled image geometry - origin & spacing values will be calculated & updated in execute
  {
    auto createDataGroupAction = std::make_unique<CreateImageGeometryAction>(pImageGeometryPathValue, dims, std::vector<float32>{0.0f, 0.0f, 0.0f}, std::vector<float32>{1.0f, 1.0f, 1.0f},
                                                                             pImageGeomCellAttributeMatrixNameValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }

  // create the IArrays for copying over to the re-sampled image geometry
  const auto& rectGridGeom = dataStructure.getDataRefAs<RectGridGeom>(pRectilinearGridPathValue);
  const DataPath srcCellDataPath = pRectilinearGridPathValue.createChildPath(rectGridGeom.getCellData()->getName());
  const DataPath destCellDataPath = pImageGeometryPathValue.createChildPath(pImageGeomCellAttributeMatrixNameValue);
  usize totalPoints = dims[0] * dims[1] * dims[2];
  for(const auto& path : pSelectedDataArrayPathsValue)
  {
    const DataPath destPath = destCellDataPath.createChildPath(path.getTargetName());
    const auto* srcArray = dataStructure.getDataAs<IArray>(path);

    if(srcCellDataPath != path.getParent())
    {
      const usize numGeomCells = rectGridGeom.getNumberOfCells();
      const usize numArrayElements = srcArray->getNumberOfTuples() * srcArray->getNumberOfComponents();
      if(numGeomCells == numArrayElements)
      {
        resultOutputActions.warnings().push_back({-7365, fmt::format("Data Object at path '{}' is not a cell level array but has the same number of elements as the parent geometry and will be copied "
                                                                     "over to the re-sampled image geometry cell level attribute matrix",
                                                                     path.toString())});
      }
      else
      {
        return MakePreflightErrorResult(
            -7365,
            fmt::format(
                "Data Object at path '{}' is not a cell level array and has {} elements. The selected arrays must have the same number of elements as the parent geometry ({}) in order to be copied "
                "over to the re-sampled image geometry",
                path.toString(), numArrayElements, numGeomCells));
      }
    }

    IArray::ArrayType arrayType = srcArray->getArrayType();
    if(arrayType == IArray::ArrayType::DataArray)
    {
      const auto* srcDataArray = dataStructure.getDataAs<IDataArray>(path);
      auto createArrayAction = std::make_unique<CreateArrayAction>(srcDataArray->getDataType(), dims, srcDataArray->getComponentShape(), destPath);
      resultOutputActions.value().actions.push_back(std::move(createArrayAction));
    }
    else if(arrayType == IArray::ArrayType::NeighborListArray)
    {
      const auto* srcDataArray = dataStructure.getDataAs<INeighborList>(path);
      auto createArrayAction = std::make_unique<CreateNeighborListAction>(srcDataArray->getDataType(), totalPoints, destPath);
      resultOutputActions.value().actions.push_back(std::move(createArrayAction));
    }
    else if(arrayType == IArray::ArrayType::StringArray)
    {
      auto createArrayAction = std::make_unique<CreateStringArrayAction>(dims, destPath);
      resultOutputActions.value().actions.push_back(std::move(createArrayAction));
    }
    else
    {
      resultOutputActions.warnings().push_back({-7366, fmt::format("Data Object at path '{}' is not an IArray type and thus cannot be copied over to the re-sampled image geometry", path.toString())});
    }
  }

  // Generate geometry info for preflight updated values
  const SizeVec3 srcDims = rectGridGeom.getDimensions();
  std::stringstream rectGridGeometryDesc;
  rectGridGeometryDesc << srcDims[0] << " Voxels\n" << srcDims[1] << " Voxels\n" << srcDims[2] << " Voxels\n";
  preflightUpdatedValues.push_back({"Rect Grid Geometry Info", rectGridGeometryDesc.str()});
  std::stringstream createdGeometryDesc;
  createdGeometryDesc << dims[0] << " Voxels\n" << dims[1] << " Voxels\n" << dims[2] << " Voxels\n";
  preflightUpdatedValues.push_back({"Created Image Geometry Info", createdGeometryDesc.str()});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ResampleRectGridToImageGeomFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  ResampleRectGridToImageGeomInputValues inputValues;

  inputValues.RectilinearGridPath = filterArgs.value<DataPath>(k_RectilinearGridPath_Key);
  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputValues.Dimensions = filterArgs.value<VectorInt32Parameter::ValueType>(k_Dimensions_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.ImageGeomCellAttributeMatrixName = filterArgs.value<std::string>(k_ImageGeomCellAttributeMatrix_Key);

  return ResampleRectGridToImageGeom(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
