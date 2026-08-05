#include "ReadH5OinaData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include "H5Support/H5Lite.h"
#include "H5Support/H5ScopedSentinel.h"
#include "H5Support/H5Utilities.h"

#include <EbsdLib/Math/EbsdLibMath.h>

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
// The EDAX/TSL convention aligns the hexagonal crystal x-axis with [2-1-10] while
// Oxford Instruments aligns it with [10-10]; converting between them is a 30 degree
// rotation about [0001] applied to phi2. An H5OINA file stores its Euler angles in
// RADIANS (unlike a .ctf file, which stores degrees), so the value added here is 30
// degrees expressed in radians -- 30 * (pi/180), i.e. pi/6 -- and not the literal 30.
// The addition runs on a double intermediate so the stored float32 is the correctly
// rounded result, matching how the .ctf importer applies the same correction.
constexpr float64 k_HexagonalAlignmentRadians = 30.0 * ebsdlib::constants::k_PiOver180D;

// The nine datasets H5OINAReader reads out of a scan's Data group, paired with the
// number of components each one carries per scan point.
const std::vector<std::pair<std::string, usize>> k_RequiredDataSets = {
    {ebsdlib::H5OINA::BandContrast, 1},         {ebsdlib::H5OINA::BandSlope, 1}, {ebsdlib::H5OINA::Bands, 1}, {ebsdlib::H5OINA::Error, 1}, {ebsdlib::H5OINA::Euler, 3},
    {ebsdlib::H5OINA::MeanAngularDeviation, 1}, {ebsdlib::H5OINA::Phase, 1},     {ebsdlib::H5OINA::X, 1},     {ebsdlib::H5OINA::Y, 1},
};

/**
 * @brief Confirms that every Data dataset of a scan holds exactly as many elements as
 * the Image Geometry expects.
 *
 * H5OINAReader sizes its buffers to whatever extent each dataset actually has, while
 * the geometry and the destination arrays are sized from the header's X Cells and
 * Y Cells. If a file's datasets are shorter than the header claims, copying
 * totalPoints elements out of those buffers reads past their end.
 *
 * No reader API exposes the sizes the reader actually allocated, so the extents are
 * re-probed from the file with H5Lite. That is the file's extents as of this call
 * rather than the buffer sizes themselves; the two agree unless the file changed
 * between H5OINAReader::readFile() and this call. A dataset LONGER than the header
 * describes is rejected as well as a shorter one: the reader would size its buffers
 * to the longer extent and the extra rows would be silently dropped, which is not a
 * result a caller can distinguish from a correct import.
 */
Result<> validateDataSetExtents(const std::filesystem::path& filePath, const std::string& scanName, usize totalPoints)
{
  hid_t fileId = H5Support::H5Utilities::openFile(filePath.string(), true);
  if(fileId < 0)
  {
    return MakeErrorResult(
        -34971, fmt::format("The file '{}' could not be reopened to verify the extents of scan '{}'. The file may have been moved or changed since preflight.", filePath.string(), scanName));
  }
  H5Support::H5ScopedFileSentinel sentinel(fileId, false);

  const std::string dataGroupPath = fmt::format("/{}/{}/{}", scanName, ebsdlib::H5OINA::EBSD, ebsdlib::H5OINA::Data);
  for(const auto& [dataSetName, componentCount] : k_RequiredDataSets)
  {
    const std::string dataSetPath = dataGroupPath + "/" + dataSetName;
    std::vector<hsize_t> dims;
    H5T_class_t classType = H5T_NO_CLASS;
    usize typeSize = 0;
    if(H5Support::H5Lite::getDatasetInfo(fileId, dataSetPath, dims, classType, typeSize) < 0)
    {
      // A missing dataset is fatal inside H5OINAReader, which has already run by the
      // time this is called, so there is nothing to add here.
      continue;
    }
    usize elementCount = 1;
    for(const hsize_t dim : dims)
    {
      elementCount *= static_cast<usize>(dim);
    }
    const usize expectedCount = totalPoints * componentCount;
    if(elementCount != expectedCount)
    {
      return MakeErrorResult(-34971, fmt::format("The dataset '{}' of scan '{}' in '{}' holds {} element(s), but the {} scan point(s) described by the scan's header require {}. The file is "
                                                 "malformed or was changed after preflight.",
                                                 dataSetPath, scanName, filePath.string(), elementCount, totalPoints, expectedCount));
    }
  }
  return {};
}

