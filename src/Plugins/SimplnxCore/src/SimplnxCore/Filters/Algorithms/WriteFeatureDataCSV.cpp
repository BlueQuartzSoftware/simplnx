#include "WriteFeatureDataCSV.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/OStreamUtilities.hpp"

#include <filesystem>
#include <fstream>

using namespace nx::core;

// -----------------------------------------------------------------------------
WriteFeatureDataCSV::WriteFeatureDataCSV(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteFeatureDataCSVInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
Result<> WriteFeatureDataCSV::operator()()
{
  auto atomicFileResult = AtomicFile::Create(m_InputValues->feature_data_file);
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  auto pOutputFilePath = atomicFile.tempFilePath();

  const std::string delimiter = o_stream_utilities::DelimiterToString(m_InputValues->delimiter_index);

  // Ensure the complete path to the output file exists or can be created
  auto parentPath = pOutputFilePath.parent_path();
  if(!std::filesystem::exists(parentPath))
  {
    if(!std::filesystem::create_directories(parentPath))
    {
      return MakeErrorResult(-64641, fmt::format("Error creating Output file at path '{}'. Parent path could not be created.", pOutputFilePath.string()));
    }
  }

  // load list of DataPaths
  std::vector<DataObject::Type> dataTypesToExtract;
  if(m_InputValues->write_neighborlist_data)
  {
    dataTypesToExtract = {DataObject::Type::DataArray, DataObject::Type::StringArray, DataObject::Type::NeighborList};
  }
  else
  {
    dataTypesToExtract = {DataObject::Type::DataArray, DataObject::Type::StringArray};
  }

  std::vector<DataPath> arrayPaths;
  std::vector<DataPath> neighborPaths;
  for(const auto& element : dataTypesToExtract)
  {
    auto requestedPaths = *std::move(GetAllChildDataPaths(m_DataStructure, m_InputValues->cell_feature_attribute_matrix_path, element));
    if(element == DataObject::Type::NeighborList)
    {
      neighborPaths.insert(neighborPaths.end(), std::make_move_iterator(requestedPaths.begin()), std::make_move_iterator(requestedPaths.end()));
    }
    else
    {
      arrayPaths.insert(arrayPaths.end(), std::make_move_iterator(requestedPaths.begin()), std::make_move_iterator(requestedPaths.end()));
    }
  }

  // Scope file writer in code block to get around file lock on windows (enforce destructor order)
  {
    std::ofstream fout(pOutputFilePath.string(), std::ofstream::out | std::ios_base::binary); // test name resolution and create file
    if(!fout.is_open())
    {
      return MakeErrorResult(-64640, fmt::format("Error opening path {}", pOutputFilePath.string()));
    }

    // call ostream function
    o_stream_utilities::PrintDataSetsToSingleFile(fout, arrayPaths, m_DataStructure, m_MessageHandler, m_ShouldCancel, delimiter, true, true, false, "Feature_ID", neighborPaths,
                                                m_InputValues->write_num_features_line);
  }

  if(m_ShouldCancel)
  {
    return MakeErrorResult(-1, "Filter cancelled");
  }
  Result<> commitResult = atomicFile.commit();
  if(commitResult.invalid())
  {
    return commitResult;
  }

  return {};
}
