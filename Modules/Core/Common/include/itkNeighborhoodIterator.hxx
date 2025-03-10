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
#ifndef itkNeighborhoodIterator_hxx
#define itkNeighborhoodIterator_hxx


namespace itk
{

template <typename TImage, typename TBoundaryCondition>
void
NeighborhoodIterator<TImage, TBoundaryCondition>::SetPixel(const unsigned int n, const PixelType & v)
{

  if (this->m_NeedToUseBoundaryCondition == false)
  {
    this->m_NeighborhoodAccessorFunctor.Set(this->operator[](n), v);
  }
  // Is this whole neighborhood in bounds?
  else if (this->InBounds())
  {
    this->m_NeighborhoodAccessorFunctor.Set(this->operator[](n), v);
  }
  else
  {
    OffsetType temp = this->ComputeInternalIndex(n);
    OffsetType OverlapLow;
    OffsetType OverlapHigh;
    OffsetType offset;

    // Calculate overlap
    for (unsigned int ii = 0; ii < Superclass::Dimension; ++ii)
    {
      OverlapLow[ii] = this->m_InnerBoundsLow[ii] - this->m_Loop[ii];
      OverlapHigh[ii] =
        static_cast<OffsetValueType>(this->GetSize(ii) - ((this->m_Loop[ii] + 2) - this->m_InnerBoundsHigh[ii]));
    }

    bool flag = true;

    // Is this pixel in bounds?
    for (unsigned int ii = 0; ii < Superclass::Dimension; ++ii)
    {
      if (this->m_InBounds[ii])
      {
        offset[ii] = 0; // this dimension in bounds
      }
      else // part of this dimension spills out of bounds
      {
        if (temp[ii] < OverlapLow[ii])
        {
          flag = false;
          offset[ii] = OverlapLow[ii] - temp[ii];
        }
        else if (OverlapHigh[ii] < temp[ii])
        {
          flag = false;
          offset[ii] = OverlapHigh[ii] - temp[ii];
        }
        else
        {
          offset[ii] = 0;
        }
      }
    }

    if (flag)
    {
      this->m_NeighborhoodAccessorFunctor.Set(this->operator[](n), v);
    }
    else
    { // Attempt to write out of bounds
      RangeError e(__FILE__, __LINE__);
      e.SetLocation(ITK_LOCATION);
      e.SetDescription("Attempt to write out of bounds.");
      throw e;
    }
  }
}

template <typename TImage, typename TBoundaryCondition>
void
NeighborhoodIterator<TImage, TBoundaryCondition>::SetPixel(const unsigned int n, const PixelType & v, bool & status)
{


  if (this->m_NeedToUseBoundaryCondition == false)
  {
    status = true;
    this->m_NeighborhoodAccessorFunctor.Set(this->operator[](n), v);
  }

  // Is this whole neighborhood in bounds?
  else if (this->InBounds())
  {
    this->m_NeighborhoodAccessorFunctor.Set(this->operator[](n), v);
    status = true;
    return;
  }
  else
  {
    OffsetType temp = this->ComputeInternalIndex(n);

    // Calculate overlap.
    // Here, we are checking whether the particular pixel in the
    // neighborhood is within the bounds (when the neighborhood is not
    // completely in bounds, it is usually partly in bounds)
    for (unsigned int i = 0; i < Superclass::Dimension; ++i)
    {
      if (!this->m_InBounds[i]) // Part of dimension spills out of bounds
      {
        const typename OffsetType::OffsetValueType OverlapLow = this->m_InnerBoundsLow[i] - this->m_Loop[i];
        const auto                                 OverlapHigh =
          static_cast<OffsetValueType>(this->GetSize(i) - ((this->m_Loop[i] + 2) - this->m_InnerBoundsHigh[i]));
        if (temp[i] < OverlapLow || OverlapHigh < temp[i])
        {
          status = false;
          return;
        }
      }
    }

    this->m_NeighborhoodAccessorFunctor.Set(this->operator[](n), v);
    status = true;
  }
}

template <typename TImage, typename TBoundaryCondition>
void
NeighborhoodIterator<TImage, TBoundaryCondition>::SetNeighborhood(const NeighborhoodType & N)
{
  const Iterator _end = this->End();

  typename NeighborhoodType::ConstIterator N_it;

  if (this->m_NeedToUseBoundaryCondition == false)
  {
    Iterator this_it = this->Begin();
    for (N_it = N.Begin(); this_it < _end; ++this_it, ++N_it)
    {
      this->m_NeighborhoodAccessorFunctor.Set(*this_it, *N_it);
    }
  }
  else if (this->InBounds())
  {
    Iterator this_it = this->Begin();
    for (N_it = N.Begin(); this_it < _end; ++this_it, ++N_it)
    {
      this->m_NeighborhoodAccessorFunctor.Set(*this_it, *N_it);
    }
  }
  else
  {
    // Calculate overlap & initialize index
    OffsetType OverlapLow;
    OffsetType OverlapHigh;
    OffsetType temp{ 0 };
    for (unsigned int i = 0; i < Superclass::Dimension; ++i)
    {
      OverlapLow[i] = this->m_InnerBoundsLow[i] - this->m_Loop[i];
      OverlapHigh[i] =
        static_cast<OffsetValueType>(this->GetSize(i) - (this->m_Loop[i] - this->m_InnerBoundsHigh[i]) - 1);
    }

    // Iterate through neighborhood
    Iterator this_it = this->Begin();
    for (N_it = N.Begin(); this_it < _end; ++N_it, ++this_it)
    {
      bool flag = true;
      for (unsigned int i = 0; i < Superclass::Dimension; ++i)
      {
        if (!this->m_InBounds[i] && ((temp[i] < OverlapLow[i]) || (temp[i] >= OverlapHigh[i])))
        {
          flag = false;
          break;
        }
      }

      if (flag)
      {
        this->m_NeighborhoodAccessorFunctor.Set(*this_it, *N_it);
      }

      for (unsigned int i = 0; i < Superclass::Dimension; ++i) // Update index
      {
        temp[i]++;
        if (static_cast<unsigned int>(temp[i]) == this->GetSize(i))
        {
          temp[i] = 0;
        }
        else
        {
          break;
        }
      }
    }
  }
}
} // namespace itk

#endif
