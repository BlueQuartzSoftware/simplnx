#include "simplnx/Utilities/ImageProcessing/RegionalExtremaEngine.hpp"

#include <array>

namespace nx::core::ImageProcessing
{
template <class T>
template <bool Maxima, class WorkStore>
Result<> RegionalExtremaSweep<T>::run2D(WorkStore& workStore, usize dimX, usize dimY, usize maxBufferValues)
{
  const T markerValue = Maxima ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
  auto beyond = [](T left, T right) {
    if constexpr(Maxima)
    {
      return left > right;
    }
    else
    {
      return left < right;
    }
  };
  const std::vector<detail::ReconOffset> offsets = detail::MakeReconstructionOffsets(m_FullyConnected).all;
  auto planResult = detail::CreateSweep2DPlan(dimX, dimY, maxBufferValues, /*fullWidthHaloRows=*/2, /*tiledRows=*/3);
  if(planResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(planResult));
  }
  detail::Sweep2DPlan plan = planResult.value();
  if(plan.fullWidth)
  {
    const std::optional<ShapeType> chunkShape = workStore.getChunkShape();
    if(chunkShape.has_value() && chunkShape->size() >= 3 && (*chunkShape)[0] == 1 && (*chunkShape)[1] > 0 && (*chunkShape)[1] <= plan.coreRows && (*chunkShape)[2] == dimX)
    {
      plan.coreRows = (plan.coreRows / (*chunkShape)[1]) * (*chunkShape)[1];
    }
  }

  if(plan.fullWidth)
  {
    const usize maximumReadRows = std::min(dimY, plan.coreRows + 2);
    const usize maximumReadValues = maximumReadRows * dimX;
    auto inputBuffer = std::make_unique<T[]>(maximumReadValues);
    auto workBuffer = std::make_unique<T[]>(maximumReadValues);

    const usize blockCount = (dimY - 1) / plan.coreRows + 1;
    auto blockBounds = [dimY, &plan](usize blockIndex) {
      const usize coreBegin = blockIndex * plan.coreRows;
      const usize coreRows = std::min(plan.coreRows, dimY - coreBegin);
      const usize coreEnd = coreBegin + coreRows;
      const usize readBegin = coreBegin > 0 ? coreBegin - 1 : coreBegin;
      const usize readEnd = coreEnd < dimY ? coreEnd + 1 : coreEnd;
      return std::array<usize, 4>{coreBegin, coreRows, readBegin, readEnd};
    };

    for(usize blockIndex = 0; blockIndex < blockCount; ++blockIndex)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const auto [coreBegin, coreRows, readBegin, readEnd] = blockBounds(blockIndex);
      const usize readValues = (readEnd - readBegin) * dimX;
      if(Result<> r = m_In.copyIntoBuffer(readBegin * dimX, nonstd::span<T>(inputBuffer.get(), readValues)); r.invalid())
      {
        return r;
      }
      for(usize rowIndex = 0; rowIndex < coreRows; ++rowIndex)
      {
        const usize y = coreBegin + rowIndex;
        const usize localY = y - readBegin;
        const usize rowOffset = localY * dimX;
        for(usize x = 0; x < dimX; ++x)
        {
          const usize position = rowOffset + x;
          const T value = inputBuffer[position];
          bool seed = false;
          if(!m_FullyConnected)
          {
            seed = (y > 0 && beyond(inputBuffer[position - dimX], value)) || (x > 0 && beyond(inputBuffer[position - 1], value)) || (x + 1 < dimX && beyond(inputBuffer[position + 1], value)) ||
                   (y + 1 < dimY && beyond(inputBuffer[position + dimX], value));
          }
          else
          {
            for(const detail::ReconOffset& offset : offsets)
            {
              if(offset.dz != 0)
              {
                continue;
              }
              const int64 neighborX = static_cast<int64>(x) + offset.dx;
              const int64 neighborY = static_cast<int64>(y) + offset.dy;
              if(neighborX < 0 || neighborX >= static_cast<int64>(dimX) || neighborY < 0 || neighborY >= static_cast<int64>(dimY))
              {
                continue;
              }
              const usize neighbor = (static_cast<usize>(neighborY) - readBegin) * dimX + static_cast<usize>(neighborX);
              if(beyond(inputBuffer[neighbor], value))
              {
                seed = true;
                break;
              }
            }
          }
          workBuffer[position] = seed ? markerValue : value;
        }
      }
      const usize coreOffset = (coreBegin - readBegin) * dimX;
      const usize coreValues = coreRows * dimX;
      if(Result<> r = workStore.copyFromBuffer(coreBegin * dimX, nonstd::span<const T>(workBuffer.get() + coreOffset, coreValues)); r.invalid())
      {
        return r;
      }
    }

