#include "AppendImageGeometryZSliceFilter.hpp"

#include "ComplexCore/Filters/Algorithms/AppendImageGeometryZSlice.hpp"
#include "FindArrayStatisticsFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Filter/Actions/CreateNeighborListAction.hpp"
#include "complex/Filter/Actions/CreateStringArrayAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AppendImageGeometryZSliceFilter::name() const
{
  return FilterTraits<AppendImageGeometryZSliceFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string AppendImageGeometryZSliceFilter::className() const
{
  return FilterTraits<AppendImageGeometryZSliceFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AppendImageGeometryZSliceFilter::uuid() const
{
  return FilterTraits<AppendImageGeometryZSliceFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AppendImageGeometryZSliceFilter::humanName() const
{
  return "Append Z Slice (Image Geometry)";
}

//------------------------------------------------------------------------------
std::vector<std::string> AppendImageGeometryZSliceFilter::defaultTags() const
{
  return {"Core", "Memory Management"};
}

//------------------------------------------------------------------------------
Parameters AppendImageGeometryZSliceFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputGeometry_Key, "Input Image Geometry", "The incoming image geometry (cell data) that is to be appended.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_DestinationGeometry_Key, "Destination Image Geometry",
                                                             "The destination image geometry (cell data) that is the final location for the appended data.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<BoolParameter>(k_CheckResolution_Key, "Check Spacing", "Checks to make sure the spacing for the input geometry and destination geometry match", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveAsNewGeometry_Key, "Save as new geometry",
                                                                 "Save the combined data as a new geometry instead of appending the input data to the destination geometry", false));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewGeometry_Key, "New Image Geometry", "The path to the new geometry with the combined data from the input & destination geometry",
                                                             DataPath({"AppendedImageGeom"})));
  params.linkParameters(k_SaveAsNewGeometry_Key, k_NewGeometry_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AppendImageGeometryZSliceFilter::clone() const
{
  return std::make_unique<AppendImageGeometryZSliceFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AppendImageGeometryZSliceFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel) const
{
  auto pInputGeometryPathValue = filterArgs.value<DataPath>(k_InputGeometry_Key);
  auto pDestinationGeometryPathValue = filterArgs.value<DataPath>(k_DestinationGeometry_Key);
  auto pCheckResolutionValue = filterArgs.value<bool>(k_CheckResolution_Key);
  auto pSaveAsNewGeometry = filterArgs.value<bool>(k_SaveAsNewGeometry_Key);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& inputGeometry = dataStructure.getDataRefAs<ImageGeom>(pInputGeometryPathValue);
  const auto& destGeometry = dataStructure.getDataRefAs<ImageGeom>(pDestinationGeometryPathValue);
  SizeVec3 inputGeomDims = inputGeometry.getDimensions();
  SizeVec3 destGeomDims = destGeometry.getDimensions();
  if(destGeomDims[0] != inputGeomDims[0])
  {
    return MakePreflightErrorResult(-8200, fmt::format("Input X Dim ({}) not equal to Destination X Dim ({})", inputGeomDims[0], destGeomDims[0]));
  }
  if(destGeomDims[1] != inputGeomDims[1])
  {
    return MakePreflightErrorResult(-8201, fmt::format("Input Y Dim ({}) not equal to Destination Y Dim ({})", inputGeomDims[1], destGeomDims[1]));
  }
  if(pCheckResolutionValue)
  {
    FloatVec3 inputRes = inputGeometry.getSpacing();
    FloatVec3 destRes = destGeometry.getSpacing();

    if(inputRes[0] != destRes[0])
    {
      return MakePreflightErrorResult(-8202, fmt::format("Input X Spacing ({}) not equal to Destination X Spacing ({})", inputRes[0], destRes[0]));
    }
    if(inputRes[1] != destRes[1])
    {
      return MakePreflightErrorResult(-8203, fmt::format("Input Y Spacing ({}) not equal to Destination Y Spacing ({})", inputRes[1], destRes[1]));
    }
    if(inputRes[2] != destRes[2])
    {
      return MakePreflightErrorResult(-8204, fmt::format("Input Z Spacing ({}) not equal to Destination Z Spacing ({})", inputRes[2], destRes[2]));
    }
  }

  DataPath pNewImageGeomPath;
  std::vector<usize> newDims = {destGeomDims[0], destGeomDims[1], inputGeomDims[2] + destGeomDims[2]};
  FloatVec3 origin = destGeometry.getOrigin();
  FloatVec3 spacing = destGeometry.getSpacing();
  if(pSaveAsNewGeometry)
  {
    pNewImageGeomPath = filterArgs.value<DataPath>(k_NewGeometry_Key);
    auto createGeomAction = std::make_unique<CreateImageGeometryAction>(pNewImageGeomPath, newDims, std::vector<float>{origin[0], origin[1], origin[2]},
                                                                        std::vector<float>{spacing[0], spacing[1], spacing[2]}, ImageGeom::k_CellDataName);
    resultOutputActions.value().appendAction(std::move(createGeomAction));
  }

  std::vector<usize> newCellDataDims(newDims.rbegin(), newDims.rend());
  const usize numNewCellDataTuples = std::accumulate(newCellDataDims.cbegin(), newCellDataDims.cend(), static_cast<size_t>(1), std::multiplies<>());
  usize tupleOffset = destGeometry.getNumberOfCells();
  if(tupleOffset > numNewCellDataTuples)
  {
    return MakePreflightErrorResult(-8205, fmt::format("Calculated tuple offset ({}) for appending the input data is larger than the total number of tuples ({}).", tupleOffset, numNewCellDataTuples));
  }
  const AttributeMatrix* inputCellData = inputGeometry.getCellData();
  const AttributeMatrix* destCellData = destGeometry.getCellData();
  const DataPath destCellDataPath = pDestinationGeometryPathValue.createChildPath(destCellData->getName());
  const DataPath inputCellDataPath = pInputGeometryPathValue.createChildPath(inputGeometry.getCellData()->getName());
  DataPath newCellDataPath;
  if(pSaveAsNewGeometry)
  {
    newCellDataPath = pNewImageGeomPath.createChildPath(ImageGeom::k_CellDataName);
  }
  std::vector<std::string> childNames1 = inputCellData->getDataMap().getNames();
  std::vector<std::string> childNames2 = destCellData->getDataMap().getNames();
  std::vector<std::string> combinedArrayNames;
  std::sort(childNames1.begin(), childNames1.end());
  std::sort(childNames2.begin(), childNames2.end());
  std::set_intersection(childNames1.begin(), childNames1.end(), childNames2.begin(), childNames2.end(), back_inserter(combinedArrayNames));
  for(const auto& name : combinedArrayNames)
  {
    auto* dataArray1 = dataStructure.getDataAs<IArray>(inputCellDataPath.createChildPath(name));
    if(dataArray1 == nullptr)
    {
      resultOutputActions.warnings().push_back(
          {-8206, fmt::format("Cannot append data array {} in cell data attribute matrix at path '{}' because it is not of type IArray.", name, inputCellDataPath.toString())});
      continue;
    }
    auto* dataArray2 = dataStructure.getDataAs<IArray>(destCellDataPath.createChildPath(name));
    if(dataArray2 == nullptr)
    {
      resultOutputActions.warnings().push_back(
          {-8207, fmt::format("Cannot append data array {} in cell data attribute matrix at path '{}' because it is not of type IArray.", name, destCellDataPath.toString())});
      continue;
    }
    const IArray::ArrayType arrayType = dataArray2->getArrayType();
    if(arrayType != dataArray1->getArrayType())
    {
      const std::string inputArrayStr = *IArray::StringListFromArrayType({dataArray1->getArrayType()}).begin();
      const std::string destArrayStr = *IArray::StringListFromArrayType({arrayType}).begin();
      resultOutputActions.warnings().push_back({-8208, fmt::format("Cannot append data from input data object of array type {} to destination data object of array type {} because "
                                                                   "the array types do not match.",
                                                                   inputArrayStr, destArrayStr)});
      continue;
    }
    const usize srcNumComps = dataArray1->getNumberOfComponents();
    const usize numComps = dataArray2->getNumberOfComponents();
    if(srcNumComps != numComps)
    {
      resultOutputActions.warnings().push_back(
          {-8209, fmt::format("Cannot append data from input data array with {} components to destination data array with {} components.", srcNumComps, numComps)});
      continue;
    }
    const usize srcNumElements = dataArray1->getNumberOfTuples() * srcNumComps;
    const usize numElements = numNewCellDataTuples * numComps;
    if(srcNumElements + tupleOffset * numComps > numElements)
    {
      resultOutputActions.warnings().push_back(
          {-8210, fmt::format("Cannot append data from input data array {} with {} total elements to destination data array with {} total elements starting at tuple "
                              "{} because there are not enough elements in the destination array.",
                              name, srcNumElements, numElements, tupleOffset)});
      continue;
    }

    if(arrayType == IArray::ArrayType::DataArray)
    {
      DataType dataType1 = dynamic_cast<const IDataArray*>(dataArray1)->getDataType();
      DataType dataType2 = dynamic_cast<const IDataArray*>(dataArray2)->getDataType();
      if(dataType1 != dataType2)
      {
        resultOutputActions.warnings().push_back(
            {-8211, fmt::format("Cannot append data from input data array with type {} to destination data array with type {} because the data array types do not match.",
                                DataTypeToString(dataType1).str(), DataTypeToString(dataType2).str())});
        continue;
      }

      if(pSaveAsNewGeometry)
      {
        auto createArrayAction = std::make_unique<CreateArrayAction>(dataType1, newCellDataDims, dataArray1->getComponentShape(), newCellDataPath.createChildPath(name));
        resultOutputActions.value().appendAction(std::move(createArrayAction));
      }
    }
    if(arrayType == IArray::ArrayType::NeighborListArray)
    {
      DataType dataType1 = dynamic_cast<const INeighborList*>(dataArray1)->getDataType();
      DataType dataType2 = dynamic_cast<const INeighborList*>(dataArray2)->getDataType();
      if(dataType1 != dataType2)
      {
        resultOutputActions.warnings().push_back(
            {-8211, fmt::format("Cannot append data from input data array with type {} to destination data array with type {} because the data array types do not match.",
                                DataTypeToString(dataType1).str(), DataTypeToString(dataType2).str())});
        continue;
      }

      if(pSaveAsNewGeometry)
      {
        auto createArrayAction = std::make_unique<CreateNeighborListAction>(dataType1, numNewCellDataTuples, newCellDataPath.createChildPath(name));
        resultOutputActions.value().appendAction(std::move(createArrayAction));
      }
    }
    if(arrayType == IArray::ArrayType::StringArray && pSaveAsNewGeometry)
    {
      auto createArrayAction = std::make_unique<CreateStringArrayAction>(newCellDataDims, newCellDataPath.createChildPath(name));
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }
  }

  if(!pSaveAsNewGeometry)
  {
    resultOutputActions.warnings().push_back(
        {-8412, "You are appending cell data together which may change the number of features. As a result, any feature level attribute matrix data will likely be invalidated!"});
    preflightUpdatedValues.push_back({"Appended Image Geometry Info", GeometryHelpers::Description::GenerateGeometryInfo(newDims, spacing, origin)});
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> AppendImageGeometryZSliceFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel) const
{
  AppendImageGeometryZSliceInputValues inputValues;

  inputValues.InputGeometryPath = filterArgs.value<DataPath>(k_InputGeometry_Key);
  inputValues.DestinationGeometryPath = filterArgs.value<DataPath>(k_DestinationGeometry_Key);
  inputValues.CheckResolution = filterArgs.value<bool>(k_CheckResolution_Key);
  inputValues.SaveAsNewGeometry = filterArgs.value<bool>(k_SaveAsNewGeometry_Key);
  inputValues.NewGeometryPath = filterArgs.value<DataPath>(k_NewGeometry_Key);

  return AppendImageGeometryZSlice(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
