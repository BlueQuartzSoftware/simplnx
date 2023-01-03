#include "FindVertexToTriangleDistancesFilter.hpp"

#include "ComplexCore/Filters/Algorithms/FindVertexToTriangleDistances.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindVertexToTriangleDistancesFilter::name() const
{
  return FilterTraits<FindVertexToTriangleDistancesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindVertexToTriangleDistancesFilter::className() const
{
  return FilterTraits<FindVertexToTriangleDistancesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindVertexToTriangleDistancesFilter::uuid() const
{
  return FilterTraits<FindVertexToTriangleDistancesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindVertexToTriangleDistancesFilter::humanName() const
{
  return "Find Vertex to Triangle Distances";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindVertexToTriangleDistancesFilter::defaultTags() const
{
  return {"#Sampling", "#Spatial"};
}

//------------------------------------------------------------------------------
Parameters FindVertexToTriangleDistancesFilter::parameters() const
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
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_VertexDataContainer_Key, "Source Vertex Geometry", "", DataPath{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_TriangleDataContainer_Key, "Target Triangle Geometry", "", DataPath{}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_TriangleNormalsArrayPath_Key, "Triangle Normals", "", DataPath{}, complex::GetAllDataTypes() /* This will allow ANY data type. Adjust as necessary for your filter*/));
  params.insert(std::make_unique<ArrayCreationParameter>(k_DistancesArrayPath_Key, "Distances", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ClosestTriangleIdArrayPath_Key, "Closest Triangle Ids", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindVertexToTriangleDistancesFilter::clone() const
{
  return std::make_unique<FindVertexToTriangleDistancesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindVertexToTriangleDistancesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pVertexDataContainerValue = filterArgs.value<DataPath>(k_VertexDataContainer_Key);
  auto pTriangleDataContainerValue = filterArgs.value<DataPath>(k_TriangleDataContainer_Key);
  auto pTriangleNormalsArrayPathValue = filterArgs.value<DataPath>(k_TriangleNormalsArrayPath_Key);
  auto pDistancesArrayPathValue = filterArgs.value<DataPath>(k_DistancesArrayPath_Key);
  auto pClosestTriangleIdArrayPathValue = filterArgs.value<DataPath>(k_ClosestTriangleIdArrayPath_Key);



  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // None found in this filter based on the filter parameters

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
    // auto createArrayAction = std::make_unique<CreateArrayAction>(complex::NumericType::FILL_ME_IN, std::vector<usize>{NUM_TUPLES_VALUE}, NUM_COMPONENTS, pDistancesArrayPathValue);
    // resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }
  // This block is commented out because it needs some variables to be filled in.
  {
    // auto createArrayAction = std::make_unique<CreateArrayAction>(complex::NumericType::FILL_ME_IN, std::vector<usize>{NUM_TUPLES_VALUE}, NUM_COMPONENTS, pClosestTriangleIdArrayPathValue);
    // resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // None found based on the filter parameters

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindVertexToTriangleDistancesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{

  FindVertexToTriangleDistancesInputValues inputValues;

    inputValues.VertexDataContainer = filterArgs.value<DataPath>(k_VertexDataContainer_Key);
  inputValues.TriangleDataContainer = filterArgs.value<DataPath>(k_TriangleDataContainer_Key);
  inputValues.TriangleNormalsArrayPath = filterArgs.value<DataPath>(k_TriangleNormalsArrayPath_Key);
  inputValues.DistancesArrayPath = filterArgs.value<DataPath>(k_DistancesArrayPath_Key);
  inputValues.ClosestTriangleIdArrayPath = filterArgs.value<DataPath>(k_ClosestTriangleIdArrayPath_Key);


  return FindVertexToTriangleDistances(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
