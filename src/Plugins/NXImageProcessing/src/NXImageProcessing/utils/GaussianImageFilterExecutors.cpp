#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/DiscreteGaussianEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/GradientMagnitudeEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/RecursiveGaussianEngine.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <fmt/format.h>
#include <memory>
#include <new>
#include <nonstd/span.hpp>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace nx::core::ImageProcessing
{
namespace detail
{
namespace
{
/**
 * @brief 3-D out-of-core route for GradientMagnitudeRecursiveGaussian. All three axis cascades below read and write
 *        @p rawAccumulator, not @p accumulator: every Set/Accumulate call and the dim-2 terminal sink (which fuses
 *        the final square root) target the raw scratch, so the real output store is touched only once, at the very
 *        end, when the finished values are streamed into it via MaterializeGaussianTemporaryStore. @p AccStoreT is a
 *        template parameter (rather than a fixed GaussianTemporaryStore<float32>) so the working-data-format route
 *        can pass @p accumulator itself as @p rawAccumulator -- in that case AccStoreT is AbstractDataStore<float32>,
 *        the compile-time check below skips the redundant final copy, and behavior is identical to writing directly
 *        into @p accumulator throughout.
 */
template <class T, class WorkStoreT, class AccStoreT>
Result<> ApplyGradientMagnitudeRecursiveGaussian3D(const AbstractDataStore<T>& input, AbstractDataStore<float32>& accumulator, AccStoreT& rawAccumulator, WorkStoreT& work, const SizeVec3& dims,
                                                   const FloatVec3& spacing, float64 sigma, bool normalizeAcrossScale, const std::atomic_bool& shouldCancel,
                                                   const IFilter::MessageHandler& messageHandler)
{
  for(uint32 dim = 0; dim < 3; ++dim)
  {
    if(shouldCancel)
    {
      return {};
    }
    std::vector<float32> accumulatorPlane(dim == 2 ? dims[0] * dims[1] : 0);
    auto terminalSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) -> Result<> {
      const double derivativeSpacing = static_cast<double>(spacing[dim]);
      if(dim == 0)
      {
        for(float32& value : filteredPlane)
        {
          const double derivative = static_cast<double>(value) / derivativeSpacing;
          value = static_cast<float32>(derivative * derivative);
        }
        return rawAccumulator.copyFromBuffer(planeOffset, nonstd::span<const float32>(filteredPlane.data(), filteredPlane.size()));
      }
      if(Result<> result = rawAccumulator.copyIntoBuffer(planeOffset, nonstd::span<float32>(accumulatorPlane.data(), filteredPlane.size())); result.invalid())
      {
        return result;
      }
      for(usize index = 0; index < filteredPlane.size(); ++index)
      {
        const double derivative = static_cast<double>(filteredPlane[index]) / derivativeSpacing;
        accumulatorPlane[index] = static_cast<float32>(static_cast<double>(accumulatorPlane[index]) + derivative * derivative);
        if(dim == 2)
        {
          accumulatorPlane[index] = static_cast<float32>(std::sqrt(static_cast<double>(accumulatorPlane[index])));
        }
      }
      return rawAccumulator.copyFromBuffer(planeOffset, nonstd::span<const float32>(accumulatorPlane.data(), filteredPlane.size()));
    };

    if(dim < 2)
    {
      const uint32 otherAxis = dim == 0 ? 1u : 0u;
      const RecursiveGaussianPlaneAxis derivative{dim, sigma, static_cast<double>(spacing[dim]), 1, normalizeAcrossScale};
      const RecursiveGaussianPlaneAxis smoothing{otherAxis, sigma, static_cast<double>(spacing[otherAxis]), 0, normalizeAcrossScale};
      auto workSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) { return work.copyFromBuffer(planeOffset, nonstd::span<const float32>(filteredPlane.data(), filteredPlane.size())); };
      if(Result<> result = RecursiveGaussianXYPlaneCascadeToSink<T>(input, dims, derivative, smoothing, shouldCancel, messageHandler, workSink); result.invalid())
      {
        return result;
      }
      if(Result<> result = RecursiveGaussianAxisPass<float32>(work, work, dims, 2, sigma, static_cast<double>(spacing[2]), 0, normalizeAcrossScale, shouldCancel, messageHandler); result.invalid())
      {
        return result;
      }
      if(dim == 0)
      {
        if(Result<> result = SetSquaredOverSpacing(rawAccumulator, work, static_cast<double>(spacing[dim]), shouldCancel); result.invalid())
        {
          return result;
        }
      }
      else if(Result<> result = AccumulateSquaredOverSpacing(rawAccumulator, work, static_cast<double>(spacing[dim]), shouldCancel); result.invalid())
      {
        return result;
      }
      continue;
    }

    if(Result<> result = RecursiveGaussianAxisPass<T>(input, work, dims, 2, sigma, static_cast<double>(spacing[2]), 1, normalizeAcrossScale, shouldCancel, messageHandler); result.invalid())
    {
      return result;
    }
    const RecursiveGaussianPlaneAxis smoothX{0, sigma, static_cast<double>(spacing[0]), 0, normalizeAcrossScale};
    const RecursiveGaussianPlaneAxis smoothY{1, sigma, static_cast<double>(spacing[1]), 0, normalizeAcrossScale};
    if(Result<> result = RecursiveGaussianXYPlaneCascadeToSink<float32>(work, dims, smoothX, smoothY, shouldCancel, messageHandler, terminalSink); result.invalid())
    {
      return result;
    }
  }
  if constexpr(!std::is_same_v<AccStoreT, AbstractDataStore<float32>>)
  {
    return MaterializeGaussianTemporaryStore(accumulator, rawAccumulator, shouldCancel);
  }
  else
  {
    return {};
  }
}

/**
 * @brief 3-D out-of-core route for LaplacianRecursiveGaussian. All three axis cascades below read and write
 *        @p rawAccumulator, not @p accumulator: every Set/Accumulate call and the dim-2 terminal sink target the raw
 *        scratch, so the real output store is touched only once, at the very end, when the finished sum of scaled
 *        second derivatives is streamed into it via MaterializeGaussianTemporaryStore. @p AccStoreT is a template
 *        parameter (rather than a fixed GaussianTemporaryStore<float32>) so the working-data-format route can pass
 *        @p accumulator itself as @p rawAccumulator -- in that case AccStoreT is AbstractDataStore<float32>, the
 *        compile-time check below skips the redundant final copy, and behavior is identical to writing directly
 *        into @p accumulator throughout.
 */
template <class T, class WorkStoreT, class AccStoreT>
Result<> ApplyLaplacianRecursiveGaussian3D(const AbstractDataStore<T>& input, AbstractDataStore<float32>& accumulator, AccStoreT& rawAccumulator, WorkStoreT& work, const SizeVec3& dims,
                                           const FloatVec3& spacing, float64 sigma, bool normalizeAcrossScale, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  for(uint32 dim = 0; dim < 3; ++dim)
  {
    if(shouldCancel)
    {
      return {};
    }
    const double derivativeSpacing = static_cast<double>(spacing[dim]);
    const double invSpacingSq = 1.0 / (derivativeSpacing * derivativeSpacing);
    std::vector<float32> accumulatorPlane(dim == 2 ? dims[0] * dims[1] : 0);
    auto terminalSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) -> Result<> {
      if(dim == 0)
      {
        for(float32& value : filteredPlane)
        {
          value = static_cast<float32>(static_cast<double>(value) * invSpacingSq);
        }
        return rawAccumulator.copyFromBuffer(planeOffset, nonstd::span<const float32>(filteredPlane.data(), filteredPlane.size()));
      }
      if(Result<> result = rawAccumulator.copyIntoBuffer(planeOffset, nonstd::span<float32>(accumulatorPlane.data(), filteredPlane.size())); result.invalid())
      {
        return result;
      }
      for(usize index = 0; index < filteredPlane.size(); ++index)
      {
        accumulatorPlane[index] = static_cast<float32>(static_cast<double>(accumulatorPlane[index]) + static_cast<double>(filteredPlane[index]) * invSpacingSq);
      }
      return rawAccumulator.copyFromBuffer(planeOffset, nonstd::span<const float32>(accumulatorPlane.data(), filteredPlane.size()));
    };

    if(dim < 2)
    {
      const uint32 otherAxis = dim == 0 ? 1u : 0u;
      const RecursiveGaussianPlaneAxis derivative{dim, sigma, static_cast<double>(spacing[dim]), 2, normalizeAcrossScale};
      const RecursiveGaussianPlaneAxis smoothing{otherAxis, sigma, static_cast<double>(spacing[otherAxis]), 0, normalizeAcrossScale};
      auto workSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) { return work.copyFromBuffer(planeOffset, nonstd::span<const float32>(filteredPlane.data(), filteredPlane.size())); };
      if(Result<> result = RecursiveGaussianXYPlaneCascadeToSink<T>(input, dims, derivative, smoothing, shouldCancel, messageHandler, workSink); result.invalid())
      {
        return result;
      }
      if(Result<> result = RecursiveGaussianAxisPass<float32>(work, work, dims, 2, sigma, static_cast<double>(spacing[2]), 0, normalizeAcrossScale, shouldCancel, messageHandler); result.invalid())
      {
        return result;
      }
      if(dim == 0)
      {
        if(Result<> result = SetScaledDerivative(rawAccumulator, work, invSpacingSq, shouldCancel); result.invalid())
        {
          return result;
        }
      }
      else if(Result<> result = AccumulateScaledDerivative(rawAccumulator, work, invSpacingSq, shouldCancel); result.invalid())
      {
        return result;
      }
      continue;
    }

    if(Result<> result = RecursiveGaussianAxisPass<T>(input, work, dims, 2, sigma, static_cast<double>(spacing[2]), 2, normalizeAcrossScale, shouldCancel, messageHandler); result.invalid())
    {
      return result;
    }
    const RecursiveGaussianPlaneAxis smoothX{0, sigma, static_cast<double>(spacing[0]), 0, normalizeAcrossScale};
    const RecursiveGaussianPlaneAxis smoothY{1, sigma, static_cast<double>(spacing[1]), 0, normalizeAcrossScale};
    if(Result<> result = RecursiveGaussianXYPlaneCascadeToSink<float32>(work, dims, smoothX, smoothY, shouldCancel, messageHandler, terminalSink); result.invalid())
    {
      return result;
    }
  }
  if constexpr(!std::is_same_v<AccStoreT, AbstractDataStore<float32>>)
  {
    return MaterializeGaussianTemporaryStore(accumulator, rawAccumulator, shouldCancel);
  }
  else
  {
    return {};
  }
}

