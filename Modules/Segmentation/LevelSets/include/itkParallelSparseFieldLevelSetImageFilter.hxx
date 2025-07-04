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
#ifndef itkParallelSparseFieldLevelSetImageFilter_hxx
#define itkParallelSparseFieldLevelSetImageFilter_hxx

#include "itkZeroCrossingImageFilter.h"
#include "itkShiftScaleImageFilter.h"
#include "itkImageRegionIterator.h"
#include "itkNumericTraits.h"
#include "itkNeighborhoodAlgorithm.h"
#include <iostream>
#include <fstream>
#include "itkMath.h"
#include "itkPlatformMultiThreader.h"
#include "itkPrintHelper.h"

namespace itk
{
template <typename TNeighborhoodType>
ParallelSparseFieldCityBlockNeighborList<TNeighborhoodType>::ParallelSparseFieldCityBlockNeighborList()
{
  using ImageType = typename NeighborhoodType::ImageType;
  auto dummy_image = ImageType::New();

  auto zero_offset = MakeFilled<OffsetType>(0);
  m_Radius.Fill(1);

  const NeighborhoodType it(m_Radius, dummy_image, dummy_image->GetRequestedRegion());
  const unsigned int     nCenter = it.Size() / 2;

  m_ArrayIndex.reserve(m_Size);
  m_NeighborhoodOffset.reserve(m_Size);

  for (unsigned int i = 0; i < m_Size; ++i)
  {
    m_NeighborhoodOffset.push_back(zero_offset);
  }

  {
    unsigned int i = 0;
    for (int d = Dimension - 1; d >= 0; --d, ++i)
    {
      m_ArrayIndex.push_back(nCenter - it.GetStride(d));
      m_NeighborhoodOffset[i][d] = -1;
    }
    for (int d = 0; d < int{ Dimension }; ++d, ++i)
    {
      m_ArrayIndex.push_back(nCenter + it.GetStride(d));
      m_NeighborhoodOffset[i][d] = 1;
    }
  }
  for (unsigned int i = 0; i < Dimension; ++i)
  {
    m_StrideTable[i] = it.GetStride(i);
  }
}

template <typename TNeighborhoodType>
void
ParallelSparseFieldCityBlockNeighborList<TNeighborhoodType>::Print(std::ostream & os, Indent indent) const
{
  using namespace print_helper;

  os << "ParallelSparseFieldCityBlockNeighborList: " << std::endl;

  os << indent << "Pad1: " << m_Pad1 << std::endl;
  os << indent << "Size: " << m_Size << std::endl;
  os << indent << "Radius: " << m_Radius << std::endl;
  os << indent << "ArrayIndex: " << m_ArrayIndex << std::endl;
  os << indent << "NeighborhoodOffset: " << m_NeighborhoodOffset << std::endl;

  os << indent << "StrideTable: " << m_StrideTable << std::endl;
  os << indent << "Pad2: " << m_Pad2 << std::endl;
}

template <typename TInputImage, typename TOutputImage>
typename ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ValueType
  ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::m_ValueOne =
    NumericTraits<typename ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ValueType>::OneValue();

template <typename TInputImage, typename TOutputImage>
typename ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ValueType
  ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::m_ValueZero =
    typename ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ValueType{};

template <typename TInputImage, typename TOutputImage>
typename ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::StatusType
  ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::m_StatusNull = NumericTraits<
    typename ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::StatusType>::NonpositiveMin();

template <typename TInputImage, typename TOutputImage>
typename ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::StatusType
  ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::m_StatusChanging = -1;

template <typename TInputImage, typename TOutputImage>
typename ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::StatusType
  ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::m_StatusActiveChangingUp = -2;

template <typename TInputImage, typename TOutputImage>
typename ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::StatusType
  ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::m_StatusActiveChangingDown = -3;

template <typename TInputImage, typename TOutputImage>
typename ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::StatusType
  ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::m_StatusBoundaryPixel = -4;

template <typename TInputImage, typename TOutputImage>
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ParallelSparseFieldLevelSetImageFilter()
  : m_NumberOfLayers(ImageDimension)
  , m_IsoSurfaceValue(m_ValueZero)
{
  this->SetRMSChange(static_cast<double>(m_ValueOne));
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::GenerateData()
{
  if (!this->m_IsInitialized)
  {
    // Clean up any memory from any aborted previous filter executions.
    this->DeallocateData();

    // Allocate the output image
    m_OutputImage = this->GetOutput();
    m_OutputImage->SetBufferedRegion(m_OutputImage->GetRequestedRegion());
    m_OutputImage->Allocate();

    // Copy the input image to the output image.  Algorithms will operate
    // directly on the output image
    this->CopyInputToOutput();

    // Perform any other necessary pre-iteration initialization.
    this->Initialize();
    this->SetElapsedIterations(0);

    // NOTE: Cannot set state to initialized yet since more initialization is
    // done in the Iterate method.
  }

  // Evolve the surface
  this->Iterate();

  // Clean up
  if (this->GetManualReinitialization() == false)
  {
    this->DeallocateData();

    // Reset the state once execution is completed
    this->m_IsInitialized = false;
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::CopyInputToOutput()
{
  // This method is the first step in initializing the level-set image, which
  // is also the output of the filter.  The input is passed through a
  // zero crossing filter, which produces zero's at pixels closest to the zero
  // level set and one's elsewhere.  The actual zero level set values will be
  // adjusted in the Initialize() step to more accurately represent the
  // position of the zero level set.

  // First need to subtract the iso-surface value from the input image.
  using ShiftScaleFilterType = ShiftScaleImageFilter<InputImageType, OutputImageType>;
  auto shiftScaleFilter = ShiftScaleFilterType::New();
  shiftScaleFilter->SetInput(this->GetInput());
  shiftScaleFilter->SetShift(-m_IsoSurfaceValue);
  // keep a handle to the shifted output
  m_ShiftedImage = shiftScaleFilter->GetOutput();

  auto zeroCrossingFilter = ZeroCrossingImageFilter<OutputImageType, OutputImageType>::New();
  zeroCrossingFilter->SetInput(m_ShiftedImage);
  zeroCrossingFilter->GraftOutput(m_OutputImage);
  zeroCrossingFilter->SetBackgroundValue(m_ValueOne);
  zeroCrossingFilter->SetForegroundValue(m_ValueZero);
  zeroCrossingFilter->SetNumberOfWorkUnits(1);
  zeroCrossingFilter->Update();

  // Here the output is the result of zerocrossings
  this->GraftOutput(zeroCrossingFilter->GetOutput());
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::Initialize()
{
  // A node pool used during initialization of the level set.
  m_LayerNodeStore = LayerNodeStorageType::New();
  m_LayerNodeStore->SetGrowthStrategyToExponential();

  // Allocate the status image.
  m_StatusImage = StatusImageType::New();
  m_StatusImage->SetRegions(m_OutputImage->GetRequestedRegion());
  m_StatusImage->Allocate();

  // Initialize the status image to contain all m_StatusNull values.
  ImageRegionIterator<StatusImageType> statusIt(m_StatusImage, m_StatusImage->GetRequestedRegion());
  for (statusIt.GoToBegin(); !statusIt.IsAtEnd(); ++statusIt)
  {
    statusIt.Set(m_StatusNull);
  }

  // Initialize the boundary pixels in the status images to
  // m_StatusBoundaryPixel values.  Uses the face calculator to find all of the
  // region faces.
  using BFCType = NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<StatusImageType>;

  auto sz = MakeFilled<typename BFCType::SizeType>(1);

  BFCType                        faceCalculator;
  typename BFCType::FaceListType faceList = faceCalculator(m_StatusImage, m_StatusImage->GetRequestedRegion(), sz);

  // skip the first (nonboundary) region
  for (auto fit = ++faceList.begin(); fit != faceList.end(); ++fit)
  {
    statusIt = ImageRegionIterator<StatusImageType>(m_StatusImage, *fit);
    for (statusIt.GoToBegin(); !statusIt.IsAtEnd(); ++statusIt)
    {
      statusIt.Set(m_StatusBoundaryPixel);
    }
  }

  // Allocate the layers of the sparse field.
  m_Layers.reserve(2 * m_NumberOfLayers + 1);
  for (unsigned int i = 0; i < 2 * static_cast<unsigned int>(m_NumberOfLayers) + 1; ++i)
  {
    m_Layers.push_back(LayerType::New());
  }

  // always the "Z" dimension
  m_SplitAxis = m_OutputImage->GetImageDimension() - 1;
  if (m_OutputImage->GetImageDimension() < 1)
  {
    // cannot split
    itkDebugMacro("Unable to choose an axis for workload distribution among threads");
    return;
  }

  typename OutputImageType::SizeType requestedRegionSize = m_OutputImage->GetRequestedRegion().GetSize();
  m_ZSize = requestedRegionSize[m_SplitAxis];

  // Histogram of number of pixels in each Z plane for the entire 3D volume
  m_GlobalZHistogram = new int[m_ZSize];
  for (unsigned int i = 0; i < m_ZSize; ++i)
  {
    m_GlobalZHistogram[i] = 0;
  }

  // Construct the active layer and initialize the first layers inside and
  // outside of the active layer
  this->ConstructActiveLayer();

  // Construct the rest of the non active set layers using the first two
  // layers. Inside layers are odd numbers, outside layers are even numbers.
  for (unsigned int i = 1; i < m_Layers.size() - 2; ++i)
  {
    this->ConstructLayer(i, i + 2);
  }

  // Set the values in the output image for the active layer.
  this->InitializeActiveLayerValues();

  // Initialize layer values using the active layer as seeds.
  this->PropagateAllLayerValues();

  // Initialize pixels inside and outside the sparse field layers to positive
  // and negative values, respectively.  This is not necessary for the
  // calculations, but is useful for presenting a more intuitive output to the
  // filter.  See PostProcessOutput method for more information.
  this->InitializeBackgroundPixels();

  m_NumOfWorkUnits = std::min(this->GetNumberOfWorkUnits(), this->GetMultiThreader()->GetMaximumNumberOfThreads());
  this->SetNumberOfWorkUnits(m_NumOfWorkUnits);

  // Cumulative frequency of number of pixels in each Z plane for the entire 3D
  // volume
  m_ZCumulativeFrequency = new int[m_ZSize];
  for (unsigned int i = 0; i < m_ZSize; ++i)
  {
    m_ZCumulativeFrequency[i] = 0;
  }

  // The mapping from a z-value to the thread in whose region the z-value lies
  m_MapZToThreadNumber = new unsigned int[m_ZSize];
  for (unsigned int i = 0; i < m_ZSize; ++i)
  {
    m_MapZToThreadNumber[i] = 0;
  }

  // The boundaries defining thread regions
  m_Boundary = new unsigned int[m_NumOfWorkUnits];
  for (unsigned int i = 0; i < m_NumOfWorkUnits; ++i)
  {
    m_Boundary[i] = 0;
  }

  // A boolean variable stating if the boundaries had been changed during
  // CheckLoadBalance()
  m_BoundaryChanged = false;

  // Allocate data for each thread.
  m_Data = new ThreadData[m_NumOfWorkUnits];
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ConstructActiveLayer()
{
  // We find the active layer by searching for 0's in the zero crossing image
  // (output image).  The first inside and outside layers are also constructed
  // by searching the neighbors of the active layer in the (shifted) input
  // image.
  // Negative neighbors not in the active set are assigned to the inside,
  // positive neighbors are assigned to the outside.

  NeighborhoodIterator<OutputImageType> shiftedIt(
    m_NeighborList.GetRadius(), m_ShiftedImage, m_OutputImage->GetRequestedRegion());
  NeighborhoodIterator<StatusImageType> statusIt(
    m_NeighborList.GetRadius(), m_StatusImage, m_OutputImage->GetRequestedRegion());


  typename OutputImageType::SizeType  regionSize = m_OutputImage->GetRequestedRegion().GetSize();
  typename OutputImageType::IndexType startIndex = m_OutputImage->GetRequestedRegion().GetIndex();
  using StartIndexValueType = IndexValueType;

  for (NeighborhoodIterator<OutputImageType> outputIt(
         m_NeighborList.GetRadius(), m_OutputImage, m_OutputImage->GetRequestedRegion());
       !outputIt.IsAtEnd();
       ++outputIt)
  {
    bool bounds_status = true;
    if (Math::ExactlyEquals(outputIt.GetCenterPixel(), m_ValueZero))
    {
      // Grab the neighborhood in the status image.
      const auto center_index = outputIt.GetIndex();
      statusIt.SetLocation(center_index);

      for (unsigned int j = 0; j < ImageDimension; ++j)
      {
        if ((center_index[j]) <= (startIndex[j]) ||
            (center_index[j]) >= startIndex[j] + static_cast<StartIndexValueType>(regionSize[j] - 1))
        {
          bounds_status = false;
          break;
        }
      }
      if (bounds_status)
      {
        // Here record the histogram information
        m_GlobalZHistogram[center_index[m_SplitAxis]]++;

        // Borrow a node from the store and set its value.
        auto node = m_LayerNodeStore->Borrow();
        node->m_Index = center_index;

        // Add the node to the active list and set the status in the status
        // image.
        m_Layers[0]->PushFront(node);
        statusIt.SetCenterPixel(0);

        // Grab the neighborhood in the image of shifted input values.
        shiftedIt.SetLocation(center_index);

        // Search the neighborhood pixels for first inside & outside layer
        // members.  Construct these lists and set status list values.

        for (unsigned int i = 0; i < m_NeighborList.GetSize(); ++i)
        {
          const auto offset_index = center_index + m_NeighborList.GetNeighborhoodOffset(i);

          if (Math::NotExactlyEquals(outputIt.GetPixel(m_NeighborList.GetArrayIndex(i)), m_ValueZero) &&
              statusIt.GetPixel(m_NeighborList.GetArrayIndex(i)) == m_StatusNull)
          {
            const auto value = shiftedIt.GetPixel(m_NeighborList.GetArrayIndex(i));
            // Assign to first outside layer. --> 1
            // Assign to first inside layer --> 2
            const StatusType layer_number = (value < m_ValueZero) ? 1 : 2;

            statusIt.SetPixel(m_NeighborList.GetArrayIndex(i), layer_number, bounds_status);

            node = m_LayerNodeStore->Borrow();
            node->m_Index = offset_index;
            m_Layers[layer_number]->PushFront(node);
            // else do nothing.
          }
        }
      }
    }
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ConstructLayer(const StatusType & from,
                                                                                  const StatusType & to)
{
  NeighborhoodIterator<StatusImageType> statusIt(
    m_NeighborList.GetRadius(), m_StatusImage, m_OutputImage->GetRequestedRegion());

  // For all indices in the "from" layer...
  for (typename LayerType::ConstIterator fromIt = m_Layers[from]->Begin(); fromIt != m_Layers[from]->End(); ++fromIt)
  {
    // Search the neighborhood of this index in the status image for
    // unassigned indices. Push those indices onto the "to" layer and
    // assign them values in the status image.  Status pixels outside the
    // boundary will be ignored.
    statusIt.SetLocation(fromIt->m_Index);

    for (unsigned int i = 0; i < m_NeighborList.GetSize(); ++i)
    {
      if (statusIt.GetPixel(m_NeighborList.GetArrayIndex(i)) == m_StatusNull)
      {
        bool boundary_status = false;
        statusIt.SetPixel(m_NeighborList.GetArrayIndex(i), to, boundary_status);

        if (boundary_status) // in bounds
        {
          auto node = m_LayerNodeStore->Borrow();
          node->m_Index = statusIt.GetIndex() + m_NeighborList.GetNeighborhoodOffset(i);
          m_Layers[to]->PushFront(node);
        }
      }
    }
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::InitializeActiveLayerValues()
{
  const ValueType CHANGE_FACTOR = m_ConstantGradientValue / 2.0;
  ValueType       MIN_NORM = 1.0e-6;

  if (this->GetUseImageSpacing())
  {
    const auto & spacing = this->GetInput()->GetSpacing();

    SpacePrecisionType minSpacing = NumericTraits<SpacePrecisionType>::max();
    for (unsigned int i = 0; i < ImageDimension; ++i)
    {
      minSpacing = std::min(minSpacing, spacing[i]);
    }
    MIN_NORM *= minSpacing;
  }

  ConstNeighborhoodIterator<OutputImageType> shiftedIt(
    m_NeighborList.GetRadius(), m_ShiftedImage, m_OutputImage->GetRequestedRegion());

  const unsigned int center = shiftedIt.Size() / 2;

  const NeighborhoodScalesType neighborhoodScales = this->GetDifferenceFunction()->ComputeNeighborhoodScales();

  // For all indices in the active layer...
  for (typename LayerType::ConstIterator activeIt = m_Layers[0]->Begin(); activeIt != m_Layers[0]->End(); ++activeIt)
  {
    // Interpolate on the (shifted) input image values at this index to
    // assign an active layer value in the output image.
    shiftedIt.SetLocation(activeIt->m_Index);

    ValueType length = m_ValueZero;
    for (unsigned int i = 0; i < ImageDimension; ++i)
    {
      const auto stride = shiftedIt.GetStride(i);

      const ValueType dx_forward =
        (shiftedIt.GetPixel(center + stride) - shiftedIt.GetCenterPixel()) * neighborhoodScales[i];
      const ValueType dx_backward =
        (shiftedIt.GetCenterPixel() - shiftedIt.GetPixel(center - stride)) * neighborhoodScales[i];

      if (itk::Math::abs(dx_forward) > itk::Math::abs(dx_backward))
      {
        length += dx_forward * dx_forward;
      }
      else
      {
        length += dx_backward * dx_backward;
      }
    }
    length = std::sqrt(length) + MIN_NORM;
    const ValueType distance = shiftedIt.GetCenterPixel() / length;

    m_OutputImage->SetPixel(activeIt->m_Index, std::clamp(distance, -CHANGE_FACTOR, CHANGE_FACTOR));
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::PropagateAllLayerValues()
{
  // Update values in the first inside and first outside layers using the
  // active layer as a seed. Inside layers are odd numbers, outside layers are
  // even numbers.
  this->PropagateLayerValues(0, 1, 3, 1); // first inside
  this->PropagateLayerValues(0, 2, 4, 0); // first outside

  // Update the rest of the layers.
  for (unsigned int i = 1; i < m_Layers.size() - 2; ++i)
  {
    this->PropagateLayerValues(i, i + 2, i + 4, (i + 2) % 2);
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::PropagateLayerValues(const StatusType & from,
                                                                                        const StatusType & to,
                                                                                        const StatusType & promote,
                                                                                        unsigned int       InOrOut)
{
  const StatusType past_end = static_cast<StatusType>(m_Layers.size()) - 1;

  // Are we propagating values inward (more negative) or outward (more
  // positive)?
  const ValueType delta = (InOrOut == 1) ? -m_ConstantGradientValue /* inward */ : m_ConstantGradientValue;

  NeighborhoodIterator<OutputImageType> outputIt(
    m_NeighborList.GetRadius(), m_OutputImage, m_OutputImage->GetRequestedRegion());
  NeighborhoodIterator<StatusImageType> statusIt(
    m_NeighborList.GetRadius(), m_StatusImage, m_OutputImage->GetRequestedRegion());

  typename LayerType::Iterator toIt = m_Layers[to]->Begin();
  while (toIt != m_Layers[to]->End())
  {
    statusIt.SetLocation(toIt->m_Index);
    // Is this index marked for deletion? If the status image has
    // been marked with another layer's value, we need to delete this node
    // from the current list then skip to the next iteration.
    if (statusIt.GetCenterPixel() != to)
    {
      auto node = toIt.GetPointer();
      ++toIt;
      m_Layers[to]->Unlink(node);
      m_LayerNodeStore->Return(node);
      continue;
    }

    outputIt.SetLocation(toIt->m_Index);

    auto value = m_ValueZero;
    bool found_neighbor_flag = false;
    for (unsigned int i = 0; i < m_NeighborList.GetSize(); ++i)
    {
      // If this neighbor is in the "from" list, compare its absolute value
      // to any previous values found in the "from" list.  Keep the value
      // that will cause the next layer to be closest to the zero level set.
      if (statusIt.GetPixel(m_NeighborList.GetArrayIndex(i)) == from)
      {
        const auto value_temp = outputIt.GetPixel(m_NeighborList.GetArrayIndex(i));

        if (found_neighbor_flag == false)
        {
          value = value_temp;
        }
        else
        {
          if (itk::Math::abs(value_temp + delta) < itk::Math::abs(value + delta))
          {
            // take the value closest to zero
            value = value_temp;
          }
        }
        found_neighbor_flag = true;
      }
    }
    if (found_neighbor_flag)
    {
      // Set the new value using the smallest magnitude
      // found in our "from" neighbors.
      outputIt.SetCenterPixel(value + delta);
      ++toIt;
    }
    else
    {
      // Did not find any neighbors on the "from" list, then promote this
      // node.  A "promote" value past the end of my sparse field size
      // means delete the node instead.  Change the status value in the
      // status image accordingly.
      auto node = toIt.GetPointer();
      ++toIt;
      m_Layers[to]->Unlink(node);
      if (promote > past_end)
      {
        m_LayerNodeStore->Return(node);
        statusIt.SetCenterPixel(m_StatusNull);
      }
      else
      {
        m_Layers[promote]->PushFront(node);
        statusIt.SetCenterPixel(promote);
      }
    }
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::InitializeBackgroundPixels()
{
  // Assign background pixels INSIDE the sparse field layers to a new level set
  // with value less than the innermost layer.  Assign background pixels
  // OUTSIDE the sparse field layers to a new level set with value greater than
  // the outermost layer.
  const auto max_layer = static_cast<ValueType>(m_NumberOfLayers);

  const ValueType outside_value = (max_layer + 1) * m_ConstantGradientValue;
  const ValueType inside_value = -(max_layer + 1) * m_ConstantGradientValue;

  ImageRegionConstIterator<StatusImageType> statusIt(m_StatusImage, this->GetOutput()->GetRequestedRegion());

  ImageRegionIterator<OutputImageType> outputIt(this->GetOutput(), this->GetOutput()->GetRequestedRegion());

  ImageRegionConstIterator<OutputImageType> shiftedIt(m_ShiftedImage, this->GetOutput()->GetRequestedRegion());

  for (outputIt.GoToBegin(), statusIt.GoToBegin(), shiftedIt.GoToBegin(); !outputIt.IsAtEnd();
       ++outputIt, ++statusIt, ++shiftedIt)
  {
    if (statusIt.Get() == m_StatusNull || statusIt.Get() == m_StatusBoundaryPixel)
    {
      if (shiftedIt.Get() > m_ValueZero)
      {
        outputIt.Set(outside_value);
      }
      else
      {
        outputIt.Set(inside_value);
      }
    }
  }

  // deallocate the shifted-image
  m_ShiftedImage = nullptr;
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ComputeInitialThreadBoundaries()
{
  // NOTE: Properties of the boundary computation algorithm
  //       1. Thread-0 always has something to work on.
  //       2. If a particular thread numbered i has the m_Boundary = (mZSize -
  //          1) then ALL threads numbered > i do NOT have anything to work on.

  // Compute the cumulative frequency distribution using the global histogram.
  m_ZCumulativeFrequency[0] = m_GlobalZHistogram[0];
  for (unsigned int i = 1; i < m_ZSize; ++i)
  {
    m_ZCumulativeFrequency[i] = m_ZCumulativeFrequency[i - 1] + m_GlobalZHistogram[i];
  }

  // Now define the regions that each thread will process and the corresponding
  // boundaries.
  m_Boundary[m_NumOfWorkUnits - 1] = m_ZSize - 1; // special case: the upper
                                                  // bound for the last thread
  for (unsigned int i = 0; i < m_NumOfWorkUnits - 1; ++i)
  {
    // compute m_Boundary[i]

    const float cutOff = 1.0 * (i + 1) * m_ZCumulativeFrequency[m_ZSize - 1] / m_NumOfWorkUnits;

    // find the position in the cumulative freq dist where this cutoff is met
    for (unsigned int j = (i == 0 ? 0 : m_Boundary[i - 1]); j < m_ZSize; ++j)
    {
      if (cutOff > m_ZCumulativeFrequency[j])
      {
        continue;
      }

      // Optimize a little.
      // Go further FORWARD and find the first index (k) in the cumulative
      // freq distribution s.t. m_ZCumulativeFrequency[k] !=
      // m_ZCumulativeFrequency[j] This is to be done because if we have a
      // flat patch in the cumulative freq. dist. then we can choose
      // a bound midway in that flat patch .
      {
        unsigned int k = 1;
        for (; j + k < m_ZSize; ++k)
        {
          if (m_ZCumulativeFrequency[j + k] != m_ZCumulativeFrequency[j])
          {
            break;
          }
        }
        //
        m_Boundary[i] = static_cast<unsigned int>((j + k / 2));
      }
      break;
    }
  }

  // Initialize the local histograms using the global one and the boundaries
  // Also initialize the mapping from the Z value --> the thread number
  // i.e. m_MapZToThreadNumber[]
  // Also divide the lists up according to the boundaries
  for (unsigned int i = 0; i <= m_Boundary[0]; ++i)
  {
    // this Z belongs to the region associated with thread-0
    m_MapZToThreadNumber[i] = 0;
  }

  for (unsigned int t = 1; t < m_NumOfWorkUnits; ++t)
  {
    for (unsigned int i = m_Boundary[t - 1] + 1; i <= m_Boundary[t]; ++i)
    {
      // this Z belongs to the region associated with thread-0
      m_MapZToThreadNumber[i] = t;
    }
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ThreadedAllocateData(ThreadIdType ThreadId)
{
  static constexpr float SAFETY_FACTOR = 4.0;

  m_Data[ThreadId].m_Semaphore[0] = 0;
  m_Data[ThreadId].m_Semaphore[1] = 0;

  const std::size_t bufferLayerSize = 2 * m_NumberOfLayers + 1;
  // Allocate the layers for the sparse field.
  m_Data[ThreadId].m_Layers.reserve(bufferLayerSize);
  for (unsigned int i = 0; i < 2 * static_cast<unsigned int>(m_NumberOfLayers) + 1; ++i)
  {
    m_Data[ThreadId].m_Layers.push_back(LayerType::New());
  }
  // Throw an exception if we don't have enough layers.
  if (m_Data[ThreadId].m_Layers.size() < 3)
  {
    itkExceptionMacro("Not enough layers have been allocated for the sparse field. Requires at least one layer.");
  }

  // Layers used as buffers for transferring pixels during load balancing

  m_Data[ThreadId].m_LoadTransferBufferLayers = new LayerListType[bufferLayerSize];
  for (unsigned int i = 0; i < 2 * static_cast<unsigned int>(m_NumberOfLayers) + 1; ++i)
  {
    m_Data[ThreadId].m_LoadTransferBufferLayers[i].reserve(m_NumOfWorkUnits);

    for (unsigned int j = 0; j < m_NumOfWorkUnits; ++j)
    {
      m_Data[ThreadId].m_LoadTransferBufferLayers[i].push_back(LayerType::New());
    }
  }

  // Every thread allocates a local node pool (improving memory locality)
  m_Data[ThreadId].m_LayerNodeStore = LayerNodeStorageType::New();
  m_Data[ThreadId].m_LayerNodeStore->SetGrowthStrategyToExponential();

  // The SAFETY_FACTOR simple ensures that the number of nodes created
  // is larger than those required to start with for each thread.
  auto nodeNum = static_cast<unsigned int>(SAFETY_FACTOR * m_Layers[0]->Size() * (bufferLayerSize) / m_NumOfWorkUnits);

  m_Data[ThreadId].m_LayerNodeStore->Reserve(nodeNum);
  m_Data[ThreadId].m_RMSChange = m_ValueZero;

  // UpLists and Downlists
  for (unsigned int i = 0; i < 2; ++i)
  {
    m_Data[ThreadId].UpList[i] = LayerType::New();
    m_Data[ThreadId].DownList[i] = LayerType::New();
  }

  // Used during the time when status lists are being processed (in
  // ThreadedApplyUpdate() )
  // for the Uplists
  m_Data[ThreadId].m_InterNeighborNodeTransferBufferLayers[0] = new LayerPointerType *[m_NumberOfLayers + 1];

  // for the Downlists
  m_Data[ThreadId].m_InterNeighborNodeTransferBufferLayers[1] = new LayerPointerType *[m_NumberOfLayers + 1];

  for (unsigned int i = 0; i < static_cast<unsigned int>(m_NumberOfLayers) + 1; ++i)
  {
    m_Data[ThreadId].m_InterNeighborNodeTransferBufferLayers[0][i] = new LayerPointerType[m_NumOfWorkUnits];
    m_Data[ThreadId].m_InterNeighborNodeTransferBufferLayers[1][i] = new LayerPointerType[m_NumOfWorkUnits];
  }

  for (unsigned int i = 0; i < static_cast<unsigned int>(m_NumberOfLayers) + 1; ++i)
  {
    for (unsigned int j = 0; j < m_NumOfWorkUnits; ++j)
    {
      m_Data[ThreadId].m_InterNeighborNodeTransferBufferLayers[0][i][j] = LayerType::New();
      m_Data[ThreadId].m_InterNeighborNodeTransferBufferLayers[1][i][j] = LayerType::New();
    }
  }

  // Local histogram for every thread (used during Iterate()), initialized to zeros.
  m_Data[ThreadId].m_ZHistogram = new int[m_ZSize]();

  // Every thread must have its own copy of the GlobalData struct.
  m_Data[ThreadId].globalData = this->GetDifferenceFunction()->GetGlobalDataPointer();

  //
  m_Data[ThreadId].m_SemaphoreArrayNumber = 0;
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ThreadedInitializeData(
  ThreadIdType             ThreadId,
  const ThreadRegionType & ThreadRegion)
{
  // divide the lists based on the boundaries
  for (unsigned int i = 0; i < 2 * static_cast<unsigned int>(m_NumberOfLayers) + 1; ++i)
  {
    typename LayerType::Iterator       layerIt = m_Layers[i]->Begin();
    const typename LayerType::Iterator layerEnd = m_Layers[i]->End();

    while (layerIt != layerEnd)
    {
      LayerNodeType * nodePtr = layerIt.GetPointer();
      ++layerIt;

      const unsigned int k = this->GetThreadNumber(nodePtr->m_Index[m_SplitAxis]);
      if (k != ThreadId)
      {
        continue; // some other thread's node => ignore
      }

      // Borrow a node from the specific thread's layer so that MEMORY LOCALITY
      // is maintained.
      // NOTE : We already pre-allocated more than enough
      // nodes for each thread implying no new nodes are created here.
      LayerNodeType * nodeTempPtr = m_Data[ThreadId].m_LayerNodeStore->Borrow();
      nodeTempPtr->m_Index = nodePtr->m_Index;
      // push the node on the appropriate layer
      m_Data[ThreadId].m_Layers[i]->PushFront(nodeTempPtr);

      // for the active layer (layer-0) build the histogram for each thread
      if (i == 0)
      {
        // this Z histogram value should be given to thread-0
        m_Data[ThreadId].m_ZHistogram[(nodePtr->m_Index)[m_SplitAxis]] =
          m_Data[ThreadId].m_ZHistogram[(nodePtr->m_Index)[m_SplitAxis]] + 1;
      }
    }
  }

  // Copy from the current status/output images to the new ones and let each
  // thread do the copy of its own region.
  // This will make each thread be the FIRST to write to "it's" data in the new
  // images and hence the memory will get allocated
  // in the corresponding thread's memory-node.
  ImageRegionConstIterator<StatusImageType> statusIt(m_StatusImage, ThreadRegion);
  ImageRegionIterator<StatusImageType>      statusItNew(m_StatusImageTemp, ThreadRegion);
  ImageRegionConstIterator<OutputImageType> outputIt(m_OutputImage, ThreadRegion);
  ImageRegionIterator<OutputImageType>      outputItNew(m_OutputImageTemp, ThreadRegion);

  for (outputIt.GoToBegin(), statusIt.GoToBegin(), outputItNew.GoToBegin(), statusItNew.GoToBegin();
       !outputIt.IsAtEnd();
       ++outputIt, ++statusIt, ++outputItNew, ++statusItNew)
  {
    statusItNew.Set(statusIt.Get());
    outputItNew.Set(outputIt.Get());
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::DeallocateData()
{
  // Delete data structures used for load distribution and balancing.
  delete[] m_GlobalZHistogram;
  m_GlobalZHistogram = nullptr;
  delete[] m_ZCumulativeFrequency;
  m_ZCumulativeFrequency = nullptr;
  delete[] m_MapZToThreadNumber;
  m_MapZToThreadNumber = nullptr;
  delete[] m_Boundary;
  m_Boundary = nullptr;

  // Deallocate the status image.
  m_StatusImage = nullptr;

  // Remove the barrier from the system.
  //  m_Barrier->Remove ();

  // Delete initial nodes, the node pool, the layers.
  if (!m_Layers.empty())
  {
    for (unsigned int i = 0; i < 2 * static_cast<unsigned int>(m_NumberOfLayers) + 1; ++i)
    {
      // return all the nodes in layer i to the main node pool
      const LayerPointerType layerPtr = m_Layers[i];
      while (!layerPtr->Empty())
      {
        LayerNodeType * nodePtr = layerPtr->Front();
        layerPtr->PopFront();
        m_LayerNodeStore->Return(nodePtr);
      }
    }
  }
  if (m_LayerNodeStore)
  {
    m_LayerNodeStore->Clear();
    m_Layers.clear();
  }

  if (m_Data != nullptr)
  {
    // Deallocate the thread local data structures.
    for (ThreadIdType ThreadId = 0; ThreadId < m_NumOfWorkUnits; ++ThreadId)
    {
      delete[] m_Data[ThreadId].m_ZHistogram;

      if (m_Data[ThreadId].globalData != nullptr)
      {
        this->GetDifferenceFunction()->ReleaseGlobalDataPointer(m_Data[ThreadId].globalData);
        m_Data[ThreadId].globalData = nullptr;
      }

      // 1. delete nodes on the thread layers
      for (unsigned int i = 0; i < 2 * static_cast<unsigned int>(m_NumberOfLayers) + 1; ++i)
      {
        // return all the nodes in layer i to thread-i's node pool
        const LayerPointerType layerPtr = m_Data[ThreadId].m_Layers[i];
        while (!layerPtr->Empty())
        {
          LayerNodeType * nodePtr = layerPtr->Front();
          layerPtr->PopFront();
          m_Data[ThreadId].m_LayerNodeStore->Return(nodePtr);
        }
      }
      m_Data[ThreadId].m_Layers.clear();

      // 2. cleanup the LoadTransferBufferLayers: empty all and return the nodes
      // to the pool
      for (unsigned int i = 0; i < 2 * static_cast<unsigned int>(m_NumberOfLayers) + 1; ++i)
      {
        for (ThreadIdType tid = 0; tid < m_NumOfWorkUnits; ++tid)
        {
          if (tid == ThreadId)
          {
            // a thread does NOT pass nodes to itself
            continue;
          }

          const LayerPointerType layerPtr = m_Data[ThreadId].m_LoadTransferBufferLayers[i][tid];

          while (!layerPtr->Empty())
          {
            LayerNodeType * nodePtr = layerPtr->Front();
            layerPtr->PopFront();
            m_Data[ThreadId].m_LayerNodeStore->Return(nodePtr);
          }
        }
        m_Data[ThreadId].m_LoadTransferBufferLayers[i].clear();
      }
      delete[] m_Data[ThreadId].m_LoadTransferBufferLayers;

      // 3. clear up the nodes in the last layer of
      // m_InterNeighborNodeTransferBufferLayers (if any)
      for (unsigned int i = 0; i < m_NumOfWorkUnits; ++i)
      {
        for (unsigned int InOrOut = 0; InOrOut < 2; ++InOrOut)
        {
          const LayerPointerType layerPtr =
            m_Data[ThreadId].m_InterNeighborNodeTransferBufferLayers[InOrOut][m_NumberOfLayers][i];

          while (!layerPtr->Empty())
          {
            LayerNodeType * nodePtr = layerPtr->Front();
            layerPtr->PopFront();
            m_Data[ThreadId].m_LayerNodeStore->Return(nodePtr);
          }
        }
      }

      // check if all last layers are empty and then delete them
      for (unsigned int i = 0; i < static_cast<unsigned int>(m_NumberOfLayers) + 1; ++i)
      {
        delete[] m_Data[ThreadId].m_InterNeighborNodeTransferBufferLayers[0][i];
        delete[] m_Data[ThreadId].m_InterNeighborNodeTransferBufferLayers[1][i];
      }
      delete[] m_Data[ThreadId].m_InterNeighborNodeTransferBufferLayers[0];
      delete[] m_Data[ThreadId].m_InterNeighborNodeTransferBufferLayers[1];

      // 4. check if all the uplists and downlists are empty

      // 5. delete all nodes in the node pool
      m_Data[ThreadId].m_LayerNodeStore->Clear();
    }

    delete[] m_Data;
  } // if m_data != 0
  m_Data = nullptr;
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ThreadedInitializeIteration(
  ThreadIdType itkNotUsed(ThreadId))
{
  // If child classes need an entry point to the start of every iteration step
  // they can override this method.
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::Iterate()
{
  m_TimeStep = TimeStepType{};

  MultiThreaderBase * mt = this->GetMultiThreader();
  mt->SetNumberOfWorkUnits(m_NumOfWorkUnits);

  // Initialize the list of time step values that will be generated by the
  // various threads.  There is one distinct slot for each possible thread,
  // so this data structure is thread-safe.
  m_TimeStepList.resize(m_NumOfWorkUnits);
  m_ValidTimeStepList.resize(m_NumOfWorkUnits, true);

  const typename TOutputImage::RegionType reqRegion = m_OutputImage->GetRequestedRegion();

  // Controls how often we check for balance of the load among the threads and
  // perform load balancing (if needed) by redistributing the load.
  constexpr unsigned int LOAD_BALANCE_ITERATION_FREQUENCY = 30;

  if (!this->m_IsInitialized)
  {
    this->ComputeInitialThreadBoundaries();

    // Create the temporary status image
    this->m_StatusImageTemp = StatusImageType::New();
    this->m_StatusImageTemp->SetRegions(reqRegion);
    this->m_StatusImageTemp->Allocate();

    // Create the temporary output image
    this->m_OutputImageTemp = OutputImageType::New();
    this->m_OutputImageTemp->CopyInformation(this->m_OutputImage);
    this->m_OutputImageTemp->SetRegions(reqRegion);
    this->m_OutputImageTemp->Allocate();

    // Data allocation and initialization performed in parallel
    mt->ParallelizeArray(
      0,
      m_NumOfWorkUnits,
      [this](SizeValueType threadId) {
        this->ThreadedAllocateData(threadId);
        this->GetThreadRegionSplitByBoundary(threadId, this->m_Data[threadId].ThreadRegion);
        this->ThreadedInitializeData(threadId, this->m_Data[threadId].ThreadRegion);
      },
      nullptr);

    this->m_StatusImage = nullptr;
    this->m_StatusImage = this->m_StatusImageTemp;
    this->m_StatusImageTemp = nullptr;

    this->m_OutputImage = nullptr;
    this->m_OutputImage = this->m_OutputImageTemp;
    this->m_OutputImageTemp = nullptr;

    this->GraftOutput(this->m_OutputImage);

    this->m_IsInitialized = true;
  }


  unsigned int iter = this->GetElapsedIterations();
  while (!(this->Halt()))
  {
    mt->ParallelizeArray(
      0,
      m_NumOfWorkUnits,
      [this](SizeValueType threadId) {
        this->ThreadedInitializeIteration(threadId);
        this->m_Data[threadId].TimeStep = this->ThreadedCalculateChange(threadId);
      },
      nullptr);

    // Calculate the timestep (no need to do this when there is just 1 thread)
    if (this->m_NumOfWorkUnits == 1)
    {
      if (iter != 0)
      {
        // Update the RMS difference here
        this->SetRMSChange(static_cast<double>(this->m_Data[0].m_RMSChange));
        const unsigned int count = this->m_Data[0].m_Count;
        if (count != 0)
        {
          this->SetRMSChange(static_cast<double>(std::sqrt((static_cast<float>(this->GetRMSChange())) / count)));
        }
      }

      // this is done by the thread0
      this->InvokeEvent(IterationEvent());
      this->InvokeEvent(ProgressEvent());
      this->SetElapsedIterations(++iter);

      // (works for the 1-thread case else redefined below)
      m_TimeStep = this->m_Data[0].TimeStep;
    }
    else
    {
      if (iter != 0)
      {
        // Update the RMS difference here
        unsigned int count = 0;
        this->SetRMSChange(static_cast<double>(m_ValueZero));
        for (unsigned int i = 0; i < this->m_NumOfWorkUnits; ++i)
        {
          this->SetRMSChange(this->GetRMSChange() + this->m_Data[i].m_RMSChange);
          count += this->m_Data[i].m_Count;
        }
        if (count != 0)
        {
          this->SetRMSChange(static_cast<double>(std::sqrt((static_cast<float>(this->m_RMSChange)) / count)));
        }
      }

      // Should we stop iterating ? (in case there are too few pixels to
      // process for every thread)
      this->m_Stop = true;
      for (unsigned int i = 0; i < this->m_NumOfWorkUnits; ++i)
      {
        if (this->m_Data[i].m_Layers[0]->Size() > 10)
        {
          this->m_Stop = false;
          break;
        }
      }

      this->InvokeEvent(IterationEvent());
      this->InvokeEvent(ProgressEvent());
      this->SetElapsedIterations(++iter);

      for (unsigned int i = 0; i < this->m_NumOfWorkUnits; ++i)
      {
        m_TimeStepList[i] = this->m_Data[i].TimeStep;
      }
      m_TimeStep = this->ResolveTimeStep(m_TimeStepList, m_ValidTimeStepList);
    }

    // The active layer is too small => stop iterating
    if (this->m_Stop)
    {
      return;
    }

    mt->ParallelizeArray(
      0,
      m_NumOfWorkUnits,
      [this](SizeValueType threadId) {
        this->ThreadedApplyUpdate(m_TimeStep, threadId);
        // We only need to wait for neighbors because ThreadedCalculateChange
        // requires information only from the neighbors.
        this->SignalNeighborsAndWait(threadId);
      },
      nullptr);


    if (this->GetElapsedIterations() % LOAD_BALANCE_ITERATION_FREQUENCY == 0)
    {
      this->CheckLoadBalance();

      if (this->m_BoundaryChanged)
      {
        // the situation at this point in time:
        // the OPTIMAL boundaries (that divide work equally) have changed but ...
        // the thread data lags behind the boundaries (it is still following the old
        // boundaries) the m_ZHistogram[], however, reflects the latest optimal
        // boundaries

        // The task:
        // 1. Every thread checks for pixels with itself that should NOT be with
        //    itself anymore (because of the changed boundaries).
        //    These pixels are now put in extra "buckets" for other threads to grab
        // 2. WaitForAll ().
        // 3. Every thread grabs those pixels, from every other thread, that come
        //    within its boundaries (from the extra buckets).
        mt->ParallelizeArray(
          0, m_NumOfWorkUnits, [this](SizeValueType threadId) { this->ThreadedLoadBalance1(threadId); }, nullptr);
        mt->ParallelizeArray(
          0, m_NumOfWorkUnits, [this](SizeValueType threadId) { this->ThreadedLoadBalance2(threadId); }, nullptr);
      }
    }
  }

  // post-process output
  mt->ParallelizeArray(
    0,
    m_NumOfWorkUnits,
    [this](SizeValueType threadId) {
      this->GetThreadRegionSplitUniformly(threadId, this->m_Data[threadId].ThreadRegion);
      this->ThreadedPostProcessOutput(this->m_Data[threadId].ThreadRegion);
    },
    nullptr);
}

template <typename TInputImage, typename TOutputImage>
auto
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ThreadedCalculateChange(ThreadIdType ThreadId)
  -> TimeStepType
{
  const typename FiniteDifferenceFunctionType::Pointer df = this->GetDifferenceFunction();
  ValueType                                            centerValue = 0.0;
  ValueType                                            MIN_NORM = 1.0e-6;
  if (this->GetUseImageSpacing())
  {
    const auto & spacing = this->GetInput()->GetSpacing();

    SpacePrecisionType minSpacing = NumericTraits<SpacePrecisionType>::max();
    for (unsigned int i = 0; i < ImageDimension; ++i)
    {
      minSpacing = std::min(minSpacing, spacing[i]);
    }
    MIN_NORM *= minSpacing;
  }

  ConstNeighborhoodIterator<OutputImageType> outputIt(
    df->GetRadius(), m_OutputImage, m_OutputImage->GetRequestedRegion());
  if (m_BoundsCheckingActive == false)
  {
    outputIt.NeedToUseBoundaryConditionOff();
  }
  const unsigned int center = outputIt.Size() / 2;

  this->GetDifferenceFunction()->ComputeNeighborhoodScales();

  // Calculates the update values for the active layer indices in this
  // iteration.  Iterates through the active layer index list, applying
  // the level set function to the output image (level set image) at each
  // index.

  typename LayerType::Iterator       layerIt = m_Data[ThreadId].m_Layers[0]->Begin();
  const typename LayerType::Iterator layerEnd = m_Data[ThreadId].m_Layers[0]->End();

  for (; layerIt != layerEnd; ++layerIt)
  {
    outputIt.SetLocation(layerIt->m_Index);
    // Calculate the offset to the surface from the center of this
    // neighborhood.  This is used by some level set functions in sampling a
    // speed, advection, or curvature term.
    if (this->m_InterpolateSurfaceLocation &&
        Math::NotExactlyEquals((centerValue = outputIt.GetCenterPixel()), ValueType{}))
    {
      // Surface is at the zero crossing, so distance to surface is:
      // phi(x) / norm(grad(phi)), where phi(x) is the center of the
      // neighborhood.  The location is therefore
      // (i,j,k) - ( phi(x) * grad(phi(x)) ) / norm(grad(phi))^2
      ValueType norm_grad_phi_squared = 0.0;

      typename FiniteDifferenceFunctionType::FloatOffsetType offset;
      for (unsigned int i = 0; i < ImageDimension; ++i)
      {
        const ValueType forwardValue = outputIt.GetPixel(center + m_NeighborList.GetStride(i));
        const ValueType backwardValue = outputIt.GetPixel(center - m_NeighborList.GetStride(i));

        if (forwardValue * backwardValue >= 0)
        {
          // 1. both neighbors have the same sign OR at least one of them is
          // ZERO
          const auto dx_forward = forwardValue - centerValue;
          const auto dx_backward = centerValue - backwardValue;

          // take the one-sided derivative with the larger magnitude
          if (itk::Math::abs(dx_forward) > itk::Math::abs(dx_backward))
          {
            offset[i] = dx_forward;
          }
          else
          {
            offset[i] = dx_backward;
          }
        }
        else
        {
          // 2. neighbors have opposite sign
          // take the one-sided derivative using the neighbor that has the
          // opposite sign w.r.t. oneself
          if (centerValue * forwardValue < 0)
          {
            offset[i] = forwardValue - centerValue;
          }
          else
          {
            offset[i] = centerValue - backwardValue;
          }
        }

        norm_grad_phi_squared += offset[i] * offset[i];
      }

      for (unsigned int i = 0; i < ImageDimension; ++i)
      {
        offset[i] = (offset[i] * outputIt.GetCenterPixel()) / (norm_grad_phi_squared + MIN_NORM);
      }

      layerIt->m_Value = df->ComputeUpdate(outputIt, (void *)m_Data[ThreadId].globalData, offset);
    }
    else // Don't do interpolation
    {
      layerIt->m_Value = df->ComputeUpdate(outputIt, (void *)m_Data[ThreadId].globalData);
    }
  }

  const TimeStepType timeStep = df->ComputeGlobalTimeStep((void *)m_Data[ThreadId].globalData);

  return timeStep;
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ThreadedApplyUpdate(const TimeStepType & dt,
                                                                                       ThreadIdType         ThreadId)
{
  this->ThreadedUpdateActiveLayerValues(dt, m_Data[ThreadId].UpList[0], m_Data[ThreadId].DownList[0], ThreadId);

  // We need to update histogram information (because some pixels are LEAVING
  // layer-0 (the active layer)

  this->SignalNeighborsAndWait(ThreadId);

  // Process status lists and update value for first inside/outside layers

  this->ThreadedProcessStatusList(0, 1, 2, 1, 1, 0, ThreadId);
  this->ThreadedProcessStatusList(0, 1, 1, 2, 0, 0, ThreadId);

  this->SignalNeighborsAndWait(ThreadId);

  // Update first layer value, process first layer
  this->ThreadedProcessFirstLayerStatusLists(1, 0, 3, 1, 1, ThreadId);
  this->ThreadedProcessFirstLayerStatusLists(1, 0, 4, 0, 1, ThreadId);

  // We need to update histogram information (because some pixels are ENTERING
  // layer-0

  this->SignalNeighborsAndWait(ThreadId);

  StatusType    up_to = 1;
  StatusType    up_search = 5;
  StatusType    down_to = 2;
  StatusType    down_search = 6;
  unsigned char j = 0;
  unsigned char k = 1;

  // The 3D case: this loop is executed at least once
  while (down_search < 2 * m_NumberOfLayers + 1)
  {
    this->ThreadedProcessStatusList(j, k, up_to, up_search, 1, (up_search - 1) / 2, ThreadId);
    this->ThreadedProcessStatusList(j, k, down_to, down_search, 0, (up_search - 1) / 2, ThreadId);

    this->SignalNeighborsAndWait(ThreadId);

    up_to += 2;
    down_to += 2;
    up_search += 2;
    down_search += 2;

    // Swap the lists so we can re-use the empty one.
    j = k;
    k = 1 - j;
  }
  // now up_search   = 2 * m_NumberOfLayers + 1 (= 7 if m_NumberOfLayers = 3)
  // now down_search = 2 * m_NumberOfLayers + 2 (= 8 if m_NumberOfLayers = 3)

  // Process the outermost inside/outside layers in the sparse field
  this->ThreadedProcessStatusList(j, k, up_to, m_StatusNull, 1, (up_search - 1) / 2, ThreadId);
  this->ThreadedProcessStatusList(j, k, down_to, m_StatusNull, 0, (up_search - 1) / 2, ThreadId);

  this->SignalNeighborsAndWait(ThreadId);

  this->ThreadedProcessOutsideList(k, (2 * m_NumberOfLayers + 1) - 2, 1, (up_search + 1) / 2, ThreadId);
  this->ThreadedProcessOutsideList(k, (2 * m_NumberOfLayers + 1) - 1, 0, (up_search + 1) / 2, ThreadId);

  if (m_OutputImage->GetImageDimension() < 3)
  {
    this->SignalNeighborsAndWait(ThreadId);
  }

  // A synchronize is NOT required here because in 3D case we have at least 7
  // layers, thus ThreadedProcessOutsideList() works on layers 5 & 6 while
  // ThreadedPropagateLayerValues() works on 0, 1, 2, 3, 4 only. => There can
  // NOT be any dependencies among different threads.

  // Finally, we update all of the layer VALUES (excluding the active layer,
  // which has already been updated)
  this->ThreadedPropagateLayerValues(0, 1, 3, 1, ThreadId); // first inside
  this->ThreadedPropagateLayerValues(0, 2, 4, 0, ThreadId); // first outside

  this->SignalNeighborsAndWait(ThreadId);

  // Update the rest of the layer values
  const unsigned int N = (2 * static_cast<unsigned int>(m_NumberOfLayers) + 1) - 2;

  for (unsigned int i = 1; i < N; i += 2)
  {
    j = i + 1;
    this->ThreadedPropagateLayerValues(i, i + 2, i + 4, 1, ThreadId);
    this->ThreadedPropagateLayerValues(j, j + 2, j + 4, 0, ThreadId);
    this->SignalNeighborsAndWait(ThreadId);
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ThreadedUpdateActiveLayerValues(
  const TimeStepType & dt,
  LayerType *          UpList,
  LayerType *          DownList,
  ThreadIdType         ThreadId)
{
  // This method scales the update buffer values by the time step and adds
  // them to the active layer pixels.  New values at an index which fall
  // outside of the active layer range trigger that index to be placed on the
  // "up" or "down" status list.  The neighbors of any such index are then
  // assigned new values if they are determined to be part of the active list
  // for the next iteration (i.e. their values will be raised or lowered into
  // the active range).
  const ValueType LOWER_ACTIVE_THRESHOLD = -(m_ConstantGradientValue / 2.0);
  const ValueType UPPER_ACTIVE_THRESHOLD = m_ConstantGradientValue / 2.0;

  typename TOutputImage::SizeValueType counter = 0;

  float rms_change_accumulator = m_ValueZero;

  const unsigned int Neighbor_Size = m_NeighborList.GetSize();

  typename LayerType::Iterator       layerIt = m_Data[ThreadId].m_Layers[0]->Begin();
  const typename LayerType::Iterator layerEnd = m_Data[ThreadId].m_Layers[0]->End();

  while (layerIt != layerEnd)
  {
    const auto centerIndex = layerIt->m_Index;
    const auto centerValue = m_OutputImage->GetPixel(centerIndex);

    const float new_value =
      this->ThreadedCalculateUpdateValue(ThreadId, centerIndex, dt, centerValue, layerIt->m_Value);

    // If this index needs to be moved to another layer, then search its
    // neighborhood for indices that need to be pulled up/down into the
    // active layer. Set those new active layer values appropriately,
    // checking first to make sure they have not been set by a more
    // influential neighbor.

    //   ...But first make sure any neighbors in the active layer are not
    // moving to a layer in the opposite direction.  This step is necessary
    // to avoid the creation of holes in the active layer.  The fix is simply
    // to not change this value and leave the index in the active set.

    if (new_value > UPPER_ACTIVE_THRESHOLD)
    {
      // This index will move UP into a positive (outside) layer
      // First check for active layer neighbors moving in the opposite
      // direction
      bool flag = false;
      for (unsigned int i = 0; i < Neighbor_Size; ++i)
      {
        if (m_StatusImage->GetPixel(centerIndex + m_NeighborList.GetNeighborhoodOffset(i)) ==
            m_StatusActiveChangingDown)
        {
          flag = true;
          break;
        }
      }
      if (flag)
      {
        ++layerIt;
        continue;
      }

      rms_change_accumulator += itk::Math::sqr(static_cast<float>(new_value - centerValue));
      // update the value of the pixel
      m_OutputImage->SetPixel(centerIndex, new_value);

      // Now remove this index from the active list.
      auto release_node = layerIt.GetPointer();
      ++layerIt;

      m_Data[ThreadId].m_Layers[0]->Unlink(release_node);
      m_Data[ThreadId].m_ZHistogram[release_node->m_Index[m_SplitAxis]] =
        m_Data[ThreadId].m_ZHistogram[release_node->m_Index[m_SplitAxis]] - 1;

      // add the release_node to status up list
      UpList->PushFront(release_node);

      //
      m_StatusImage->SetPixel(centerIndex, m_StatusActiveChangingUp);
    }
    else if (new_value < LOWER_ACTIVE_THRESHOLD)
    {
      // This index will move DOWN into a negative (inside) layer.
      // First check for active layer neighbors moving in the opposite direction
      bool flag = false;
      for (unsigned int i = 0; i < Neighbor_Size; ++i)
      {
        if (m_StatusImage->GetPixel(centerIndex + m_NeighborList.GetNeighborhoodOffset(i)) == m_StatusActiveChangingUp)
        {
          flag = true;
          break;
        }
      }
      if (flag)
      {
        ++layerIt;
        continue;
      }

      rms_change_accumulator += itk::Math::sqr(static_cast<float>(new_value - centerValue));
      // update the value of the pixel
      m_OutputImage->SetPixel(centerIndex, new_value);

      // Now remove this index from the active list.
      auto release_node = layerIt.GetPointer();
      ++layerIt;

      m_Data[ThreadId].m_Layers[0]->Unlink(release_node);
      m_Data[ThreadId].m_ZHistogram[release_node->m_Index[m_SplitAxis]] =
        m_Data[ThreadId].m_ZHistogram[release_node->m_Index[m_SplitAxis]] - 1;

      // now add release_node to status down list
      DownList->PushFront(release_node);

      m_StatusImage->SetPixel(centerIndex, m_StatusActiveChangingDown);
    }
    else
    {
      rms_change_accumulator += itk::Math::sqr(static_cast<float>(new_value - centerValue));
      // update the value of the pixel
      m_OutputImage->SetPixel(centerIndex, new_value);
      ++layerIt;
    }
    ++counter;
  }

  // Determine the average change during this iteration
  if (counter == 0)
  {
    m_Data[ThreadId].m_RMSChange = m_ValueZero;
  }
  else
  {
    m_Data[ThreadId].m_RMSChange = rms_change_accumulator;
  }

  m_Data[ThreadId].m_Count = counter;
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::CopyInsertList(ThreadIdType     ThreadId,
                                                                                  LayerPointerType FromListPtr,
                                                                                  LayerPointerType ToListPtr)
{
  typename LayerType::Iterator       layerIt = FromListPtr->Begin();
  const typename LayerType::Iterator layerEnd = FromListPtr->End();

  while (layerIt != layerEnd)
  {
    // copy the node
    auto nodePtr = layerIt.GetPointer();
    ++layerIt;

    auto nodeTempPtr = m_Data[ThreadId].m_LayerNodeStore->Borrow();
    nodeTempPtr->m_Index = nodePtr->m_Index;

    // insert
    ToListPtr->PushFront(nodeTempPtr);
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ClearList(ThreadIdType     ThreadId,
                                                                             LayerPointerType ListPtr)
{
  while (!ListPtr->Empty())
  {
    auto nodePtr = ListPtr->Front();
    // remove node from layer
    ListPtr->PopFront();
    // return node to node-pool
    m_Data[ThreadId].m_LayerNodeStore->Return(nodePtr);
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::CopyInsertInterNeighborNodeTransferBufferLayers(
  ThreadIdType     ThreadId,
  LayerPointerType List,
  unsigned int     InOrOut,
  unsigned int     BufferLayerNumber)
{
  if (ThreadId != 0)
  {
    CopyInsertList(ThreadId,
                   m_Data[this->GetThreadNumber(m_Boundary[ThreadId - 1])]
                     .m_InterNeighborNodeTransferBufferLayers[InOrOut][BufferLayerNumber][ThreadId],
                   List);
  }

  if (m_Boundary[ThreadId] != m_ZSize - 1)
  {
    CopyInsertList(ThreadId,
                   m_Data[this->GetThreadNumber(m_Boundary[ThreadId] + 1)]
                     .m_InterNeighborNodeTransferBufferLayers[InOrOut][BufferLayerNumber][ThreadId],
                   List);
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ClearInterNeighborNodeTransferBufferLayers(
  ThreadIdType ThreadId,
  unsigned int InOrOut,
  unsigned int BufferLayerNumber)
{
  for (unsigned int i = 0; i < m_NumOfWorkUnits; ++i)
  {
    ClearList(ThreadId, m_Data[ThreadId].m_InterNeighborNodeTransferBufferLayers[InOrOut][BufferLayerNumber][i]);
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ThreadedProcessFirstLayerStatusLists(
  unsigned int       InputLayerNumber,
  unsigned int       OutputLayerNumber,
  const StatusType & SearchForStatus,
  unsigned int       InOrOut,
  unsigned int       BufferLayerNumber,
  ThreadIdType       ThreadId)
{
  const unsigned int neighbor_Size = m_NeighborList.GetSize();

  // InOrOut == 1, inside, more negative, uplist
  // InOrOut == 0, outside
  StatusType       from = 1;
  ValueType        delta = m_ConstantGradientValue;
  LayerPointerType InputList;
  LayerPointerType OutputList;
  if (InOrOut == 1)
  {
    delta = -m_ConstantGradientValue;
    from = 2;
    InputList = m_Data[ThreadId].UpList[InputLayerNumber];
    OutputList = m_Data[ThreadId].UpList[OutputLayerNumber];
  }
  else
  {
    InputList = m_Data[ThreadId].DownList[InputLayerNumber];
    OutputList = m_Data[ThreadId].DownList[OutputLayerNumber];
  }

  // 1. nothing to clear
  // 2. make a copy of the node on the
  //    m_InterNeighborNodeTransferBufferLayers[InOrOut][BufferLayerNumber -
  // 1][i]
  //    for all neighbors i ... and insert it in one's own InputList
  CopyInsertInterNeighborNodeTransferBufferLayers(ThreadId, InputList, InOrOut, BufferLayerNumber - 1);

  typename LayerType::Iterator       layerIt = InputList->Begin();
  const typename LayerType::Iterator layerEnd = InputList->End();
  while (layerIt != layerEnd)
  {
    auto nodePtr = layerIt.GetPointer();
    ++layerIt;

    const auto center_index = nodePtr->m_Index;

    // remove node from InputList
    InputList->Unlink(nodePtr);

    // check if this is not a duplicate pixel in the InputList
    // In the case when the thread boundaries differ by just 1 pixel some
    // nodes may get added twice in the InputLists Even if the boundaries are
    // more than 1 pixel wide the *_shape_* of the layer may allow this to
    // happen. Solution: If a pixel comes multiple times than we would find
    // that the Status image would already be reflecting the new status after
    // the pixel was encountered the first time
    if (m_StatusImage->GetPixel(center_index) == 0)
    {
      // duplicate node => return it to the node pool
      m_Data[ThreadId].m_LayerNodeStore->Return(nodePtr);
      continue;
    }

    // set status to zero
    m_StatusImage->SetPixel(center_index, 0);
    // add node to the layer-0
    m_Data[ThreadId].m_Layers[0]->PushFront(nodePtr);

    m_Data[ThreadId].m_ZHistogram[nodePtr->m_Index[m_SplitAxis]] =
      m_Data[ThreadId].m_ZHistogram[nodePtr->m_Index[m_SplitAxis]] + 1;

    auto value = m_OutputImage->GetPixel(center_index);
    bool found_neighbor_flag = false;
    for (unsigned int i = 0; i < neighbor_Size; ++i)
    {
      const auto n_index = center_index + m_NeighborList.GetNeighborhoodOffset(i);
      const auto neighbor_status = m_StatusImage->GetPixel(n_index);

      // Have we bumped up against the boundary?  If so, turn on bounds
      // checking.
      if (neighbor_status == m_StatusBoundaryPixel)
      {
        m_BoundsCheckingActive = true;
      }

      if (neighbor_status == from)
      {
        const auto value_temp = m_OutputImage->GetPixel(n_index);

        if (found_neighbor_flag == false)
        {
          value = value_temp;
        }
        else
        {
          if (itk::Math::abs(value_temp + delta) < itk::Math::abs(value + delta))
          {
            // take the value closest to zero
            value = value_temp;
          }
        }
        found_neighbor_flag = true;
      }

      if (neighbor_status == SearchForStatus)
      {
        // mark this pixel so we MAY NOT add it twice
        // This STILL DOES NOT GUARANTEE RACE CONDITIONS BETWEEN THREADS. This
        // is handled at the next stage
        m_StatusImage->SetPixel(n_index, m_StatusChanging);

        const unsigned int tmpId = this->GetThreadNumber(n_index[m_SplitAxis]);

        nodePtr = m_Data[ThreadId].m_LayerNodeStore->Borrow();
        nodePtr->m_Index = n_index;

        if (tmpId != ThreadId)
        {
          m_Data[ThreadId].m_InterNeighborNodeTransferBufferLayers[InOrOut][BufferLayerNumber][tmpId]->PushFront(
            nodePtr);
        }
        else
        {
          OutputList->PushFront(nodePtr);
        }
      }
    }
    m_OutputImage->SetPixel(center_index, value + delta);
    // This function can be overridden in the derived classes to process pixels
    // entering the active layer.
    this->ThreadedProcessPixelEnteringActiveLayer(center_index, value + delta, ThreadId);
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ThreadedProcessPixelEnteringActiveLayer(
  const IndexType & itkNotUsed(index),
  const ValueType & itkNotUsed(value),
  ThreadIdType      itkNotUsed(ThreadId))
{
  // This function can be overridden in the derived classes to process pixels
  // entering the active layer.
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ThreadedProcessStatusList(
  unsigned int       InputLayerNumber,
  unsigned int       OutputLayerNumber,
  const StatusType & ChangeToStatus,
  const StatusType & SearchForStatus,
  unsigned int       InOrOut,
  unsigned int       BufferLayerNumber,
  ThreadIdType       ThreadId)
{
  // Push each index in the input list into its appropriate status layer
  // (ChangeToStatus) and update the status image value at that index.
  // Also examine the neighbors of the index to determine which need to go onto
  // the output list (search for SearchForStatus).
  LayerPointerType InputList;
  LayerPointerType OutputList;
  if (InOrOut == 1)
  {
    InputList = m_Data[ThreadId].UpList[InputLayerNumber];
    OutputList = m_Data[ThreadId].UpList[OutputLayerNumber];
  }
  else
  {
    InputList = m_Data[ThreadId].DownList[InputLayerNumber];
    OutputList = m_Data[ThreadId].DownList[OutputLayerNumber];
  }

  // 1. clear one's own
  // m_InterNeighborNodeTransferBufferLayers[InOrOut][BufferLayerNumber - 2][i]
  // for all threads i.
  if (BufferLayerNumber >= 2)
  {
    ClearInterNeighborNodeTransferBufferLayers(ThreadId, InOrOut, BufferLayerNumber - 2);
  }
  // SPECIAL CASE: clear one's own
  // m_InterNeighborNodeTransferBufferLayers[InOrOut][m_NumberOfLayers][i] for
  // all threads i
  if (BufferLayerNumber == 0)
  {
    ClearInterNeighborNodeTransferBufferLayers(ThreadId, InOrOut, m_NumberOfLayers);
  }
  // obtain the pixels (from last iteration) that were given to you from other
  // (neighboring) threads 2. make a copy of the node on the
  // m_InterNeighborNodeTransferBufferLayers[InOrOut][LastLayer - 1][i] for all
  // thread neighbors i ... ... and insert it in one's own InputList
  if (BufferLayerNumber > 0)
  {
    CopyInsertInterNeighborNodeTransferBufferLayers(ThreadId, InputList, InOrOut, BufferLayerNumber - 1);
  }

  const unsigned int neighbor_size = m_NeighborList.GetSize();
  while (!InputList->Empty())
  {
    LayerNodeType * nodePtr = InputList->Front();
    const IndexType center_index = nodePtr->m_Index;

    InputList->PopFront();

    // Check if this is not a duplicate pixel in the InputList.
    // Solution: If a pixel comes multiple times than we would find that the
    // Status image would already be reflecting
    // the new status after the pixel was encountered the first time
    if ((BufferLayerNumber != 0) && (m_StatusImage->GetPixel(center_index) == ChangeToStatus))
    {
      // duplicate node => return it to the node pool
      m_Data[ThreadId].m_LayerNodeStore->Return(nodePtr);

      continue;
    }

    // add to layer
    m_Data[ThreadId].m_Layers[ChangeToStatus]->PushFront(nodePtr);
    // change the status
    m_StatusImage->SetPixel(center_index, ChangeToStatus);

    for (unsigned int i = 0; i < neighbor_size; ++i)
    {
      IndexType n_index = center_index + m_NeighborList.GetNeighborhoodOffset(i);

      const StatusType neighbor_status = m_StatusImage->GetPixel(n_index);

      // Have we bumped up against the boundary?  If so, turn on bounds
      // checking.
      if (neighbor_status == m_StatusBoundaryPixel)
      {
        m_BoundsCheckingActive = true;
      }

      if (neighbor_status == SearchForStatus)
      {
        // mark this pixel so we MAY NOT add it twice
        // This STILL DOES NOT AVOID RACE CONDITIONS BETWEEN THREADS (This is
        // handled at the next stage)
        m_StatusImage->SetPixel(n_index, m_StatusChanging);

        const unsigned int tmpId = this->GetThreadNumber(n_index[m_SplitAxis]);

        nodePtr = m_Data[ThreadId].m_LayerNodeStore->Borrow();
        nodePtr->m_Index = n_index;

        if (tmpId != ThreadId)
        {
          m_Data[ThreadId].m_InterNeighborNodeTransferBufferLayers[InOrOut][BufferLayerNumber][tmpId]->PushFront(
            nodePtr);
        }
        else
        {
          OutputList->PushFront(nodePtr);
        }
      }
    }
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ThreadedProcessOutsideList(
  unsigned int       InputLayerNumber,
  const StatusType & ChangeToStatus,
  unsigned int       InOrOut,
  unsigned int       BufferLayerNumber,
  ThreadIdType       ThreadId)
{
  LayerPointerType OutsideList;
  if (InOrOut == 1)
  {
    OutsideList = m_Data[ThreadId].UpList[InputLayerNumber];
  }
  else
  {
    OutsideList = m_Data[ThreadId].DownList[InputLayerNumber];
  }

  // obtain the pixels (from last iteration of ThreadedProcessStatusList() )
  // that were given to you from other (neighboring) threads
  // 1. clear one's own
  //    m_InterNeighborNodeTransferBufferLayers[InOrOut][BufferLayerNumber -
  // 2][i]
  //    for all threads i.
  ClearInterNeighborNodeTransferBufferLayers(ThreadId, InOrOut, BufferLayerNumber - 2);

  // 2. make a copy of the node on the
  //    m_InterNeighborNodeTransferBufferLayers[InOrOut][LastLayer - 1][i] for
  //    all thread neighbors i ... ... and insert it in one's own InOutList
  CopyInsertInterNeighborNodeTransferBufferLayers(ThreadId, OutsideList, InOrOut, BufferLayerNumber - 1);

  // Push each index in the input list into its appropriate status layer
  // (ChangeToStatus) and ... ... update the status image value at that index
  while (!OutsideList->Empty())
  {
    LayerNodeType * nodePtr = OutsideList->Front();
    OutsideList->PopFront();

    m_StatusImage->SetPixel(nodePtr->m_Index, ChangeToStatus);
    m_Data[ThreadId].m_Layers[ChangeToStatus]->PushFront(nodePtr);
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ThreadedPropagateLayerValues(
  const StatusType & from,
  const StatusType & to,
  const StatusType & promote,
  unsigned int       InOrOut,
  ThreadIdType       ThreadId)
{
  const StatusType past_end = static_cast<StatusType>(m_Layers.size()) - 1;

  // Are we propagating values inward (more negative) or outward (more positive)?
  const ValueType delta = (InOrOut == 1) ? -m_ConstantGradientValue : m_ConstantGradientValue;

  const unsigned int                 Neighbor_Size = m_NeighborList.GetSize();
  typename LayerType::Iterator       toIt = m_Data[ThreadId].m_Layers[to]->Begin();
  const typename LayerType::Iterator toEnd = m_Data[ThreadId].m_Layers[to]->End();

  while (toIt != toEnd)
  {
    const IndexType centerIndex = toIt->m_Index;

    const StatusType centerStatus = m_StatusImage->GetPixel(centerIndex);

    if (centerStatus != to)
    {
      // delete nodes NOT deleted earlier
      LayerNodeType * nodePtr = toIt.GetPointer();
      ++toIt;

      // remove the node from the layer
      m_Data[ThreadId].m_Layers[to]->Unlink(nodePtr);
      m_Data[ThreadId].m_LayerNodeStore->Return(nodePtr);
      continue;
    }

    ValueType value = m_ValueZero;
    bool      found_neighbor_flag = false;
    for (unsigned int i = 0; i < Neighbor_Size; ++i)
    {
      const IndexType  nIndex = centerIndex + m_NeighborList.GetNeighborhoodOffset(i);
      const StatusType nStatus = m_StatusImage->GetPixel(nIndex);
      // If this neighbor is in the "from" list, compare its absolute value
      // to any previous values found in the "from" list.  Keep only the
      // value with the smallest magnitude.

      if (nStatus == from)
      {
        const ValueType value_temp = m_OutputImage->GetPixel(nIndex);

        if (found_neighbor_flag == false)
        {
          value = value_temp;
        }
        else
        {
          if (itk::Math::abs(value_temp + delta) < itk::Math::abs(value + delta))
          {
            // take the value closest to zero
            value = value_temp;
          }
        }
        found_neighbor_flag = true;
      }
    }
    if (found_neighbor_flag)
    {
      // Set the new value using the smallest magnitude found in our "from"
      // neighbors
      m_OutputImage->SetPixel(centerIndex, value + delta);
      ++toIt;
    }
    else
    {
      // Did not find any neighbors on the "from" list, then promote this
      // node.  A "promote" value past the end of my sparse field size
      // means delete the node instead.  Change the status value in the
      // status image accordingly.
      LayerNodeType * nodePtr = toIt.GetPointer();
      ++toIt;
      m_Data[ThreadId].m_Layers[to]->Unlink(nodePtr);

      if (promote > past_end)
      {
        m_Data[ThreadId].m_LayerNodeStore->Return(nodePtr);
        m_StatusImage->SetPixel(centerIndex, m_StatusNull);
      }
      else
      {
        m_Data[ThreadId].m_Layers[promote]->PushFront(nodePtr);
        m_StatusImage->SetPixel(centerIndex, promote);
      }
    }
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::CheckLoadBalance()
{
  // This parameter defines a degree of unbalancedness of the load among
  // threads.
  constexpr float MAX_PIXEL_DIFFERENCE_PERCENT = 0.025;

  m_BoundaryChanged = false;

  // work load division based on the nodes on the active layer (layer-0)
  using NodeCounterType = IndexValueType;
  NodeCounterType min = NumericTraits<NodeCounterType>::max();
  NodeCounterType max = 0;
  NodeCounterType total = 0; // the total nodes in the active layer of the surface

  for (unsigned int i = 0; i < m_NumOfWorkUnits; ++i)
  {
    const NodeCounterType count = m_Data[i].m_Layers[0]->Size();
    total += count;
    if (min > count)
    {
      min = count;
    }
    if (max < count)
    {
      max = count;
    }
  }

  if (max - min < MAX_PIXEL_DIFFERENCE_PERCENT * total / m_NumOfWorkUnits)
  {
    // if the difference between max and min is NOT even x% of the average
    // nodes in the thread layers then no need to change the boundaries next
    return;
  }

  // Change the boundaries --------------------------

  // compute the global histogram from the individual histograms
  for (unsigned int i = 0; i < m_NumOfWorkUnits; ++i)
  {
    for (unsigned int j = (i == 0 ? 0 : m_Boundary[i - 1] + 1); j <= m_Boundary[i]; ++j)
    {
      m_GlobalZHistogram[j] = m_Data[i].m_ZHistogram[j];
    }
  }

  // compute the cumulative frequency distribution using the histogram
  m_ZCumulativeFrequency[0] = m_GlobalZHistogram[0];
  for (unsigned int i = 1; i < m_ZSize; ++i)
  {
    m_ZCumulativeFrequency[i] = m_ZCumulativeFrequency[i - 1] + m_GlobalZHistogram[i];
  }

  // now define the boundaries
  m_Boundary[m_NumOfWorkUnits - 1] = m_ZSize - 1; // special case: the last bound

  for (unsigned int i = 0; i < m_NumOfWorkUnits - 1; ++i)
  {
    // compute m_Boundary[i]
    const float cutOff = 1.0f * (i + 1) * m_ZCumulativeFrequency[m_ZSize - 1] / m_NumOfWorkUnits;

    // find the position in the cumulative freq dist where this cutoff is met
    for (unsigned int j = (i == 0 ? 0 : m_Boundary[i - 1]); j < m_ZSize; ++j)
    {
      if (cutOff > m_ZCumulativeFrequency[j])
      {
        continue;
      }

      // do some optimization !
      // go further FORWARD and find the first index (k) in the cumulative
      // freq distribution s.t. m_ZCumulativeFrequency[k] !=
      // m_ZCumulativeFrequency[j]. This is to be done because if we have a
      // flat patch in the cum freq dist then ... . we can choose a bound
      // midway in that flat patch
      unsigned int k = 1;
      for (; j + k < m_ZSize; ++k)
      {
        if (m_ZCumulativeFrequency[j + k] != m_ZCumulativeFrequency[j])
        {
          break;
        }
      }

      // if ALL new boundaries same as the original then NO NEED TO DO
      // ThreadedLoadBalance() next !!!
      auto newBoundary = static_cast<unsigned int>((j + (j + k)) / 2);
      if (newBoundary != m_Boundary[i])
      {
        //
        m_BoundaryChanged = true;
        m_Boundary[i] = newBoundary;
      }
      break;
    }
  }

  if (m_BoundaryChanged == false)
  {
    return;
  }

  // Reset the individual histograms to reflect the new distribution
  // Also reset the mapping from the Z value --> the thread number i.e.
  // m_MapZToThreadNumber[]
  for (unsigned int i = 0; i < m_NumOfWorkUnits; ++i)
  {
    if (i != 0)
    {
      for (unsigned int j = 0; j <= m_Boundary[i - 1]; ++j)
      {
        m_Data[i].m_ZHistogram[j] = 0;
      }
    }

    for (unsigned int j = (i == 0 ? 0 : m_Boundary[i - 1] + 1); j <= m_Boundary[i]; ++j)
    {
      // this Z histogram value should be given to thread-i
      m_Data[i].m_ZHistogram[j] = m_GlobalZHistogram[j];

      // this Z belongs to the region associated with thread-i
      m_MapZToThreadNumber[j] = i;
    }

    for (unsigned int j = m_Boundary[i] + 1; j < m_ZSize; ++j)
    {
      m_Data[i].m_ZHistogram[j] = 0;
    }
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ThreadedLoadBalance1(ThreadIdType ThreadId)
{
  // cleanup the layers first
  for (unsigned int i = 0; i < 2 * static_cast<unsigned int>(m_NumberOfLayers) + 1; ++i)
  {
    for (ThreadIdType tid = 0; tid < m_NumOfWorkUnits; ++tid)
    {
      if (tid == ThreadId)
      {
        // a thread does NOT pass nodes to itself
        continue;
      }

      ClearList(ThreadId, m_Data[ThreadId].m_LoadTransferBufferLayers[i][tid]);
    }
  }

  // for all layers
  for (unsigned int i = 0; i < 2 * static_cast<unsigned int>(m_NumberOfLayers) + 1; ++i)
  {
    typename LayerType::Iterator       layerIt = m_Data[ThreadId].m_Layers[i]->Begin();
    const typename LayerType::Iterator layerEnd = m_Data[ThreadId].m_Layers[i]->End();

    while (layerIt != layerEnd)
    {
      LayerNodeType * nodePtr = layerIt.GetPointer();
      ++layerIt;

      // use the latest (just updated in CheckLoadBalance) boundaries to
      // determine to which thread region does the pixel now belong
      const ThreadIdType tmpId = this->GetThreadNumber(nodePtr->m_Index[m_SplitAxis]);

      if (tmpId != ThreadId) // this pixel no longer belongs to this thread
      {
        // remove from the layer
        m_Data[ThreadId].m_Layers[i]->Unlink(nodePtr);

        // insert temporarily into the special-layers TO BE LATER taken by the
        // other thread
        // NOTE: What is pushed is a node belonging to the LayerNodeStore of
        // ThreadId. This is deleted later (during the start of the next
        // SpecialIteration).  What is taken by the other thread is NOT this
        // node BUT a copy of it.
        m_Data[ThreadId].m_LoadTransferBufferLayers[i][tmpId]->PushFront(nodePtr);
      }
    }
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ThreadedLoadBalance2(ThreadIdType ThreadId)
{
  for (unsigned int i = 0; i < 2 * static_cast<unsigned int>(m_NumberOfLayers) + 1; ++i)
  {
    // check all other threads
    for (ThreadIdType tid = 0; tid < m_NumOfWorkUnits; ++tid)
    {
      if (tid == ThreadId)
      {
        continue; // exclude oneself
      }

      CopyInsertList(ThreadId, m_Data[tid].m_LoadTransferBufferLayers[i][ThreadId], m_Data[ThreadId].m_Layers[i]);
    }
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::GetThreadRegionSplitByBoundary(
  ThreadIdType       ThreadId,
  ThreadRegionType & ThreadRegion)
{
  // Initialize the ThreadRegion to the output's requested region
  ThreadRegion = m_OutputImage->GetRequestedRegion();

  // compute lower bound on the index
  typename TOutputImage::IndexType threadRegionIndex = ThreadRegion.GetIndex();
  if (ThreadId != 0)
  {
    if (m_Boundary[ThreadId - 1] < m_Boundary[m_NumOfWorkUnits - 1])
    {
      threadRegionIndex[m_SplitAxis] += m_Boundary[ThreadId - 1] + 1;
    }
    else
    {
      threadRegionIndex[m_SplitAxis] += m_Boundary[ThreadId - 1];
    }
  }

  ThreadRegion.SetIndex(threadRegionIndex);

  // compute the size of the region
  typename TOutputImage::SizeType threadRegionSize = ThreadRegion.GetSize();
  threadRegionSize[m_SplitAxis] =
    (ThreadId == 0 ? (m_Boundary[0] + 1) : m_Boundary[ThreadId] - m_Boundary[ThreadId - 1]);
  ThreadRegion.SetSize(threadRegionSize);
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::GetThreadRegionSplitUniformly(
  ThreadIdType       ThreadId,
  ThreadRegionType & ThreadRegion)
{
  // Initialize the ThreadRegion to the output's requested region
  ThreadRegion = m_OutputImage->GetRequestedRegion();

  typename TOutputImage::IndexType threadRegionIndex = ThreadRegion.GetIndex();
  threadRegionIndex[m_SplitAxis] += static_cast<unsigned int>(1.0 * ThreadId * m_ZSize / m_NumOfWorkUnits);
  ThreadRegion.SetIndex(threadRegionIndex);

  typename TOutputImage::SizeType threadRegionSize = ThreadRegion.GetSize();

  // compute lower bound on the index and the size of the region
  if (ThreadId < m_NumOfWorkUnits - 1) // this is NOT the last thread
  {
    threadRegionSize[m_SplitAxis] = static_cast<unsigned int>(1.0 * (ThreadId + 1) * m_ZSize / m_NumOfWorkUnits) -
                                    static_cast<unsigned int>(1.0 * ThreadId * m_ZSize / m_NumOfWorkUnits);
  }
  else
  {
    threadRegionSize[m_SplitAxis] = m_ZSize - static_cast<unsigned int>(1.0 * ThreadId * m_ZSize / m_NumOfWorkUnits);
  }
  ThreadRegion.SetSize(threadRegionSize);
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::ThreadedPostProcessOutput(
  const ThreadRegionType & regionToProcess)
{
  // Assign background pixels INSIDE the sparse field layers to a new level set
  // with value less than the innermost layer.  Assign background pixels
  // OUTSIDE the sparse field layers to a new level set with value greater than
  // the outermost layer.
  const auto      max_layer = static_cast<ValueType>(m_NumberOfLayers);
  const ValueType outside_value = (max_layer + 1) * m_ConstantGradientValue;
  const ValueType inside_value = -(max_layer + 1) * m_ConstantGradientValue;

  ImageRegionConstIterator<StatusImageType> statusIt(m_StatusImage, regionToProcess);
  ImageRegionIterator<OutputImageType>      outputIt(m_OutputImage, regionToProcess);

  for (outputIt.GoToBegin(), statusIt.GoToBegin(); !outputIt.IsAtEnd(); ++outputIt, ++statusIt)
  {
    if (statusIt.Get() == m_StatusNull || statusIt.Get() == m_StatusBoundaryPixel)
    {
      if (outputIt.Get() > m_ValueZero)
      {
        outputIt.Set(outside_value);
      }
      else
      {
        outputIt.Set(inside_value);
      }
    }
  }
}

template <typename TInputImage, typename TOutputImage>
unsigned int
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::GetThreadNumber(unsigned int splitAxisValue)
{
  return (m_MapZToThreadNumber[splitAxisValue]);
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::SignalNeighborsAndWait(ThreadIdType ThreadId)
{
  // This is the case when a thread has no pixels to process
  // This case is analogous to NOT using that thread at all
  // Hence this thread does not need to signal / wait for any other neighbor
  // thread during the iteration
  if (ThreadId != 0)
  {
    if (m_Boundary[ThreadId - 1] == m_Boundary[ThreadId])
    {
      m_Data[ThreadId].m_SemaphoreArrayNumber = 1 - m_Data[ThreadId].m_SemaphoreArrayNumber;
      return;
    }
  }

  const ThreadIdType lastThreadId = m_NumOfWorkUnits - 1;
  if (lastThreadId == 0)
  {
    return; // only 1 thread => no need to wait
  }

  // signal neighbors that work is done
  if (ThreadId != 0) // not the first thread
  {
    this->SignalNeighbor(m_Data[ThreadId].m_SemaphoreArrayNumber, this->GetThreadNumber(m_Boundary[ThreadId - 1]));
  }
  if (m_Boundary[ThreadId] != m_ZSize - 1) // not the last thread
  {
    this->SignalNeighbor(m_Data[ThreadId].m_SemaphoreArrayNumber, this->GetThreadNumber(m_Boundary[ThreadId] + 1));
  }

  // wait for signal from neighbors signifying that their work is done
  if ((ThreadId == 0) || (m_Boundary[ThreadId] == m_ZSize - 1))
  {
    // do it just once for the first and the last threads because they share
    // just 1 boundary (just 1 neighbor)
    this->WaitForNeighbor(m_Data[ThreadId].m_SemaphoreArrayNumber, ThreadId);
    m_Data[ThreadId].m_SemaphoreArrayNumber = 1 - m_Data[ThreadId].m_SemaphoreArrayNumber;
  }
  else
  {
    // do twice because share 2 boundaries with neighbors
    this->WaitForNeighbor(m_Data[ThreadId].m_SemaphoreArrayNumber, ThreadId);
    this->WaitForNeighbor(m_Data[ThreadId].m_SemaphoreArrayNumber, ThreadId);

    m_Data[ThreadId].m_SemaphoreArrayNumber = 1 - m_Data[ThreadId].m_SemaphoreArrayNumber;
  }
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::SignalNeighbor(unsigned int SemaphoreArrayNumber,
                                                                                  ThreadIdType ThreadId)
{
  ThreadData &                      td = m_Data[ThreadId];
  const std::lock_guard<std::mutex> lockGuard(td.m_Lock[SemaphoreArrayNumber]);
  ++td.m_Semaphore[SemaphoreArrayNumber];
  td.m_Condition[SemaphoreArrayNumber].notify_one();
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::WaitForNeighbor(unsigned int SemaphoreArrayNumber,
                                                                                   ThreadIdType ThreadId)
{
  ThreadData &                 td = m_Data[ThreadId];
  std::unique_lock<std::mutex> mutexHolder(td.m_Lock[SemaphoreArrayNumber]);
  if (td.m_Semaphore[SemaphoreArrayNumber] == 0)
  {
    td.m_Condition[SemaphoreArrayNumber].wait(
      mutexHolder, [&td, SemaphoreArrayNumber] { return (td.m_Semaphore[SemaphoreArrayNumber] != 0); });
  }
  --td.m_Semaphore[SemaphoreArrayNumber];
}

template <typename TInputImage, typename TOutputImage>
void
ParallelSparseFieldLevelSetImageFilter<TInputImage, TOutputImage>::PrintSelf(std::ostream & os, Indent indent) const
{
  using namespace print_helper;

  Superclass::PrintSelf(os, indent);

  m_NeighborList.Print(os, indent);

  os << indent << "ConstantGradientValue: " << m_ConstantGradientValue << std::endl;

  itkPrintSelfObjectMacro(ShiftedImage);

  os << indent << "Layers: " << m_Layers << std::endl;

  os << indent << "NumberOfLayers: " << static_cast<typename NumericTraits<StatusType>::PrintType>(m_NumberOfLayers)
     << std::endl;

  itkPrintSelfObjectMacro(StatusImage);
  itkPrintSelfObjectMacro(OutputImage);

  itkPrintSelfObjectMacro(StatusImageTemp);
  itkPrintSelfObjectMacro(OutputImageTemp);

  itkPrintSelfObjectMacro(LayerNodeStore);

  os << indent << "IsoSurfaceValue: " << static_cast<typename NumericTraits<ValueType>::PrintType>(m_IsoSurfaceValue)
     << std::endl;

  os << indent << "TimeStepList: " << m_TimeStepList << std::endl;
  os << indent << "ValidTimeStepList: " << m_ValidTimeStepList << std::endl;

  os << indent << "TimeStep: " << static_cast<typename NumericTraits<TimeStepType>::PrintType>(m_TimeStep) << std::endl;

  os << indent << "NumOfWorkUnits: " << static_cast<typename NumericTraits<ThreadIdType>::PrintType>(m_NumOfWorkUnits)
     << std::endl;

  os << indent << "SplitAxis: " << m_SplitAxis << std::endl;
  os << indent << "ZSize: " << m_ZSize << std::endl;
  itkPrintSelfBooleanMacro(BoundaryChanged);

  os << indent << "Boundary: ";
  if (m_Boundary != nullptr)
  {
    os << *m_Boundary << std::endl;
  }
  else
  {
    os << "(null)" << std::endl;
  }

  os << indent << "GlobalZHistogram: ";
  if (m_GlobalZHistogram != nullptr)
  {
    os << *m_GlobalZHistogram << std::endl;
  }
  else
  {
    os << "(null)" << std::endl;
  }

  os << indent << "MapZToThreadNumber: ";
  if (m_MapZToThreadNumber != nullptr)
  {
    os << *m_MapZToThreadNumber << std::endl;
  }
  else
  {
    os << "(null)" << std::endl;
  }

  os << indent << "ZCumulativeFrequency: ";
  if (m_ZCumulativeFrequency != nullptr)
  {
    os << *m_ZCumulativeFrequency << std::endl;
  }
  else
  {
    os << "(null)" << std::endl;
  }

  os << indent << "Data: ";
  for (ThreadIdType ThreadId = 0; ThreadId < m_NumOfWorkUnits; ++ThreadId)
  {
    os << indent << "ThreadId: " << ThreadId << std::endl;
    if (m_Data != nullptr)
    {
      for (unsigned int i = 0; i < m_Data[ThreadId].m_Layers.size(); ++i)
      {
        os << indent << "m_Layers[" << i << "] size: " << m_Data[ThreadId].m_Layers[i]->Size() << std::endl;
        os << indent << m_Data[ThreadId].m_Layers[i];
      }
    }
  }
  os << std::endl;

  itkPrintSelfBooleanMacro(Stop);
  itkPrintSelfBooleanMacro(InterpolateSurfaceLocation);
  itkPrintSelfBooleanMacro(BoundsCheckingActive);
}
} // end namespace itk

#endif
