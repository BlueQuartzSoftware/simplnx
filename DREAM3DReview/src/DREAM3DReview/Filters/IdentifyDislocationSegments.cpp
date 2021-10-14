#include "IdentifyDislocationSegments.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
std::string IdentifyDislocationSegments::name() const
{
  return FilterTraits<IdentifyDislocationSegments>::name.str();
}

std::string IdentifyDislocationSegments::className() const
{
  return FilterTraits<IdentifyDislocationSegments>::className;
}

Uuid IdentifyDislocationSegments::uuid() const
{
  return FilterTraits<IdentifyDislocationSegments>::uuid;
}

std::string IdentifyDislocationSegments::humanName() const
{
  return "Identify Dislocation Segments";
}

Parameters IdentifyDislocationSegments::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Edge Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_BurgersVectorsArrayPath_Key, "Burgers Vectors", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SlipPlaneNormalsArrayPath_Key, "Slip Plane Normals", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Edge Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_DislocationIdsArrayName_Key, "Dislocation Ids", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Edge Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_EdgeFeatureAttributeMatrixName_Key, "Edge Feature Attribute Matrix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ActiveArrayName_Key, "Active", "", DataPath{}));

  return params;
}

IFilter::UniquePointer IdentifyDislocationSegments::clone() const
{
  return std::make_unique<IdentifyDislocationSegments>();
}

Result<OutputActions> IdentifyDislocationSegments::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pBurgersVectorsArrayPathValue = filterArgs.value<DataPath>(k_BurgersVectorsArrayPath_Key);
  auto pSlipPlaneNormalsArrayPathValue = filterArgs.value<DataPath>(k_SlipPlaneNormalsArrayPath_Key);
  auto pDislocationIdsArrayNameValue = filterArgs.value<DataPath>(k_DislocationIdsArrayName_Key);
  auto pEdgeFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_EdgeFeatureAttributeMatrixName_Key);
  auto pActiveArrayNameValue = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<IdentifyDislocationSegmentsAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> IdentifyDislocationSegments::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pBurgersVectorsArrayPathValue = filterArgs.value<DataPath>(k_BurgersVectorsArrayPath_Key);
  auto pSlipPlaneNormalsArrayPathValue = filterArgs.value<DataPath>(k_SlipPlaneNormalsArrayPath_Key);
  auto pDislocationIdsArrayNameValue = filterArgs.value<DataPath>(k_DislocationIdsArrayName_Key);
  auto pEdgeFeatureAttributeMatrixNameValue = filterArgs.value<DataPath>(k_EdgeFeatureAttributeMatrixName_Key);
  auto pActiveArrayNameValue = filterArgs.value<DataPath>(k_ActiveArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