template <class T, class WorkStoreT>
Result<> ApplySmoothingRecursiveGaussian3D(const AbstractDataStore<T>& input, AbstractDataStore<T>& output, WorkStoreT& work, const SizeVec3& dims, const FloatVec3& spacing,
                                           const std::vector<float64>& sigma, bool normalizeAcrossScale, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  if(Result<> result = RecursiveGaussianAxisPass<T>(input, work, dims, 2, sigma[2], static_cast<double>(spacing[2]), 0, normalizeAcrossScale, shouldCancel, messageHandler); result.invalid())
  {
    return result;
  }
  std::vector<T> outputPlane(dims[0] * dims[1]);
  auto outputSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) -> Result<> {
    for(usize index = 0; index < filteredPlane.size(); ++index)
    {
      outputPlane[index] = static_cast<T>(filteredPlane[index]);
    }
    return output.copyFromBuffer(planeOffset, nonstd::span<const T>(outputPlane.data(), filteredPlane.size()));
  };
  const RecursiveGaussianPlaneAxis first{0, sigma[0], static_cast<double>(spacing[0]), 0, normalizeAcrossScale};
  const RecursiveGaussianPlaneAxis second{1, sigma[1], static_cast<double>(spacing[1]), 0, normalizeAcrossScale};
  return RecursiveGaussianXYPlaneCascadeToSink<float32>(work, dims, first, second, shouldCancel, messageHandler, outputSink);
}

