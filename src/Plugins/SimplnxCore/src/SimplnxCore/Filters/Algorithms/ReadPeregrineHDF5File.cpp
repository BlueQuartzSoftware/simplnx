#include "ReadPeregrineHDF5File.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

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

Result<> ReadPeregrineHDF5File::readSliceDatasets(nx::core::HDF5::FileReader& h5FileReader)
{
  // Construct the "start" and "count" vectors
  std::optional<std::vector<hsize_t>> start = std::nullopt;
  std::optional<std::vector<hsize_t>> count = std::nullopt;
  if(m_InputValues->readSlicesSubvolume)
  {
    start = {m_InputValues->slicesSubvolumeMinMaxZ[0], m_InputValues->slicesSubvolumeMinMaxY[0], m_InputValues->slicesSubvolumeMinMaxX[0]};
    count = {
        m_InputValues->slicesSubvolumeMinMaxZ[1] - m_InputValues->slicesSubvolumeMinMaxZ[0] + 1,
        m_InputValues->slicesSubvolumeMinMaxY[1] - m_InputValues->slicesSubvolumeMinMaxY[0] + 1,
        m_InputValues->slicesSubvolumeMinMaxX[1] - m_InputValues->slicesSubvolumeMinMaxX[0] + 1,
    };
  }

  // Read the segmentation results
  auto segmentationResultsList = StringUtilities::split(m_InputValues->segmentationResultsStr, std::vector<char>{','}, true);
  for(usize i = 0; i < segmentationResultsList.size(); i++)
  {
    const auto& segmentationResult = segmentationResultsList[i];
    m_MessageHandler(fmt::format("Reading Segmentation Result '{}' ({}/{})...", segmentationResult, i + 1, segmentationResultsList.size()));
    DataPath segmentationResultPath = m_InputValues->sliceDataImageGeomPath.createChildPath(m_InputValues->sliceDataCellAttrMatName).createChildPath(segmentationResult);

    std::string segmentationResultH5Path;
    segmentationResultH5Path.append(ReadPeregrineHDF5File::k_SegmentationResultsH5ParentPath).append("/").append(segmentationResult);
    nx::core::HDF5::DatasetReader datasetReader = h5FileReader.openDataset(segmentationResultH5Path);
    Result<> fillArrayResults = HDF5::Support::FillDataArray<uint8>(m_DataStructure, segmentationResultPath, datasetReader, start, count);
    if(fillArrayResults.invalid())
    {
      return fillArrayResults;
    }
  }

  // Read the camera data
  if(m_InputValues->readCameraData)
  {
    {
      m_MessageHandler("Reading Camera Dataset 0...");
      DataPath cameraData0Path = m_InputValues->sliceDataImageGeomPath.createChildPath(m_InputValues->sliceDataCellAttrMatName).createChildPath(m_InputValues->cameraData0ArrayName);
      nx::core::HDF5::DatasetReader datasetReader = h5FileReader.openDataset(k_CameraData0H5Path);
      Result<> fillArrayResults = HDF5::Support::FillDataArray<float32>(m_DataStructure, cameraData0Path, datasetReader, start, count);
      if(fillArrayResults.invalid())
      {
        return fillArrayResults;
      }
    }
    {
      m_MessageHandler("Reading Camera Dataset 1...");
      DataPath cameraData1Path = m_InputValues->sliceDataImageGeomPath.createChildPath(m_InputValues->sliceDataCellAttrMatName).createChildPath(m_InputValues->cameraData1ArrayName);
      nx::core::HDF5::DatasetReader datasetReader = h5FileReader.openDataset(k_CameraData1H5Path);
      Result<> fillArrayResults = HDF5::Support::FillDataArray<float32>(m_DataStructure, cameraData1Path, datasetReader, start, count);
      if(fillArrayResults.invalid())
      {
        return fillArrayResults;
      }
    }
  }

  // Read the part ids
  if(m_InputValues->readPartIds)
  {
    m_MessageHandler("Reading Part Ids...");
    DataPath partIdsPath = m_InputValues->sliceDataImageGeomPath.createChildPath(m_InputValues->sliceDataCellAttrMatName).createChildPath(m_InputValues->partIdsArrayName);
    nx::core::HDF5::DatasetReader datasetReader = h5FileReader.openDataset(k_PartIdsH5Path);
    Result<> fillArrayResults = HDF5::Support::FillDataArray<uint32>(m_DataStructure, partIdsPath, datasetReader, start, count);
    if(fillArrayResults.invalid())
    {
      return fillArrayResults;
    }
  }

  // Read the sample ids
  if(m_InputValues->readSampleIds)
  {
    m_MessageHandler("Reading Sample Ids...");
    DataPath sampleIdsPath = m_InputValues->sliceDataImageGeomPath.createChildPath(m_InputValues->sliceDataCellAttrMatName).createChildPath(m_InputValues->sampleIdsArrayName);
    nx::core::HDF5::DatasetReader datasetReader = h5FileReader.openDataset(k_SampleIdsH5Path);
    Result<> fillArrayResults = HDF5::Support::FillDataArray<uint32>(m_DataStructure, sampleIdsPath, datasetReader, start, count);
    if(fillArrayResults.invalid())
    {
      return fillArrayResults;
    }
  }

  return {};
}

