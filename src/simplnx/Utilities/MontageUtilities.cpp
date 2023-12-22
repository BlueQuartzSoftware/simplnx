#include "MontageUtilities.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
std::map<int32, std::vector<usize>> MontageUtilities::Burn(int32 tolerance, const std::vector<int32>& input)
{
  int32 halfTol = tolerance / 2;
  usize count = input.size();
  int32 seed = input[0];
  std::vector<bool> visited(input.size(), false);
  std::map<int32, std::vector<usize>> avg_indices;

  bool completed = false;
  while(!completed)
  {
    std::vector<usize> values;
    for(usize i = 0; i < count; i++)
    {
      if(input[i] < seed + halfTol && input[i] > seed - halfTol)
      {
        values.push_back(i);
        visited[i] = true;
      }
    }

    int32 avg = 0;
    for(const auto& v : values)
    {
      avg = avg + input.at(v);
    }
    avg = avg / static_cast<int32>(values.size());
    avg_indices[avg] = values;
    seed = 0;
    completed = true;
    for(usize i = 0; i < count; i++)
    {
      if(!visited[i])
      {
        seed = input[i];
        completed = false;
        break;
      }
    }
  }
  return avg_indices;
}

// -----------------------------------------------------------------------------
int32 MontageUtilities::CalculatePaddingDigits(usize count)
{
  int32 zeroPadding = 0;
  if(count > 0)
  {
    zeroPadding++;
  }
  if(count > 9)
  {
    zeroPadding++;
  }
  if(count > 99)
  {
    zeroPadding++;
  }
  if(count > 999)
  {
    zeroPadding++;
  }
  if(count > 9999)
  {
    zeroPadding++;
  }
  return zeroPadding;
}
