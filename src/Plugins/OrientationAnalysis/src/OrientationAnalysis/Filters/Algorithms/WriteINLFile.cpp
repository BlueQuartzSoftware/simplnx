#include "WriteINLFile.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/SIMPLNXVersion.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/IO/TSL/AngConstants.h>

#include <algorithm>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkTuples = 65536;
constexpr usize k_EulerComponents = 3;

/**
 * @brief Maps a simplnx crystal structure to the INL TSL symmetry code.
 * @param symmetry Simplnx crystal-structure value.
 * @return Matching TSL phase symmetry, or UnknownCrystalStructure when unsupported.
 */
uint32 mapCrystalSymmetryToTslSymmetry(uint32 symmetry)
{
  switch(symmetry)
  {
  case ebsdlib::CrystalStructure::Cubic_High:
    return ebsdlib::Ang::PhaseSymmetry::Cubic;
  case ebsdlib::CrystalStructure::Cubic_Low:
    return ebsdlib::Ang::PhaseSymmetry::Tetrahedral;
  case ebsdlib::CrystalStructure::Tetragonal_High:
    return ebsdlib::Ang::PhaseSymmetry::DiTetragonal;
  case ebsdlib::CrystalStructure::Tetragonal_Low:
    return ebsdlib::Ang::PhaseSymmetry::Tetragonal;
  case ebsdlib::CrystalStructure::OrthoRhombic:
    return ebsdlib::Ang::PhaseSymmetry::Orthorhombic;
  case ebsdlib::CrystalStructure::Monoclinic:
    return ebsdlib::Ang::PhaseSymmetry::Monoclinic_c;
  case ebsdlib::CrystalStructure::Triclinic:
    return ebsdlib::Ang::PhaseSymmetry::Triclinic;
  case ebsdlib::CrystalStructure::Hexagonal_High:
    return ebsdlib::Ang::PhaseSymmetry::DiHexagonal;
  case ebsdlib::CrystalStructure::Hexagonal_Low:
    return ebsdlib::Ang::PhaseSymmetry::Hexagonal;
  case ebsdlib::CrystalStructure::Trigonal_High:
    return ebsdlib::Ang::PhaseSymmetry::DiTrigonal;
  case ebsdlib::CrystalStructure::Trigonal_Low:
    return ebsdlib::Ang::PhaseSymmetry::Trigonal;
  default:
    return ebsdlib::CrystalStructure::UnknownCrystalStructure;
  }
}
} // namespace

WriteINLFile::WriteINLFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteINLFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

WriteINLFile::~WriteINLFile() noexcept = default;

const std::atomic_bool& WriteINLFile::getCancel()
{
  return m_ShouldCancel;
}

