#include "ReadHDF5Dataset.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5DataStore.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <filesystem>
#include <map>

using namespace nx::core;
namespace fs = std::filesystem;

// -----------------------------------------------------------------------------
ReadHDF5Dataset::ReadHDF5Dataset(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadHDF5DatasetInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReadHDF5Dataset::~ReadHDF5Dataset() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadHDF5Dataset::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("Reading HDF5 dataset...");

  auto pSelectedAttributeMatrixValue = m_InputValues->ImportHdf5Object.parent;
  auto inputFile = m_InputValues->ImportHdf5Object.inputFile;
  fs::path inputFilePath(inputFile);
  auto datasetImportInfoList = m_InputValues->ImportHdf5Object.datasets;

  auto h5FileReader = nx::core::HDF5::FileIO::ReadFile(inputFilePath);
  if(h5FileReader.isValid() == false)
  {
    return MakeErrorResult(-21000, fmt::format("Error Reading HDF5 file: '{}'", inputFile));
  }

  std::map<std::string, hid_t> openedParentPathsMap;
  for(const auto& datasetImportInfo : datasetImportInfoList)
  {
    std::string datasetPath = datasetImportInfo.dataSetPath;
    auto datasetReader = h5FileReader.openDataset(datasetPath);

    std::string objectName = datasetReader.getName();

    // Read dataset into DREAM3D-NX structure
    DataPath dataArrayPath = pSelectedAttributeMatrixValue.has_value() ? pSelectedAttributeMatrixValue.value().createChildPath(objectName) : DataPath::FromString(objectName).value();
    Result<> fillArrayResults;
    auto h5TypeResult = datasetReader.getDataType();
    const auto type = std::move(h5TypeResult.value());
    switch(type)
    {
    case DataType::float32: {
      fillArrayResults = HDF5::Support::FillDataArray<float32>(m_DataStructure, dataArrayPath, datasetReader);
      break;
    }
    case DataType::float64: {
      fillArrayResults = HDF5::Support::FillDataArray<float64>(m_DataStructure, dataArrayPath, datasetReader);
      break;
    }
    case DataType::int8: {
      fillArrayResults = HDF5::Support::FillDataArray<int8>(m_DataStructure, dataArrayPath, datasetReader);
      break;
    }
    case DataType::int16: {
      fillArrayResults = HDF5::Support::FillDataArray<int16>(m_DataStructure, dataArrayPath, datasetReader);
      break;
    }
    case DataType::int32: {
      fillArrayResults = HDF5::Support::FillDataArray<int32>(m_DataStructure, dataArrayPath, datasetReader);
      break;
    }
    case DataType::int64: {
      fillArrayResults = HDF5::Support::FillDataArray<int64>(m_DataStructure, dataArrayPath, datasetReader);
      break;
    }
    case DataType::uint8: {
      fillArrayResults = HDF5::Support::FillDataArray<uint8>(m_DataStructure, dataArrayPath, datasetReader);
      break;
    }
    case DataType::uint16: {
      fillArrayResults = HDF5::Support::FillDataArray<uint16>(m_DataStructure, dataArrayPath, datasetReader);
      break;
    }
    case DataType::uint32: {
      fillArrayResults = HDF5::Support::FillDataArray<uint32>(m_DataStructure, dataArrayPath, datasetReader);
      break;
    }
    case DataType::uint64: {
      fillArrayResults = HDF5::Support::FillDataArray<uint64>(m_DataStructure, dataArrayPath, datasetReader);
      break;
    }
    default: {
      return MakeErrorResult(-21001,
                             fmt::format("The selected dataset '{}' with type '{}' is not a supported type for importing. Please select a different data set", datasetPath, fmt::underlying(type)));
    }
    }
    if(fillArrayResults.invalid())
    {
      return fillArrayResults;
    }
  } // End For Loop over dataset import info list

  return {};
}
