#include "ReadPeregrineHDF5File.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/Utilities/GeometryUtilities.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

using namespace nx::core;

namespace
{
struct ScanData
{
  std::vector<float32> data;
  std::vector<usize> dims;
};

EdgeGeom* createScanEdgeGeometry(DataStructure& ds)
{
  EdgeGeom* tmpEdgeGeom = EdgeGeom::Create(ds, "PerScanGeom");
  AttributeMatrix* tmpEdgeAttrMatrix = AttributeMatrix::Create(ds, "PerScanEdgeAttrMatrix", std::vector<usize>{0});
  tmpEdgeGeom->setEdgeAttributeMatrix(*tmpEdgeAttrMatrix);
  EdgeGeom::SharedEdgeList* tmpEdgeList = EdgeGeom::SharedEdgeList::CreateWithStore<DataStore<uint64>>(ds, "PerScanEdges", std::vector<usize>{0}, std::vector<usize>{2});
  tmpEdgeGeom->setEdgeList(*tmpEdgeList);
  AttributeMatrix* tmpVertexAttrMatrix = AttributeMatrix::Create(ds, "PerScanVertexAttrMatrix", std::vector<usize>{0});
  tmpEdgeGeom->setVertexAttributeMatrix(*tmpVertexAttrMatrix);
  EdgeGeom::SharedVertexList* tmpVertexList = EdgeGeom::SharedVertexList::CreateWithStore<DataStore<float32>>(ds, "PerScanVertices", std::vector<usize>{0}, std::vector<usize>{3});
  tmpEdgeGeom->setVertices(*tmpVertexList);
  return tmpEdgeGeom;
}

Result<ScanData> readScan(const fs::path& scanPath, HDF5::FileReader& h5FileReader)
{
  nx::core::HDF5::DatasetReader datasetReader = h5FileReader.openDataset(scanPath.string());
  Result<std::vector<usize>> scanDimsResult = ReadPeregrineHDF5File::ReadDatasetDimensions(h5FileReader, scanPath.string());
  if(scanDimsResult.invalid())
  {
    return {nonstd::make_unexpected(scanDimsResult.errors())};
  }
  std::vector<usize> scanDims = scanDimsResult.value();
  usize totalElements = std::accumulate(scanDims.begin(), scanDims.end(), 1ull, std::multiplies<>());
  std::vector<float32> inputData(totalElements);
  nonstd::span<float32> span{inputData.data(), inputData.size()};

  // Read the scan into memory
  Result<> result = datasetReader.readIntoSpan<float32>(span);
  if(result.invalid())
  {
    return MakeErrorResult<ScanData>(-4033, fmt::format("Error reading scan dataset at path '{}' in HDF5 file '{}'", scanPath.string(), h5FileReader.getName()));
  }

  return {ScanData{inputData, scanDims}};
}
} // namespace

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
    if(m_ShouldCancel)
    {
      return {};
    }

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
  auto& edgeGeom = m_DataStructure.getDataRefAs<EdgeGeom>(m_InputValues->scanDataEdgeGeomPath);
  DataPath vertexListPath = m_InputValues->scanDataEdgeGeomPath.createChildPath(m_InputValues->scanDataVertexListArrayName);
  auto& vertexList = m_DataStructure.getDataRefAs<Float32Array>(vertexListPath);
  auto& vertexListStore = vertexList.getIDataStoreRefAs<DataStore<float32>>();
  auto& vertexAttrMatrix = edgeGeom.getVertexAttributeMatrixRef();

  DataPath edgeListPath = m_InputValues->scanDataEdgeGeomPath.createChildPath(m_InputValues->scanDataEdgeListArrayName);
  auto& edgeList = m_DataStructure.getDataRefAs<UInt64Array>(edgeListPath);
  auto& edgeListStore = edgeList.getIDataStoreRefAs<DataStore<uint64>>();

  DataPath timeOfTravelArrayPath = m_InputValues->scanDataEdgeGeomPath.createChildPath(m_InputValues->scanDataEdgeAttrMatName).createChildPath(m_InputValues->scanDataTimeOfTravelArrayName);
  auto& timeOfTravelArray = m_DataStructure.getDataRefAs<Float32Array>(timeOfTravelArrayPath);

  // Resize the vertex attribute matrix and vertex list to the estimated size
  vertexAttrMatrix.resizeTuples(std::vector<usize>{edgeList.getNumberOfTuples() * 2});
  vertexListStore.resizeTuples(std::vector<usize>{edgeList.getNumberOfTuples() * 2});

  // Read scan datasets
  nx::core::HDF5::GroupReader groupReader = h5FileReader.openGroup(k_ScansGroupPath);
  if(!groupReader.isValid())
  {
    return MakeErrorResult(-4032, fmt::format("Error opening group at path '{}' in HDF5 file '{}'", k_ScansGroupPath, h5FileReader.getName()));
  }

  // Read the Z thickness value
  auto zThicknessReader = h5FileReader.getAttribute(k_ScansZThicknessPath);
  if(!zThicknessReader.isValid())
  {
    return MakeErrorResult(-4035, fmt::format("Attribute at path '{}' cannot be found, so the Z thickness cannot be determined!", k_ScansZThicknessPath));
  }
  const auto zThickness = zThicknessReader.readAsValue<float64>();

  // Calculate the start and end values for the scans
  usize zStart = 0;
  usize zEnd = groupReader.getNumChildren();
  if(m_InputValues->readScanDataSubvolume)
  {
    zStart = m_InputValues->scanDataSubvolumeMinMax[0];
    zEnd = m_InputValues->scanDataSubvolumeMinMax[1] + 1; // Must add 1 since this max value is inclusive
  }

  // This section loops over each scan, reads the scan data into a temporary edge geometry, eliminates any duplicate vertices, and then copies the data into the actual edge geometry.
  DataStructure tmpDataStructure;
  EdgeGeom* tmpEdgeGeom = createScanEdgeGeometry(tmpDataStructure);
  auto& tmpEdgeList = tmpEdgeGeom->getEdgesRef();
  auto& tmpEdgeListStore = tmpEdgeList.getIDataStoreRefAs<DataStore<uint64>>();
  auto& tmpVertexList = tmpEdgeGeom->getVerticesRef();
  auto& tmpVertexListStore = tmpVertexList.getIDataStoreRefAs<DataStore<float32>>();
  usize overallVertexTuple = 0;
  usize overallEdgeTuple = 0;
  for(usize z = zStart; z < zEnd; z++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    // Read the scan data into memory
    fs::path scanPath = fs::path(ReadPeregrineHDF5File::k_ScansGroupPath.view()) / std::to_string(z);
    m_MessageHandler(fmt::format("Reading Scan Dataset '{}' ({}/{})...", scanPath.string(), z - zStart + 1, zEnd - zStart));
    Result<ScanData> scanDataResult = readScan(scanPath, h5FileReader);
    if(scanDataResult.invalid())
    {
      return ConvertResult(std::move(scanDataResult));
    }
    ScanData scanData = scanDataResult.value();

    // Resize the temporary edge geometry's edge list and vertex list
    tmpEdgeGeom->resizeEdgeList(scanData.dims[0]);
    tmpEdgeGeom->resizeVertexList(scanData.dims[0] * 2);

    // Iterate through each row of the scan data, setting the temporary edge geometry's vertices and edges
    auto currentZOffset = static_cast<float32>(static_cast<float64>(z) * zThickness);
    usize tmpVertexTuple = 0;
    usize tmpEdgeTuple = 0;
    for(usize i = 0; i < scanData.data.size(); i += scanData.dims[1])
    {
      timeOfTravelArray[overallEdgeTuple + tmpEdgeTuple] = scanData.data[i + 4];
      tmpEdgeGeom->setVertexCoordinate(tmpVertexTuple, {scanData.data[i], scanData.data[i + 2], currentZOffset});
      tmpEdgeGeom->setVertexCoordinate(tmpVertexTuple + 1, {scanData.data[i + 1], scanData.data[i + 3], currentZOffset});
      std::array<usize, 2> verts = {tmpVertexTuple, tmpVertexTuple + 1};
      tmpEdgeGeom->setEdgePointIds(tmpEdgeTuple, verts);
      tmpEdgeTuple++;
      tmpVertexTuple += 2;
    }

    // Eliminate any duplicate vertices from the temporary edge geometry
    Result<> result = GeometryUtilities::EliminateDuplicateNodes(*tmpEdgeGeom);
    if(result.invalid())
    {
      return result;
    }

    // Copy the edges from the temporary edge geometry into the actual edge geometry
    Result<> copyResult = edgeListStore.copyFrom(overallEdgeTuple, tmpEdgeListStore, 0, tmpEdgeGeom->getNumberOfEdges());
    if(copyResult.invalid())
    {
      std::string ss = fmt::format("Error copying edge data from scan dataset '{}' edge geometry to final edge geometry.\n\n{}", z - zStart);
      for(const auto& err : copyResult.errors())
      {
        ss.append(err.message).append("\n");
      }
      return MakeErrorResult(-4034, ss);
    }

    // Copy the vertices from the temporary edge geometry into the actual edge geometry
    copyResult = vertexListStore.copyFrom(overallVertexTuple, tmpVertexListStore, 0, tmpEdgeGeom->getNumberOfVertices());
    if(copyResult.invalid())
    {
      std::string ss = fmt::format("Error copying vertex data from scan dataset '{}' edge geometry to final edge geometry.\n\n{}", z - zStart);
      for(const auto& err : copyResult.errors())
      {
        ss.append(err.message).append("\n");
      }
      return MakeErrorResult(-4034, ss);
    }

    // Set the overall edge and vertex tuples to be used on the next run through the loop
    overallEdgeTuple += tmpEdgeGeom->getNumberOfEdges();
    overallVertexTuple += tmpEdgeGeom->getNumberOfVertices();
  }

  // Resize the vertex attribute matrix and vertex list to the actual size.
  // This needs to be done because duplicate vertices may have been removed.
  vertexAttrMatrix.resizeTuples(std::vector<usize>{overallVertexTuple});
  vertexListStore.resizeTuples(std::vector<usize>{overallVertexTuple});

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
