#include "InitializeImageGeomCellDataFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/InitializeImageGeomCellData.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace
{
using RangeType = std::pair<float64, float64>;
using InitType = InitializeImageGeomCellDataFilter::InitType;

InitType ConvertIndexToInitType(uint64 index)
{
  switch(index)
  {
  case static_cast<uint64>(InitType::Manual): {
    return InitType::Manual;
  }
  case static_cast<uint64>(InitType::Random): {
    return InitType::Random;
  }
  case static_cast<uint64>(InitType::RandomWithRange): {
    return InitType::RandomWithRange;
  }
  default: {
    throw std::runtime_error("InitializeImageGeomCellData: Invalid value for InitType");
  }
  }
}

struct CheckInitializationFunctor
{
  template <class T>
  std::optional<Error> operator()(const std::string& arrayName, InitializeImageGeomCellDataFilter::InitType initType, float64 initValue, const std::pair<float64, float64>& initRange)
  {
    if(initType == InitializeImageGeomCellDataFilter::InitType::Manual)
    {
      if(initValue < static_cast<double>(std::numeric_limits<T>().lowest()) || initValue > static_cast<double>(std::numeric_limits<T>().max()))
      {
        return Error{-4000, fmt::format("{}: The initialization value could not be converted. The valid range is {} to {}", arrayName, std::numeric_limits<T>::min(), std::numeric_limits<T>::max())};
      }
    }
    else if(initType == InitializeImageGeomCellDataFilter::InitType::RandomWithRange)
    {
      float64 min = initRange.first;
      float64 max = initRange.second;
      if(min > max)
      {
        return Error{-5550, fmt::format("{}: Invalid initialization range.  Minimum value is larger than maximum value.", arrayName)};
      }
      if(min < static_cast<double>(std::numeric_limits<T>().lowest()) || max > static_cast<double>(std::numeric_limits<T>().max()))
      {
        return Error{-4001, fmt::format("{}: The initialization range can only be from {} to {}", arrayName, std::numeric_limits<T>::min(), std::numeric_limits<T>::max())};
      }
      if(min == max)
      {
        return Error{-4002, fmt::format("{}: The initialization range must have differing values", arrayName)};
      }
    }

    return {};
  }
};
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string InitializeImageGeomCellDataFilter::name() const
{
  return FilterTraits<InitializeImageGeomCellDataFilter>::name;
}

//------------------------------------------------------------------------------
std::string InitializeImageGeomCellDataFilter::className() const
{
  return FilterTraits<InitializeImageGeomCellDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid InitializeImageGeomCellDataFilter::uuid() const
{
  return FilterTraits<InitializeImageGeomCellDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string InitializeImageGeomCellDataFilter::humanName() const
{
  return "Initialize Image Geometry Cell Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> InitializeImageGeomCellDataFilter::defaultTags() const
{
  return {className(), "Memory Management", "Initialize", "Create", "Generate", "Data"};
}

//------------------------------------------------------------------------------
Parameters InitializeImageGeomCellDataFilter::parameters() const
{
  Parameters params;

  // TODO: restrict types
  params.insertSeparator(Parameters::Separator{"Random Number Seed Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseSeed_Key, "Use Seed for Random Generation", "When true the user will be able to put in a seed for random generation", false));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed Value", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "InitializeImageGeomCellDataFilter SeedValue"));

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorUInt64Parameter>(k_MinPoint_Key, "Min Point", "The minimum x, y, z bound in cells", std::vector<uint64>{0, 0, 0},
                                                        std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_MaxPoint_Key, "Max Point", "The maximum x, y, z bound in cells", std::vector<uint64>{0, 0, 0},
                                                        std::vector<std::string>{"X (Column)", "Y (Row)", "Z (Plane)"}));
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_InitType_Key, "Initialization Type", "Tells how to initialize the data", 0, ChoicesParameter::Choices{"Manual", "Random", "Random With Range"}));
  params.insert(std::make_unique<Float64Parameter>(k_InitValue_Key, "Initialization Value", "The initialization value if Manual Initialization Type is selected", 0.0f));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_InitRange_Key, "Initialization Range", "The initialization range if Random With Range Initialization Type is selected",
                                                         VectorFloat64Parameter::ValueType{0.0, 0.0}));
  params.linkParameters(k_InitType_Key, k_InitValue_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_InitType_Key, k_InitRange_Key, std::make_any<ChoicesParameter::ValueType>(2));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CellArrayPaths_Key, "Cell Arrays", "The cell data arrays in which to initialize a sub-volume to zeros", std::vector<DataPath>{},
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, nx::core::GetAllDataTypes()));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "The geometry containing the cell data for which to initialize", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseSeed_Key, k_SeedValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType InitializeImageGeomCellDataFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer InitializeImageGeomCellDataFilter::clone() const
{
  return std::make_unique<InitializeImageGeomCellDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult InitializeImageGeomCellDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                          const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto cellArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_CellArrayPaths_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto minPoint = filterArgs.value<std::vector<uint64>>(k_MinPoint_Key);
  auto maxPoint = filterArgs.value<std::vector<uint64>>(k_MaxPoint_Key);
  auto initTypeIndex = filterArgs.value<uint64>(k_InitType_Key);
  auto initValue = filterArgs.value<float64>(k_InitValue_Key);
  auto initRangeVec = filterArgs.value<std::vector<float64>>(k_InitRange_Key);
  auto pSeedArrayNameValue = filterArgs.value<std::string>(k_SeedArrayName_Key);

  uint64 xMin = minPoint.at(0);
  uint64 yMin = minPoint.at(1);
  uint64 zMin = minPoint.at(2);

  uint64 xMax = maxPoint.at(0);
  uint64 yMax = maxPoint.at(1);
  uint64 zMax = maxPoint.at(2);

  InitType initType = ConvertIndexToInitType(initTypeIndex);
  RangeType initRange = {initRangeVec.at(0), initRangeVec.at(1)};

  if(cellArrayPaths.empty())
  {
    return {MakeErrorResult<OutputActions>(-3550, "At least one data array must be selected.")};
  }

  std::vector<Error> errors;

  if(xMax < xMin)
  {
    errors.push_back(Error{-3551, fmt::format("X Max ({}) less than X Min ({})", xMax, xMin)});
  }
  if(yMax < yMin)
  {
    errors.push_back(Error{-3552, fmt::format("Y Max ({}) less than Y Min ({})", yMax, yMin)});
  }
  if(zMax < zMin)
  {
    errors.push_back(Error{-3553, fmt::format("Z Max ({}) less than Z Min ({})", zMax, zMin)});
  }

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  if(xMax > (static_cast<int64>(imageGeom.getNumXCells()) - 1))
  {
    errors.push_back(Error{-3554, fmt::format("The X Max you entered of {} is greater than your Max X Point of {}", xMax, static_cast<int64>(imageGeom.getNumXCells()) - 1)});
  }
  if(yMax > (static_cast<int64>(imageGeom.getNumYCells()) - 1))
  {
    errors.push_back(Error{-3555, fmt::format("The Y Max you entered of {} is greater than your Max Y Point of {}", yMax, static_cast<int64>(imageGeom.getNumYCells()) - 1)});
  }
  if(zMax > (static_cast<int64>(imageGeom.getNumZCells()) - 1))
  {
    errors.push_back(Error{-3556, fmt::format("The Z Max you entered of {} is greater than your Max Z Point of {}", zMax, static_cast<int64>(imageGeom.getNumZCells()) - 1)});
  }

  SizeVec3 imageDims = imageGeom.getDimensions();

  std::vector<usize> reversedImageDims(std::make_reverse_iterator(imageDims.end()), std::make_reverse_iterator(imageDims.begin()));

  for(const DataPath& path : cellArrayPaths)
  {
    const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(path);
    ShapeType tupleShape = dataArray.getIDataStoreRef().getTupleShape();

    if(tupleShape.size() != reversedImageDims.size())
    {
      errors.push_back(Error{-5560, fmt::format("DataArray at '{}' does not match dimensions of ImageGeometry at '{}'", path.toString(), imageGeomPath.toString())});
      continue;
    }

    std::optional<Error> maybeError = ExecuteNeighborFunction(CheckInitializationFunctor{}, dataArray.getDataType(), dataArray.getName(), initType, initValue, initRange); // NO BOOL
    if(maybeError.has_value())
    {
      errors.push_back(std::move(*maybeError));
    }
  }

  if(!errors.empty())
  {
    return {nonstd::make_unexpected(std::move(errors))};
  }

  nx::core::Result<OutputActions> resultOutputActions;

  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({pSeedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  // Inform users that the following arrays are going to be modified in place
  for(const DataPath& path : cellArrayPaths)
  {
    nx::core::MarkDataPathModified(dataStructure, resultOutputActions, path);
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> InitializeImageGeomCellDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  InitializeImageGeomCellDataInputValues inputValues;
  inputValues.CellArrays = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_CellArrayPaths_Key);
  inputValues.InputImageGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_ImageGeometryPath_Key);
  inputValues.MinPoint = filterArgs.value<VectorUInt64Parameter::ValueType>(k_MinPoint_Key);
  inputValues.MaxPoint = filterArgs.value<VectorUInt64Parameter::ValueType>(k_MaxPoint_Key);
  inputValues.InitTypeIndex = filterArgs.value<ChoicesParameter::ValueType>(k_InitType_Key);
  inputValues.InitValue = filterArgs.value<Float64Parameter::ValueType>(k_InitValue_Key);
  inputValues.InitRange = filterArgs.value<VectorFloat64Parameter::ValueType>(k_InitRange_Key);
  inputValues.SeedValue = filterArgs.value<UInt64Parameter::ValueType>(k_SeedValue_Key);
  inputValues.UseSeed = filterArgs.value<BoolParameter::ValueType>(k_UseSeed_Key);
  inputValues.SeedArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_SeedArrayName_Key);

  return InitializeImageGeomCellData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_CellAttributeMatrixPathsKey = "CellAttributeMatrixPaths";
constexpr StringLiteral k_XMinKey = "XMin";
constexpr StringLiteral k_YMinKey = "YMin";
constexpr StringLiteral k_ZMinKey = "ZMin";
constexpr StringLiteral k_XMaxKey = "XMax";
constexpr StringLiteral k_YMaxKey = "YMax";
constexpr StringLiteral k_ZMaxKey = "ZMax";
constexpr StringLiteral k_InitTypeKey = "InitType";
constexpr StringLiteral k_InitValueKey = "InitValue";
constexpr StringLiteral k_InitRangeKey = "InitRange";
} // namespace SIMPL
} // namespace

Result<Arguments> InitializeImageGeomCellDataFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = InitializeImageGeomCellDataFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerFromMultiSelectionFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixPathsKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixPathsKey, k_CellArrayPaths_Key));
  results.push_back(SIMPLConversion::Convert3Parameters<SIMPLConversion::UInt64ToVec3FilterParameterConverter>(args, json, SIMPL::k_XMinKey, SIMPL::k_YMinKey, SIMPL::k_ZMinKey, k_MinPoint_Key));
  results.push_back(SIMPLConversion::Convert3Parameters<SIMPLConversion::UInt64ToVec3FilterParameterConverter>(args, json, SIMPL::k_XMaxKey, SIMPL::k_YMaxKey, SIMPL::k_ZMaxKey, k_MaxPoint_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_InitTypeKey, k_InitType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_InitValueKey, k_InitValue_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::RangeFilterParameterConverter>(args, json, SIMPL::k_InitRangeKey, k_InitRange_Key));
  // Invert Data parameter is not applicable in NX

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
