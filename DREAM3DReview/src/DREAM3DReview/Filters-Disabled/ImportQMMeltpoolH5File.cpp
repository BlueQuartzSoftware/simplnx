#include "ImportQMMeltpoolH5File.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/MultiInputFileFilterParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportQMMeltpoolH5File::name() const
{
  return FilterTraits<ImportQMMeltpoolH5File>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportQMMeltpoolH5File::className() const
{
  return FilterTraits<ImportQMMeltpoolH5File>::className;
}

//------------------------------------------------------------------------------
Uuid ImportQMMeltpoolH5File::uuid() const
{
  return FilterTraits<ImportQMMeltpoolH5File>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportQMMeltpoolH5File::humanName() const
{
  return "Import QM Meltpool HDF5 File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportQMMeltpoolH5File::defaultTags() const
{
  return {"#IO", "#Input", "#Read", "#Import"};
}

//------------------------------------------------------------------------------
Parameters ImportQMMeltpoolH5File::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  /*[x]*/ params.insert(std::make_unique<MultiInputFileFilterParameter>(k_InputFiles_Key, "Input File(s)", "", {}));
  params.insert(std::make_unique<VectorInt32Parameter>(k_SliceRange_Key, "Slice Index Start/End [Inclusive]", "", std::vector<int32>(2), std::vector<std::string>(2)));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerPath_Key, "Data Container Name", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_VertexAttributeMatrixName_Key, "Vertex Attribute Matrix Name", "", "SomeString"));
  params.insert(std::make_unique<Float32Parameter>(k_Power_Key, "Power", "", 1.23345f));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportQMMeltpoolH5File::clone() const
{
  return std::make_unique<ImportQMMeltpoolH5File>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportQMMeltpoolH5File::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pInputFilesValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InputFiles_Key);
  auto pSliceRangeValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_SliceRange_Key);
  auto pDataContainerPathValue = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pPowerValue = filterArgs.value<float32>(k_Power_Key);

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
  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::string possibleIndices;

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
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pDataContainerPathValue);
    resultOutputActions.value().actions.push_back(std::move(createDataGroupAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // These values should have been updated during the preflightImpl(...) method
  preflightUpdatedValues.push_back({"PossibleIndices", possibleIndices});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImportQMMeltpoolH5File::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pInputFilesValue = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InputFiles_Key);
  auto pSliceRangeValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_SliceRange_Key);
  auto pDataContainerPathValue = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pPowerValue = filterArgs.value<float32>(k_Power_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
