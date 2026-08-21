#include "AlignSectionsFeatureCentroidFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/AlignSectionsFeatureCentroid.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace
{

// Error Code constants
constexpr nx::core::int32 k_NegativeReferenceSliceValue = -68064;
constexpr nx::core::int32 k_MissingImageGeometry = -68070;
constexpr nx::core::int32 k_ReferenceSliceBeyondZDim = -68071;
constexpr nx::core::int32 k_GeometryNotThreeDimensional = -68072;
constexpr nx::core::int32 k_NonDataArrayCellChild = -68073;
constexpr nx::core::int32 k_MissingCellAttributeMatrix = -68074;
constexpr nx::core::int32 k_MaskCellCountMismatch = -68075;

// Only the tuples of the slices that actually move are written, so every alignment-shift array is
// filled with zeros to keep the untouched anchor tuple deterministic instead of uninitialized memory.
constexpr nx::core::StringLiteral k_ZeroFillValue = "0";

} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string AlignSectionsFeatureCentroidFilter::name() const
{
  return FilterTraits<AlignSectionsFeatureCentroidFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string AlignSectionsFeatureCentroidFilter::className() const
{
  return FilterTraits<AlignSectionsFeatureCentroidFilter>::className;
}

//------------------------------------------------------------------------------
Uuid AlignSectionsFeatureCentroidFilter::uuid() const
{
  return FilterTraits<AlignSectionsFeatureCentroidFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string AlignSectionsFeatureCentroidFilter::humanName() const
{
  return "Align Sections (Feature Centroid)";
}

//------------------------------------------------------------------------------
std::vector<std::string> AlignSectionsFeatureCentroidFilter::defaultTags() const
{
  return {className(), "Reconstruction", "Alignment"};
}

//------------------------------------------------------------------------------
Parameters AlignSectionsFeatureCentroidFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseReferenceSlice_Key, "Use Reference Slice",
                                                                 "Whether the centroids of each section should be compared to a reference slice instead of their neighboring section", false));
  params.insert(std::make_unique<Int32Parameter>(k_ReferenceSlice_Key, "Reference Slice", "Slice number to use as reference", 0));
  params.linkParameters(k_UseReferenceSlice_Key, k_ReferenceSlice_Key, true);

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry on which to perform the alignment",
                                                             DataPath({"Data Container"}), GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "Specifies if the Cell is to be counted in the algorithm.", DataPath({"Mask"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Optional Alignment Output"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreAlignmentShifts_Key, "Store Alignment Shifts",
                                                                 "Whether to store the shifts applied to each section to a collection of Arrays in a new Attribute Matrix", false));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AlignmentAMName_Key, "Alignment Attribute Matrix Name",
                                                          "The output attribute matrix where the shifts applied to the section to be stored as DataArrays.", "Alignment Shifts Data"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SlicesArrayName_Key, "Alignment Slices Data Array Name",
                                                          "The output array name where the slice information related to shifts will be stored.", "Slice Indices"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_RelativeShiftsArrayName_Key, "Alignment Relative Shifts Data Array Name",
                                                          "The output array name where the new shifts relative to previous slice information will be stored.", "Relative Shifts"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CumulativeShiftsArrayName_Key, "Alignment Cumulative Shifts Data Array Name",
                                                          "The output array name where the accumulated shift information will be stored.", "Cumulative Shifts"));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_CentroidsArrayName_Key, "Alignment Centroids Data Array Name", "The output array name where the centroid information will be stored.", "Centroids"));

  params.linkParameters(k_StoreAlignmentShifts_Key, k_AlignmentAMName_Key, true);
  params.linkParameters(k_StoreAlignmentShifts_Key, k_SlicesArrayName_Key, true);
  params.linkParameters(k_StoreAlignmentShifts_Key, k_RelativeShiftsArrayName_Key, true);
  params.linkParameters(k_StoreAlignmentShifts_Key, k_CumulativeShiftsArrayName_Key, true);
  params.linkParameters(k_StoreAlignmentShifts_Key, k_CentroidsArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType AlignSectionsFeatureCentroidFilter::parametersVersion() const
{
  return 2;

  // Version 1 -> 2
  // Description:
  // Swapped Writing Shifts to a file to storing them in Data Arrays
  //
  // Change 1:
  // Replaced - k_WriteAlignmentShifts_Key = "write_alignment_shifts" -> k_StoreAlignmentShifts_Key = "store_alignment_shifts";
  // Solution - `k_StoreAlignmentShifts_Key Value` = `k_WriteAlignmentShifts_Key Value`;
  //
  // Change 2:
  // Replaced - k_AlignmentShiftFileName_Key = "alignment_shift_file_name" -> k_AlignmentAMName_Key = "alignment_attribute_matrix_name";
  // Solution: (For backwards pipeline conversion, else just use default)
  // Steps:
  //  1. Read File Name
  //  2. Strip extension
  //  3. `k_AlignmentShiftArrayName_Key Value` = cleaned up file name
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AlignSectionsFeatureCentroidFilter::clone() const
{
  return std::make_unique<AlignSectionsFeatureCentroidFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AlignSectionsFeatureCentroidFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pUseReferenceSliceValue = filterArgs.value<bool>(k_UseReferenceSlice_Key);
  auto pReferenceSliceValue = filterArgs.value<int32>(k_ReferenceSlice_Key);
  auto pGoodVoxelsArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  auto pStoreAlignmentShifts = filterArgs.value<bool>(k_StoreAlignmentShifts_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  const auto* imageGeomPtr = dataStructure.getDataAs<ImageGeom>(inputImageGeometry);
  if(imageGeomPtr == nullptr)
  {
    return MakePreflightErrorResult(k_MissingImageGeometry, fmt::format("An Image Geometry could not be found at the selected path '{}'.", inputImageGeometry.toString()));
  }

  const SizeVec3 imageGeomDims = imageGeomPtr->getDimensions();
  if(imageGeomDims[0] <= 1 || imageGeomDims[1] <= 1 || imageGeomDims[2] <= 1)
  {
    return MakePreflightErrorResult(k_GeometryNotThreeDimensional,
                                    fmt::format("The Image Geometry '{}' is not 3D and cannot be aligned section by section. The dimensions are ({}, {}, {}) and every dimension must be at least 2.",
                                                inputImageGeometry.toString(), imageGeomDims[0], imageGeomDims[1], imageGeomDims[2]));
  }

  // The Reference Slice is only consulted when Use Reference Slice is enabled, so it is only
  // validated in that case.
  if(pUseReferenceSliceValue)
  {
    if(pReferenceSliceValue < 0)
    {
      return MakePreflightErrorResult(k_NegativeReferenceSliceValue, fmt::format("Reference Slice value ({}) must be ZERO or greater.", pReferenceSliceValue));
    }
    if(static_cast<usize>(pReferenceSliceValue) >= imageGeomDims[2])
    {
      return MakePreflightErrorResult(k_ReferenceSliceBeyondZDim, fmt::format("Reference Slice value ({}) is outside the Image Geometry '{}', which has {} slices. The valid range is 0 to {}.",
                                                                              pReferenceSliceValue, inputImageGeometry.toString(), imageGeomDims[2], imageGeomDims[2] - 1));
    }
  }

  // Every child of the Cell Attribute Matrix is shifted, and the shift requires an IDataArray.
  // StringArray and NeighborList are IArray but not IDataArray, so they have to be rejected here.
  const AttributeMatrix* cellDataAmPtr = imageGeomPtr->getCellData();
  if(cellDataAmPtr == nullptr)
  {
    return MakePreflightErrorResult(k_MissingCellAttributeMatrix,
                                    fmt::format("The Image Geometry '{}' does not have a Cell Attribute Matrix, so there is no Cell data to align.", inputImageGeometry.toString()));
  }
  const DataPath cellDataAmPath = inputImageGeometry.createChildPath(cellDataAmPtr->getName());
  if(auto childPathsResult = GetAllChildDataPaths(dataStructure, cellDataAmPath); childPathsResult.has_value())
  {
    std::vector<std::string> unsupportedChildren;
    for(const auto& childPath : childPathsResult.value())
    {
      if(dataStructure.getDataAs<IDataArray>(childPath) == nullptr)
      {
        unsupportedChildren.push_back(childPath.getTargetName());
      }
    }
    if(!unsupportedChildren.empty())
    {
      return MakePreflightErrorResult(k_NonDataArrayCellChild,
                                      fmt::format("The Cell Attribute Matrix '{}' contains {} object(s) that are not Data Arrays and cannot be shifted: '{}'. Remove or move them before aligning.",
                                                  cellDataAmPath.toString(), unsupportedChildren.size(), fmt::join(unsupportedChildren, "', '")));
    }
  }

  // The Mask Array is indexed by Cell id across the whole geometry, and MaskCompare reaches its
  // store through AbstractDataStore::at(), which throws std::out_of_range past the end. IFilter does
  // not wrap execute in a try/catch, so a Mask Array that does not hold exactly one tuple per Cell
  // would escape the filter as an uncaught exception instead of an error.
  const usize numCells = imageGeomPtr->getNumberOfCells();
  const usize numMaskTuples = dataStructure.getDataRefAs<IDataArray>(pGoodVoxelsArrayPath).getNumberOfTuples();
  if(numMaskTuples != numCells)
  {
    return MakePreflightErrorResult(k_MaskCellCountMismatch, fmt::format("The Mask Array '{}' has {} tuples but the Image Geometry '{}' has {} Cells. The Mask Array must have exactly one tuple per "
                                                                         "Cell of the selected Image Geometry.",
                                                                         pGoodVoxelsArrayPath.toString(), numMaskTuples, inputImageGeometry.toString(), numCells));
  }

  // Handle Array Creation
  if(pStoreAlignmentShifts)
  {
    const usize dims = imageGeomDims.getZ();
    auto pAlignmentAMName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentAMName_Key);
    const DataPath amPath = inputImageGeometry.createChildPath(pAlignmentAMName);

    // Create Parent AM
    resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(amPath, ShapeType{dims}));

    // Create slices Array
    auto pSlicesName = filterArgs.value<DataObjectNameParameter::ValueType>(k_SlicesArrayName_Key);
    resultOutputActions.value().appendAction(
        std::make_unique<CreateArrayAction>(DataType::uint32, std::vector<usize>{dims}, std::vector<usize>{2}, amPath.createChildPath(pSlicesName), "", k_ZeroFillValue.str()));

    // Create positioning Array
    auto pRelativeShiftsName = filterArgs.value<DataObjectNameParameter::ValueType>(k_RelativeShiftsArrayName_Key);
    resultOutputActions.value().appendAction(
        std::make_unique<CreateArrayAction>(DataType::int64, std::vector<usize>{dims}, std::vector<usize>{2}, amPath.createChildPath(pRelativeShiftsName), "", k_ZeroFillValue.str()));

    // Create shifts Array
    auto pCumulativeShiftsName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CumulativeShiftsArrayName_Key);
    resultOutputActions.value().appendAction(
        std::make_unique<CreateArrayAction>(DataType::int64, std::vector<usize>{dims}, std::vector<usize>{2}, amPath.createChildPath(pCumulativeShiftsName), "", k_ZeroFillValue.str()));

    // Create centroids Array
    auto pCentroidsName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CentroidsArrayName_Key);
    resultOutputActions.value().appendAction(
        std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{dims}, std::vector<usize>{2}, amPath.createChildPath(pCentroidsName), "", k_ZeroFillValue.str()));
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, pGoodVoxelsArrayPath.getParent(), {});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> AlignSectionsFeatureCentroidFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  AlignSectionsFeatureCentroidInputValues inputValues;

  inputValues.UseReferenceSlice = filterArgs.value<bool>(k_UseReferenceSlice_Key);
  inputValues.ReferenceSlice = filterArgs.value<int32>(k_ReferenceSlice_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  inputValues.StoreAlignmentShifts = filterArgs.value<bool>(k_StoreAlignmentShifts_Key);
  inputValues.AlignmentAMPath = inputValues.ImageGeometryPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentAMName_Key));
  inputValues.SlicesArrayPath = inputValues.AlignmentAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_SlicesArrayName_Key));
  inputValues.RelativeShiftsArrayPath = inputValues.AlignmentAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_RelativeShiftsArrayName_Key));
  inputValues.CumulativeShiftsArrayPath = inputValues.AlignmentAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_CumulativeShiftsArrayName_Key));
  inputValues.CentroidsArrayPath = inputValues.AlignmentAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_CentroidsArrayName_Key));

  return AlignSectionsFeatureCentroid(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_WriteAlignmentShiftsKey = "WriteAlignmentShifts";
constexpr StringLiteral k_UseReferenceSliceKey = "UseReferenceSlice";
constexpr StringLiteral k_ReferenceSliceKey = "ReferenceSlice";
constexpr StringLiteral k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> AlignSectionsFeatureCentroidFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = AlignSectionsFeatureCentroidFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_WriteAlignmentShiftsKey, k_StoreAlignmentShifts_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseReferenceSliceKey, k_UseReferenceSlice_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_ReferenceSliceKey, k_ReferenceSlice_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