Result<> ReadPeregrineHDF5File::readRegisteredDatasets(HDF5::FileReader& h5FileReader)
{
  // Construct the "start" and "count" vectors
  std::optional<std::vector<hsize_t>> start = std::nullopt;
  std::optional<std::vector<hsize_t>> count = std::nullopt;
  if(m_InputValues->readRegisteredDataSubvolume)
  {
    start = {m_InputValues->registeredDataSubvolumeMinMaxZ[0], m_InputValues->registeredDataSubvolumeMinMaxY[0], m_InputValues->registeredDataSubvolumeMinMaxX[0]};
    count = {
        m_InputValues->registeredDataSubvolumeMinMaxZ[1] - m_InputValues->registeredDataSubvolumeMinMaxZ[0] + 1,
        m_InputValues->registeredDataSubvolumeMinMaxY[1] - m_InputValues->registeredDataSubvolumeMinMaxY[0] + 1,
        m_InputValues->registeredDataSubvolumeMinMaxX[1] - m_InputValues->registeredDataSubvolumeMinMaxX[0] + 1,
    };
  }

  // Read the anomaly detection dataset
  if(m_InputValues->readAnomalyDetection)
  {
    m_MessageHandler("Reading Anomaly Detection...");
    DataPath anomalyDetectionPath = m_InputValues->registeredDataImageGeomPath.createChildPath(m_InputValues->registeredDataCellAttrMatName).createChildPath(m_InputValues->anomalyDetectionArrayName);
    nx::core::HDF5::DatasetReader datasetReader = h5FileReader.openDataset(k_RegisteredAnomalyDetectionH5Path);
    Result<> fillArrayResults = HDF5::Support::FillDataArray<uint8>(m_DataStructure, anomalyDetectionPath, datasetReader, start, count);
    if(fillArrayResults.invalid())
    {
      return fillArrayResults;
    }
  }

  // Read the x-ray CT dataset
  if(m_InputValues->readXRayCT)
  {
    m_MessageHandler("Reading X-Ray CT...");
    DataPath xRayCTPath = m_InputValues->registeredDataImageGeomPath.createChildPath(m_InputValues->registeredDataCellAttrMatName).createChildPath(m_InputValues->xRayCTArrayName);
    nx::core::HDF5::DatasetReader datasetReader = h5FileReader.openDataset(k_RegisteredXRayCtH5Path);
    Result<> fillArrayResults = HDF5::Support::FillDataArray<uint8>(m_DataStructure, xRayCTPath, datasetReader, start, count);
    if(fillArrayResults.invalid())
    {
      return fillArrayResults;
    }
  }

  return {};
}

