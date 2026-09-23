#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <limits>
#include <memory>
#include <utility>

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/task_arena.h>
#endif

namespace nx::core::ImageProcessing::detail
{
inline constexpr usize k_RadiusOneStencil2DTargetBytes = 64ULL * 1024ULL * 1024ULL;
inline constexpr usize k_RadiusOneStencil2DFixedInputBytes = 48ULL * 1024ULL * 1024ULL;
inline constexpr usize k_RadiusOneStencil2DFixedOutputBytes = 16ULL * 1024ULL * 1024ULL;
inline constexpr usize k_RadiusOneStencil2DFixedPlaneBytes = 48ULL * 1024ULL * 1024ULL;

struct RadiusOneStencil2DPlan
{
  bool fullWidth = false;
  bool fixedInput = false;
  bool fixedOutput = false;
  usize totalValues = 0;
  usize coreRows = 0;
  usize coreColumns = 0;
  usize maximumInputRows = 0;
  usize maximumInputColumns = 0;
  usize inputBufferValues = 0;
  usize outputBufferValues = 0;
  usize residentBytes = 0;
};

inline bool TryMultiplyRadiusOneStencilSize(usize left, usize right, usize& product)
{
  if(left != 0 && right > std::numeric_limits<usize>::max() / left)
  {
    return false;
  }
  product = left * right;
  return true;
}

inline bool TryAddRadiusOneStencilSize(usize left, usize right, usize& sum)
{
  if(right > std::numeric_limits<usize>::max() - left)
  {
    return false;
  }
  sum = left + right;
  return true;
}

inline usize AddRadiusOneHalo(usize coreSize, usize dimension)
{
  return coreSize + std::min<usize>(2, dimension - coreSize);
}

inline bool TryComputeRadiusOneStencilLayout(usize inputRows, usize inputColumns, usize outputRows, usize outputColumns, usize inputValueBytes, usize outputValueBytes, usize& inputValues,
                                             usize& outputValues, usize& residentBytes)
{
  usize inputBytes = 0;
  usize outputBytes = 0;
  return TryMultiplyRadiusOneStencilSize(inputRows, inputColumns, inputValues) && TryMultiplyRadiusOneStencilSize(outputRows, outputColumns, outputValues) &&
         TryMultiplyRadiusOneStencilSize(inputValues, inputValueBytes, inputBytes) && TryMultiplyRadiusOneStencilSize(outputValues, outputValueBytes, outputBytes) &&
         TryAddRadiusOneStencilSize(inputBytes, outputBytes, residentBytes);
}

inline Result<RadiusOneStencil2DPlan> CreateRadiusOneStencil2DPlan(usize dimX, usize dimY, usize inputValueBytes, usize outputValueBytes, usize targetBytes, bool preferFixedInput,
                                                                   bool preferFixedOutput = false, usize workerScratchBytes = 0)
{
  if(dimX == 0 || dimY == 0 || inputValueBytes == 0 || outputValueBytes == 0)
  {
    return MakeErrorResult<RadiusOneStencil2DPlan>(
        -8600, fmt::format("Radius-one 2D stencil dimensions and value sizes must be nonzero. Dimensions: {} x {}; input value size: {} bytes; output value size: {} bytes.", dimX, dimY,
                           inputValueBytes, outputValueBytes));
  }
  constexpr usize k_Int64Max = static_cast<usize>(std::numeric_limits<int64>::max());
  if(dimX > k_Int64Max || dimY > k_Int64Max)
  {
    return MakeErrorResult<RadiusOneStencil2DPlan>(-8600, fmt::format("Radius-one 2D stencil dimensions exceed the supported signed coordinate range. Dimensions: {} x {}.", dimX, dimY));
  }

  RadiusOneStencil2DPlan plan;
  if(!TryMultiplyRadiusOneStencilSize(dimX, dimY, plan.totalValues))
  {
    return MakeErrorResult<RadiusOneStencil2DPlan>(-8600, fmt::format("Radius-one 2D stencil dimensions overflow the addressable value count. Dimensions: {} x {}.", dimX, dimY));
  }

  usize fixedOutputPlaneBytes = 0;
  const usize fixedOutputCapacityValues = k_RadiusOneStencil2DFixedPlaneBytes / outputValueBytes;
  usize fixedOutputAllocationBytes = 0;
  usize fixedOutputBaseBytes = 0;
  const bool fixedOutputLayoutValid = TryMultiplyRadiusOneStencilSize(plan.totalValues, outputValueBytes, fixedOutputPlaneBytes) &&
                                      TryMultiplyRadiusOneStencilSize(fixedOutputCapacityValues, outputValueBytes, fixedOutputAllocationBytes) &&
                                      TryAddRadiusOneStencilSize(fixedOutputAllocationBytes, workerScratchBytes, fixedOutputBaseBytes);
  if(fixedOutputLayoutValid && preferFixedOutput && targetBytes == k_RadiusOneStencil2DTargetBytes && plan.totalValues <= fixedOutputCapacityValues && fixedOutputBaseBytes < targetBytes)
  {
    auto fullWidthInputFits = [&](usize coreRows) {
      usize inputValues = 0;
      usize inputBytes = 0;
      usize residentBytes = 0;
      return TryMultiplyRadiusOneStencilSize(AddRadiusOneHalo(coreRows, dimY), dimX, inputValues) && TryMultiplyRadiusOneStencilSize(inputValues, inputValueBytes, inputBytes) &&
             TryAddRadiusOneStencilSize(fixedOutputBaseBytes, inputBytes, residentBytes) && residentBytes <= targetBytes;
    };
    if(fullWidthInputFits(1))
    {
      usize lower = 1;
      usize upper = dimY;
      while(lower < upper)
      {
        const usize midpoint = lower + (upper - lower) / 2 + 1;
        if(fullWidthInputFits(midpoint))
        {
          lower = midpoint;
        }
        else
        {
          upper = midpoint - 1;
        }
      }
      plan.fullWidth = true;
      plan.coreRows = lower;
      plan.coreColumns = dimX;
    }
    else
    {
      const usize inputRows = std::min<usize>(3, dimY);
      auto tiledInputFits = [&](usize coreColumns) {
        usize inputValues = 0;
        usize inputBytes = 0;
        usize residentBytes = 0;
        return TryMultiplyRadiusOneStencilSize(inputRows, AddRadiusOneHalo(coreColumns, dimX), inputValues) && TryMultiplyRadiusOneStencilSize(inputValues, inputValueBytes, inputBytes) &&
               TryAddRadiusOneStencilSize(fixedOutputBaseBytes, inputBytes, residentBytes) && residentBytes <= targetBytes;
      };
      if(tiledInputFits(1))
      {
        usize lower = 1;
        usize upper = dimX;
        while(lower < upper)
        {
          const usize midpoint = lower + (upper - lower) / 2 + 1;
          if(tiledInputFits(midpoint))
          {
            lower = midpoint;
          }
          else
          {
            upper = midpoint - 1;
          }
        }
        plan.fullWidth = false;
        plan.coreRows = 1;
        plan.coreColumns = lower;
      }
    }

    if(plan.coreRows != 0)
    {
      plan.fixedOutput = true;
      plan.maximumInputRows = plan.fullWidth ? AddRadiusOneHalo(plan.coreRows, dimY) : std::min<usize>(3, dimY);
      plan.maximumInputColumns = plan.fullWidth ? dimX : AddRadiusOneHalo(plan.coreColumns, dimX);
      plan.outputBufferValues = fixedOutputCapacityValues;
      usize inputBytes = 0;
      if(!TryMultiplyRadiusOneStencilSize(plan.maximumInputRows, plan.maximumInputColumns, plan.inputBufferValues) ||
         !TryMultiplyRadiusOneStencilSize(plan.inputBufferValues, inputValueBytes, inputBytes) || !TryAddRadiusOneStencilSize(fixedOutputBaseBytes, inputBytes, plan.residentBytes))
      {
        return MakeErrorResult<RadiusOneStencil2DPlan>(-8600, fmt::format("Radius-one 2D fixed-output buffer layout overflows for dimensions {} x {}.", dimX, dimY));
      }
      return {plan};
    }
  }

  const usize fixedInputCapacityValues = k_RadiusOneStencil2DFixedInputBytes / inputValueBytes;
  usize fixedInputAllocationBytes = 0;
  usize fixedNonOutputBytes = 0;
  if(!TryMultiplyRadiusOneStencilSize(fixedInputCapacityValues, inputValueBytes, fixedInputAllocationBytes) ||
     !TryAddRadiusOneStencilSize(fixedInputAllocationBytes, workerScratchBytes, fixedNonOutputBytes))
  {
    return MakeErrorResult<RadiusOneStencil2DPlan>(-8600, fmt::format("Radius-one 2D fixed-input byte layout overflows for dimensions {} x {}.", dimX, dimY));
  }
  const usize fixedOutputBytes = fixedNonOutputBytes <= targetBytes ? std::min(k_RadiusOneStencil2DFixedOutputBytes, targetBytes - fixedNonOutputBytes) : 0;
  const usize fixedInputOutputCapacityValues = fixedOutputBytes / outputValueBytes;
  if(preferFixedInput && targetBytes == k_RadiusOneStencil2DTargetBytes && plan.totalValues <= fixedInputCapacityValues && dimX <= fixedInputOutputCapacityValues)
  {
    plan.fullWidth = true;
    plan.fixedInput = true;
    plan.coreRows = std::min(dimY, fixedInputOutputCapacityValues / dimX);
    plan.coreColumns = dimX;
    plan.maximumInputRows = dimY;
    plan.maximumInputColumns = dimX;
    plan.inputBufferValues = fixedInputCapacityValues;
    if(!TryMultiplyRadiusOneStencilSize(plan.coreRows, dimX, plan.outputBufferValues))
    {
      return MakeErrorResult<RadiusOneStencil2DPlan>(-8600, fmt::format("Radius-one 2D fixed-output layout overflows for dimensions {} x {}.", dimX, dimY));
    }
    usize outputBytes = 0;
    if(!TryMultiplyRadiusOneStencilSize(plan.outputBufferValues, outputValueBytes, outputBytes) || !TryAddRadiusOneStencilSize(fixedNonOutputBytes, outputBytes, plan.residentBytes))
    {
      return MakeErrorResult<RadiusOneStencil2DPlan>(-8600, fmt::format("Radius-one 2D fixed-capacity byte layout overflows for dimensions {} x {}.", dimX, dimY));
    }
    return {plan};
  }

  auto fullWidthFits = [&](usize coreRows) {
    usize inputValues = 0;
    usize outputValues = 0;
    usize bufferBytes = 0;
    usize residentBytes = 0;
    return TryComputeRadiusOneStencilLayout(AddRadiusOneHalo(coreRows, dimY), dimX, coreRows, dimX, inputValueBytes, outputValueBytes, inputValues, outputValues, bufferBytes) &&
           TryAddRadiusOneStencilSize(bufferBytes, workerScratchBytes, residentBytes) && residentBytes <= targetBytes;
  };
  if(fullWidthFits(1))
  {
    usize lower = 1;
    usize upper = dimY;
    while(lower < upper)
    {
      const usize midpoint = lower + (upper - lower) / 2 + 1;
      if(fullWidthFits(midpoint))
      {
        lower = midpoint;
      }
      else
      {
        upper = midpoint - 1;
      }
    }
    plan.fullWidth = true;
    plan.coreRows = lower;
    plan.coreColumns = dimX;
  }
  else
  {
    const usize inputRows = std::min<usize>(3, dimY);
    auto tileFits = [&](usize coreColumns) {
      usize inputValues = 0;
      usize outputValues = 0;
      usize bufferBytes = 0;
      usize residentBytes = 0;
      return TryComputeRadiusOneStencilLayout(inputRows, AddRadiusOneHalo(coreColumns, dimX), 1, coreColumns, inputValueBytes, outputValueBytes, inputValues, outputValues, bufferBytes) &&
             TryAddRadiusOneStencilSize(bufferBytes, workerScratchBytes, residentBytes) && residentBytes <= targetBytes;
    };
    if(!tileFits(1))
    {
      return MakeErrorResult<RadiusOneStencil2DPlan>(
          -8601,
          fmt::format("Radius-one 2D stencil target ({} bytes) cannot hold one output value and its clipped halo for dimensions {} x {}, input value size {} bytes, and output value size {} bytes.",
                      targetBytes, dimX, dimY, inputValueBytes, outputValueBytes));
    }
    usize lower = 1;
    usize upper = dimX;
    while(lower < upper)
    {
      const usize midpoint = lower + (upper - lower) / 2 + 1;
      if(tileFits(midpoint))
      {
        lower = midpoint;
      }
      else
      {
        upper = midpoint - 1;
      }
    }
    plan.fullWidth = false;
    plan.coreRows = 1;
    plan.coreColumns = lower;
  }

  plan.maximumInputRows = plan.fullWidth ? AddRadiusOneHalo(plan.coreRows, dimY) : std::min<usize>(3, dimY);
  plan.maximumInputColumns = plan.fullWidth ? dimX : AddRadiusOneHalo(plan.coreColumns, dimX);
  usize bufferBytes = 0;
  if(!TryComputeRadiusOneStencilLayout(plan.maximumInputRows, plan.maximumInputColumns, plan.coreRows, plan.coreColumns, inputValueBytes, outputValueBytes, plan.inputBufferValues,
                                       plan.outputBufferValues, bufferBytes) ||
     !TryAddRadiusOneStencilSize(bufferBytes, workerScratchBytes, plan.residentBytes))
  {
    return MakeErrorResult<RadiusOneStencil2DPlan>(-8600, fmt::format("Radius-one 2D stencil buffer layout overflows for dimensions {} x {}.", dimX, dimY));
  }
  return {plan};
}

template <class InputT, class OutputT, class BodyFactory>
Result<> ExecuteRadiusOneStencil2D(const AbstractDataStore<InputT>& inputStore, AbstractDataStore<OutputT>& outputStore, const SizeVec3& dims, const std::atomic_bool& shouldCancel, usize targetBytes,
                                   usize workerScratchBytes, usize maximumWorkers, BodyFactory&& bodyFactory)
{
  if(dims[2] != 1)
  {
    return MakeErrorResult(-8600, fmt::format("Radius-one 2D stencil execution requires exactly one Z slice. Dimensions: {} x {} x {}.", dims[0], dims[1], dims[2]));
  }
  const bool preferFixedInput = inputStore.getStoreType() == IDataStore::StoreType::OutOfCore && targetBytes == k_RadiusOneStencil2DTargetBytes;
  const bool preferFixedOutput = outputStore.getStoreType() == IDataStore::StoreType::OutOfCore && targetBytes == k_RadiusOneStencil2DTargetBytes;
  auto planResult = CreateRadiusOneStencil2DPlan(dims[0], dims[1], sizeof(InputT), sizeof(OutputT), targetBytes, preferFixedInput, preferFixedOutput, workerScratchBytes);
  if(planResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(planResult));
  }
  const RadiusOneStencil2DPlan plan = planResult.value();
  if(inputStore.getSize() != plan.totalValues)
  {
    return MakeErrorResult(
        -8602, fmt::format("Radius-one 2D stencil input store size ({}) does not match the expected {} values for dimensions {} x {} x 1.", inputStore.getSize(), plan.totalValues, dims[0], dims[1]));
  }
  if(outputStore.getSize() != plan.totalValues)
  {
    return MakeErrorResult(-8603, fmt::format("Radius-one 2D stencil output store size ({}) does not match the expected {} values for dimensions {} x {} x 1.", outputStore.getSize(), plan.totalValues,
                                              dims[0], dims[1]));
  }
  if(shouldCancel)
  {
    return {};
  }

