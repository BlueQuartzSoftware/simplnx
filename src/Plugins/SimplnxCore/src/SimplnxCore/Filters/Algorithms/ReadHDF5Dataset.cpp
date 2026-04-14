#include "ReadHDF5Dataset.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5DataStore.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <filesystem>
#include <map>

using namespace nx::core;
namespace fs = std::filesystem;

// -----------------------------------------------------------------------------
/**
 * @brief Constructs ReadHDF5Dataset with the given DataStructure, message handler, cancel flag, and input values.
 */
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
/**
 * @brief Reads one or more HDF5 datasets from the input file and populates the DataStructure.
 *
 * Each selected dataset is read via HDF5::Support::FillDataArray, which internally uses
 * the HDF5 library for bulk reads. Progress messages report the current dataset index
 * and name. Cancel checking occurs between datasets so that long multi-dataset imports
 * can be interrupted.
 *
 * @section ooc_note OOC Note
 * The OOC changes here are minor (progress messaging and cancel support between datasets).
 * The actual HDF5 -> DataStore transfer is handled by the FillDataArray utility which
 * already uses bulk I/O internally.
 *
 * @return Result<> indicating success or an error from the HDF5 reading infrastructure.
 */
Result<> ReadHDF5Dataset::operator()()
{
  auto pSelectedAttributeMatrixValue = m_InputValues->ImportHdf5Object.parent;
  auto inputFile = m_InputValues->ImportHdf5Object.inputFile;
  fs::path inputFilePath(inputFile);
  auto datasetImportInfoList = m_InputValues->ImportHdf5Object.datasets;

  auto h5FileReader = nx::core::HDF5::FileIO::ReadFile(inputFilePath);
  if(h5FileReader.isValid() == false)
  {
    return MakeErrorResult(-21000, fmt::format("Error Reading HDF5 file: '{}'", inputFile));
  }

  const usize totalDatasets = datasetImportInfoList.size();
  m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Reading {} dataset(s) from '{}'", totalDatasets, inputFilePath.filename().string())});

  std::map<std::string, hid_t> openedParentPathsMap;
  usize dsIdx = 0;
  for(const auto& datasetImportInfo : datasetImportInfoList)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    std::string datasetPath = datasetImportInfo.dataSetPath;
    m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Importing dataset {}/{}: '{}'", dsIdx + 1, totalDatasets, datasetPath)});
    ++dsIdx;
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
                             fmt::format("The selected dataset '{}' with type '{}' is not a supported type for importing. Please select a different data set", datasetPath, DataTypeToString(type)));
    }
    }
    if(fillArrayResults.invalid())
    {
      return fillArrayResults;
    }
  } // End For Loop over dataset import info list

  return {};
}