Result<> ReadPeregrineHDF5File::readScanDatasets(HDF5::FileReader& h5FileReader)
{
  DataPath vertexListPath = m_InputValues->scanDataEdgeGeomPath.createChildPath(m_InputValues->scanDataVertexListArrayName);
  Float32Array& vertexList = m_DataStructure.getDataRefAs<Float32Array>(vertexListPath);
  DataStore<float32>& vertexListStore = vertexList.getIDataStoreRefAs<DataStore<float32>>();
  
  DataPath edgeListPath = m_InputValues->scanDataEdgeGeomPath.createChildPath(m_InputValues->scanDataEdgeListArrayName);
  UInt64Array& edgeList = m_DataStructure.getDataRefAs<UInt64Array>(edgeListPath);
  DataStore<uint64>& edgeListStore = edgeList.getIDataStoreRefAs<DataStore<uint64>>();

  DataPath timeOfTravelArrayPath = m_InputValues->scanDataEdgeGeomPath.createChildPath(m_InputValues->scanDataEdgeAttrMatName).createChildPath(m_InputValues->scanDataTimeOfTravelArrayName);
  Float32Array& timeOfTravelArray = m_DataStructure.getDataRefAs<Float32Array>(timeOfTravelArrayPath);

  // Resize the vertex list to the proper size
  vertexListStore.resizeTuples(std::vector<usize>{edgeList.getNumberOfTuples() * 2});

  // Read scan datasets
  nx::core::HDF5::GroupReader groupReader = h5FileReader.openGroup(k_ScansGroupPath);
  if(!groupReader.isValid())
  {
    return MakeErrorResult(-4032, fmt::format("Error opening group at path '{}' in HDF5 file '{}'", k_ScansGroupPath, h5FileReader.getName()));
  }

  auto zThicknessReader = h5FileReader.getAttribute(k_ScansZThicknessPath);
  if(!zThicknessReader.isValid())
  {
    return MakeErrorResult(-4035, fmt::format("Attribute at path '{}' cannot be found, so the Z thickness cannot be determined!", k_ScansZThicknessPath));
  }
  const auto zThickness = zThicknessReader.readAsValue<float64>();

  usize zCount = groupReader.getNumChildren();
  usize numVertexComps = vertexListStore.getNumberOfComponents();
  usize numEdgeComps = edgeListStore.getNumberOfComponents();
  //  std::unordered_map<Vec3<float32>, int, Vec3Float32Hash> vertexIndexMap;
  usize currentVertexTuple = 0;
  usize currentEdgeTuple = 0;
  for(usize z = 0; z < zCount; z++)
  {
    m_MessageHandler(fmt::format("Reading Scan Dataset '{}' ({}/{})...", z, z + 1, zCount));
    fs::path scanPath = fs::path(ReadPeregrineHDF5File::k_ScansGroupPath) / std::to_string(z);
    nx::core::HDF5::DatasetReader datasetReader = h5FileReader.openDataset(scanPath.string());
    Result<std::vector<usize>> scanDimsResult = ReadDatasetDimensions(h5FileReader, scanPath.string());
    if(scanDimsResult.invalid())
    {
      return {nonstd::make_unexpected(scanDimsResult.errors())};
    }
    std::vector<usize> scanDims = scanDimsResult.value();
    usize totalElements = std::accumulate(scanDims.begin(), scanDims.end(), 1, std::multiplies<>());
    std::vector<float32> inputData(totalElements);
    nonstd::span<float32> span{inputData.data(), inputData.size()};

    // Read the scan into memory
    Result<> result = datasetReader.readIntoSpan<float32>(span);
    if(result.invalid())
    {
      return MakeErrorResult(-4033, fmt::format("Error reading scan dataset at path '{}' in HDF5 file '{}'", scanPath.string(), h5FileReader.getName()));
    }

    // Iterate through each row
    for(usize i = 0; i < totalElements; i += scanDims[1])
    {
      Vec3<float32> vertex1 = Vec3<float32>{inputData[i], inputData[i + 2], static_cast<float32>(z * zThickness)};
      Vec3<float32> vertex2 = Vec3<float32>{inputData[i + 1], inputData[i + 3], static_cast<float32>(z * zThickness)};
      float32 timeOfTravel = inputData[i + 4];

      timeOfTravelArray[currentEdgeTuple] = timeOfTravel;

      vertexList[currentVertexTuple * numVertexComps + 0] = vertex1.getX();
      vertexList[currentVertexTuple * numVertexComps + 1] = vertex1.getY();
      vertexList[currentVertexTuple * numVertexComps + 2] = vertex1.getZ();
      edgeList[currentEdgeTuple * numEdgeComps + 0] = currentVertexTuple;
      currentVertexTuple++;
      vertexList[currentVertexTuple * numVertexComps + 0] = vertex2.getX();
      vertexList[currentVertexTuple * numVertexComps + 1] = vertex2.getY();
      vertexList[currentVertexTuple * numVertexComps + 2] = vertex2.getZ();
      edgeList[currentEdgeTuple * numEdgeComps + 1] = currentVertexTuple;
      currentVertexTuple++;
      currentEdgeTuple++;
    }
  }

  return {};
}

Result<> ReadPeregrineHDF5File::operator()()
{
  nx::core::HDF5::FileReader h5FileReader(m_InputValues->inputFilePath.string());
  hid_t fileId = h5FileReader.getId();
  if(fileId < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-3001, fmt::format("Error opening Peregrine HDF5 file: '{}'", m_InputValues->inputFilePath.string())}})};
  }

  Result<> result = readSliceDatasets(h5FileReader);
  if(result.invalid())
  {
    return result;
  }

  result = readRegisteredDatasets(h5FileReader);
  if(result.invalid())
  {
    return result;
  }

  result = readScanDatasets(h5FileReader);
  if(result.invalid())
  {
    return result;
  }

  return {};
}

//------------------------------------------------------------------------------
Result<std::vector<usize>> ReadPeregrineHDF5File::ReadDatasetDimensions(const nx::core::HDF5::FileReader& h5FileReader, const std::string& h5DatasetPath)
{
  nx::core::HDF5::DatasetReader datasetReader = h5FileReader.openDataset(h5DatasetPath);
  if(!datasetReader.isValid())
  {
    return MakeErrorResult<std::vector<usize>>(-3002, fmt::format("Error opening dataset at path '{}' in HDF5 file '{}'", h5DatasetPath, h5FileReader.getName()));
  }

  std::vector<hsize_t> dims = datasetReader.getDimensions();
  std::vector<usize> dimsUSize(dims.size());
  std::transform(dims.begin(), dims.end(), dimsUSize.begin(), [](hsize_t val) { return static_cast<usize>(val); });
  return {dimsUSize};
}