template <class T, class WorkStoreT, class ContributionStoreT, class CheckpointStoreT>
Result<> ApplyGradientMagnitudeRecursiveGaussian2D(const AbstractDataStore<T>& input, AbstractDataStore<float32>& accumulator, WorkStoreT& work, ContributionStoreT& contribution,
                                                   CheckpointStoreT& checkpoints, const SizeVec3& dims, const FloatVec3& spacing, float64 sigma, bool normalizeAcrossScale,
                                                   const detail::RecursiveGaussian2DBufferPlan& plan, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  {
    auto terminalSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> filtered) -> Result<> {
      const double derivativeSpacing = static_cast<double>(spacing[0]);
      for(float32& value : filtered)
      {
        const double derivative = static_cast<double>(value) / derivativeSpacing;
        value = static_cast<float32>(derivative * derivative);
      }
      return ImageProcessing::WriteRecursiveGaussian2DBlock<float32>(contribution, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const float32>(filtered.data(), filtered.size()));
    };
    const RecursiveGaussianPlaneAxis derivative{0, sigma, static_cast<double>(spacing[0]), 1, normalizeAcrossScale};
    const RecursiveGaussianPlaneAxis smoothing{1, sigma, static_cast<double>(spacing[1]), 0, normalizeAcrossScale};
    if(Result<> result = RecursiveGaussian2DXYCascadeToSink<T>(input, work, checkpoints, dims, derivative, smoothing, plan, shouldCancel, messageHandler, terminalSink); result.invalid())
    {
      return result;
    }
  }

  {
    auto terminalSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> filtered, nonstd::span<float32> reusable) -> Result<> {
      const double derivativeSpacing = static_cast<double>(spacing[1]);
      if(Result<> result = detail::ReadRecursiveGaussian2DBlock<float32>(contribution, dims[0], yBegin, rowCount, xBegin, columnCount, reusable); result.invalid())
      {
        return result;
      }
      for(usize index = 0; index < filtered.size(); ++index)
      {
        const double derivative = static_cast<double>(filtered[index]) / derivativeSpacing;
        reusable[index] = static_cast<float32>(static_cast<double>(reusable[index]) + derivative * derivative);
        reusable[index] = static_cast<float32>(std::sqrt(static_cast<double>(reusable[index])));
      }
      return ImageProcessing::WriteRecursiveGaussian2DBlock<float32>(accumulator, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const float32>(reusable.data(), reusable.size()));
    };
    const RecursiveGaussianPlaneAxis derivative{1, sigma, static_cast<double>(spacing[1]), 1, normalizeAcrossScale};
    const RecursiveGaussianPlaneAxis smoothing{0, sigma, static_cast<double>(spacing[0]), 0, normalizeAcrossScale};
    if(Result<> result = RecursiveGaussian2DYXCascadeToSink<T>(input, work, checkpoints, dims, derivative, smoothing, plan, shouldCancel, messageHandler, terminalSink); result.invalid())
    {
      return result;
    }
  }
  return {};
}

template <class T, class WorkStoreT, class ContributionStoreT, class CheckpointStoreT>
Result<> ApplyLaplacianRecursiveGaussian2D(const AbstractDataStore<T>& input, AbstractDataStore<float32>& accumulator, WorkStoreT& work, ContributionStoreT& contribution,
                                           CheckpointStoreT& checkpoints, const SizeVec3& dims, const FloatVec3& spacing, float64 sigma, bool normalizeAcrossScale,
                                           const detail::RecursiveGaussian2DBufferPlan& plan, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  {
    auto terminalSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> filtered) -> Result<> {
      const double spacingValue = static_cast<double>(spacing[0]);
      const double invSpacingSq = 1.0 / (spacingValue * spacingValue);
      for(float32& value : filtered)
      {
        value = static_cast<float32>(static_cast<double>(value) * invSpacingSq);
      }
      return ImageProcessing::WriteRecursiveGaussian2DBlock<float32>(contribution, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const float32>(filtered.data(), filtered.size()));
    };
    const RecursiveGaussianPlaneAxis derivative{0, sigma, static_cast<double>(spacing[0]), 2, normalizeAcrossScale};
    const RecursiveGaussianPlaneAxis smoothing{1, sigma, static_cast<double>(spacing[1]), 0, normalizeAcrossScale};
    if(Result<> result = RecursiveGaussian2DXYCascadeToSink<T>(input, work, checkpoints, dims, derivative, smoothing, plan, shouldCancel, messageHandler, terminalSink); result.invalid())
    {
      return result;
    }
  }

  {
    auto terminalSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> filtered, nonstd::span<float32> reusable) -> Result<> {
      const double spacingValue = static_cast<double>(spacing[1]);
      const double invSpacingSq = 1.0 / (spacingValue * spacingValue);
      if(Result<> result = detail::ReadRecursiveGaussian2DBlock<float32>(contribution, dims[0], yBegin, rowCount, xBegin, columnCount, reusable); result.invalid())
      {
        return result;
      }
      for(usize index = 0; index < filtered.size(); ++index)
      {
        reusable[index] = static_cast<float32>(static_cast<double>(reusable[index]) + static_cast<double>(filtered[index]) * invSpacingSq);
      }
      return ImageProcessing::WriteRecursiveGaussian2DBlock<float32>(accumulator, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const float32>(reusable.data(), reusable.size()));
    };
    const RecursiveGaussianPlaneAxis derivative{1, sigma, static_cast<double>(spacing[1]), 2, normalizeAcrossScale};
    const RecursiveGaussianPlaneAxis smoothing{0, sigma, static_cast<double>(spacing[0]), 0, normalizeAcrossScale};
    if(Result<> result = RecursiveGaussian2DYXCascadeToSink<T>(input, work, checkpoints, dims, derivative, smoothing, plan, shouldCancel, messageHandler, terminalSink); result.invalid())
    {
      return result;
    }
  }
  return {};
}

template <class T, class WorkStoreT, class CheckpointStoreT>
Result<> ApplySmoothingRecursiveGaussian2D(const AbstractDataStore<T>& input, AbstractDataStore<T>& output, WorkStoreT& work, CheckpointStoreT& checkpoints, const SizeVec3& dims,
                                           const FloatVec3& spacing, const std::vector<float64>& sigma, bool normalizeAcrossScale, const detail::RecursiveGaussian2DBufferPlan& plan,
                                           const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  std::vector<T> outputBuffer;
  auto terminalSink = [&](usize yBegin, usize xBegin, usize rowCount, usize columnCount, nonstd::span<float32> filtered, nonstd::span<float32>) -> Result<> {
    if constexpr(std::is_same_v<T, float32>)
    {
      return ImageProcessing::WriteRecursiveGaussian2DBlock<float32>(output, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const float32>(filtered.data(), filtered.size()));
    }
    outputBuffer.resize(filtered.size());
    for(usize index = 0; index < filtered.size(); ++index)
    {
      outputBuffer[index] = static_cast<T>(filtered[index]);
    }
    return ImageProcessing::WriteRecursiveGaussian2DBlock<T>(output, dims[0], yBegin, xBegin, rowCount, columnCount, nonstd::span<const T>(outputBuffer.data(), outputBuffer.size()));
  };
  const RecursiveGaussianPlaneAxis smoothY{1, sigma[1], static_cast<double>(spacing[1]), 0, normalizeAcrossScale};
  const RecursiveGaussianPlaneAxis smoothX{0, sigma[0], static_cast<double>(spacing[0]), 0, normalizeAcrossScale};
  return RecursiveGaussian2DYXCascadeToSink<T>(input, work, checkpoints, dims, smoothY, smoothX, plan, shouldCancel, messageHandler, terminalSink);
}

