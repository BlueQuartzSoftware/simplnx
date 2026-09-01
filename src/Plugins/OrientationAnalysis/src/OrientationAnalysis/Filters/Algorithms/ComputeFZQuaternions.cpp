#include "ComputeFZQuaternions.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/LaueOps/LaueOps.h"

#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <memory>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_QuaternionComponents = 4;

/**
 * @brief Number of tuples held by the OOC streaming path at one time.
 *
 * This chunk bounds local memory and amortizes OOC transfer overhead.
 */
constexpr usize k_ChunkTuples = 65536;

/**
 * @brief Creates an out-of-range phase error.
 * @param numPhases Specifies crystal-structure array tuples.
 * @param warningCount Specifies invalid phase IDs.
 * @return Success when warningCount is zero, or error -49008 otherwise.
 */
Result<> CreatePhaseErrorResult(int32 numPhases, int32 warningCount)
{
  if(warningCount <= 0)
  {
    return {};
  }

  std::string errorMessage = fmt::format("The Ensemble Phase information only references {} phase(s) but {} cell(s) had a phase value greater than {}. \
This indicates a problem with the input cell phase data. DREAM3D-NX may have given INCORRECT RESULTS.",
                                         numPhases - 1, warningCount, numPhases - 1);

  return MakeErrorResult<>(-49008, errorMessage);
}

/**
 * @class GenerateFZQuatsAbstractImpl
 * @brief Parallel fallback worker for forced-direct execution on non-contiguous stores.
 * @tparam MaskArrayType Specifies the mask array type.
 *
 * Normal in-memory execution uses GenerateFZQuatsContiguousImpl. This worker
 * supports tests that force Direct for another store type.
 */
template <typename MaskArrayType>
class GenerateFZQuatsAbstractImpl
{
public:
  GenerateFZQuatsAbstractImpl(Float32Array& quats, Int32Array& phases, const std::vector<ebsdlib::LaueOps::Pointer>& phaseOps, int32 numPhases, MaskArrayType* goodVoxels, Float32Array& fzQuats,
                              const std::atomic_bool& shouldCancel, std::atomic_int32_t& warningCount)
  : m_Quats(quats)
  , m_CellPhases(phases)
  , m_PhaseOps(phaseOps)
  , m_NumPhases(numPhases)
  , m_GoodVoxels(goodVoxels)
  , m_FZQuats(fzQuats)
  , m_ShouldCancel(shouldCancel)
  , m_WarningCount(warningCount)
  {
  }

  ~GenerateFZQuatsAbstractImpl() = default;

  void convert(usize start, usize end) const
  {
    for(usize tupleIndex = start; tupleIndex < end; tupleIndex++)
    {
      if(m_ShouldCancel)
      {
        break;
      }

      const int32 phase = m_CellPhases[tupleIndex];
      bool generateFZQuat = true;
      if(m_GoodVoxels != nullptr)
      {
        generateFZQuat = static_cast<bool>((*m_GoodVoxels)[tupleIndex]);
      }

      if(phase >= m_NumPhases)
      {
        m_WarningCount++;
      }

      const usize quaternionIndex = tupleIndex * k_QuaternionComponents;
      m_FZQuats[quaternionIndex] = 0.0F;
      m_FZQuats[quaternionIndex + 1] = 0.0F;
      m_FZQuats[quaternionIndex + 2] = 0.0F;
      m_FZQuats[quaternionIndex + 3] = 0.0F;

      if(phase < m_NumPhases && generateFZQuat && m_PhaseOps[phase] != nullptr)
      {
        ebsdlib::QuatD quat(m_Quats[quaternionIndex], m_Quats[quaternionIndex + 1], m_Quats[quaternionIndex + 2], m_Quats[quaternionIndex + 3]);
        quat = m_PhaseOps[phase]->getFZQuat(quat);
        m_FZQuats[quaternionIndex] = quat.x();
        m_FZQuats[quaternionIndex + 1] = quat.y();
        m_FZQuats[quaternionIndex + 2] = quat.z();
        m_FZQuats[quaternionIndex + 3] = quat.w();
      }
    }
  }

  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  Float32Array& m_Quats;
  Int32Array& m_CellPhases;
  const std::vector<ebsdlib::LaueOps::Pointer>& m_PhaseOps;
  int32 m_NumPhases = 0;
  MaskArrayType* m_GoodVoxels = nullptr;
  Float32Array& m_FZQuats;
  const std::atomic_bool& m_ShouldCancel;
  std::atomic_int32_t& m_WarningCount;
};

