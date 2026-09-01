#include "WriteGBCDTriangleData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"

using namespace nx::core;

WriteGBCDTriangleData::WriteGBCDTriangleData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             WriteGBCDTriangleDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

WriteGBCDTriangleData::~WriteGBCDTriangleData() noexcept = default;

const std::atomic_bool& WriteGBCDTriangleData::getCancel()
{
  return m_ShouldCancel;
}

Result<> WriteGBCDTriangleData::operator()()
{
  auto& faceLabels = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->SurfaceMeshFaceLabelsArrayPath);
  auto& faceNormals = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->SurfaceMeshFaceNormalsArrayPath);
  auto& faceAreas = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->SurfaceMeshFaceAreasArrayPath);
  auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureEulerAnglesArrayPath);
  usize numTriangles = faceAreas.getNumberOfTuples();

  // Cache all feature orientations because face labels can reference features
  // in arbitrary order. Memory scales with the feature count.
  const usize numEulerElements = eulerAngles.getSize();
  std::vector<float32> eulerCache(numEulerElements);
  eulerAngles.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(eulerCache.data(), numEulerElements));

  std::ofstream outStream(m_InputValues->OutputFile, std::ios_base::out | std::ios_base::binary);
  if(!outStream.is_open())
  {
    return MakeErrorResult(-87000, fmt::format("Error opening output file '{}'", m_InputValues->OutputFile.string()));
  }

  outStream << "# Column 1-3:    right hand average orientation (phi1, PHI, phi2 in RADIANS)\n"
            << "# Column 4-6:    left hand average orientation (phi1, PHI, phi2 in RADIANS)\n"
            << "# Column 7-9:    triangle normal\n"
            << "# Column 10:     surface area\n";

  // Page triangle arrays and batch formatted records into one write per page.
  constexpr usize k_ChunkSize = 8192;
  const auto& labelsStore = faceLabels.getDataStoreRef();
  const auto& normalsStore = faceNormals.getDataStoreRef();
  const auto& areasStore = faceAreas.getDataStoreRef();

  std::vector<int32> labelsBuf(k_ChunkSize * 2);
  std::vector<float64> normalsBuf(k_ChunkSize * 3);
  std::vector<float64> areasBuf(k_ChunkSize);
  fmt::memory_buffer writeBuf;

  for(usize chunkStart = 0; chunkStart < numTriangles; chunkStart += k_ChunkSize)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    usize count = std::min(k_ChunkSize, numTriangles - chunkStart);

    labelsStore.copyIntoBuffer(chunkStart * 2, nonstd::span<int32>(labelsBuf.data(), count * 2));
    normalsStore.copyIntoBuffer(chunkStart * 3, nonstd::span<float64>(normalsBuf.data(), count * 3));
    areasStore.copyIntoBuffer(chunkStart, nonstd::span<float64>(areasBuf.data(), count));

    writeBuf.clear();
    for(usize i = 0; i < count; i++)
    {
      int32 gid0 = labelsBuf[i * 2];
      int32 gid1 = labelsBuf[i * 2 + 1];

      if(gid0 < 0 || gid1 < 0)
      {
        continue;
      }

      fmt::format_to(std::back_inserter(writeBuf), "{:0.4f} {:0.4f} {:0.4f} {:0.4f} {:0.4f} {:0.4f} {:0.4f} {:0.4f} {:0.4f} {:0.4f}\n", eulerCache[gid0 * 3], eulerCache[gid0 * 3 + 1],
                     eulerCache[gid0 * 3 + 2], eulerCache[gid1 * 3], eulerCache[gid1 * 3 + 1], eulerCache[gid1 * 3 + 2], normalsBuf[i * 3], normalsBuf[i * 3 + 1], normalsBuf[i * 3 + 2], areasBuf[i]);
    }
    outStream.write(writeBuf.data(), static_cast<std::streamsize>(writeBuf.size()));
  }

  return {};
}