/// @brief Dispatched execute body for the GradientMagnitudeRecursiveGaussian filter.
/// Input is a T-typed scalar image and output is a fixed float32 gradient-magnitude image. For each filtered axis, it
/// differentiates along that axis, smooths along every other axis, and accumulates the squared, spacing-scaled result.
/// Actual OOC endpoints use raw fixed-record float32 scratch; forced plane-cascade tests over resident endpoints retain a
/// format-matched DataStore. Output doubles as the accumulator. This preserves ITK's double-coefficient IIR and float32
/// inter-pass behavior.
struct GradientMagnitudeRecursiveGaussianExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  FloatVec3 spacing;
  float64 sigma;
  bool normalizeAcrossScale;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<float32>>(outputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<float32>& acc = outArray.getDataStoreRef(); // output doubles as the accumulator

    const bool usesOutOfCoreStore = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || acc.getStoreType() == IDataStore::StoreType::OutOfCore;
    const bool usePlaneCascade = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
    RecordAlgorithmPathExecution(usePlaneCascade ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);

    const uint32 effDim = (dims[2] > 1) ? 3u : 2u;
    const detail::RecursiveGaussianStoragePlan storagePlan = detail::SelectRecursiveGaussianStoragePlan(inStore.getStoreType(), inStore.getDataFormat(), acc.getStoreType(), acc.getDataFormat());
    if(usePlaneCascade && effDim == 2 && storagePlan.useTemporaryRecordStore)
    {
      detail::RecursiveGaussian2DBufferPlan plan =
          detail::BuildRecursiveGaussian2DBufferPlan(dims[0], dims[1], RecursiveGaussianPassOptions::k_Default2DResidentLimit, std::max<usize>(20, sizeof(T) + 16));
      if(plan.valid && plan.coreCols != dims[0])
      {
        plan = detail::BuildRecursiveGaussian2DBufferPlan(dims[0], dims[1], RecursiveGaussianPassOptions::k_Default2DResidentLimit, std::max<usize>(24, sizeof(T) + 20));
      }
      plan = detail::AlignRecursiveGaussian2DBufferPlanToChunks(plan, dims[0], dims[1], acc.getChunkShape());
      if(plan.overflow || !plan.valid)
      {
        return MakeErrorResult(-23620, fmt::format("Gradient magnitude recursive Gaussian could not build a bounded 2D plan for dimensions {} x {}.", dims[0], dims[1]));
      }
      usize checkpointCount = 0;
      usize workBatchValues = 0;
      if(!detail::RecursiveGaussianCheckedMultiply(plan.blockCount, plan.coreCols, checkpointCount) || !detail::RecursiveGaussianCheckedMultiply(plan.blockRows, plan.coreCols, workBatchValues))
      {
        return MakeErrorResult(-23620, fmt::format("Gradient magnitude recursive Gaussian bounded 2D scratch dimensions overflow for {} x {}.", dims[0], dims[1]));
      }
      checkpointCount = std::max(checkpointCount, detail::RecursiveGaussian2DXBlockCount(dims[0], plan.coreCols));
      auto checkpointResult = detail::CreateGaussianTemporaryStore<detail::RecursiveGaussian2DCheckpoint>(checkpointCount, plan.coreCols, shouldCancel);
      if(checkpointResult.invalid())
      {
        return ConvertResult(std::move(checkpointResult));
      }
      auto workResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), workBatchValues, shouldCancel);
      if(workResult.invalid())
      {
        return ConvertResult(std::move(workResult));
      }
      auto contributionResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), workBatchValues, shouldCancel);
      if(contributionResult.invalid())
      {
        return ConvertResult(std::move(contributionResult));
      }
      std::unique_ptr<detail::GaussianTemporaryStore<detail::RecursiveGaussian2DCheckpoint>> checkpoints = std::move(checkpointResult.value());
      std::unique_ptr<detail::GaussianTemporaryStore<float32>> work = std::move(workResult.value());
      std::unique_ptr<detail::GaussianTemporaryStore<float32>> contribution = std::move(contributionResult.value());
      return ApplyGradientMagnitudeRecursiveGaussian2D<T>(inStore, acc, *work, *contribution, *checkpoints, dims, spacing, sigma, normalizeAcrossScale, plan, shouldCancel, messageHandler);
    }
    if(usePlaneCascade && effDim == 3)
    {
      if(storagePlan.useTemporaryRecordStore)
      {
        auto temporaryResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), dims[0] * dims[1], shouldCancel);
        if(temporaryResult.invalid())
        {
          return ConvertResult(std::move(temporaryResult));
        }
        std::unique_ptr<detail::GaussianTemporaryStore<float32>> work = std::move(temporaryResult.value());
        // Raw (non-deflate-backed) full-volume accumulator scratch, created the same way as `work`: every axis
        // cascade's Set/Accumulate below targets this instead of the real deflate-backed output store, so the
        // running gradient-magnitude sum never inflates/deflates against `acc`. Only the finished, square-rooted
        // result is streamed into `acc` once, at the end of ApplyGradientMagnitudeRecursiveGaussian3D.
        auto accumulatorResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), dims[0] * dims[1], shouldCancel);
        if(accumulatorResult.invalid())
        {
          return ConvertResult(std::move(accumulatorResult));
        }
        std::unique_ptr<detail::GaussianTemporaryStore<float32>> rawAccumulator = std::move(accumulatorResult.value());
        return ApplyGradientMagnitudeRecursiveGaussian3D<T>(inStore, acc, *rawAccumulator, *work, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
      }
      auto work = DataStoreUtilities::CreateDataStoreWithFormat<float32>(storagePlan.workingDataFormat, inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
      return ApplyGradientMagnitudeRecursiveGaussian3D<T>(inStore, acc, acc, *work, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
    }
    if(usePlaneCascade)
    {
      std::vector<float32> accumulatorPlane(dims[0] * dims[1]);
      for(uint32 dim = 0; dim < 2; ++dim)
      {
        if(shouldCancel)
        {
          return {};
        }

        auto terminalSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) -> Result<> {
          const double derivativeSpacing = static_cast<double>(spacing[dim]);
          if(dim == 0)
          {
            for(float32& value : filteredPlane)
            {
              const double derivative = static_cast<double>(value) / derivativeSpacing;
              value = static_cast<float32>(derivative * derivative);
            }
            return acc.copyFromBuffer(planeOffset, nonstd::span<const float32>(filteredPlane.data(), filteredPlane.size()));
          }

          if(Result<> result = acc.copyIntoBuffer(planeOffset, nonstd::span<float32>(accumulatorPlane.data(), filteredPlane.size())); result.invalid())
          {
            return result;
          }
          for(usize i = 0; i < filteredPlane.size(); ++i)
          {
            const double derivative = static_cast<double>(filteredPlane[i]) / derivativeSpacing;
            accumulatorPlane[i] = static_cast<float32>(static_cast<double>(accumulatorPlane[i]) + derivative * derivative);
            if(dim == 1)
            {
              accumulatorPlane[i] = static_cast<float32>(std::sqrt(static_cast<double>(accumulatorPlane[i])));
            }
          }
          return acc.copyFromBuffer(planeOffset, nonstd::span<const float32>(accumulatorPlane.data(), filteredPlane.size()));
        };

        const uint32 otherAxis = dim == 0 ? 1u : 0u;
        const RecursiveGaussianPlaneAxis derivative{dim, sigma, static_cast<double>(spacing[dim]), 1, normalizeAcrossScale};
        const RecursiveGaussianPlaneAxis smoothing{otherAxis, sigma, static_cast<double>(spacing[otherAxis]), 0, normalizeAcrossScale};
        if(Result<> result = RecursiveGaussianXYPlaneCascadeToSink<T>(inStore, dims, derivative, smoothing, shouldCancel, messageHandler, terminalSink); result.invalid())
        {
          return result;
        }
      }
      return {};
    }

    const std::string format = detail::SelectRecursiveGaussianWorkingDataFormat(inStore.getStoreType(), inStore.getDataFormat(), acc.getStoreType(), acc.getDataFormat());
    const std::vector<usize> tupleShape = inArray.getTupleShape();
    auto workPtr = DataStoreUtilities::CreateDataStoreWithFormat<float32>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
    AbstractDataStore<float32>& work = *workPtr;

    for(uint32 dim = 0; dim < effDim; ++dim)
    {
      if(shouldCancel)
      {
        return {};
      }
      // order-1 derivative along `dim` (reads raw input T)
      if(Result<> r = RecursiveGaussianAxisPass<T>(inStore, work, dims, dim, sigma, static_cast<double>(spacing[dim]), /*order=*/1, normalizeAcrossScale, shouldCancel, messageHandler); r.invalid())
      {
        return r;
      }
      // order-0 smoothing along every other axis, in increasing order
      for(uint32 a = 0; a < effDim; ++a)
      {
        if(a == dim)
        {
          continue;
        }
        if(Result<> r = RecursiveGaussianAxisPass<float32>(work, work, dims, a, sigma, static_cast<double>(spacing[a]), /*order=*/0, normalizeAcrossScale, shouldCancel, messageHandler); r.invalid())
        {
          return r;
        }
      }
      // dim 0 OVERWRITES acc (no prior fill needed); later dims accumulate. effDim >= 2 always, so dim 0 always runs and
      // fully writes acc. Set-then-Accumulate is bit-identical to the old Fill(0)+Accumulate for every dim.
      if(dim == 0)
      {
        if(Result<> r = SetSquaredOverSpacing(acc, work, static_cast<double>(spacing[dim]), shouldCancel); r.invalid())
        {
          return r;
        }
      }
      else
      {
        if(Result<> r = AccumulateSquaredOverSpacing(acc, work, static_cast<double>(spacing[dim]), shouldCancel); r.invalid())
        {
          return r;
        }
      }
    }
    return SqrtStoreInPlace(acc, shouldCancel);
  }
};