Result<> WriteINLFile::operator()()
{
  // Create the destination directory before opening and truncating the file.
  Result<> createDirectoriesResult = nx::core::CreateOutputDirectories(m_InputValues->OutputFile.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  std::ofstream fout(m_InputValues->OutputFile, std::ios_base::out | std::ios_base::binary);
  if(!fout.is_open())
  {
    return MakeErrorResult(-74100, fmt::format("Error creating and opening output file at path: {}", m_InputValues->OutputFile.string()));
  }

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeomPath);

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEulerAnglesArrayPath);
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  const auto& numFeatures = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->NumFeaturesArrayPath);
  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->MaterialNameArrayPath);

  const usize totalPoints = featureIds.getNumberOfTuples();

  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 res = imageGeom.getSpacing();
  const FloatVec3 origin = imageGeom.getOrigin();
  const usize totalCells = imageGeom.getNumberOfCells();

  const auto& featureIdsStore = featureIds.getDataStoreRef();
  const auto& eulerAnglesStore = eulerAngles.getDataStoreRef();
  const auto& cellPhasesStore = cellPhases.getDataStoreRef();

  // Concrete DataStore instances bypass staging unless tests force the bulk path.
  // Other store implementations retain bounded sequential reads.
  const auto* directFeatureIdsStore = dynamic_cast<const Int32DataStore*>(&featureIdsStore);
  const auto* directEulerAnglesStore = dynamic_cast<const Float32DataStore*>(&eulerAnglesStore);
  const auto* directCellPhasesStore = dynamic_cast<const Int32DataStore*>(&cellPhasesStore);
  const bool forceBulkAccess = !ForceInCoreAlgorithm() && ForceOocAlgorithm();
  const bool useDirectFeatureIds = !forceBulkAccess && directFeatureIdsStore != nullptr;
  const bool useDirectEulerAngles = !forceBulkAccess && directEulerAnglesStore != nullptr;
  const bool useDirectCellPhases = !forceBulkAccess && directCellPhasesStore != nullptr;
  const bool useDirectCellData = useDirectFeatureIds && useDirectEulerAngles && useDirectCellPhases;
  const bool usesOutOfCoreStore = AnyOutOfCore({&featureIds, &eulerAngles, &cellPhases});
  RecordAlgorithmPathExecution(useDirectCellData ? AlgorithmPath::InCore : AlgorithmPath::OutOfCore, usesOutOfCoreStore);

  // Cache ensemble arrays in full. Memory scales with the phase count.
  std::vector<uint32> crystalStructuresCache(crystalStructures.getSize());
  if(Result<> readResult = crystalStructures.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresCache.data(), crystalStructuresCache.size())); readResult.invalid())
  {
    return readResult;
  }

  std::vector<int32> numFeaturesCache(numFeatures.getSize());
  if(Result<> readResult = numFeatures.getDataStoreRef().copyIntoBuffer(0, nonstd::span<int32>(numFeaturesCache.data(), numFeaturesCache.size())); readResult.invalid())
  {
    return readResult;
  }

  std::unique_ptr<int32[]> featureIdsBuffer;
  std::unique_ptr<float32[]> eulerAnglesBuffer;
  std::unique_ptr<int32[]> cellPhasesBuffer;
  if(!useDirectFeatureIds)
  {
    featureIdsBuffer = std::make_unique<int32[]>(k_ChunkTuples);
  }
  if(!useDirectEulerAngles)
  {
    eulerAnglesBuffer = std::make_unique<float32[]>(k_ChunkTuples * k_EulerComponents);
  }
  if(!useDirectCellPhases)
  {
    cellPhasesBuffer = std::make_unique<int32[]>(k_ChunkTuples);
  }

  std::set<int32> uniqueFeatureIds;
  for(usize tupleOffset = 0; tupleOffset < totalPoints; tupleOffset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize tupleCount = std::min(k_ChunkTuples, totalPoints - tupleOffset);
    const int32* featureIdsChunk = nullptr;
    if(useDirectFeatureIds)
    {
      featureIdsChunk = directFeatureIdsStore->data() + tupleOffset;
    }
    else if(Result<> readResult = featureIdsStore.copyIntoBuffer(tupleOffset, nonstd::span<int32>(featureIdsBuffer.get(), tupleCount)); readResult.invalid())
    {
      return readResult;
    }
    else
    {
      featureIdsChunk = featureIdsBuffer.get();
    }

    uniqueFeatureIds.insert(featureIdsChunk, featureIdsChunk + tupleCount);
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  // Build the comment-prefixed INL header before the first output write.
  std::ostringstream headerBuffer;
  headerBuffer << "# File written from " << nx::core::Version::PackageComplete() << "\n";
  headerBuffer << "# X_STEP: " << std::fixed << res[0] << "\n";
  headerBuffer << "# Y_STEP: " << std::fixed << res[1] << "\n";
  headerBuffer << "# Z_STEP: " << std::fixed << res[2] << "\n";
  headerBuffer << "#\n";
  headerBuffer << "# X_MIN: " << std::fixed << origin[0] << "\n";
  headerBuffer << "# Y_MIN: " << std::fixed << origin[1] << "\n";
  headerBuffer << "# Z_MIN: " << std::fixed << origin[2] << "\n";
  headerBuffer << "#\n";
  headerBuffer << "# X_MAX: " << std::fixed << origin[0] + (static_cast<float64>(dims[0]) * res[0]) << "\n";
  headerBuffer << "# Y_MAX: " << std::fixed << origin[1] + (static_cast<float64>(dims[1]) * res[1]) << "\n";
  headerBuffer << "# Z_MAX: " << std::fixed << origin[2] + (static_cast<float64>(dims[2]) * res[2]) << "\n";
  headerBuffer << "#\n";
  headerBuffer << "# X_DIM: " << dims[0] << "\n";
  headerBuffer << "# Y_DIM: " << dims[1] << "\n";
  headerBuffer << "# Z_DIM: " << dims[2] << "\n";
  headerBuffer << "#\n";

  const int32 materialCount = static_cast<int32>(materialNames.getNumberOfTuples());
  for(uint32 i = 1; i < materialCount; ++i)
  {
    headerBuffer << "# Phase_" << i << ": " << materialNames[i].c_str() << "\n";
    headerBuffer << "# Symmetry_" << i << ": " << mapCrystalSymmetryToTslSymmetry(crystalStructuresCache[i]) << "\n";
    headerBuffer << "# Features_" << i << ": " << numFeaturesCache[i] << "\n";
    headerBuffer << "#\n";
  }

  const int32 count = static_cast<int32>(uniqueFeatureIds.size());
  headerBuffer << "# Num_Features: " << count << " \n";
  headerBuffer << "#\n";

  headerBuffer << "# phi1 PHI phi2 x y z FeatureId PhaseId Symmetry\n";
  const std::string header = headerBuffer.str();
  fout.write(header.data(), static_cast<std::streamsize>(header.size()));
  if(!fout)
  {
    return MakeErrorResult(-74101, fmt::format("Failed writing INL output file '{}'. Check available disk space and write permissions.", m_InputValues->OutputFile.string()));
  }

  std::ostringstream textBuffer;
  std::ostream& textStream = useDirectCellData ? static_cast<std::ostream&>(fout) : static_cast<std::ostream&>(textBuffer);
  textStream << std::fixed;
  for(usize tupleOffset = 0; tupleOffset < totalCells; tupleOffset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize tupleCount = std::min(k_ChunkTuples, totalCells - tupleOffset);
    const int32* featureIdsChunk = nullptr;
    if(useDirectFeatureIds)
    {
      featureIdsChunk = directFeatureIdsStore->data() + tupleOffset;
    }
    else if(Result<> readResult = featureIdsStore.copyIntoBuffer(tupleOffset, nonstd::span<int32>(featureIdsBuffer.get(), tupleCount)); readResult.invalid())
    {
      return readResult;
    }
    else
    {
      featureIdsChunk = featureIdsBuffer.get();
    }

    const float32* eulerAnglesChunk = nullptr;
    if(useDirectEulerAngles)
    {
      eulerAnglesChunk = directEulerAnglesStore->data() + (tupleOffset * k_EulerComponents);
    }
    else if(Result<> readResult = eulerAnglesStore.copyIntoBuffer(tupleOffset * k_EulerComponents, nonstd::span<float32>(eulerAnglesBuffer.get(), tupleCount * k_EulerComponents));
            readResult.invalid())
    {
      return readResult;
    }
    else
    {
      eulerAnglesChunk = eulerAnglesBuffer.get();
    }

    const int32* cellPhasesChunk = nullptr;
    if(useDirectCellPhases)
    {
      cellPhasesChunk = directCellPhasesStore->data() + tupleOffset;
    }
    else if(Result<> readResult = cellPhasesStore.copyIntoBuffer(tupleOffset, nonstd::span<int32>(cellPhasesBuffer.get(), tupleCount)); readResult.invalid())
    {
      return readResult;
    }
    else
    {
      cellPhasesChunk = cellPhasesBuffer.get();
    }

    if(!useDirectCellData)
    {
      textBuffer.str(std::string{});
      textBuffer.clear();
    }

    // Resolve the first tuple once, then advance coordinates in INL X-fastest order.
    usize x = tupleOffset % dims[0];
    const usize yzOffset = tupleOffset / dims[0];
    usize y = yzOffset % dims[1];
    usize z = yzOffset / dims[1];
    for(usize chunkIndex = 0; chunkIndex < tupleCount; chunkIndex++)
    {
      const usize eulerOffset = chunkIndex * k_EulerComponents;
      const float32 phi1 = eulerAnglesChunk[eulerOffset];
      const float32 phi = eulerAnglesChunk[eulerOffset + 1];
      const float32 phi2 = eulerAnglesChunk[eulerOffset + 2];
      const float64 xPos = origin[0] + (static_cast<float64>(x) * res[0]);
      const float64 yPos = origin[1] + (static_cast<float64>(y) * res[1]);
      const float64 zPos = origin[2] + (static_cast<float64>(z) * res[2]);
      const int32 phaseId = cellPhasesChunk[chunkIndex];
      uint32 symmetry = crystalStructuresCache[phaseId];
      if(phaseId > 0)
      {
        if(symmetry == ebsdlib::CrystalStructure::Cubic_High)
        {
          symmetry = ebsdlib::Ang::PhaseSymmetry::Cubic;
        }
        else if(symmetry == ebsdlib::CrystalStructure::Hexagonal_High)
        {
          symmetry = ebsdlib::Ang::PhaseSymmetry::DiHexagonal;
        }
        else
        {
          symmetry = ebsdlib::Ang::PhaseSymmetry::UnknownSymmetry;
        }
      }
      else
      {
        symmetry = ebsdlib::Ang::PhaseSymmetry::UnknownSymmetry;
      }

      textStream << phi1 << " " << phi << " " << phi2 << " " << xPos << " " << yPos << " " << zPos << " " << featureIdsChunk[chunkIndex] << " " << phaseId << " " << symmetry << "\n";

      x++;
      if(x == dims[0])
      {
        x = 0;
        y++;
        if(y == dims[1])
        {
          y = 0;
          z++;
        }
      }
    }

    if(!useDirectCellData)
    {
      const std::string text = textBuffer.str();
      fout.write(text.data(), static_cast<std::streamsize>(text.size()));
    }
    if(!fout)
    {
      return MakeErrorResult(-74101, fmt::format("Failed writing INL output file '{}'. Check available disk space and write permissions.", m_InputValues->OutputFile.string()));
    }
  }

  fout.flush();
  if(!fout)
  {
    return MakeErrorResult(-74101, fmt::format("Failed writing INL output file '{}'. Check available disk space and write permissions.", m_InputValues->OutputFile.string()));
  }

  return {};
}