/**
 * @class GenerateFZQuatsContiguousImpl
 * @brief Parallel worker using raw pointers from contiguous in-memory DataStores.
 * @tparam MaskType Specifies the mask value type.
 *
 * Direct pointers remove DataArray and virtual DataStore access from the tuple
 * loop. This is not a generic DataArray or DataStore concurrency guarantee.
 */
template <typename MaskType>
class GenerateFZQuatsContiguousImpl
{
public:
  GenerateFZQuatsContiguousImpl(const float32* quats, const int32* phases, const std::vector<ebsdlib::LaueOps::Pointer>& phaseOps, int32 numPhases, const MaskType* goodVoxels, float32* fzQuats,
                                const std::atomic_bool& shouldCancel, std::atomic_int32_t& warningCount)
  : m_Quats(quats)
  , m_CellPhases(phases)
  , m_PhaseOps(phaseOps)
  , m_NumPhases(numPhases)
  , m_GoodVoxels(goodVoxels)
  , m_FZQuats(fzQuats)
  , m_ShouldCancel(shouldCancel)
  , m_WarningCount(warningCount)
  {
  }

  void operator()(const Range& range) const
  {
    for(usize tupleIndex = range.min(); tupleIndex < range.max(); tupleIndex++)
    {
      if(m_ShouldCancel)
      {
        break;
      }

      const int32 phase = m_CellPhases[tupleIndex];
      const bool generateFZQuat = m_GoodVoxels == nullptr || static_cast<bool>(m_GoodVoxels[tupleIndex]);
      if(phase >= m_NumPhases)
      {
        m_WarningCount++;
      }

      const usize quaternionIndex = tupleIndex * k_QuaternionComponents;
      m_FZQuats[quaternionIndex] = 0.0F;
      m_FZQuats[quaternionIndex + 1] = 0.0F;
      m_FZQuats[quaternionIndex + 2] = 0.0F;
      m_FZQuats[quaternionIndex + 3] = 0.0F;

      if(phase < m_NumPhases && generateFZQuat && m_PhaseOps[phase] != nullptr)
      {
        ebsdlib::QuatD quat(m_Quats[quaternionIndex], m_Quats[quaternionIndex + 1], m_Quats[quaternionIndex + 2], m_Quats[quaternionIndex + 3]);
        quat = m_PhaseOps[phase]->getFZQuat(quat);
        m_FZQuats[quaternionIndex] = quat.x();
        m_FZQuats[quaternionIndex + 1] = quat.y();
        m_FZQuats[quaternionIndex + 2] = quat.z();
        m_FZQuats[quaternionIndex + 3] = quat.w();
      }
    }
  }

private:
  const float32* m_Quats = nullptr;
  const int32* m_CellPhases = nullptr;
  const std::vector<ebsdlib::LaueOps::Pointer>& m_PhaseOps;
  int32 m_NumPhases = 0;
  const MaskType* m_GoodVoxels = nullptr;
  float32* m_FZQuats = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  std::atomic_int32_t& m_WarningCount;
};

/**
 * @class ComputeFZQuaternionsDirect
 * @brief Uses direct access for in-memory arrays.
 *
 * Symmetry operators are resolved once per phase. requireArraysInMemory()
 * disables parallel scheduling when a listed array is not in-memory.
 */