/**
 * @brief Dispatched execute body for the (ITK-free) GradientMagnitude filter. Input is the T-typed (any scalar)
 *        image; output is a FIXED float32 gradient-magnitude image, computed by the single-pass central-difference
 *        streaming engine (@ref ApplyGradientMagnitude) rather than the separable recursive-Gaussian engine used by
 *        @ref GradientMagnitudeRecursiveGaussianExecuteFn above. Matches the legacy ITK GradientMagnitudeImageFilter.
 */
struct GradientMagnitudeExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  FloatVec3 spacing;
  bool useImageSpacing;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<float32>>(outputArrayPath);
    return ApplyGradientMagnitude<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, useImageSpacing, spacing, shouldCancel, messageHandler);
  }
};

template <class T>
Result<> ApplyLaplacianRecursiveGaussianResident(const AbstractDataStore<T>& input, AbstractDataStore<float32>& accumulator, AbstractDataStore<float32>& work, const SizeVec3& dims,
                                                 const FloatVec3& spacing, float64 sigma, bool normalizeAcrossScale, const std::atomic_bool& shouldCancel,
                                                 const IFilter::MessageHandler& messageHandler)
{
  const uint32 effectiveDimensions = dims[2] > 1 ? 3u : 2u;
  for(uint32 dim = 0; dim < effectiveDimensions; ++dim)
  {
    if(shouldCancel)
    {
      return {};
    }
    if(Result<> result = RecursiveGaussianAxisPass<T>(input, work, dims, dim, sigma, static_cast<double>(spacing[dim]), /*order=*/2, normalizeAcrossScale, shouldCancel, messageHandler);
       result.invalid())
    {
      return result;
    }
    for(uint32 axis = 0; axis < effectiveDimensions; ++axis)
    {
      if(axis == dim)
      {
        continue;
      }
      if(Result<> result = RecursiveGaussianAxisPass<float32>(work, work, dims, axis, sigma, static_cast<double>(spacing[axis]), /*order=*/0, normalizeAcrossScale, shouldCancel, messageHandler);
         result.invalid())
      {
        return result;
      }
    }
    const double derivativeSpacing = static_cast<double>(spacing[dim]);
    const double inverseSpacingSquared = 1.0 / (derivativeSpacing * derivativeSpacing);
    if(dim == 0)
    {
      if(Result<> result = SetScaledDerivative(accumulator, work, inverseSpacingSquared, shouldCancel); result.invalid())
      {
        return result;
      }
    }
    else if(Result<> result = AccumulateScaledDerivative(accumulator, work, inverseSpacingSquared, shouldCancel); result.invalid())
    {
      return result;
    }
  }
  return {};
}

