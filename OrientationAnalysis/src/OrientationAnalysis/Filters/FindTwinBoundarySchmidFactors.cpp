#include "FindTwinBoundarySchmidFactors.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindTwinBoundarySchmidFactors::name() const
{
  return FilterTraits<FindTwinBoundarySchmidFactors>::name.str();
}

//------------------------------------------------------------------------------
std::string FindTwinBoundarySchmidFactors::className() const
{
  return FilterTraits<FindTwinBoundarySchmidFactors>::className;
}

//------------------------------------------------------------------------------
Uuid FindTwinBoundarySchmidFactors::uuid() const
{
  return FilterTraits<FindTwinBoundarySchmidFactors>::uuid;
}

//------------------------------------------------------------------------------
std::string FindTwinBoundarySchmidFactors::humanName() const
{
  return "Find Twin Boundary Schmid Factors";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindTwinBoundarySchmidFactors::defaultTags() const
{
  return {"#Statistics", "#Crystallography"};
}

//------------------------------------------------------------------------------
Parameters FindTwinBoundarySchmidFactors::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<VectorFloat32Parameter>(k_LoadingDir_Key, "Loading Direction", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_WriteFile_Key, "Write Twin Boundary Info File", "", false));
  params.insert(std::make_unique<FileSystemPathParameter>(k_TwinBoundarySchmidFactorsFile_Key, "Twin Boundary Info File", "", fs::path("<default file to read goes here>"),
                                                          FileSystemPathParameter::PathType::OutputFile));
  params.insertSeparator(Parameters::Separator{"Cell Feature Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_AvgQuatsArrayPath_Key, "Average Quaternions", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeaturePhasesArrayPath_Key, "Phases", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceNormalsArrayPath_Key, "Face Normals", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshTwinBoundaryArrayPath_Key, "Twin Boundary", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Face Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_SurfaceMeshTwinBoundarySchmidFactorsArrayName_Key, "Twin Boundary Schmid Factors", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_WriteFile_Key, k_TwinBoundarySchmidFactorsFile_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindTwinBoundarySchmidFactors::clone() const
{
  return std::make_unique<FindTwinBoundarySchmidFactors>();
}

//------------------------------------------------------------------------------
Result<OutputActions> FindTwinBoundarySchmidFactors::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pLoadingDirValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LoadingDir_Key);
  auto pWriteFileValue = filterArgs.value<bool>(k_WriteFile_Key);
  auto pTwinBoundarySchmidFactorsFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_TwinBoundarySchmidFactorsFile_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pSurfaceMeshTwinBoundaryArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTwinBoundaryArrayPath_Key);
  auto pSurfaceMeshTwinBoundarySchmidFactorsArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshTwinBoundarySchmidFactorsArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<FindTwinBoundarySchmidFactorsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> FindTwinBoundarySchmidFactors::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pLoadingDirValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LoadingDir_Key);
  auto pWriteFileValue = filterArgs.value<bool>(k_WriteFile_Key);
  auto pTwinBoundarySchmidFactorsFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_TwinBoundarySchmidFactorsFile_Key);
  auto pAvgQuatsArrayPathValue = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  auto pFeaturePhasesArrayPathValue = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pSurfaceMeshFaceNormalsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceNormalsArrayPath_Key);
  auto pSurfaceMeshTwinBoundaryArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshTwinBoundaryArrayPath_Key);
  auto pSurfaceMeshTwinBoundarySchmidFactorsArrayNameValue = filterArgs.value<DataPath>(k_SurfaceMeshTwinBoundarySchmidFactorsArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
