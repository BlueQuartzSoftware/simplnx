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
  bool full_width = false;
  bool fixed_input = false;
  bool fixed_output = false;
  usize total_values = 0;
  usize core_rows = 0;
  usize core_columns = 0;
  usize maximum_input_rows = 0;
  usize maximum_input_columns = 0;
  usize input_buffer_values = 0;
  usize output_buffer_values = 0;
  usize resident_bytes = 0;
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
  if(!TryMultiplyRadiusOneStencilSize(dimX, dimY, plan.total_values))
  {
    return MakeErrorResult<RadiusOneStencil2DPlan>(-8600, fmt::format("Radius-one 2D stencil dimensions overflow the addressable value count. Dimensions: {} x {}.", dimX, dimY));
  }

  usize fixedOutputPlaneBytes = 0;
  const usize fixedOutputCapacityValues = k_RadiusOneStencil2DFixedPlaneBytes / outputValueBytes;
  usize fixedOutputAllocationBytes = 0;
  usize fixedOutputBaseBytes = 0;
  const bool fixedOutputLayoutValid = TryMultiplyRadiusOneStencilSize(plan.total_values, outputValueBytes, fixedOutputPlaneBytes) &&
                                      TryMultiplyRadiusOneStencilSize(fixedOutputCapacityValues, outputValueBytes, fixedOutputAllocationBytes) &&
                                      TryAddRadiusOneStencilSize(fixedOutputAllocationBytes, workerScratchBytes, fixedOutputBaseBytes);
  if(fixedOutputLayoutValid && preferFixedOutput && targetBytes == k_RadiusOneStencil2DTargetBytes && plan.total_values <= fixedOutputCapacityValues && fixedOutputBaseBytes < targetBytes)
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
      plan.full_width = true;
      plan.core_rows = lower;
      plan.core_columns = dimX;
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
        plan.full_width = false;
        plan.core_rows = 1;
        plan.core_columns = lower;
      }
    }

    if(plan.core_rows != 0)
    {
      plan.fixed_output = true;
      plan.maximum_input_rows = plan.full_width ? AddRadiusOneHalo(plan.core_rows, dimY) : std::min<usize>(3, dimY);
      plan.maximum_input_columns = plan.full_width ? dimX : AddRadiusOneHalo(plan.core_columns, dimX);
      plan.output_buffer_values = fixedOutputCapacityValues;
      usize inputBytes = 0;
      if(!TryMultiplyRadiusOneStencilSize(plan.maximum_input_rows, plan.maximum_input_columns, plan.input_buffer_values) ||
         !TryMultiplyRadiusOneStencilSize(plan.input_buffer_values, inputValueBytes, inputBytes) || !TryAddRadiusOneStencilSize(fixedOutputBaseBytes, inputBytes, plan.resident_bytes))
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
  if(preferFixedInput && targetBytes == k_RadiusOneStencil2DTargetBytes && plan.total_values <= fixedInputCapacityValues && dimX <= fixedInputOutputCapacityValues)
  {
    plan.full_width = true;
    plan.fixed_input = true;
    plan.core_rows = std::min(dimY, fixedInputOutputCapacityValues / dimX);
    plan.core_columns = dimX;
    plan.maximum_input_rows = dimY;
    plan.maximum_input_columns = dimX;
    plan.input_buffer_values = fixedInputCapacityValues;
    if(!TryMultiplyRadiusOneStencilSize(plan.core_rows, dimX, plan.output_buffer_values))
    {
      return MakeErrorResult<RadiusOneStencil2DPlan>(-8600, fmt::format("Radius-one 2D fixed-output layout overflows for dimensions {} x {}.", dimX, dimY));
    }
    usize outputBytes = 0;
    if(!TryMultiplyRadiusOneStencilSize(plan.output_buffer_values, outputValueBytes, outputBytes) || !TryAddRadiusOneStencilSize(fixedNonOutputBytes, outputBytes, plan.resident_bytes))
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
    plan.full_width = true;
    plan.core_rows = lower;
    plan.core_columns = dimX;
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
    plan.full_width = false;
    plan.core_rows = 1;
    plan.core_columns = lower;
  }

  plan.maximum_input_rows = plan.full_width ? AddRadiusOneHalo(plan.core_rows, dimY) : std::min<usize>(3, dimY);
  plan.maximum_input_columns = plan.full_width ? dimX : AddRadiusOneHalo(plan.core_columns, dimX);
  usize bufferBytes = 0;
  if(!TryComputeRadiusOneStencilLayout(plan.maximum_input_rows, plan.maximum_input_columns, plan.core_rows, plan.core_columns, inputValueBytes, outputValueBytes, plan.input_buffer_values,
                                       plan.output_buffer_values, bufferBytes) ||
     !TryAddRadiusOneStencilSize(bufferBytes, workerScratchBytes, plan.resident_bytes))
  {
    return MakeErrorResult<RadiusOneStencil2DPlan>(-8600, fmt::format("Radius-one 2D stencil buffer layout overflows for dimensions {} x {}.", dimX, dimY));
  }
  return {plan};
}