class ComputeFZQuaternionsDirect
{
public:
  ComputeFZQuaternionsDirect(DataStructure& dataStructure, const IFilter::MessageHandler&, const std::atomic_bool& shouldCancel, const ComputeFZQuaternionsInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_InputValues(inputValues)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> operator()()
  {
    auto& phaseArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
    auto& quatArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->InputQuatsArrayPath);
    auto& crystalStructuresArray = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
    IDataArray* maskArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath);
    auto& fzQuatArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->InputQuatsArrayPath.replaceName(m_InputValues->OutputFzQuatsArrayName));

    std::atomic_int32_t warningCount = 0;
    const int32 numPhases = static_cast<int32>(crystalStructuresArray.getNumberOfTuples());
    const std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
    std::vector<ebsdlib::LaueOps::Pointer> phaseOps(static_cast<usize>(numPhases));
    for(int32 phase = 0; phase < numPhases; phase++)
    {
      const uint32 crystalStructure = crystalStructuresArray[phase];
      if(crystalStructure < ebsdlib::CrystalStructure::LaueGroupEnd)
      {
        phaseOps[phase] = orientationOps[crystalStructure];
      }
    }

    IParallelAlgorithm::AlgorithmArrays algorithmArrays = {&phaseArray, &quatArray, &crystalStructuresArray, &fzQuatArray};
    if(m_InputValues->UseMask)
    {
      algorithmArrays.push_back(maskArray);
    }

    ParallelDataAlgorithm dataAlgorithm;
    dataAlgorithm.setRange(0ULL, quatArray.getNumberOfTuples());
    dataAlgorithm.requireArraysInMemory(algorithmArrays);

    if(m_InputValues->UseMask)
    {
      if(maskArray->getDataType() == DataType::boolean)
      {
        auto* goodVoxelsArray = m_DataStructure.getDataAs<BoolArray>(m_InputValues->MaskArrayPath);
        executeWithMaskType(dataAlgorithm, quatArray, phaseArray, phaseOps, numPhases, goodVoxelsArray, fzQuatArray, warningCount);
      }
      else if(maskArray->getDataType() == DataType::uint8)
      {
        auto* goodVoxelsArray = m_DataStructure.getDataAs<UInt8Array>(m_InputValues->MaskArrayPath);
        executeWithMaskType(dataAlgorithm, quatArray, phaseArray, phaseOps, numPhases, goodVoxelsArray, fzQuatArray, warningCount);
      }
      else if(maskArray->getDataType() == DataType::int8)
      {
        auto* goodVoxelsArray = m_DataStructure.getDataAs<Int8Array>(m_InputValues->MaskArrayPath);
        executeWithMaskType(dataAlgorithm, quatArray, phaseArray, phaseOps, numPhases, goodVoxelsArray, fzQuatArray, warningCount);
      }
    }
    else
    {
      executeWithMaskType<int8>(dataAlgorithm, quatArray, phaseArray, phaseOps, numPhases, nullptr, fzQuatArray, warningCount);
    }

    return CreatePhaseErrorResult(numPhases, warningCount.load());
  }

