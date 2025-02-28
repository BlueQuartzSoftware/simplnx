#include "AlignSectionsFeatureCentroidFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/AlignSectionsFeatureCentroid.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace
{

// Error Code constants
constexpr nx::core::int32 k_InputRepresentationTypeError = -68001;
constexpr nx::core::int32 k_OutputRepresentationTypeError = -68002;
constexpr nx::core::int32 k_InputComponentDimensionError = -68003;
constexpr nx::core::int32 k_InputComponentCountError = -68004;
constexpr nx::core::int32 k_InconsistentTupleCount = -68063;
constexpr nx::core::int32 k_OutOfRangeReferenceSliceValue = -68064;

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
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_SelectedCellDataGroup_Key, "Cell Data Attribute Matrix", "Cell Data Attribute Matrix", DataPath{}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "Path to the DataArray Mask", DataPath({"Mask"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Optional Alignment Output"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreAlignmentShifts_Key, "Store Alignment Shifts",
                                                                 "Whether to store the shifts applied to each section to a collection of Arrays in a new Attribute Matrix", false));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AlignmentAMName_Key, "Alignment Attribute Matrix Name",
                                                          "The output attribute matrix where the shifts applied to the section to be stored as DataArrays.", "Feature Centroid Alignment Shifts"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AlignmentSlicesArrayName_Key, "Alignment Slices Data Array Name",
                                                          "The output array name where the slice information related to shifts will be stored.", "Slice"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AlignmentPositioningArrayName_Key, "Alignment Positioning Data Array Name",
                                                          "The output array name where the new positioning information will be stored.", "Positioning"));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_AlignmentShiftsArrayName_Key, "Alignment Shifts Data Array Name", "The output array name where the shift information will be stored.", "Shifts"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AlignmentCentroidsArrayName_Key, "Alignment Centroids Data Array Name",
                                                          "The output array name where the centroid information will be stored.", "Shifts"));

  params.linkParameters(k_StoreAlignmentShifts_Key, k_AlignmentAMName_Key, true);
  params.linkParameters(k_StoreAlignmentShifts_Key, k_AlignmentSlicesArrayName_Key, true);
  params.linkParameters(k_StoreAlignmentShifts_Key, k_AlignmentPositioningArrayName_Key, true);
  params.linkParameters(k_StoreAlignmentShifts_Key, k_AlignmentShiftsArrayName_Key, true);
  params.linkParameters(k_StoreAlignmentShifts_Key, k_AlignmentCentroidsArrayName_Key, true);

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
                                                                           const std::atomic_bool& shouldCancel) const
{
  auto pReferenceSliceValue = filterArgs.value<int32>(k_ReferenceSlice_Key);
  auto pGoodVoxelsArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto cellDataGroupPath = filterArgs.value<DataPath>(k_SelectedCellDataGroup_Key);

  auto pStoreAlignmentShifts = filterArgs.value<bool>(k_StoreAlignmentShifts_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  if(pReferenceSliceValue < 0)
  {
    return MakePreflightErrorResult(k_OutOfRangeReferenceSliceValue, "Reference Slice value must be ZERO or greater.");
  }

  std::vector<DataPath> dataPaths;
  dataPaths.push_back(pGoodVoxelsArrayPath);

  // Ensure all DataArrays have the same number of Tuples
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return MakePreflightErrorResult(k_InconsistentTupleCount, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()));
  }

  // Handle Array Creation
  if(pStoreAlignmentShifts)
  {
    const auto* gridGeom = dataStructure.getDataAs<ImageGeom>(inputImageGeometry);

    if(gridGeom == nullptr)
    {
      return MakePreflightErrorResult(-68070, fmt::format("Store Alignment Shifts was selected, but an invalid image geometry was provided. Input Geometry Path :{}", inputImageGeometry.toString()));
    }

    const usize dims = gridGeom->getDimensions().getZ();
    auto pAlignmentAMName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentAMName_Key);
    const DataPath amPath = inputImageGeometry.createChildPath(pAlignmentAMName);

    // Create Parent AM
    resultOutputActions.value().appendAction(std::make_unique<CreateAttributeMatrixAction>(amPath, AttributeMatrix::ShapeType{dims}));

    // Create slices Array
    auto pAlignmentSlicesName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentSlicesArrayName_Key);
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{dims}, std::vector<usize>{2}, amPath.createChildPath(pAlignmentSlicesName)));

    // Create positioning Array
    auto pAlignmentPositioningName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentPositioningArrayName_Key);
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{dims}, std::vector<usize>{2}, amPath.createChildPath(pAlignmentPositioningName)));

    // Create shifts Array
    auto pAlignmentShiftsName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentShiftsArrayName_Key);
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::int64, std::vector<usize>{dims}, std::vector<usize>{2}, amPath.createChildPath(pAlignmentShiftsName)));

    // Create centroids Array
    auto pAlignmentCentroidsName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentCentroidsArrayName_Key);
    resultOutputActions.value().appendAction(std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{dims}, std::vector<usize>{2}, amPath.createChildPath(pAlignmentCentroidsName)));
  }

  // Inform users that the following arrays are going to be modified in place
  // Cell Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, cellDataGroupPath, {});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> AlignSectionsFeatureCentroidFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  AlignSectionsFeatureCentroidInputValues inputValues;

  inputValues.UseReferenceSlice = filterArgs.value<bool>(k_UseReferenceSlice_Key);
  inputValues.ReferenceSlice = filterArgs.value<int32>(k_ReferenceSlice_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.cellDataGroupPath = filterArgs.value<DataPath>(k_SelectedCellDataGroup_Key);

  inputValues.StoreAlignmentShifts = filterArgs.value<bool>(k_StoreAlignmentShifts_Key);
  inputValues.AlignmentAMPath = inputValues.ImageGeometryPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentAMName_Key));
  inputValues.AlignmentSlicesArrayPath = inputValues.AlignmentAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentSlicesArrayName_Key));
  inputValues.AlignmentPositioningArrayPath = inputValues.AlignmentAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentPositioningArrayName_Key));
  inputValues.AlignmentShiftsArrayPath = inputValues.AlignmentAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentShiftsArrayName_Key));
  inputValues.AlignmentCentroidsArrayPath = inputValues.AlignmentAMPath.createChildPath(filterArgs.value<DataObjectNameParameter::ValueType>(k_AlignmentCentroidsArrayName_Key));

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
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_SelectedCellDataGroup_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