/// @brief Dispatched execute body for the LaplacianRecursiveGaussian filter.
/// Input is a T-typed scalar image and output is a fixed float32 Laplacian image. For each filtered axis, it computes the
/// second derivative, smooths along every other axis, and accumulates the spacing-scaled derivative without squaring or
/// a final square root. Actual OOC endpoints use raw fixed-record float32 scratch; forced plane-cascade tests over
/// resident endpoints retain a format-matched DataStore. Output doubles as the accumulator. This preserves ITK's
/// double-coefficient IIR and float32 inter-pass behavior.
struct LaplacianRecursiveGaussianExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  FloatVec3 spacing;
  float64 sigma;
  bool normalizeAcrossScale;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<float32>>(outputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<float32>& acc = outArray.getDataStoreRef(); // output doubles as the accumulator

    const bool usesOutOfCoreStore = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || acc.getStoreType() == IDataStore::StoreType::OutOfCore;
    const bool usePlaneCascade = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
    RecordAlgorithmPathExecution(usePlaneCascade ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);

    const uint32 effDim = (dims[2] > 1) ? 3u : 2u;
    if(usesOutOfCoreStore && !ForceInCoreAlgorithm() && detail::ShouldUseLaplacianRecursiveGaussianResidentState(dims))
    {
      auto allocationResult = detail::ReserveLaplacianRecursiveGaussianResidentWorkingMemory<T>(dims);
      if(allocationResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(allocationResult));
      }
      auto allocation = std::move(allocationResult.value());
      if(allocation.holdsCompleteState())
      {
        try
        {
          DataStore<T> residentInput(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, std::nullopt);
          DataStore<float32> residentAccumulator(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, std::nullopt);
          DataStore<float32> residentWork(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, std::nullopt);
          if(Result<> result = inStore.copyIntoBuffer(0, residentInput.createSpan()); result.invalid())
          {
            return result;
          }
          if(Result<> result = ApplyLaplacianRecursiveGaussianResident(residentInput, residentAccumulator, residentWork, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
             result.invalid())
          {
            return result;
          }
          if(shouldCancel)
          {
            return {};
          }
          const auto outputSpan = residentAccumulator.createSpan();
          return acc.copyFromBuffer(0, nonstd::span<const float32>(outputSpan.data(), outputSpan.size()));
        } catch(const std::bad_alloc&)
        {
          // Release the complete-state reservation before entering the raw-record plane-cascade fallback.
        }
      }
    }
    const detail::RecursiveGaussianStoragePlan storagePlan = detail::SelectRecursiveGaussianStoragePlan(inStore.getStoreType(), inStore.getDataFormat(), acc.getStoreType(), acc.getDataFormat());
    if(usePlaneCascade && effDim == 2 && storagePlan.useTemporaryRecordStore)
    {
      detail::RecursiveGaussian2DBufferPlan plan =
          detail::BuildRecursiveGaussian2DBufferPlan(dims[0], dims[1], RecursiveGaussianPassOptions::k_Default2DResidentLimit, std::max<usize>(20, sizeof(T) + 16));
      if(plan.valid && plan.coreCols != dims[0])
      {
        plan = detail::BuildRecursiveGaussian2DBufferPlan(dims[0], dims[1], RecursiveGaussianPassOptions::k_Default2DResidentLimit, std::max<usize>(24, sizeof(T) + 20));
      }
      plan = detail::AlignRecursiveGaussian2DBufferPlanToChunks(plan, dims[0], dims[1], acc.getChunkShape());
      if(plan.overflow || !plan.valid)
      {
        return MakeErrorResult(-23621, fmt::format("Laplacian recursive Gaussian could not build a bounded 2D plan for dimensions {} x {}.", dims[0], dims[1]));
      }
      usize checkpointCount = 0;
      usize workBatchValues = 0;
      if(!detail::RecursiveGaussianCheckedMultiply(plan.blockCount, plan.coreCols, checkpointCount) || !detail::RecursiveGaussianCheckedMultiply(plan.blockRows, plan.coreCols, workBatchValues))
      {
        return MakeErrorResult(-23621, fmt::format("Laplacian recursive Gaussian bounded 2D scratch dimensions overflow for {} x {}.", dims[0], dims[1]));
      }
      checkpointCount = std::max(checkpointCount, detail::RecursiveGaussian2DXBlockCount(dims[0], plan.coreCols));
      auto checkpointResult = detail::CreateGaussianTemporaryStore<detail::RecursiveGaussian2DCheckpoint>(checkpointCount, plan.coreCols, shouldCancel);
      if(checkpointResult.invalid())
      {
        return ConvertResult(std::move(checkpointResult));
      }
      auto workResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), workBatchValues, shouldCancel);
      if(workResult.invalid())
      {
        return ConvertResult(std::move(workResult));
      }
      auto contributionResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), workBatchValues, shouldCancel);
      if(contributionResult.invalid())
      {
        return ConvertResult(std::move(contributionResult));
      }
      std::unique_ptr<detail::GaussianTemporaryStore<detail::RecursiveGaussian2DCheckpoint>> checkpoints = std::move(checkpointResult.value());
      std::unique_ptr<detail::GaussianTemporaryStore<float32>> work = std::move(workResult.value());
      std::unique_ptr<detail::GaussianTemporaryStore<float32>> contribution = std::move(contributionResult.value());
      return ApplyLaplacianRecursiveGaussian2D<T>(inStore, acc, *work, *contribution, *checkpoints, dims, spacing, sigma, normalizeAcrossScale, plan, shouldCancel, messageHandler);
    }
    if(usePlaneCascade && effDim == 3)
    {
      if(storagePlan.useTemporaryRecordStore)
      {
        auto temporaryResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), dims[0] * dims[1], shouldCancel);
        if(temporaryResult.invalid())
        {
          return ConvertResult(std::move(temporaryResult));
        }
        std::unique_ptr<detail::GaussianTemporaryStore<float32>> work = std::move(temporaryResult.value());
        // Raw (non-deflate-backed) full-volume accumulator scratch, created the same way as `work`: every axis
        // cascade's Set/Accumulate below targets this instead of the real deflate-backed output store, so the
        // running Laplacian sum never inflates/deflates against `acc`. Only the finished result is streamed into
        // `acc` once, at the end of ApplyLaplacianRecursiveGaussian3D.
        auto accumulatorResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), dims[0] * dims[1], shouldCancel);
        if(accumulatorResult.invalid())
        {
          return ConvertResult(std::move(accumulatorResult));
        }
        std::unique_ptr<detail::GaussianTemporaryStore<float32>> rawAccumulator = std::move(accumulatorResult.value());
        return ApplyLaplacianRecursiveGaussian3D<T>(inStore, acc, *rawAccumulator, *work, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
      }
      auto work = DataStoreUtilities::CreateDataStoreWithFormat<float32>(storagePlan.workingDataFormat, inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
      return ApplyLaplacianRecursiveGaussian3D<T>(inStore, acc, acc, *work, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
    }
    if(usePlaneCascade)
    {
      std::vector<float32> accumulatorPlane(dims[0] * dims[1]);
      for(uint32 dim = 0; dim < 2; ++dim)
      {
        if(shouldCancel)
        {
          return {};
        }
        const double derivativeSpacing = static_cast<double>(spacing[dim]);
        const double invSpacingSq = 1.0 / (derivativeSpacing * derivativeSpacing);
        auto terminalSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) -> Result<> {
          if(dim == 0)
          {
            for(float32& value : filteredPlane)
            {
              value = static_cast<float32>(static_cast<double>(value) * invSpacingSq);
            }
            return acc.copyFromBuffer(planeOffset, nonstd::span<const float32>(filteredPlane.data(), filteredPlane.size()));
          }

          if(Result<> result = acc.copyIntoBuffer(planeOffset, nonstd::span<float32>(accumulatorPlane.data(), filteredPlane.size())); result.invalid())
          {
            return result;
          }
          for(usize i = 0; i < filteredPlane.size(); ++i)
          {
            accumulatorPlane[i] = static_cast<float32>(static_cast<double>(accumulatorPlane[i]) + static_cast<double>(filteredPlane[i]) * invSpacingSq);
          }
          return acc.copyFromBuffer(planeOffset, nonstd::span<const float32>(accumulatorPlane.data(), filteredPlane.size()));
        };

        const uint32 otherAxis = dim == 0 ? 1u : 0u;
        const RecursiveGaussianPlaneAxis derivative{dim, sigma, static_cast<double>(spacing[dim]), 2, normalizeAcrossScale};
        const RecursiveGaussianPlaneAxis smoothing{otherAxis, sigma, static_cast<double>(spacing[otherAxis]), 0, normalizeAcrossScale};
        if(Result<> result = RecursiveGaussianXYPlaneCascadeToSink<T>(inStore, dims, derivative, smoothing, shouldCancel, messageHandler, terminalSink); result.invalid())
        {
          return result;
        }
      }
      return {};
    }

    const std::string format = detail::SelectRecursiveGaussianWorkingDataFormat(inStore.getStoreType(), inStore.getDataFormat(), acc.getStoreType(), acc.getDataFormat());
    auto work = DataStoreUtilities::CreateDataStoreWithFormat<float32>(format, inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
    return ApplyLaplacianRecursiveGaussianResident(inStore, acc, *work, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
  }
};

