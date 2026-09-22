#include "simplnx/Utilities/ImageProcessing/RegionalExtremaEngine.hpp"

namespace nx::core::ImageProcessing
{
template <class T>
Result<> RegionalExtremaFlood<T>::operator()()
{
  return (m_Op == RegionalExtremaOp::Maxima) ? runImpl<true>() : runImpl<false>();
}

template <class T>
template <bool Maxima>
Result<> RegionalExtremaFlood<T>::runImpl()
{
  const int64 nX = static_cast<int64>(m_Dims[0]);
  const int64 nY = static_cast<int64>(m_Dims[1]);
  const int64 nZ = static_cast<int64>(m_Dims[2]);
  const usize vol = static_cast<usize>(nX * nY * nZ);
  if(vol == 0)
  {
    return {};
  }
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

  auto input = std::make_unique<T[]>(vol);
  auto work = std::make_unique<T[]>(vol);
  if(Result<> r = m_In.copyIntoBuffer(0, nonstd::span<T>(input.get(), vol)); r.invalid())
  {
    return r;
  }
  std::copy(input.get(), input.get() + vol, work.get());

  const std::vector<detail::ReconOffset> offsets = detail::MakeReconstructionOffsets(m_FullyConnected).all;
  auto flat = [nX, nY](int64 x, int64 y, int64 z) { return static_cast<usize>((z * nY + y) * nX + x); };
  auto inBounds = [nX, nY, nZ](int64 x, int64 y, int64 z) { return x >= 0 && x < nX && y >= 0 && y < nY && z >= 0 && z < nZ; };

  std::vector<usize> stack;
  for(int64 z = 0; z < nZ; ++z)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    for(int64 y = 0; y < nY; ++y)
    {
      for(int64 x = 0; x < nX; ++x)
      {
        const usize p = flat(x, y, z);
        const T v = work[p];
        if(v == markerValue) // ITK's compareOut(V, markerValue): skip already-flooded / marker-valued input
        {
          continue;
        }
        // Does this pixel have a strictly-more-extreme in-bounds input neighbor? Then its whole flat zone is not a
        // regional extremum and must be flooded.
        bool hasBeyond = false;
        for(const detail::ReconOffset& o : offsets)
        {
          if(inBounds(x + o.dx, y + o.dy, z + o.dz) && beyond(input[flat(x + o.dx, y + o.dy, z + o.dz)], v))
          {
            hasBeyond = true;
            break;
          }
        }
        if(!hasBeyond)
        {
          continue;
        }
        // Flood the connected zone of work pixels still equal to v, setting them to the marker value.
        stack.clear();
        stack.push_back(p);
        work[p] = markerValue;
        while(!stack.empty())
        {
          const usize idx = stack.back();
          stack.pop_back();
          const int64 iz = static_cast<int64>(idx) / (nX * nY);
          const int64 rem = static_cast<int64>(idx) - iz * (nX * nY);
          const int64 iy = rem / nX;
          const int64 ix = rem - iy * nX;
          for(const detail::ReconOffset& o : offsets)
          {
            const int64 qx = ix + o.dx;
            const int64 qy = iy + o.dy;
            const int64 qz = iz + o.dz;
            if(inBounds(qx, qy, qz))
            {
              const usize q = flat(qx, qy, qz);
              if(work[q] == v)
              {
                work[q] = markerValue;
                stack.push_back(q);
              }
            }
          }
        }
      }
    }
  }
  return m_Out.copyFromBuffer(0, nonstd::span<const T>(work.get(), vol));
}

template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<int8>;
template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<uint8>;

template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<int16>;
template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<uint16>;

template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<int32>;
template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<uint32>;

template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<int64>;
template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<uint64>;

template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<float32>;
template class SIMPLNX_TEMPLATE_EXPORT RegionalExtremaFlood<float64>;
} // namespace nx::core::ImageProcessing
