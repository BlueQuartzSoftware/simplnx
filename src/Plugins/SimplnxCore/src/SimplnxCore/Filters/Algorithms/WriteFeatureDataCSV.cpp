#include "WriteFeatureDataCSV.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
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
WriteFeatureDataCSV::~WriteFeatureDataCSV() noexcept = default;

// -----------------------------------------------------------------------------
Result<> WriteFeatureDataCSV::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("Writing Feature Data CSV file...");

  auto atomicFileResult = AtomicFile::Create(m_InputValues->FeatureDataFile);
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());

  auto pOutputFilePath = atomicFile.tempFilePath();

  const std::string delimiter = OStreamUtilities::DelimiterToString(m_InputValues->DelimiterIndex);

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
  if(m_InputValues->WriteNeighborlistData)
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
    auto requestedPaths = *std::move(GetAllChildDataPaths(m_DataStructure, m_InputValues->CellFeatureAttributeMatrixPath, element));
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
    OStreamUtilities::PrintDataSetsToSingleFile(fout, arrayPaths, m_DataStructure, m_MessageHandler, m_ShouldCancel, delimiter, true, true, false, "Feature_ID", neighborPaths,
                                                m_InputValues->WriteNumFeaturesLine);
  }

  Result<> commitResult = atomicFile.commit();
  if(commitResult.invalid())
  {
    return commitResult;
  }

  return {};
}