template <typename T>
void copyRawData(const ReadH5DataInputValues* inputValues, usize count, DataStructure& dataStructure, ebsdlib::H5OINAReader& reader, const std::string& name, usize offset)
{
  using ArrayType = DataArray<T>;
  auto& dataRef = dataStructure.getDataRefAs<ArrayType>(inputValues->CellAttributeMatrixPath.createChildPath(name));
  auto* dataStorePtr = dataRef.getDataStore();

  const nonstd::span<T> rawDataSpan(reinterpret_cast<T*>(reader.getPointerByName(name)), count);
  std::copy(rawDataSpan.begin(), rawDataSpan.end(), dataStorePtr->begin() + offset);
}

/**
 * @brief Applies the EDAX hexagonal x-axis alignment to the phi2 of every hexagonal
 * point of one scan's tuple slab.
 */
template <typename T>
void convertHexEulerAngle(const ReadH5DataInputValues* inputValues, usize totalPoints, usize tupleOffset, DataStructure& dataStructure)
{
  using ArrayType = DataArray<T>;

  const auto& crystalStructuresRef = dataStructure.getDataRefAs<UInt32Array>(inputValues->CellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::AngFile::CrystalStructures));
  const auto& crystalStructuresDSRef = crystalStructuresRef.getDataStoreRef();

  const auto& cellPhasesRef = dataStructure.getDataRefAs<ArrayType>(inputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::H5OINA::Phase));
  const auto& cellPhasesDSRef = cellPhasesRef.getDataStoreRef();

  auto& eulerRef = dataStructure.getDataRefAs<Float32Array>(inputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::H5OINA::Euler));
  auto& eulerDataStoreRef = eulerRef.getDataStoreRef();

  // Only this scan's slab is visited. Looping from 0 every time would shift the first
  // scan's points once per scan and never reach the later scans' points.
  for(usize tupleIdx = tupleOffset; tupleIdx < tupleOffset + totalPoints; tupleIdx++)
  {
    if(crystalStructuresDSRef[cellPhasesDSRef[tupleIdx]] == ebsdlib::CrystalStructure::Hexagonal_High)
    {
      const auto phi2 = static_cast<float64>(eulerDataStoreRef[3 * tupleIdx + 2]);
      eulerDataStoreRef[3 * tupleIdx + 2] = static_cast<float32>(phi2 + k_HexagonalAlignmentRadians);
    }
  }
}

} // namespace

// -----------------------------------------------------------------------------
ReadH5OinaData::ReadH5OinaData(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, ReadH5DataInputValues* inputValues)
: IEbsdOemReader<ebsdlib::H5OINAReader>(dataStructure, messageHandler, shouldCancel, inputValues)
{
}

// -----------------------------------------------------------------------------
ReadH5OinaData::~ReadH5OinaData() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadH5OinaData::operator()()
{
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  imageGeom.setUnits(IGeometry::LengthUnit::Micrometer);

  // The scan loop is kept here rather than in IEbsdOemReader::execute() so that the
  // cancel checks, the progress messages and the stacking order below apply to this
  // filter only.
  //
  // The stacking order chooses which end of the selection list lands in tuple slab 0:
  // Low-to-High reads the scans in the order they are listed, High-to-Low reads them in
  // the reverse of that order, so the last selected scan occupies slab 0.
  std::vector<std::string> orderedScanNames(m_InputValues->SelectedScanNames.scanNames.cbegin(), m_InputValues->SelectedScanNames.scanNames.cend());
  if(m_InputValues->SelectedScanNames.stackingOrder == RefFrameZDir::k_HightoLow)
  {
    std::reverse(orderedScanNames.begin(), orderedScanNames.end());
  }

  const usize scanCount = orderedScanNames.size();
  for(usize scanIdx = 0; scanIdx < scanCount; scanIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    m_CurrentScanName = orderedScanNames[scanIdx];
    m_MessageHandler.sendInfoMessage(fmt::format("Reading scan '{}' ({} of {})", m_CurrentScanName, scanIdx + 1, scanCount));
    Result<> readResults = readData(m_CurrentScanName);
    if(readResults.invalid())
    {
      return readResults;
    }

    if(m_ShouldCancel)
    {
      return {};
    }

    m_MessageHandler.sendInfoMessage(fmt::format("Copying the cell data of scan '{}' ({} of {})", m_CurrentScanName, scanIdx + 1, scanCount));
    Result<> copyDataResults = copyRawEbsdData(static_cast<int>(scanIdx));
    if(copyDataResults.invalid())
    {
      return copyDataResults;
    }
  }
  return {};
}