private:
  template <typename MaskType>
  void executeWithMaskType(ParallelDataAlgorithm& dataAlgorithm, Float32Array& quatArray, Int32Array& phaseArray, const std::vector<ebsdlib::LaueOps::Pointer>& phaseOps, int32 numPhases,
                           DataArray<MaskType>* maskArray, Float32Array& fzQuatArray, std::atomic_int32_t& warningCount) const
  {
    const auto* quatStore = dynamic_cast<const DataStore<float32>*>(&quatArray.getDataStoreRef());
    const auto* phaseStore = dynamic_cast<const DataStore<int32>*>(&phaseArray.getDataStoreRef());
    auto* fzQuatStore = dynamic_cast<DataStore<float32>*>(&fzQuatArray.getDataStoreRef());
    const auto* maskStore = maskArray == nullptr ? nullptr : dynamic_cast<const DataStore<MaskType>*>(&maskArray->getDataStoreRef());

    const bool hasContiguousMask = maskArray == nullptr || maskStore != nullptr;
    if(quatStore != nullptr && phaseStore != nullptr && fzQuatStore != nullptr && hasContiguousMask)
    {
      const MaskType* maskData = maskStore == nullptr ? nullptr : maskStore->data();
      dataAlgorithm.execute(GenerateFZQuatsContiguousImpl<MaskType>(quatStore->data(), phaseStore->data(), phaseOps, numPhases, maskData, fzQuatStore->data(), m_ShouldCancel, warningCount));
      return;
    }

    dataAlgorithm.execute(GenerateFZQuatsAbstractImpl<DataArray<MaskType>>(quatArray, phaseArray, phaseOps, numPhases, maskArray, fzQuatArray, m_ShouldCancel, warningCount));
  }

  DataStructure& m_DataStructure;
  const ComputeFZQuaternionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @class ComputeFZQuaternionsScanline
 * @brief Processes cell arrays through fixed-size bulk buffers.
 *
 * All DataStore I/O occurs outside the tuple loop. Crystal structures stay
 * local, while cell data remains bounded to one reusable chunk.
 */
class ComputeFZQuaternionsScanline
{
public:
  ComputeFZQuaternionsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const ComputeFZQuaternionsInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_InputValues(inputValues)
  , m_ShouldCancel(shouldCancel)
  , m_MessageHandler(messageHandler)
  {
  }