/// @brief Dispatched execute body for the SmoothingRecursiveGaussian filter.
/// Input and output share the same signed scalar type T. The cascade applies one order-0 pass per active axis in ITK's
/// last-axis-first order. Actual OOC endpoints use raw fixed-record float32 scratch; forced plane-cascade tests over
/// resident endpoints retain a format-matched DataStore. Later passes run float32 in place before the result is cast back
/// to T, preserving ITK's double-coefficient IIR and float32 inter-pass behavior.
struct SmoothingRecursiveGaussianExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  FloatVec3 spacing;
  std::vector<float64> sigma; // per-axis (x,y,z)
  bool normalizeAcrossScale;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    // Output is SameAsInput (type T). Inter-pass values use ITK's float32 InternalRealType. The first pass reads the raw T
    // input; later passes run float32 in place.
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    const AbstractDataStore<T>& inStore = inArray.getDataStoreRef();
    AbstractDataStore<T>& outStore = outArray.getDataStoreRef();

    const bool usesOutOfCoreStore = inStore.getStoreType() == IDataStore::StoreType::OutOfCore || outStore.getStoreType() == IDataStore::StoreType::OutOfCore;
    const bool usePlaneCascade = !ForceInCoreAlgorithm() && (usesOutOfCoreStore || ForceOocAlgorithm());
    RecordAlgorithmPathExecution(usePlaneCascade ? AlgorithmPath::OutOfCore : AlgorithmPath::InCore, usesOutOfCoreStore);

    const uint32 effDim = (dims[2] > 1) ? 3u : 2u;
    const detail::RecursiveGaussianStoragePlan storagePlan =
        detail::SelectRecursiveGaussianStoragePlan(inStore.getStoreType(), inStore.getDataFormat(), outStore.getStoreType(), outStore.getDataFormat());
    if(usePlaneCascade && effDim == 2 && storagePlan.useTemporaryRecordStore)
    {
      constexpr usize k_BlockBytesPerValue = std::is_same_v<T, float32> ? sizeof(T) + 16 : 2 * sizeof(T) + 16;
      detail::RecursiveGaussian2DBufferPlan plan = detail::BuildRecursiveGaussian2DBufferPlan(dims[0], dims[1], RecursiveGaussianPassOptions::k_Default2DResidentLimit, k_BlockBytesPerValue);
      if(plan.valid && plan.coreCols != dims[0])
      {
        plan = detail::BuildRecursiveGaussian2DBufferPlan(dims[0], dims[1], RecursiveGaussianPassOptions::k_Default2DResidentLimit, k_BlockBytesPerValue + sizeof(float32));
      }
      plan = detail::AlignRecursiveGaussian2DBufferPlanToChunks(plan, dims[0], dims[1], outStore.getChunkShape());
      if(plan.overflow || !plan.valid)
      {
        return MakeErrorResult(-23622, fmt::format("Smoothing recursive Gaussian could not build a bounded 2D plan for dimensions {} x {}.", dims[0], dims[1]));
      }
      usize checkpointCount = 0;
      usize workBatchValues = 0;
      if(!detail::RecursiveGaussianCheckedMultiply(plan.blockCount, plan.coreCols, checkpointCount) || !detail::RecursiveGaussianCheckedMultiply(plan.blockRows, plan.coreCols, workBatchValues))
      {
        return MakeErrorResult(-23622, fmt::format("Smoothing recursive Gaussian bounded 2D checkpoint dimensions overflow for {} x {}.", dims[0], dims[1]));
      }
      checkpointCount = std::max(checkpointCount, detail::RecursiveGaussian2DXBlockCount(dims[0], plan.coreCols));
      auto checkpointResult = detail::CreateGaussianTemporaryStore<detail::RecursiveGaussian2DCheckpoint>(checkpointCount, plan.coreCols, shouldCancel);
      if(checkpointResult.invalid())
      {
        return ConvertResult(std::move(checkpointResult));
      }
      auto workResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), workBatchValues, shouldCancel);
      if(workResult.invalid())
      {
        return ConvertResult(std::move(workResult));
      }
      std::unique_ptr<detail::GaussianTemporaryStore<detail::RecursiveGaussian2DCheckpoint>> checkpoints = std::move(checkpointResult.value());
      std::unique_ptr<detail::GaussianTemporaryStore<float32>> work = std::move(workResult.value());
      return ApplySmoothingRecursiveGaussian2D<T>(inStore, outStore, *work, *checkpoints, dims, spacing, sigma, normalizeAcrossScale, plan, shouldCancel, messageHandler);
    }
    if(usePlaneCascade && effDim == 3)
    {
      if(storagePlan.useTemporaryRecordStore)
      {
        auto temporaryResult = detail::CreateGaussianTemporaryStore<float32>(inStore.getSize(), dims[0] * dims[1], shouldCancel);
        if(temporaryResult.invalid())
        {
          return ConvertResult(std::move(temporaryResult));
        }
        std::unique_ptr<detail::GaussianTemporaryStore<float32>> work = std::move(temporaryResult.value());
        return ApplySmoothingRecursiveGaussian3D<T>(inStore, outStore, *work, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
      }
      auto work = DataStoreUtilities::CreateDataStoreWithFormat<float32>(storagePlan.workingDataFormat, inArray.getTupleShape(), std::vector<usize>{1}, IDataAction::Mode::Execute);
      return ApplySmoothingRecursiveGaussian3D<T>(inStore, outStore, *work, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler);
    }
    if(usePlaneCascade)
    {
      std::vector<T> outputPlane(dims[0] * dims[1]);
      auto outputSink = [&](usize planeOffset, nonstd::span<float32> filteredPlane) -> Result<> {
        for(usize i = 0; i < filteredPlane.size(); ++i)
        {
          outputPlane[i] = static_cast<T>(filteredPlane[i]);
        }
        return outStore.copyFromBuffer(planeOffset, nonstd::span<const T>(outputPlane.data(), filteredPlane.size()));
      };

      const RecursiveGaussianPlaneAxis first{1, sigma[1], static_cast<double>(spacing[1]), 0, normalizeAcrossScale};
      const RecursiveGaussianPlaneAxis second{0, sigma[0], static_cast<double>(spacing[0]), 0, normalizeAcrossScale};
      return RecursiveGaussianXYPlaneCascadeToSink<T>(inStore, dims, first, second, shouldCancel, messageHandler, outputSink);
    }

    const std::string format = detail::SelectRecursiveGaussianWorkingDataFormat(inStore.getStoreType(), inStore.getDataFormat(), outStore.getStoreType(), outStore.getDataFormat());
    const std::vector<usize> tupleShape = inArray.getTupleShape();
    auto workPtr = DataStoreUtilities::CreateDataStoreWithFormat<float32>(format, tupleShape, std::vector<usize>{1}, IDataAction::Mode::Execute);
    AbstractDataStore<float32>& work = *workPtr;

    // Cascade axis order mirrors ITK SmoothingRecursiveGaussian (last axis first, then 0,1,...); order-0 passes commute, so this order only affects float32 rounding, not correctness.
    std::vector<uint32> axisOrder;
    axisOrder.push_back(effDim - 1);
    for(uint32 a = 0; a < effDim - 1; ++a)
    {
      axisOrder.push_back(a);
    }

    bool first = true;
    for(uint32 axis : axisOrder)
    {
      const double s = sigma[axis];
      const double sp = static_cast<double>(spacing[axis]);
      if(first)
      {
        if(Result<> r = RecursiveGaussianAxisPass<T>(inStore, work, dims, axis, s, sp, /*order=*/0, normalizeAcrossScale, shouldCancel, messageHandler); r.invalid())
        {
          return r;
        }
        first = false;
      }
      else
      {
        if(Result<> r = RecursiveGaussianAxisPass<float32>(work, work, dims, axis, s, sp, /*order=*/0, normalizeAcrossScale, shouldCancel, messageHandler); r.invalid())
        {
          return r;
        }
      }
    }
    return CastFloat32StoreTo<T>(work, outStore, shouldCancel);
  }
};