template <class InputT, class OutputT, class BodyFactory>
Result<> ExecuteRadiusOneStencil2D(const AbstractDataStore<InputT>& inputStore, AbstractDataStore<OutputT>& outputStore, const SizeVec3Type& dims, const std::atomic_bool& shouldCancel,
                                   usize targetBytes, usize workerScratchBytes, usize maximumWorkers, BodyFactory&& bodyFactory)
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
  if(inputStore.getSize() != plan.total_values)
  {
    return MakeErrorResult(
        -8602, fmt::format("Radius-one 2D stencil input store size ({}) does not match the expected {} values for dimensions {} x {} x 1.", inputStore.getSize(), plan.total_values, dims[0], dims[1]));
  }
  if(outputStore.getSize() != plan.total_values)
  {
    return MakeErrorResult(-8603, fmt::format("Radius-one 2D stencil output store size ({}) does not match the expected {} values for dimensions {} x {} x 1.", outputStore.getSize(),
                                              plan.total_values, dims[0], dims[1]));
  }
  if(shouldCancel)
  {
    return {};
  }

  auto inputBuffer = std::make_unique<InputT[]>(plan.input_buffer_values);
  auto outputBuffer = std::make_unique<OutputT[]>(plan.output_buffer_values);
  if(plan.fixed_input)
  {
    if(Result<> result = inputStore.copyIntoBuffer(0, nonstd::span<InputT>(inputBuffer.get(), plan.total_values)); result.invalid())
    {
      return result;
    }
  }

  const usize dimX = dims[0];
  const usize dimY = dims[1];
  const usize yStep = plan.full_width ? plan.core_rows : 1;
  const usize xStep = plan.full_width ? dimX : plan.core_columns;
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
      const usize inputXBegin = plan.fixed_input ? 0 : (outputXBegin > 0 ? outputXBegin - 1 : 0);
      const usize inputXEnd = plan.fixed_input ? dimX : outputXEnd + (outputXEnd < dimX ? 1 : 0);
      const usize inputYBegin = plan.fixed_input ? 0 : (outputYBegin > 0 ? outputYBegin - 1 : 0);
      const usize inputYEnd = plan.fixed_input ? dimY : outputYEnd + (outputYEnd < dimY ? 1 : 0);
      const usize inputWidth = inputXEnd - inputXBegin;
      if(!plan.fixed_input)
      {
        if(plan.full_width)
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
      OutputT* blockOutputPtr = plan.fixed_output ? outputBuffer.get() + outputStart : outputBuffer.get();
      const auto body = bodyFactory(inputBuffer.get(), blockOutputPtr, inputXBegin, inputYBegin, inputWidth, outputXBegin, outputYBegin, outputColumns);
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

      if(!plan.fixed_output)
      {
        const usize outputValues = plan.full_width ? outputRows * dimX : outputColumns;
        if(Result<> result = outputStore.copyFromBuffer(outputStart, nonstd::span<const OutputT>(outputBuffer.get(), outputValues)); result.invalid())
        {
          return result;
        }
      }
    }
  }
  if(plan.fixed_output)
  {
    if(shouldCancel)
    {
      return {};
    }
    return outputStore.copyFromBuffer(0, nonstd::span<const OutputT>(outputBuffer.get(), plan.total_values));
  }
  return {};
}
} // namespace nx::core::ImageProcessing::detail
