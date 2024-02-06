#include "ReadPeregrineHDF5File.hpp"

#include "SimplnxCore/utils/StlUtilities.hpp"

#include "simplnx/Common/Range.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <cstdio>
#include <utility>

using namespace nx::core;

ReadPeregrineHDF5File::ReadPeregrineHDF5File(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                             ReadPeregrineHDF5FileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

ReadPeregrineHDF5File::~ReadPeregrineHDF5File() noexcept = default;

Result<> ReadPeregrineHDF5File::operator()()
{
  //  fs::path inputFilePath(inputFile);
  //  nx::core::HDF5::FileReader h5FileReader(inputFilePath);
  //  hid_t fileId = h5FileReader.getId();
  //  if(fileId < 0)
  //  {
  //    return {MakeErrorResult(-21000, fmt::format("Error Reading HDF5 file: '{}'", inputFile))};
  //  }
  //
  //  std::map<std::string, hid_t> openedParentPathsMap;
  //  for(const auto& datasetImportInfo : datasetImportInfoList)
  //  {
  //    std::string datasetPath = datasetImportInfo.dataSetPath;
  //    nx::core::HDF5::DatasetReader datasetReader = h5FileReader.openDataset(datasetPath);
  //    std::string objectName = datasetReader.getName();
  //
  //    // Read dataset into DREAM.3D structure
  //    DataPath dataArrayPath = pSelectedAttributeMatrixValue.has_value() ? pSelectedAttributeMatrixValue.value().createChildPath(objectName) : DataPath::FromString(objectName).value();
  //    Result<> fillArrayResults;
  //    auto type = datasetReader.getType();
  //    switch(type)
  //    {
  //    case nx::core::HDF5::Type::float32: {
  //      fillArrayResults = fillDataArray<float32>(dataStructure, dataArrayPath, datasetReader);
  //      break;
  //    }
  //    case nx::core::HDF5::Type::float64: {
  //      fillArrayResults = fillDataArray<float64>(dataStructure, dataArrayPath, datasetReader);
  //      break;
  //    }
  //    case nx::core::HDF5::Type::int8: {
  //      fillArrayResults = fillDataArray<int8>(dataStructure, dataArrayPath, datasetReader);
  //      break;
  //    }
  //    case nx::core::HDF5::Type::int16: {
  //      fillArrayResults = fillDataArray<int16>(dataStructure, dataArrayPath, datasetReader);
  //      break;
  //    }
  //    case nx::core::HDF5::Type::int32: {
  //      fillArrayResults = fillDataArray<int32>(dataStructure, dataArrayPath, datasetReader);
  //      break;
  //    }
  //    case nx::core::HDF5::Type::int64: {
  //      fillArrayResults = fillDataArray<int64>(dataStructure, dataArrayPath, datasetReader);
  //      break;
  //    }
  //    case nx::core::HDF5::Type::uint8: {
  //      fillArrayResults = fillDataArray<uint8>(dataStructure, dataArrayPath, datasetReader);
  //      break;
  //    }
  //    case nx::core::HDF5::Type::uint16: {
  //      fillArrayResults = fillDataArray<uint16>(dataStructure, dataArrayPath, datasetReader);
  //      break;
  //    }
  //    case nx::core::HDF5::Type::uint32: {
  //      fillArrayResults = fillDataArray<uint32>(dataStructure, dataArrayPath, datasetReader);
  //      break;
  //    }
  //    case nx::core::HDF5::Type::uint64: {
  //      fillArrayResults = fillDataArray<uint64>(dataStructure, dataArrayPath, datasetReader);
  //      break;
  //    }
  //    default: {
  //      return {MakeErrorResult(-21001,
  //                              fmt::format("The selected datatset '{}' with type '{}' is not a supported type for importing. Please select a different data set", datasetPath,
  //                              fmt::underlying(type)))};
  //    }
  //    }
  //    if(fillArrayResults.invalid())
  //    {
  //      return fillArrayResults;
  //    }
  //  } // End For Loop over dataset imoprt info list

  return {};
}
