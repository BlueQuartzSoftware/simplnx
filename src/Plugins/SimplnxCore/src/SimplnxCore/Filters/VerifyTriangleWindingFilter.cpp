#include "VerifyTriangleWindingFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/VerifyTriangleWinding.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{
Result<DataPath> FindParentGeometry(const DataPath& path, const DataStructure& dataStructure)
{
  auto* maybeTriGeomPtr = dataStructure.getDataAs<TriangleGeom>(path);
  if(maybeTriGeomPtr != nullptr)
  {
    return {path};
  }

  const DataPath parentPath = path.getParent();
  if(parentPath.empty())
  {
    return {{nonstd::make_unexpected(ErrorCollection{Error{-25740, "Unable to find a Triangle Geometry on the supplied path"}})}};
  }

  return FindParentGeometry(parentPath, dataStructure);
}

constexpr StringLiteral k_TempName = "__INTERNAL_!_TEMP_!_PATH__";
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string VerifyTriangleWindingFilter::name() const
{
  return FilterTraits<VerifyTriangleWindingFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string VerifyTriangleWindingFilter::className() const
{
  return FilterTraits<VerifyTriangleWindingFilter>::className;
}

//------------------------------------------------------------------------------
Uuid VerifyTriangleWindingFilter::uuid() const
{
  return FilterTraits<VerifyTriangleWindingFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string VerifyTriangleWindingFilter::humanName() const
{
  return "Verify Triangle Winding";
}

//------------------------------------------------------------------------------
std::vector<std::string> VerifyTriangleWindingFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Connectivity Arrangement"};
}

//------------------------------------------------------------------------------
Parameters VerifyTriangleWindingFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_LabelsPath_Key, "Face labels or Region Ids Array",
                                                          "The path to the face labels or region ids array, **MUST** reside in target surface mesh (Triangle Geom)", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}));

  params.insertSeparator(Parameters::Separator{"Optional"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_RepairNormals_Key, "Repair Triangle Normals", "If enabled, the triangle normals will be recomputed if the triangle normals exist", true));
  params.insert(std::make_unique<ArraySelectionParameter>(k_TriangleNormalsPath_Key, "Triangle Normals Array", "The path to the triangle normals array", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float64}, ArraySelectionParameter::AllowedComponentShapes{{3}}));

  params.linkParameters(k_RepairNormals_Key, k_TriangleNormalsPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType VerifyTriangleWindingFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer VerifyTriangleWindingFilter::clone() const
{
  return std::make_unique<VerifyTriangleWindingFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult VerifyTriangleWindingFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pLabelsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_LabelsPath_Key);
  Result<DataPath> targetGeomResult = ::FindParentGeometry(pLabelsPath, dataStructure);
  if(targetGeomResult.invalid())
  {
    return MakePreflightErrorResult(-25741, fmt::format("Error trying to locate parent TriangleGeometry on path {}", pLabelsPath.toString()));
  }

  usize numComp = dataStructure.getDataAs<IDataArray>(pLabelsPath)->getNumberOfComponents();
  if(numComp != 1 && numComp != 2)
  {
    return MakePreflightErrorResult(
        -25742, fmt::format("Input labels array must be 1 component (Region Ids) or 2 component (Face Labels). Supplied array {} has {} components.", pLabelsPath.getTargetName(), numComp));
  }

  if(filterArgs.value<BoolParameter::ValueType>(k_RepairNormals_Key))
  {
    nx::core::Result<OutputActions> resultOutputActions;

    auto pNormalsArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_TriangleNormalsPath_Key);

    const auto* triangleGeom = dataStructure.getDataAs<INodeGeometry2D>(targetGeomResult.value());

    const auto* existingNormalsPtr = dataStructure.getDataAs<Float64Array>(pNormalsArrayPath);
    if(existingNormalsPtr != nullptr)
    {
      if(existingNormalsPtr->getNumberOfTuples() != triangleGeom->getNumberOfFaces() || existingNormalsPtr->getNumberOfComponents() != 3)
      {
        resultOutputActions.value().appendAction(std::make_unique<RenameDataAction>(pNormalsArrayPath, ::k_TempName));
        resultOutputActions.value().appendAction(
            std::make_unique<CreateArrayAction>(nx::core::DataType::float64, std::vector<usize>{triangleGeom->getNumberOfFaces()}, std::vector<usize>{3}, pNormalsArrayPath));
        resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(pNormalsArrayPath.getParent().createChildPath(::k_TempName)));
      }

      resultOutputActions.value().appendDataObjectModificationNotification(pNormalsArrayPath, DataObjectModification::ModifiedType::Modified);
      return {std::move(resultOutputActions)};
    }
    const auto* existingObjectPtr = dataStructure.getDataAs<DataObject>(pNormalsArrayPath);
    if(existingObjectPtr != nullptr)
    {
      resultOutputActions.value().appendAction(std::make_unique<RenameDataAction>(pNormalsArrayPath, ::k_TempName));
      resultOutputActions.value().appendAction(
          std::make_unique<CreateArrayAction>(nx::core::DataType::float64, std::vector<usize>{triangleGeom->getNumberOfFaces()}, std::vector<usize>{3}, pNormalsArrayPath));
      resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(pNormalsArrayPath.getParent().createChildPath(::k_TempName)));
      return {std::move(resultOutputActions)};
    }
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> VerifyTriangleWindingFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  VerifyTriangleWindingInputValues inputValues;

  inputValues.LabelsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_LabelsPath_Key);

  Result<DataPath> targetGeomResult = ::FindParentGeometry(inputValues.LabelsPath, dataStructure);
  if(targetGeomResult.invalid())
  {
    return ConvertResult(std::move(targetGeomResult));
  }
  inputValues.TargetGeometryPath = targetGeomResult.value();
  inputValues.RepairNormals = filterArgs.value<BoolParameter::ValueType>(k_RepairNormals_Key);
  inputValues.TriangleNormalsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_TriangleNormalsPath_Key);

  return VerifyTriangleWinding(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SurfaceMeshFaceLabelsPathKey = "SurfaceMeshFaceLabelsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> VerifyTriangleWindingFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = VerifyTriangleWindingFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceLabelsPathKey, k_LabelsPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
