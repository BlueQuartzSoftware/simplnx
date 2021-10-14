#include "WaveFrontObjectFileWriter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace complex
{
std::string WaveFrontObjectFileWriter::name() const
{
  return FilterTraits<WaveFrontObjectFileWriter>::name.str();
}

std::string WaveFrontObjectFileWriter::className() const
{
  return FilterTraits<WaveFrontObjectFileWriter>::className;
}

Uuid WaveFrontObjectFileWriter::uuid() const
{
  return FilterTraits<WaveFrontObjectFileWriter>::uuid;
}

std::string WaveFrontObjectFileWriter::humanName() const
{
  return "Surface Mesh To Wavefront";
}

Parameters WaveFrontObjectFileWriter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_OutputWaveFrontFile_Key, "Output Wavefront File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_TriangleGeometry_Key, "Triangle Geometry", "", DataPath{}));

  return params;
}

IFilter::UniquePointer WaveFrontObjectFileWriter::clone() const
{
  return std::make_unique<WaveFrontObjectFileWriter>();
}

Result<OutputActions> WaveFrontObjectFileWriter::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pOutputWaveFrontFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputWaveFrontFile_Key);
  auto pTriangleGeometryValue = filterArgs.value<DataPath>(k_TriangleGeometry_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<WaveFrontObjectFileWriterAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

Result<> WaveFrontObjectFileWriter::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
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
