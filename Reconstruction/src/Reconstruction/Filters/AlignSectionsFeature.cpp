#include "AlignSectionsFeature.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string AlignSectionsFeature::name() const
{
  return FilterTraits<AlignSectionsFeature>::name.str();
}

//------------------------------------------------------------------------------
std::string AlignSectionsFeature::className() const
{
  return FilterTraits<AlignSectionsFeature>::className;
}

//------------------------------------------------------------------------------
Uuid AlignSectionsFeature::uuid() const
{
  return FilterTraits<AlignSectionsFeature>::uuid;
}

//------------------------------------------------------------------------------
std::string AlignSectionsFeature::humanName() const
{
  return "Align Sections (Feature)";
}

//------------------------------------------------------------------------------
std::vector<std::string> AlignSectionsFeature::defaultTags() const
{
  return {"#Reconstruction", "#Alignment"};
}

//------------------------------------------------------------------------------
Parameters AlignSectionsFeature::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_WriteAlignmentShifts_Key, "Write Alignment Shift File", "", false));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_AlignmentShiftFileName_Key, "Alignment File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "", DataPath{}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_WriteAlignmentShifts_Key, k_AlignmentShiftFileName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer AlignSectionsFeature::clone() const
{
  return std::make_unique<AlignSectionsFeature>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult AlignSectionsFeature::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pWriteAlignmentShiftsValue = filterArgs.value<bool>(k_WriteAlignmentShifts_Key);
  auto pAlignmentShiftFileNameValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_AlignmentShiftFileName_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);

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

  // Assuming this filter did make some structural changes to the DataStructure then store
  // the outputAction into the resultOutputActions object via a std::move().
  // NOTE: That using std::move() means that you can *NOT* use the outputAction variable
  // past this point so let us scope this part which will stop stupid subtle bugs
  // from being introduced. If you have multiple `Actions` classes that you are
  // using such as a CreateDataGroupAction followed by a CreateArrayAction you might
  // want to consider scoping each of those bits of code into their own section of code
  {
    // Replace the "EmptyAction" with one of the prebuilt actions that apply changes
    // to the DataStructure. If none are available then create a new custom Action subclass.
    // If your filter does not make any structural modifications to the DataStructure then
    // you can skip this code.

    auto outputAction = std::make_unique<EmptyAction>();
    resultOutputActions.value().actions.push_back(std::move(outputAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> AlignSectionsFeature::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pWriteAlignmentShiftsValue = filterArgs.value<bool>(k_WriteAlignmentShifts_Key);
  auto pAlignmentShiftFileNameValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_AlignmentShiftFileName_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
