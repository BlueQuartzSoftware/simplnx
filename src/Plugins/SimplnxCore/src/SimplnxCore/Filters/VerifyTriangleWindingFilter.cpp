#include "VerifyTriangleWindingFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/VerifyTriangleWinding.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

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
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsPath_Key, "Face labels Array",
                                                          "The path to the face labels array, **MUST** reside in target surface mesh (Triangle Geom)", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));

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
  Result<DataPath> targetGeomResult = ::FindParentGeometry(filterArgs.value<ArraySelectionParameter::ValueType>(k_SurfaceMeshFaceLabelsPath_Key), dataStructure);
  if(targetGeomResult.invalid())
  {
    return MakePreflightErrorResult(
        -25741, fmt::format("Error trying to locate parent TriangleGeometry on path {}", filterArgs.value<ArraySelectionParameter::ValueType>(k_SurfaceMeshFaceLabelsPath_Key).toString()));
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> VerifyTriangleWindingFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  VerifyTriangleWindingInputValues inputValues;

  inputValues.FaceLabelsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_SurfaceMeshFaceLabelsPath_Key);

  Result<DataPath> targetGeomResult = ::FindParentGeometry(inputValues.FaceLabelsPath, dataStructure);
  if(targetGeomResult.invalid())
  {
    return ConvertResult(std::move(targetGeomResult));
  }
  inputValues.TargetGeometryPath = targetGeomResult.value();

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

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceLabelsPathKey, k_SurfaceMeshFaceLabelsPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
