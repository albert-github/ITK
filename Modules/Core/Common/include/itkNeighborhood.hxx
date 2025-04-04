/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkNeighborhood_hxx
#define itkNeighborhood_hxx

#include "itkNumericTraits.h"

namespace itk
{
template <typename TPixel, unsigned int VDimension, typename TContainer>
void
Neighborhood<TPixel, VDimension, TContainer>::ComputeNeighborhoodStrideTable()
{
  OffsetValueType accum = 1;

  for (DimensionValueType dim = 0; dim < VDimension; ++dim)
  {
    m_StrideTable[dim] = accum;
    accum *= m_Size[dim];
  }
}

template <typename TPixel, unsigned int VDimension, typename TContainer>
void
Neighborhood<TPixel, VDimension, TContainer>::ComputeNeighborhoodOffsetTable()
{
  m_OffsetTable.clear();
  m_OffsetTable.reserve(this->Size());

  OffsetType o;
  for (DimensionValueType j = 0; j < VDimension; ++j)
  {
    o[j] = -(static_cast<OffsetValueType>(this->GetRadius(j)));
  }

  for (DimensionValueType i = 0; i < this->Size(); ++i)
  {
    m_OffsetTable.push_back(o);
    for (DimensionValueType j = 0; j < VDimension; ++j)
    {
      o[j] = o[j] + 1;
      if (o[j] > static_cast<OffsetValueType>(this->GetRadius(j)))
      {
        o[j] = -(static_cast<OffsetValueType>(this->GetRadius(j)));
      }
      else
      {
        break;
      }
    }
  }
}

template <typename TPixel, unsigned int VDimension, typename TContainer>
void
Neighborhood<TPixel, VDimension, TContainer>::SetRadius(const SizeValueType s)
{
  auto k = MakeFilled<SizeType>(s);
  this->SetRadius(k);
}

template <typename TPixel, unsigned int VDimension, typename TContainer>
void
Neighborhood<TPixel, VDimension, TContainer>::SetRadius(const SizeType & r)
{
  this->m_Radius = r;
  this->SetSize();
  this->Allocate(m_Size.CalculateProductOfElements());
  this->ComputeNeighborhoodStrideTable();
  this->ComputeNeighborhoodOffsetTable();
}

template <typename TPixel, unsigned int VDimension, typename TContainer>
std::slice
Neighborhood<TPixel, VDimension, TContainer>::GetSlice(unsigned int d) const
{
  const OffsetValueType t = this->GetStride(d);
  const auto            s = static_cast<OffsetValueType>(this->GetSize()[d]);

  OffsetValueType n = this->Size() / 2;
  n -= t * s / 2;

  return { static_cast<size_t>(n), static_cast<size_t>(s), static_cast<size_t>(t) };
}

template <typename TPixel, unsigned int VDimension, typename TContainer>
auto
Neighborhood<TPixel, VDimension, TContainer>::GetNeighborhoodIndex(const OffsetType & o) const -> NeighborIndexType
{
  unsigned int idx = (this->Size() / 2);

  for (unsigned int i = 0; i < VDimension; ++i)
  {
    idx += o[i] * m_StrideTable[i];
  }
  return idx;
}

template <typename TPixel, unsigned int VDimension, typename TContainer>
void
Neighborhood<TPixel, VDimension, TContainer>::PrintSelf(std::ostream & os, Indent indent) const
{
  os << indent << "Size: " << static_cast<typename NumericTraits<SizeType>::PrintType>(m_Size) << std::endl;
  os << indent << "Radius: " << static_cast<typename NumericTraits<SizeType>::PrintType>(m_Radius) << std::endl;

  os << indent << "StrideTable: [ ";
  for (DimensionValueType i = 0; i < VDimension; ++i)
  {
    os << indent.GetNextIndent() << m_StrideTable[i] << ' ';
  }
  os << ']' << std::endl;

  os << indent << "OffsetTable: [ ";
  for (DimensionValueType i = 0; i < m_OffsetTable.size(); ++i)
  {
    os << indent.GetNextIndent() << m_OffsetTable[i] << ' ';
  }
  os << ']' << std::endl;
}
} // namespace itk

#endif