// -----------------------------------------------------------------------------
Result<> ReadH5OinaData::copyRawEbsdData(int scanIndex)
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const usize totalPoints = imageGeom.getNumXCells() * imageGeom.getNumYCells();
  // Scan `scanIndex` occupies the tuple slab [scanIndex * totalPoints, (scanIndex + 1) * totalPoints).
  const usize tupleOffset = static_cast<usize>(scanIndex) * totalPoints;

  const std::string& scanName = m_CurrentScanName;

  if(Result<> extentResults = validateDataSetExtents(m_InputValues->SelectedScanNames.inputFilePath, scanName, totalPoints); extentResults.invalid())
  {
    return extentResults;
  }

  copyRawData<uint8>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, ebsdlib::H5OINA::BandContrast, tupleOffset);
  copyRawData<uint8>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, ebsdlib::H5OINA::BandSlope, tupleOffset);
  copyRawData<uint8>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, ebsdlib::H5OINA::Bands, tupleOffset);
  copyRawData<uint8>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, ebsdlib::H5OINA::Error, tupleOffset);
  // Euler carries three components per scan point, so both the element count and the
  // destination offset are three times the tuple counts.
  copyRawData<float32>(m_InputValues, totalPoints * 3, m_DataStructure, *m_Reader, ebsdlib::H5OINA::Euler, tupleOffset * 3);
  copyRawData<float32>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, ebsdlib::H5OINA::MeanAngularDeviation, tupleOffset);

  // The phase value of every point indexes the ensemble arrays, both in the alignment
  // loop below and in every downstream filter, so it is range checked before it is
  // stored. The valid range is [0, phase count]: 0 is the reserved Invalid Phase slot.
  {
    const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::AngFile::CrystalStructures));
    const usize ensembleTupleCount = crystalStructures.getNumberOfTuples();
    const nonstd::span<uint8> rawPhaseSpan(reinterpret_cast<uint8*>(m_Reader->getPointerByName(ebsdlib::H5OINA::Phase)), totalPoints);
    for(usize tupleIdx = 0; tupleIdx < totalPoints; tupleIdx++)
    {
      if(static_cast<usize>(rawPhaseSpan[tupleIdx]) >= ensembleTupleCount)
      {
        return MakeErrorResult(-34972, fmt::format("Scan point {} of scan '{}' in '{}' carries phase value {}, which is outside the valid range [0, {}] established by the file's phase definitions.",
                                                   tupleIdx, scanName, m_InputValues->SelectedScanNames.inputFilePath.string(), rawPhaseSpan[tupleIdx], ensembleTupleCount - 1));
      }
    }
  }

  if(m_InputValues->ConvertPhaseToInt32)
  {
    const nonstd::span<uint8> rawDataSpan(reinterpret_cast<uint8*>(m_Reader->getPointerByName(ebsdlib::H5OINA::Phase)), totalPoints);
    auto& dataRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::H5OINA::Phase));
    auto* dataStorePtr = dataRef.getDataStore();
    for(usize tupleIdx = 0; tupleIdx < totalPoints; tupleIdx++)
    {
      dataStorePtr->setValue(tupleIdx + tupleOffset, static_cast<int32>(rawDataSpan[tupleIdx]));
    }
  }
  else
  {
    copyRawData<uint8>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, ebsdlib::H5OINA::Phase, tupleOffset);
  }
  copyRawData<float32>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, ebsdlib::H5OINA::X, tupleOffset);
  copyRawData<float32>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, ebsdlib::H5OINA::Y, tupleOffset);

  if(m_ShouldCancel)
  {
    return {};
  }

  if(m_InputValues->EdaxHexagonalAlignment)
  {
    if(m_InputValues->ConvertPhaseToInt32)
    {
      convertHexEulerAngle<int32>(m_InputValues, totalPoints, tupleOffset, m_DataStructure);
    }
    else
    {
      convertHexEulerAngle<uint8>(m_InputValues, totalPoints, tupleOffset, m_DataStructure);
    }
  }

  return {};
}
