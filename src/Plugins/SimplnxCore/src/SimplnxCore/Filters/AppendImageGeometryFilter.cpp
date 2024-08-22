#include "AppendImageGeometryFilter.hpp"

#include "ComputeArrayStatisticsFilter.hpp"
#include "SimplnxCore/Filters/Algorithms/AppendImageGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Utilities/GeometryHelpers.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string AppendImageGeometryFilter::name() const
{
  return FilterTraits<AppendImageGeometryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string AppendImageGeometryFilter::className() const
{
  return FilterTraits<AppendImageGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AppendImageGeometryFilter::uuid() const
{
  return FilterTraits<AppendImageGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AppendImageGeometryFilter::humanName() const
{
  return "Append Image Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> AppendImageGeometryFilter::defaultTags() const
{
  return {className(), "Core", "Memory Management"};
}

//------------------------------------------------------------------------------
Parameters AppendImageGeometryFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<MultiPathSelectionParameter>(k_InputGeometries_Key, "Input Image Geometries",
                                                              "The incoming image geometries (cell data) that will be appended to the destination image geometry.", std::vector<DataPath>{}));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_DestinationGeometry_Key, "Destination Image Geometry",
                                                             "The destination image geometry (cell data) that is the final location for the appended data.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ChoicesParameter>(k_AppendDimension_Key, "Append Dimension", "The dimension that will be used to append the geometries.", to_underlying(CopyFromArray::Direction::Z),
                                                   std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<BoolParameter>(k_MirrorGeometry_Key, "Mirror Geometry", "Mirrors the resulting geometry.", false));
  params.insert(std::make_unique<BoolParameter>(k_CheckResolution_Key, "Check Spacing", "Checks to make sure the spacing for the input geometry and destination geometry match", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveAsNewGeometry_Key, "Save as new geometry",
                                                                 "Save the combined data as a new geometry instead of appending the input data to the destination geometry", false));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewGeometry_Key, "New Image Geometry", "The path to the new geometry with the combined data from the input & destination geometry",
                                                             DataPath({"AppendedImageGeom"})));

  params.linkParameters(k_SaveAsNewGeometry_Key, k_NewGeometry_Key, true);
  params.linkParameters(k_SaveAsNewGeometry_Key, k_DestinationGeometry_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AppendImageGeometryFilter::clone() const
{
  return std::make_unique<AppendImageGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AppendImageGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  auto pInputGeometriesPathsValue = filterArgs.value<std::vector<DataPath>>(k_InputGeometries_Key);
  auto pDestinationGeometryPathValue = filterArgs.value<DataPath>(k_DestinationGeometry_Key);
  auto pAppendDimension = static_cast<CopyFromArray::Direction>(filterArgs.value<ChoicesParameter::ValueType>(k_AppendDimension_Key));
  auto pCheckResolutionValue = filterArgs.value<bool>(k_CheckResolution_Key);
  auto pSaveAsNewGeometry = filterArgs.value<bool>(k_SaveAsNewGeometry_Key);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& destGeometry = dataStructure.getDataRefAs<ImageGeom>(pDestinationGeometryPathValue);
  SizeVec3 destGeomDims = destGeometry.getDimensions();
  auto newDims = destGeomDims.toContainer<std::vector<usize>>();
  for(const DataPath& pInputGeometryPathValue : pInputGeometriesPathsValue)
  {
    const auto& inputGeometry = dataStructure.getDataRefAs<ImageGeom>(pInputGeometryPathValue);
    SizeVec3 inputGeomDims = inputGeometry.getDimensions();
    if((pAppendDimension == CopyFromArray::Direction::Y || pAppendDimension == CopyFromArray::Direction::Z) && destGeomDims[0] != inputGeomDims[0])
    {
      return MakePreflightErrorResult(-8200, fmt::format("Input X Dim ({}) not equal to Destination X Dim ({})", inputGeomDims[0], destGeomDims[0]));
    }
    if((pAppendDimension == CopyFromArray::Direction::X || pAppendDimension == CopyFromArray::Direction::Z) && destGeomDims[1] != inputGeomDims[1])
    {
      return MakePreflightErrorResult(-8201, fmt::format("Input Y Dim ({}) not equal to Destination Y Dim ({})", inputGeomDims[1], destGeomDims[1]));
    }
    if((pAppendDimension == CopyFromArray::Direction::X || pAppendDimension == CopyFromArray::Direction::Y) && destGeomDims[2] != inputGeomDims[2])
    {
      return MakePreflightErrorResult(-8201, fmt::format("Input Z Dim ({}) not equal to Destination Z Dim ({})", inputGeomDims[2], destGeomDims[2]));
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

    IGeometry::LengthUnit inputUnits = inputGeometry.getUnits();
    IGeometry::LengthUnit destUnits = destGeometry.getUnits();
    if(inputUnits != destUnits)
    {
      return MakePreflightErrorResult(-8310, fmt::format("Input units ({}) not equal to Destination units ({})", IGeometry::LengthUnitToString(inputUnits), IGeometry::LengthUnitToString(destUnits)));
    }

    switch(pAppendDimension)
    {
    case CopyFromArray::Direction::X:
      newDims[0] += inputGeomDims[0];
      break;
    case CopyFromArray::Direction::Y:
      newDims[1] += inputGeomDims[1];
      break;
    case CopyFromArray::Direction::Z:
      newDims[2] += inputGeomDims[2];
      break;
    }
  }

  FloatVec3 origin = destGeometry.getOrigin();
  FloatVec3 spacing = destGeometry.getSpacing();
  DataPath pNewImageGeomPath;
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

  std::vector<DataPath> addedArrayPaths;
  for(const DataPath& pInputGeometryPathValue : pInputGeometriesPathsValue)
  {
    const auto& inputGeometry = dataStructure.getDataRefAs<ImageGeom>(pInputGeometryPathValue);
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
      auto newArrayPath = newCellDataPath.createChildPath(name);
      if(std::find(addedArrayPaths.begin(), addedArrayPaths.end(), newArrayPath) != addedArrayPaths.end())
      {
        // Do not validate an array that's already been validated and added
        continue;
      }

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
          auto createArrayAction = std::make_unique<CreateArrayAction>(dataType1, newCellDataDims, dataArray1->getComponentShape(), newArrayPath);
          resultOutputActions.value().appendAction(std::move(createArrayAction));
          addedArrayPaths.push_back(newArrayPath);
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
          auto createArrayAction = std::make_unique<CreateNeighborListAction>(dataType1, numNewCellDataTuples, newArrayPath);
          resultOutputActions.value().appendAction(std::move(createArrayAction));
          addedArrayPaths.push_back(newArrayPath);
        }
      }
      if(arrayType == IArray::ArrayType::StringArray && pSaveAsNewGeometry)
      {
        auto createArrayAction = std::make_unique<CreateStringArrayAction>(newCellDataDims, newArrayPath);
        resultOutputActions.value().appendAction(std::move(createArrayAction));
        addedArrayPaths.push_back(newArrayPath);
      }
    }
  }

  if(!pSaveAsNewGeometry)
  {
    resultOutputActions.warnings().push_back(
        {-8412, "You are appending cell data together which may change the number of features. As a result, any feature level attribute matrix data will likely be invalidated!"});
    preflightUpdatedValues.push_back({"Appended Image Geometry Info", GeometryHelpers::Description::GenerateGeometryInfo(newDims, spacing, origin, destGeometry.getUnits())});
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> AppendImageGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  AppendImageGeometryInputValues inputValues;

  inputValues.InputGeometriesPaths = filterArgs.value<std::vector<DataPath>>(k_InputGeometries_Key);
  inputValues.DestinationGeometryPath = filterArgs.value<DataPath>(k_DestinationGeometry_Key);
  inputValues.Direction = static_cast<CopyFromArray::Direction>(filterArgs.value<ChoicesParameter::ValueType>(k_AppendDimension_Key));
  inputValues.CheckResolution = filterArgs.value<bool>(k_CheckResolution_Key);
  inputValues.MirrorGeometry = filterArgs.value<bool>(k_MirrorGeometry_Key);
  inputValues.SaveAsNewGeometry = filterArgs.value<bool>(k_SaveAsNewGeometry_Key);
  inputValues.NewGeometryPath = filterArgs.value<DataPath>(k_NewGeometry_Key);

  return AppendImageGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputAttributeMatrixKey = "InputAttributeMatrix";
constexpr StringLiteral k_DestinationAttributeMatrixKey = "DestinationAttributeMatrix";
constexpr StringLiteral k_CheckResolutionKey = "CheckResolution";
} // namespace SIMPL
} // namespace

Result<Arguments> AppendImageGeometryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = AppendImageGeometryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::SingleToMultiDataPathSelectionFilterParameterConverter>(args, json, SIMPL::k_InputAttributeMatrixKey, k_InputGeometries_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_DestinationAttributeMatrixKey, k_DestinationGeometry_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_CheckResolutionKey, k_CheckResolution_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