  auto inputBuffer = std::make_unique<InputT[]>(plan.inputBufferValues);
  auto outputBuffer = std::make_unique<OutputT[]>(plan.outputBufferValues);
  if(plan.fixedInput)
  {
    if(Result<> result = inputStore.copyIntoBuffer(0, nonstd::span<InputT>(inputBuffer.get(), plan.totalValues)); result.invalid())
    {
      return result;
    }
  }

  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize yStep = plan.fullWidth ? plan.coreRows : 1;
  const usize xStep = plan.fullWidth ? dimX : plan.coreColumns;
  for(usize outputYBegin = 0; outputYBegin < dimY; outputYBegin += yStep)
  {
    const usize outputRows = std::min(yStep, dimY - outputYBegin);
    for(usize outputXBegin = 0; outputXBegin < dimX; outputXBegin += xStep)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize outputColumns = std::min(xStep, dimX - outputXBegin);
      const usize outputXEnd = outputXBegin + outputColumns;
      const usize outputYEnd = outputYBegin + outputRows;
      const usize inputXBegin = plan.fixedInput ? 0 : (outputXBegin > 0 ? outputXBegin - 1 : 0);
      const usize inputXEnd = plan.fixedInput ? dimX : outputXEnd + (outputXEnd < dimX ? 1 : 0);
      const usize inputYBegin = plan.fixedInput ? 0 : (outputYBegin > 0 ? outputYBegin - 1 : 0);
      const usize inputYEnd = plan.fixedInput ? dimY : outputYEnd + (outputYEnd < dimY ? 1 : 0);
      const usize inputWidth = inputXEnd - inputXBegin;
      if(!plan.fixedInput)
      {
        if(plan.fullWidth)
        {
          const usize inputValues = (inputYEnd - inputYBegin) * dimX;
          if(Result<> result = inputStore.copyIntoBuffer(inputYBegin * dimX, nonstd::span<InputT>(inputBuffer.get(), inputValues)); result.invalid())
          {
            return result;
          }
        }
        else
        {
          for(usize inputY = inputYBegin; inputY < inputYEnd; ++inputY)
          {
            if(Result<> result = inputStore.copyIntoBuffer(inputY * dimX + inputXBegin, nonstd::span<InputT>(inputBuffer.get() + (inputY - inputYBegin) * inputWidth, inputWidth)); result.invalid())
            {
              return result;
            }
          }
        }
      }

      ParallelDataAlgorithm parallelAlgorithm;
      parallelAlgorithm.setRange(0, outputRows * outputColumns);
      const usize outputStart = outputYBegin * dimX + outputXBegin;
      OutputT* blockOutput = plan.fixedOutput ? outputBuffer.get() + outputStart : outputBuffer.get();
      const auto body = bodyFactory(inputBuffer.get(), blockOutput, inputXBegin, inputYBegin, inputWidth, outputXBegin, outputYBegin, outputColumns);
#ifdef SIMPLNX_ENABLE_MULTICORE
      if(maximumWorkers > 0)
      {
        const usize boundedWorkers = std::min(maximumWorkers, static_cast<usize>(std::numeric_limits<int>::max()));
        tbb::task_arena arena(static_cast<int>(boundedWorkers));
        arena.execute([&] { parallelAlgorithm.execute(body); });
      }
      else
#endif
      {
        parallelAlgorithm.execute(body);
      }

      if(!plan.fixedOutput)
      {
        const usize outputValues = plan.fullWidth ? outputRows * dimX : outputColumns;
        if(Result<> result = outputStore.copyFromBuffer(outputStart, nonstd::span<const OutputT>(outputBuffer.get(), outputValues)); result.invalid())
        {
          return result;
        }
      }
    }
  }
  if(plan.fixedOutput)
  {
    if(shouldCancel)
    {
      return {};
    }
    return outputStore.copyFromBuffer(0, nonstd::span<const OutputT>(outputBuffer.get(), plan.totalValues));
  }
  return {};
}
} // namespace nx::core::ImageProcessing::detail