/**
 * @brief Dispatched execute body for the (ITK-free) DiscreteGaussian filter. Input is the T-typed (any scalar) image;
 *        output is SameAsInput (type T). The full separable FIR cascade (verbatim GaussianOperator kernel per axis,
 *        descending axis order, T-typed intermediates with per-pass static_cast<T>, ZeroFluxNeumann boundary) lives in
 *        the streaming engine @ref ApplyDiscreteGaussian, which this functor simply dispatches to. Matches the legacy
 *        ITK DiscreteGaussianImageFilter.
 */
struct DiscreteGaussianExecuteFn
{
  DataStructure& dataStructure;
  const DataPath& inputArrayPath;
  const DataPath& outputArrayPath;
  SizeVec3 dims;
  FloatVec3 spacing;
  std::vector<float64> variance;     // per-axis (x,y,z)
  std::vector<float64> maximumError; // per-axis (x,y,z)
  uint32 maximumKernelWidth;
  bool useImageSpacing;
  const std::atomic_bool& shouldCancel;
  const IFilter::MessageHandler& messageHandler;

  template <class T>
  Result<> operator()() const
  {
    const auto& inArray = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath);
    auto& outArray = dataStructure.getDataRefAs<DataArray<T>>(outputArrayPath);
    return ApplyDiscreteGaussian<T>(inArray.getDataStoreRef(), outArray.getDataStoreRef(), dims, variance, maximumKernelWidth, maximumError, useImageSpacing, spacing, shouldCancel, messageHandler);
  }
};

} // namespace
} // namespace detail

// -----------------------------------------------------------------------------
Result<> ExecuteGradientMagnitudeRecursiveGaussianImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                              float64 sigma, bool normalizeAcrossScale, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::GradientMagnitudeRecursiveGaussianExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteGradientMagnitudeImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, bool useImageSpacing,
                                             const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::GradientMagnitudeExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, spacing, useImageSpacing, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteLaplacianRecursiveGaussianImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath, float64 sigma,
                                                      bool normalizeAcrossScale, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::LaplacianRecursiveGaussianExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteSmoothingRecursiveGaussianImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                      const std::vector<float64>& sigma, bool normalizeAcrossScale, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = SignedScalar;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::SmoothingRecursiveGaussianExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, spacing, sigma, normalizeAcrossScale, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

// -----------------------------------------------------------------------------
Result<> ExecuteDiscreteGaussianImageFilter(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                            const std::vector<float64>& variance, uint32 maximumKernelWidth, const std::vector<float64>& maximumError, bool useImageSpacing,
                                            const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
{
  using TypeSetPolicy = AllNumeric;
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const DataType inputType = inputArray.getDataType();
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const SizeVec3 dims = imageGeom.getDimensions();
  const FloatVec3 spacing = imageGeom.getSpacing();
  detail::DiscreteGaussianExecuteFn executeFn{dataStructure, inputArrayPath, outputArrayPath, dims, spacing, variance, maximumError, maximumKernelWidth, useImageSpacing, shouldCancel, messageHandler};
  return TypeSetPolicy::dispatch(inputType, executeFn);
}

} // namespace nx::core::ImageProcessing