  Result<> operator()()
  {
    if(m_InputValues->UseMask)
    {
      const auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
      if(maskArray.getDataType() == DataType::boolean)
      {
        return executeWithMaskType<bool>();
      }
      if(maskArray.getDataType() == DataType::uint8)
      {
        return executeWithMaskType<uint8>();
      }
      if(maskArray.getDataType() == DataType::int8)
      {
        return executeWithMaskType<int8>();
      }

      // Legacy behavior treats an unsupported mask type as a no-op.
      return {};
    }

    return executeWithMaskType<uint8>();
  }

private:
  template <typename MaskType>
  Result<> executeWithMaskType()
  {
    const auto& phaseArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
    const auto& quatArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->InputQuatsArrayPath);
    const auto& crystalStructuresArray = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
    auto& fzQuatArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->InputQuatsArrayPath.replaceName(m_InputValues->OutputFzQuatsArrayName));

    const usize totalTuples = quatArray.getNumberOfTuples();
    const int32 numPhases = static_cast<int32>(crystalStructuresArray.getNumberOfTuples());

    std::vector<uint32> crystalStructures(static_cast<usize>(numPhases));
    if(Result<> result = crystalStructuresArray.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), crystalStructures.size())); result.invalid())
    {
      return result;
    }

    const auto& phaseStore = phaseArray.getDataStoreRef();
    const auto& quatStore = quatArray.getDataStoreRef();
    auto& fzQuatStore = fzQuatArray.getDataStoreRef();

    const AbstractDataStore<MaskType>* maskStore = nullptr;
    if(m_InputValues->UseMask)
    {
      const auto& maskArray = m_DataStructure.getDataRefAs<DataArray<MaskType>>(m_InputValues->MaskArrayPath);
      maskStore = &maskArray.getDataStoreRef();
    }

    std::vector<ebsdlib::LaueOps::Pointer> ops = ebsdlib::LaueOps::GetAllOrientationOps();
    auto quatBuffer = std::make_unique<float32[]>(k_ChunkTuples * k_QuaternionComponents);
    auto phaseBuffer = std::make_unique<int32[]>(k_ChunkTuples);
    auto outputBuffer = std::make_unique<float32[]>(k_ChunkTuples * k_QuaternionComponents);
    std::unique_ptr<MaskType[]> maskBuffer;
    if(maskStore != nullptr)
    {
      maskBuffer = std::make_unique<MaskType[]>(k_ChunkTuples);
    }

    int32 warningCount = 0;
    for(usize offset = 0; offset < totalTuples; offset += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const usize count = std::min(k_ChunkTuples, totalTuples - offset);
      if(Result<> result = quatStore.copyIntoBuffer(offset * k_QuaternionComponents, nonstd::span<float32>(quatBuffer.get(), count * k_QuaternionComponents)); result.invalid())
      {
        return result;
      }
      if(Result<> result = phaseStore.copyIntoBuffer(offset, nonstd::span<int32>(phaseBuffer.get(), count)); result.invalid())
      {
        return result;
      }
      if(maskStore != nullptr)
      {
        if(Result<> result = maskStore->copyIntoBuffer(offset, nonstd::span<MaskType>(maskBuffer.get(), count)); result.invalid())
        {
          return result;
        }
      }

      for(usize tupleIndex = 0; tupleIndex < count; tupleIndex++)
      {
        const usize quaternionIndex = tupleIndex * k_QuaternionComponents;
        outputBuffer[quaternionIndex] = 0.0F;
        outputBuffer[quaternionIndex + 1] = 0.0F;
        outputBuffer[quaternionIndex + 2] = 0.0F;
        outputBuffer[quaternionIndex + 3] = 0.0F;

        const int32 phase = phaseBuffer[tupleIndex];
        const bool generateFZQuat = maskStore == nullptr || static_cast<bool>(maskBuffer[tupleIndex]);
        if(phase >= numPhases)
        {
          warningCount++;
        }

        if(phase < numPhases && generateFZQuat && crystalStructures[phase] < ebsdlib::CrystalStructure::LaueGroupEnd)
        {
          ebsdlib::QuatD quat(quatBuffer[quaternionIndex], quatBuffer[quaternionIndex + 1], quatBuffer[quaternionIndex + 2], quatBuffer[quaternionIndex + 3]);
          const int32 crystalStructure = static_cast<int32>(crystalStructures[phase]);
          quat = ops[crystalStructure]->getFZQuat(quat);
          outputBuffer[quaternionIndex] = quat.x();
          outputBuffer[quaternionIndex + 1] = quat.y();
          outputBuffer[quaternionIndex + 2] = quat.z();
          outputBuffer[quaternionIndex + 3] = quat.w();
        }
      }

      if(Result<> result = fzQuatStore.copyFromBuffer(offset * k_QuaternionComponents, nonstd::span<const float32>(outputBuffer.get(), count * k_QuaternionComponents)); result.invalid())
      {
        return result;
      }
    }

    return CreatePhaseErrorResult(numPhases, warningCount);
  }

  DataStructure& m_DataStructure;
  const ComputeFZQuaternionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace

ComputeFZQuaternions::ComputeFZQuaternions(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFZQuaternionsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFZQuaternions::~ComputeFZQuaternions() noexcept = default;

Result<> ComputeFZQuaternions::operator()()
{
  const auto& phaseArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  const auto& quatArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->InputQuatsArrayPath);
  const auto& crystalStructuresArray = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  const auto& fzQuatArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->InputQuatsArrayPath.replaceName(m_InputValues->OutputFzQuatsArrayName));

  if(m_InputValues->UseMask)
  {
    const auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
    return DispatchAlgorithm<ComputeFZQuaternionsDirect, ComputeFZQuaternionsScanline>({&phaseArray, &quatArray, &crystalStructuresArray, &maskArray, &fzQuatArray}, m_DataStructure, m_MessageHandler,
                                                                                       m_ShouldCancel, m_InputValues);
  }

  return DispatchAlgorithm<ComputeFZQuaternionsDirect, ComputeFZQuaternionsScanline>({&phaseArray, &quatArray, &crystalStructuresArray, &fzQuatArray}, m_DataStructure, m_MessageHandler,
                                                                                     m_ShouldCancel, m_InputValues);
}
