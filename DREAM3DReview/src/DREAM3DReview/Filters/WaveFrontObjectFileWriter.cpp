#include "WaveFrontObjectFileWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string WaveFrontObjectFileWriter::name() const
{
  return FilterTraits<WaveFrontObjectFileWriter>::name.str();
}

//------------------------------------------------------------------------------
std::string WaveFrontObjectFileWriter::className() const
{
  return FilterTraits<WaveFrontObjectFileWriter>::className;
}

//------------------------------------------------------------------------------
Uuid WaveFrontObjectFileWriter::uuid() const
{
  return FilterTraits<WaveFrontObjectFileWriter>::uuid;
}

//------------------------------------------------------------------------------
std::string WaveFrontObjectFileWriter::humanName() const
{
  return "Surface Mesh To Wavefront";
}

//------------------------------------------------------------------------------
std::vector<std::string> WaveFrontObjectFileWriter::defaultTags() const
{
  return {"#Unsupported", "#DREAM3DReview"};
}

//------------------------------------------------------------------------------
Parameters WaveFrontObjectFileWriter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_OutputWaveFrontFile_Key, "Output Wavefront File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_TriangleGeometry_Key, "Triangle Geometry", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer WaveFrontObjectFileWriter::clone() const
{
  return std::make_unique<WaveFrontObjectFileWriter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult WaveFrontObjectFileWriter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/

  /**
   * These are the values that were gathered from the UI or the pipeline file or
   * otherwise passed into the filter. These are here for your convenience. If you
   * do not need some of them remove them.
   */
  auto pOutputWaveFrontFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputWaveFrontFile_Key);
  auto pTriangleGeometryValue = filterArgs.value<DataPath>(k_TriangleGeometry_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

#if 0
  // Define the OutputActions Object that will hold the actions that would take
  // place if the filter were to execute. This is mainly what would happen to the
  // DataStructure during this filter, i.e., what modificationst to the DataStructure
  // would take place.
  OutputActions actions;
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<WaveFrontObjectFileWriterAction>();
  actions.actions.push_back(std::move(action));
  // Assign the generated outputActions to the PreflightResult::OutputActions property
  preflightResult.outputActions = std::move(actions);
#endif

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> WaveFrontObjectFileWriter::executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pOutputWaveFrontFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputWaveFrontFile_Key);
  auto pTriangleGeometryValue = filterArgs.value<DataPath>(k_TriangleGeometry_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