    bool cacheValid = false;
    usize cachedCoreBegin = 0;
    usize cachedCoreRows = 0;
    usize cachedReadBegin = 0;
    usize cachedReadEnd = 0;
    bool changed = true;
    while(changed)
    {
      changed = false;
      for(int direction = 0; direction < 2; ++direction)
      {
        const bool forward = direction == 0;
        for(usize blockIteration = 0; blockIteration < blockCount; ++blockIteration)
        {
          if(m_ShouldCancel)
          {
            return {};
          }
          const usize blockIndex = forward ? blockIteration : blockCount - 1 - blockIteration;
          const auto [coreBegin, coreRows, requiredReadBegin, requiredReadEnd] = blockBounds(blockIndex);
          const bool reuseCachedBlock = cacheValid && cachedCoreBegin == coreBegin && cachedCoreRows == coreRows && cachedReadBegin <= requiredReadBegin && cachedReadEnd >= requiredReadEnd;
          const usize readBegin = reuseCachedBlock ? cachedReadBegin : requiredReadBegin;
          const usize readEnd = reuseCachedBlock ? cachedReadEnd : requiredReadEnd;
          const usize readValues = (readEnd - readBegin) * dimX;
          const usize readStart = readBegin * dimX;
          if(!reuseCachedBlock)
          {
            if(Result<> r = m_In.copyIntoBuffer(readStart, nonstd::span<T>(inputBuffer.get(), readValues)); r.invalid())
            {
              return r;
            }
            if(Result<> r = workStore.copyIntoBuffer(readStart, nonstd::span<T>(workBuffer.get(), readValues)); r.invalid())
            {
              return r;
            }
          }
          bool blockChanged = false;
          const usize coreEnd = coreBegin + coreRows;
          for(usize rowIndex = 0; rowIndex < coreRows; ++rowIndex)
          {
            const usize y = forward ? coreBegin + rowIndex : coreEnd - 1 - rowIndex;
            const usize localY = y - readBegin;
            const usize rowOffset = localY * dimX;
            for(usize columnIndex = 0; columnIndex < dimX; ++columnIndex)
            {
              const usize x = forward ? columnIndex : dimX - 1 - columnIndex;
              const usize position = rowOffset + x;
              if(workBuffer[position] == markerValue)
              {
                continue;
              }
              const T value = inputBuffer[position];
              auto isMarkedEqual = [&](usize neighbor) { return inputBuffer[neighbor] == value && workBuffer[neighbor] == markerValue; };
              bool mark = false;
              if(!m_FullyConnected)
              {
                mark = (y > 0 && isMarkedEqual(position - dimX)) || (x > 0 && isMarkedEqual(position - 1)) || (x + 1 < dimX && isMarkedEqual(position + 1)) ||
                       (y + 1 < dimY && isMarkedEqual(position + dimX));
              }
              else
              {
                for(const detail::ReconOffset& offset : offsets)
                {
                  if(offset.dz != 0)
                  {
                    continue;
                  }
                  const int64 neighborX = static_cast<int64>(x) + offset.dx;
                  const int64 neighborY = static_cast<int64>(y) + offset.dy;
                  if(neighborX < 0 || neighborX >= static_cast<int64>(dimX) || neighborY < 0 || neighborY >= static_cast<int64>(dimY))
                  {
                    continue;
                  }
                  const usize neighbor = (static_cast<usize>(neighborY) - readBegin) * dimX + static_cast<usize>(neighborX);
                  if(isMarkedEqual(neighbor))
                  {
                    mark = true;
                    break;
                  }
                }
              }
              if(mark)
              {
                workBuffer[position] = markerValue;
                changed = true;
                blockChanged = true;
              }
            }
          }
          if(blockChanged)
          {
            const usize coreOffset = (coreBegin - readBegin) * dimX;
            const usize coreValues = coreRows * dimX;
            if(Result<> r = workStore.copyFromBuffer(coreBegin * dimX, nonstd::span<const T>(workBuffer.get() + coreOffset, coreValues)); r.invalid())
            {
              return r;
            }
          }
          cacheValid = true;
          cachedCoreBegin = coreBegin;
          cachedCoreRows = coreRows;
          cachedReadBegin = readBegin;
          cachedReadEnd = readEnd;
        }
      }
    }
    return {};
  }

  const usize maximumTileColumns = plan.coreColumns + 2;
  auto inputTile = std::make_unique<T[]>(3 * maximumTileColumns);
  auto workTile = std::make_unique<T[]>(3 * maximumTileColumns);

  auto readRectangle = [&](const auto& store, usize yBegin, usize yEnd, usize xBegin, usize xEnd, T* buffer) -> Result<> {
    const usize columns = xEnd - xBegin;
    for(usize y = yBegin; y < yEnd; ++y)
    {
      if(Result<> r = store.copyIntoBuffer(y * dimX + xBegin, nonstd::span<T>(buffer + (y - yBegin) * columns, columns)); r.invalid())
      {
        return r;
      }
    }
    return {};
  };

  for(usize y = 0; y < dimY; ++y)
  {
    for(usize xBegin = 0; xBegin < dimX; xBegin += plan.coreColumns)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize coreColumns = std::min(plan.coreColumns, dimX - xBegin);
      const usize xEnd = xBegin + coreColumns;
      const usize readXBegin = xBegin > 0 ? xBegin - 1 : xBegin;
      const usize readXEnd = xEnd < dimX ? xEnd + 1 : xEnd;
      const usize readYBegin = y > 0 ? y - 1 : y;
      const usize readYEnd = y + 1 < dimY ? y + 2 : y + 1;
      const usize readColumns = readXEnd - readXBegin;
      if(Result<> r = readRectangle(m_In, readYBegin, readYEnd, readXBegin, readXEnd, inputTile.get()); r.invalid())
      {
        return r;
      }
      const usize currentRowOffset = (y - readYBegin) * readColumns;
      const usize coreOffset = currentRowOffset + (xBegin - readXBegin);
      for(usize localX = 0; localX < coreColumns; ++localX)
      {
        const usize x = xBegin + localX;
        const usize position = coreOffset + localX;
        const T value = inputTile[position];
        bool seed = false;
        if(!m_FullyConnected)
        {
          seed = (y > 0 && beyond(inputTile[position - readColumns], value)) || (x > 0 && beyond(inputTile[position - 1], value)) || (x + 1 < dimX && beyond(inputTile[position + 1], value)) ||
                 (y + 1 < dimY && beyond(inputTile[position + readColumns], value));
        }
        else
        {
          for(const detail::ReconOffset& offset : offsets)
          {
            if(offset.dz != 0)
            {
              continue;
            }
            const int64 neighborX = static_cast<int64>(x) + offset.dx;
            const int64 neighborY = static_cast<int64>(y) + offset.dy;
            if(neighborX < 0 || neighborX >= static_cast<int64>(dimX) || neighborY < 0 || neighborY >= static_cast<int64>(dimY))
            {
              continue;
            }
            const usize neighbor = (static_cast<usize>(neighborY) - readYBegin) * readColumns + (static_cast<usize>(neighborX) - readXBegin);
            if(beyond(inputTile[neighbor], value))
            {
              seed = true;
              break;
            }
          }
        }
        workTile[position] = seed ? markerValue : value;
      }
      if(Result<> r = workStore.copyFromBuffer(y * dimX + xBegin, nonstd::span<const T>(workTile.get() + coreOffset, coreColumns)); r.invalid())
      {
        return r;
      }
    }
  }

  bool changed = true;
  while(changed)
  {
    changed = false;
    for(int direction = 0; direction < 2; ++direction)
    {
      const bool forward = direction == 0;
      for(usize rowIndex = 0; rowIndex < dimY; ++rowIndex)
      {
        const usize y = forward ? rowIndex : dimY - 1 - rowIndex;
        for(usize processedColumns = 0; processedColumns < dimX; processedColumns += plan.coreColumns)
        {
          if(m_ShouldCancel)
          {
            return {};
          }
          const usize coreColumns = std::min(plan.coreColumns, dimX - processedColumns);
          const usize xBegin = forward ? processedColumns : dimX - processedColumns - coreColumns;
          const usize xEnd = xBegin + coreColumns;
          const usize readXBegin = xBegin > 0 ? xBegin - 1 : xBegin;
          const usize readXEnd = xEnd < dimX ? xEnd + 1 : xEnd;
          const usize readYBegin = y > 0 ? y - 1 : y;
          const usize readYEnd = y + 1 < dimY ? y + 2 : y + 1;
          const usize readColumns = readXEnd - readXBegin;
          if(Result<> r = readRectangle(m_In, readYBegin, readYEnd, readXBegin, readXEnd, inputTile.get()); r.invalid())
          {
            return r;
          }
          if(Result<> r = readRectangle(workStore, readYBegin, readYEnd, readXBegin, readXEnd, workTile.get()); r.invalid())
          {
            return r;
          }
          const usize currentRowOffset = (y - readYBegin) * readColumns;
          const usize coreOffset = currentRowOffset + (xBegin - readXBegin);
          bool tileChanged = false;
          for(usize columnIndex = 0; columnIndex < coreColumns; ++columnIndex)
          {
            const usize localX = forward ? columnIndex : coreColumns - 1 - columnIndex;
            const usize x = xBegin + localX;
            const usize position = coreOffset + localX;
            if(workTile[position] == markerValue)
            {
              continue;
            }
            const T value = inputTile[position];
            auto isMarkedEqual = [&](usize neighbor) { return inputTile[neighbor] == value && workTile[neighbor] == markerValue; };
            bool mark = false;
            if(!m_FullyConnected)
            {
              mark = (y > 0 && isMarkedEqual(position - readColumns)) || (x > 0 && isMarkedEqual(position - 1)) || (x + 1 < dimX && isMarkedEqual(position + 1)) ||
                     (y + 1 < dimY && isMarkedEqual(position + readColumns));
            }
            else
            {
              for(const detail::ReconOffset& offset : offsets)
              {
                if(offset.dz != 0)
                {
                  continue;
                }
                const int64 neighborX = static_cast<int64>(x) + offset.dx;
                const int64 neighborY = static_cast<int64>(y) + offset.dy;
                if(neighborX < 0 || neighborX >= static_cast<int64>(dimX) || neighborY < 0 || neighborY >= static_cast<int64>(dimY))
                {
                  continue;
                }
                const usize neighbor = (static_cast<usize>(neighborY) - readYBegin) * readColumns + (static_cast<usize>(neighborX) - readXBegin);
                if(isMarkedEqual(neighbor))
                {
                  mark = true;
                  break;
                }
              }
            }
            if(mark)
            {
              workTile[position] = markerValue;
              changed = true;
              tileChanged = true;
            }
          }
          if(tileChanged)
          {
            if(Result<> r = workStore.copyFromBuffer(y * dimX + xBegin, nonstd::span<const T>(workTile.get() + coreOffset, coreColumns)); r.invalid())
            {
              return r;
            }
          }
        }
      }
    }
  }
  return {};
}

