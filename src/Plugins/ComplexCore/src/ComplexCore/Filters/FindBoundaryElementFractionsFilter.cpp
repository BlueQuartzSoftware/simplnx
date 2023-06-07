#include "FindBoundaryElementFractionsFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindBoundaryElementFractionsFilter::name() const
{
  return FilterTraits<FindBoundaryElementFractionsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindBoundaryElementFractionsFilter::className() const
{
  return FilterTraits<FindBoundaryElementFractionsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindBoundaryElementFractionsFilter::uuid() const
{
  return FilterTraits<FindBoundaryElementFractionsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindBoundaryElementFractionsFilter::humanName() const
{
  return "Find Feature Boundary Element Fractions";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindBoundaryElementFractionsFilter::defaultTags() const
{
  return {"Statistics", "Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindBoundaryElementFractionsFilter::parameters() const
{
  Parameters params;

  /**
   * Please separate the parameters into groups generally of the following:
   *
   * params.insertSeparator(Parameters::Separator{"Input Parameters"});
   * params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
   * params.insertSeparator(Parameters::Separator{"Required Input Feature Data"});
   * params.insertSeparator(Parameters::Separator{"Created Cell Data"});
   * params.insertSeparator(Parameters::Separator{"Created Cell Feature Data"});
   *
   * .. or create appropriate separators as needed. The UI in COMPLEX no longer
   * does this for the developer by using catgories as in SIMPL
   */

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Element Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insert(std::make_unique<ArraySelectionParameter>(k_BoundaryCellsArrayPath_Key, "Surface Elements", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insertSeparator(Parameters::Separator{"Feature Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_BoundaryCellFractionsArrayPath_Key, "Surface Element Fractions", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindBoundaryElementFractionsFilter::clone() const
{
  return std::make_unique<FindBoundaryElementFractionsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindBoundaryElementFractionsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pBoundaryCellsArrayPathValue = filterArgs.value<DataPath>(k_BoundaryCellsArrayPath_Key);
  auto pBoundaryCellFractionsArrayPathValue = filterArgs.value<DataPath>(k_BoundaryCellFractionsArrayPath_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs
  // These are some proposed Actions based on the FilterParameters used. Please check them for correctness.
  // This block is commented out because it needs some variables to be filled in.
  {
    // auto createArrayAction = std::make_unique<CreateArrayAction>(complex::NumericType::FILL_ME_IN, std::vector<usize>{NUM_TUPLES_VALUE}, NUM_COMPONENTS, pBoundaryCellFractionsArrayPathValue);
    // resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindBoundaryElementFractionsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