template <class T>
Result<> RegionalExtremaSweep<T>::operator()()
{
  return (m_Op == RegionalExtremaOp::Maxima) ? runImpl<true>() : runImpl<false>();
}

template <class T>
template <bool Maxima>
Result<> RegionalExtremaSweep<T>::runImpl()
{
  const usize dimX = m_Dims[0];
  const usize dimY = m_Dims[1];
  const usize dimZ = m_Dims[2];
  if(dimX == 0 || dimY == 0 || dimZ == 0)
  {
    return {};
  }
  constexpr usize k_Int64Max = static_cast<usize>(std::numeric_limits<int64>::max());
  if(dimX > k_Int64Max || dimY > k_Int64Max || dimZ > k_Int64Max)
  {
    return MakeErrorResult(-8660, fmt::format("Regional extrema dimensions must fit signed 64-bit traversal coordinates. Dimensions: {} x {} x {}.", dimX, dimY, dimZ));
  }
  if(dimX > std::numeric_limits<usize>::max() / dimY)
  {
    return MakeErrorResult(-8661, fmt::format("Regional extrema dimensions overflow while computing the XY plane size: {} x {} x {}.", dimX, dimY, dimZ));
  }
  const usize planeSize = dimX * dimY;
  if(planeSize > std::numeric_limits<usize>::max() / dimZ)
  {
    return MakeErrorResult(-8662, fmt::format("Regional extrema dimensions overflow while computing the XYZ volume size: {} x {} x {}.", dimX, dimY, dimZ));
  }
  const usize vol = planeSize * dimZ;
  if(m_In.getSize() != vol || m_Out.getSize() != vol)
  {
    return MakeErrorResult(-8663, fmt::format("Regional extrema store sizes must match the expected image volume ({} values for dimensions {} x {} x {}). Input size: {}; output size: {}.", vol, dimX,
                                              dimY, dimZ, m_In.getSize(), m_Out.getSize()));
  }
  if(dimZ == 1)
  {
    auto fixedPlanResult = detail::CreateSweep2DPlan(dimX, dimY, m_Fixed2DBufferValues, /*fullWidthHaloRows=*/2, /*tiledRows=*/3);
    if(m_Fixed2DWorkValues > 0 && m_Fixed2DBufferValues > 0 && m_In.getStoreType() == IDataStore::StoreType::OutOfCore && vol <= m_Fixed2DWorkValues && fixedPlanResult.valid() &&
       fixedPlanResult.value().fullWidth)
    {
      auto fixedStoreResult = detail::BoundedMemorySweepStore<T>::Create(vol, m_Fixed2DWorkValues);
      if(fixedStoreResult.invalid())
      {
        return ConvertInvalidResult<void>(std::move(fixedStoreResult));
      }
      std::unique_ptr<detail::BoundedMemorySweepStore<T>> fixedStore = std::move(fixedStoreResult.value());
      if(Result<> r = run2D<Maxima>(*fixedStore, dimX, dimY, m_Fixed2DBufferValues); r.invalid())
      {
        return r;
      }
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize maxBatchValues = std::min(vol, m_MaxSlabValues);
      auto buffer = std::make_unique<T[]>(maxBatchValues);
      for(usize start = 0; start < vol; start += maxBatchValues)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize count = std::min(maxBatchValues, vol - start);
        if(Result<> r = fixedStore->copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
        {
          return r;
        }
        if(Result<> r = m_Out.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
        {
          return r;
        }
      }
      return {};
    }
    if(!DataStoreUtilities::GetIOCollection().hasTemporaryRecordStoreCapability())
    {
      return run2D<Maxima>(m_Out, dimX, dimY, m_MaxSlabValues);
    }
    const usize maxBatchValues = std::min(vol, m_MaxSlabValues);
    auto workResult = detail::CreateSweepTemporaryStore<T>(vol, maxBatchValues, m_ShouldCancel, "Regional extrema 2-D work");
    if(workResult.invalid())
    {
      return ConvertInvalidResult<void>(std::move(workResult));
    }
    std::unique_ptr<detail::SweepTemporaryStore<T>> workStore = std::move(workResult.value());
    if(Result<> r = run2D<Maxima>(*workStore, dimX, dimY, m_MaxSlabValues); r.invalid())
    {
      return r;
    }
    if(m_ShouldCancel)
    {
      return {};
    }
    auto buffer = std::make_unique<T[]>(maxBatchValues);
    for(usize start = 0; start < vol; start += maxBatchValues)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const usize count = std::min(maxBatchValues, vol - start);
      if(Result<> r = workStore->copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
      {
        return r;
      }
      if(Result<> r = m_Out.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
      {
        return r;
      }
    }
    return {};
  }
  auto runSweep = [&]<class WorkStore>(WorkStore& workStore) -> Result<> {
    const int64 nX = static_cast<int64>(dimX);
    const int64 nY = static_cast<int64>(dimY);
    const int64 nZ = static_cast<int64>(dimZ);
    const T markerValue = Maxima ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
    auto beyond = [](T a, T b) {
      if constexpr(Maxima)
      {
        return a > b;
      }
      else
      {
        return a < b;
      }
    };
    const std::vector<detail::ReconOffset> offsets = detail::MakeReconstructionOffsets(m_FullyConnected).all;

    // The value budget includes both halo planes whenever possible. At least one core plane must be processed, so
    // exceptionally wide planes can necessarily exceed the nominal target.
    const usize budgetPlanes = std::max<usize>(1, m_MaxSlabValues / planeSize);
    const usize slabPlanes = budgetPlanes >= dimZ ? dimZ : std::min(dimZ, std::max<usize>(1, budgetPlanes > 2 ? budgetPlanes - 2 : 1));
    const usize extraPlanes = std::min<usize>(2, dimZ - slabPlanes);
    const usize slabCapacity = (slabPlanes + extraPlanes) * planeSize;
    auto inSlab = std::make_unique<T[]>(slabCapacity);
    auto outSlab = std::make_unique<T[]>(slabCapacity);

    auto slabBounds = [dimZ, slabPlanes](usize processedPlanes, bool forward) {
      const usize corePlanes = std::min(slabPlanes, dimZ - processedPlanes);
      const usize coreBegin = forward ? processedPlanes : (dimZ - processedPlanes - corePlanes);
      const usize coreEnd = coreBegin + corePlanes;
      const usize readBegin = coreBegin > 0 ? coreBegin - 1 : 0;
      const usize readEnd = coreEnd < dimZ ? coreEnd + 1 : coreEnd;
      return std::array<usize, 4>{coreBegin, corePlanes, readBegin, readEnd};
    };

    // Seeding pass: output = markerValue where the input has a strictly-more-extreme neighbor, else the input value.
    for(usize processedPlanes = 0; processedPlanes < dimZ; processedPlanes += slabPlanes)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      const auto [coreBegin, corePlanes, readBegin, readEnd] = slabBounds(processedPlanes, true);
      const usize readValues = (readEnd - readBegin) * planeSize;
      if(Result<> r = m_In.copyIntoBuffer(readBegin * planeSize, nonstd::span<T>(inSlab.get(), readValues)); r.invalid())
      {
        return r;
      }
      for(usize planeIndex = 0; planeIndex < corePlanes; ++planeIndex)
      {
        if(m_ShouldCancel)
        {
          return {};
        }
        const usize z = coreBegin + planeIndex;
        const usize localZ = z - readBegin;
        const usize planeOffset = localZ * planeSize;
        for(usize y = 0; y < dimY; ++y)
        {
          for(usize x = 0; x < dimX; ++x)
          {
            const usize idx2d = y * dimX + x;
            const usize idx = planeOffset + idx2d;
            const T v = inSlab[idx];
            bool seed = false;
            if(!m_FullyConnected)
            {
              seed = (z > 0 && beyond(inSlab[idx - planeSize], v)) || (y > 0 && beyond(inSlab[idx - dimX], v)) || (x > 0 && beyond(inSlab[idx - 1], v)) ||
                     (x + 1 < dimX && beyond(inSlab[idx + 1], v)) || (y + 1 < dimY && beyond(inSlab[idx + dimX], v)) || (z + 1 < dimZ && beyond(inSlab[idx + planeSize], v));
            }
            else
            {
              for(const detail::ReconOffset& o : offsets)
              {
                const int64 qx = static_cast<int64>(x) + o.dx;
                const int64 qy = static_cast<int64>(y) + o.dy;
                const int64 qz = static_cast<int64>(z) + o.dz;
                if(qx < 0 || qx >= nX || qy < 0 || qy >= nY || qz < 0 || qz >= nZ)
                {
                  continue;
                }
                const usize qLocalZ = static_cast<usize>(qz) - readBegin;
                const usize q = qLocalZ * planeSize + static_cast<usize>(qy) * dimX + static_cast<usize>(qx);
                if(beyond(inSlab[q], v))
                {
                  seed = true;
                  break;
                }
              }
            }
            outSlab[idx] = seed ? markerValue : v;
          }
        }
      }
      const usize coreOffset = (coreBegin - readBegin) * planeSize;
      const usize coreValues = corePlanes * planeSize;
      if(Result<> r = workStore.copyFromBuffer(coreBegin * planeSize, nonstd::span<const T>(outSlab.get() + coreOffset, coreValues)); r.invalid())
      {
        return r;
      }
    }

    // Propagation sweeps: mark a pixel if a same-input-value neighbor is already the marker value; alternate
    // forward/reverse over z until a full pass changes nothing.
    //
    // Per-plane, per-direction convergence tracking: propagation localizes as it converges, so late passes mostly
    // re-touch a small active region while the rest of the volume has already reached its fixpoint. Unlike a
    // forward/backward split over half the neighborhood, this sweep's per-pixel check (isMarkedEqual above) always
    // consults the FULL neighborhood -- both Z neighbors, and (for face connectivity) both in-plane neighbors on
    // each axis -- and only the traversal ORDER differs between forward and reverse. So a forward pass depends on
    // plane z+1 exactly as much as a reverse pass depends on plane z-1: a plane's dependency set is the same three
    // planes {z-1, z, z+1} in both directions, only the intra-plane visitation order flips. planeDirty[z] is true
    // whenever direction d MUST reprocess plane z because its own stored contents, or either Z-neighbor's contents,
    // may have changed since d last visited it; a pass may treat plane z as a proven no-op only while it is false.
    // All flags start dirty so the freshly seeded volume is fully processed on the first pass.
    std::vector<bool> forwardPlaneDirty(dimZ, true);
    std::vector<bool> reversePlaneDirty(dimZ, true);

    bool changed = true;
    while(changed)
    {
      changed = false;
      for(int direction = 0; direction < 2; ++direction) // 0 = forward, 1 = reverse
      {
        const bool forward = direction == 0;
        std::vector<bool>& planeDirty = forward ? forwardPlaneDirty : reversePlaneDirty;
        // Reverse the in-plane (y, x) traversal too on the reverse sweep, not just z: a reverse pass must be a full
        // anti-raster so an equal-input-value zone winding AGAINST the forward raster still propagates a whole pass'
        // worth in one pass. Traversing only z in reverse (y/x always ascending) left such in-plane "snakes"
        // advancing ~1 pixel per pass -> O(L) passes on a pathological zone; full reversal makes it O(1) pass-pairs.
        // This in-plane reversal does not change what a plane's result depends on (still just its own contents,
        // the static input, and its two Z-neighbors' contents): it only reorders visits WITHIN each plane, so the
        // per-plane dirty tracking below stays correct regardless of direction.
        for(usize processedPlanes = 0; processedPlanes < dimZ; processedPlanes += slabPlanes)
        {
          if(m_ShouldCancel)
          {
            return {};
          }
          const auto [coreBegin, corePlanes, readBegin, readEnd] = slabBounds(processedPlanes, forward);

          // A slab is a provable no-op this pass iff every plane it covers is already clean for this direction.
          // Skip its read, compute, and write entirely. No boundary carry needs preserving for a later slab: every
          // slab (skipped or not) reads its own one-plane halo straight from the input and work stores rather than
          // from an in-memory buffer handed off between slabs, so a proven no-op simply leaves those stores exactly
          // as correct as they already were, and whichever slab is processed next reads them itself.
          bool slabClean = true;
          for(usize z = coreBegin; z < coreBegin + corePlanes; ++z)
          {
            if(planeDirty[z])
            {
              slabClean = false;
              break;
            }
          }
          if(slabClean)
          {
            continue;
          }

          const usize readValues = (readEnd - readBegin) * planeSize;
          const usize readStart = readBegin * planeSize;
          if(Result<> r = m_In.copyIntoBuffer(readStart, nonstd::span<T>(inSlab.get(), readValues)); r.invalid())
          {
            return r;
          }
          if(Result<> r = workStore.copyIntoBuffer(readStart, nonstd::span<T>(outSlab.get(), readValues)); r.invalid())
          {
            return r;
          }
          bool slabChanged = false;
          for(usize planeIndex = 0; planeIndex < corePlanes; ++planeIndex)
          {
            if(m_ShouldCancel)
            {
              return {};
            }
            const usize z = forward ? (coreBegin + planeIndex) : (coreBegin + corePlanes - 1 - planeIndex);
            const usize localZ = z - readBegin;
            const usize planeOffset = localZ * planeSize;
            bool planeChanged = false;
            for(usize yi = 0; yi < dimY; ++yi)
            {
              const usize y = forward ? yi : dimY - 1 - yi;
              for(usize xi = 0; xi < dimX; ++xi)
              {
                const usize x = forward ? xi : dimX - 1 - xi;
                const usize idx = planeOffset + y * dimX + x;
                if(outSlab[idx] == markerValue)
                {
                  continue;
                }
                const T v = inSlab[idx];
                bool mark = false;
                auto isMarkedEqual = [&](usize q) { return inSlab[q] == v && outSlab[q] == markerValue; };
                if(!m_FullyConnected)
                {
                  mark = (z > 0 && isMarkedEqual(idx - planeSize)) || (y > 0 && isMarkedEqual(idx - dimX)) || (x > 0 && isMarkedEqual(idx - 1)) || (x + 1 < dimX && isMarkedEqual(idx + 1)) ||
                         (y + 1 < dimY && isMarkedEqual(idx + dimX)) || (z + 1 < dimZ && isMarkedEqual(idx + planeSize));
                }
                else
                {
                  for(const detail::ReconOffset& o : offsets)
                  {
                    const int64 qx = static_cast<int64>(x) + o.dx;
                    const int64 qy = static_cast<int64>(y) + o.dy;
                    const int64 qz = static_cast<int64>(z) + o.dz;
                    if(qx < 0 || qx >= nX || qy < 0 || qy >= nY || qz < 0 || qz >= nZ)
                    {
                      continue;
                    }
                    const usize qLocalZ = static_cast<usize>(qz) - readBegin;
                    const usize q = qLocalZ * planeSize + static_cast<usize>(qy) * dimX + static_cast<usize>(qx);
                    if(isMarkedEqual(q))
                    {
                      mark = true;
                      break;
                    }
                  }
                }
                if(mark)
                {
                  outSlab[idx] = markerValue;
                  changed = true;
                  slabChanged = true;
                  planeChanged = true;
                }
              }
            }
            if(planeChanged)
            {
              // This plane's contents changed. Both directions read both Z-neighbors (only the traversal order
              // differs), so a future visit to z-1 or z+1 in EITHER direction may now see a different outcome --
              // dirty both neighbors in both direction arrays, plus plane z itself since the carry it used to
              // reach this value may not yet be final.
              forwardPlaneDirty[z] = true;
              reversePlaneDirty[z] = true;
              if(z > 0)
              {
                forwardPlaneDirty[z - 1] = true;
                reversePlaneDirty[z - 1] = true;
              }
              if(z + 1 < dimZ)
              {
                forwardPlaneDirty[z + 1] = true;
                reversePlaneDirty[z + 1] = true;
              }
            }
            else
            {
              // Nothing changed under the neighbor contents this direction just read, so this direction can trust
              // the plane unchanged until something invalidates it again.
              planeDirty[z] = false;
            }
          }
          if(slabChanged)
          {
            const usize coreOffset = (coreBegin - readBegin) * planeSize;
            const usize coreValues = corePlanes * planeSize;
            if(Result<> r = workStore.copyFromBuffer(coreBegin * planeSize, nonstd::span<const T>(outSlab.get() + coreOffset, coreValues)); r.invalid())
            {
              return r;
            }
          }
        }
      }
    }
    return {};
  };

  if(dimZ == 1 || !DataStoreUtilities::GetIOCollection().hasTemporaryRecordStoreCapability())
  {
    return runSweep(m_Out);
  }

  const usize maxBatchValues = std::min(vol, m_MaxSlabValues);
  auto workResult = detail::CreateSweepTemporaryStore<T>(vol, maxBatchValues, m_ShouldCancel, "Regional extrema work");
  if(workResult.invalid())
  {
    return ConvertInvalidResult<void>(std::move(workResult));
  }
  std::unique_ptr<detail::SweepTemporaryStore<T>> workStore = std::move(workResult.value());
  if(Result<> r = runSweep(*workStore); r.invalid())
  {
    return r;
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  auto buffer = std::make_unique<T[]>(maxBatchValues);
  for(usize start = 0; start < vol; start += maxBatchValues)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize count = std::min(maxBatchValues, vol - start);
    if(Result<> r = workStore->copyIntoBuffer(start, nonstd::span<T>(buffer.get(), count)); r.invalid())
    {
      return r;
    }
    if(Result<> r = m_Out.copyFromBuffer(start, nonstd::span<const T>(buffer.get(), count)); r.invalid())
    {
      return r;
    }
  }
  return {};
}

template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<int8>;
template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<uint8>;

template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<int16>;
template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<uint16>;

template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<int32>;
template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<uint32>;

template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<int64>;
template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<uint64>;

template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<float32>;
template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaSweep<float64>;
} // namespace nx::core::ImageProcessing
