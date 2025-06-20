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
/*=========================================================================
 *
 *  Portions of this file are subject to the VTK Toolkit Version 3 copyright.
 *
 *  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 *
 *  For complete copyright, license and disclaimer of warranty information
 *  please refer to the NOTICE file at the top of the ITK source tree.
 *
 *=========================================================================*/
#ifndef itkBSplineUpsampleImageFilter_hxx
#define itkBSplineUpsampleImageFilter_hxx

namespace itk
{

/**
 * Standard "PrintSelf" method
 */
template <typename TInputImage, typename TOutputImage, typename ResamplerType>
void
BSplineUpsampleImageFilter<TInputImage, TOutputImage, ResamplerType>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

template <typename TInputImage, typename TOutputImage, typename ResamplerType>
void
BSplineUpsampleImageFilter<TInputImage, TOutputImage, ResamplerType>::GenerateData()
{
  itkDebugMacro("Actually executing");

  // Get the input and output pointers
  const InputImagePointer  inputPtr = const_cast<TInputImage *>(this->GetInput());
  const OutputImagePointer outputPtr = this->GetOutput();

  // Since we are providing a GenerateData() method, we need to allocate the
  // output buffer memory (if we provided a ThreadedGenerateData(), then
  // the memory would have already been allocated for us).
  outputPtr->SetBufferedRegion(outputPtr->GetRequestedRegion());
  outputPtr->Allocate();

  // Iterator for walking the output region is defined in the Superclass
  OutputImageIterator outIt(outputPtr, outputPtr->GetRequestedRegion());

  // Calculate actual output
  // TODO:  Need to verify outIt is correctly sized.
  Superclass::ExpandNDImage(outIt);
}

/**
 *
 */
template <typename TInputImage, typename TOutputImage, typename ResamplerType>
void
BSplineUpsampleImageFilter<TInputImage, TOutputImage, ResamplerType>::GenerateInputRequestedRegion()
{
  // call the superclass' implementation of this method
  Superclass::GenerateInputRequestedRegion();

  // get pointers to the input and output
  const InputImagePointer  inputPtr = const_cast<TInputImage *>(this->GetInput());
  const OutputImagePointer outputPtr = this->GetOutput();

  if (!outputPtr || !inputPtr)
  {
    return;
  }

  inputPtr->SetRequestedRegionToLargestPossibleRegion();

  // Compute the input requested region (size and start index)
  const typename TOutputImage::SizeType &  outputRequestedRegionSize = outputPtr->GetRequestedRegion().GetSize();
  const typename TOutputImage::IndexType & outputRequestedRegionStartIndex = outputPtr->GetRequestedRegion().GetIndex();

  typename TInputImage::SizeType  inputRequestedRegionSize;
  typename TInputImage::IndexType inputRequestedRegionStartIndex;

  for (unsigned int i = 0; i < TInputImage::ImageDimension; ++i)
  {
    inputRequestedRegionSize[i] = outputRequestedRegionSize[i] / 2;
    inputRequestedRegionStartIndex[i] = outputRequestedRegionStartIndex[i] / 2;
  }

  const typename TInputImage::RegionType inputRequestedRegion(inputRequestedRegionStartIndex, inputRequestedRegionSize);

  inputPtr->SetRequestedRegion(inputRequestedRegion);
}

/**
 *
 */
template <typename TInputImage, typename TOutputImage, typename ResamplerType>
void
BSplineUpsampleImageFilter<TInputImage, TOutputImage, ResamplerType>::GenerateOutputInformation()
{
  // call the superclass' implementation of this method
  Superclass::GenerateOutputInformation();

  // get pointers to the input and output
  const InputImagePointer  inputPtr = const_cast<TInputImage *>(this->GetInput());
  const OutputImagePointer outputPtr = this->GetOutput();

  if (!outputPtr || !inputPtr)
  {
    return;
  }

  // we need to compute the output spacing, the output image size, and the
  // output image start index
  const typename TInputImage::SpacingType & inputSpacing = inputPtr->GetSpacing();
  const typename TInputImage::SizeType &    inputSize = inputPtr->GetLargestPossibleRegion().GetSize();
  const typename TInputImage::IndexType &   inputStartIndex = inputPtr->GetLargestPossibleRegion().GetIndex();

  typename TOutputImage::SpacingType outputSpacing;
  typename TOutputImage::SizeType    outputSize;
  typename TOutputImage::IndexType   outputStartIndex;

  for (unsigned int i = 0; i < TOutputImage::ImageDimension; ++i)
  {
    // TODO:  Verify this is being rounded correctly.
    outputSpacing[i] = inputSpacing[i] / 2.0;
    // TODO:  Verify this is being rounded correctly.
    outputSize[i] = static_cast<unsigned int>(std::floor(static_cast<double>(inputSize[i] * 2.0)));
    outputStartIndex[i] = static_cast<int>(std::ceil(static_cast<double>(inputStartIndex[i]) * 2.0));
  }

  outputPtr->SetSpacing(outputSpacing);

  const typename TOutputImage::RegionType outputLargestPossibleRegion(outputStartIndex, outputSize);

  outputPtr->SetLargestPossibleRegion(outputLargestPossibleRegion);
}

/*
 * EnlargeOutputRequestedRegion method.
 */
template <typename TInputImage, typename TOutputImage, typename ResamplerType>
void
BSplineUpsampleImageFilter<TInputImage, TOutputImage, ResamplerType>::EnlargeOutputRequestedRegion(DataObject * output)
{
  // this filter requires the all of the output image to be in
  // the buffer
  auto * imgData = dynamic_cast<TOutputImage *>(output);
  if (imgData)
  {
    imgData->SetRequestedRegionToLargestPossibleRegion();
  }
  else
  {
    // pointer could not be cast to TLevelSet *
    itkWarningMacro("itk::BSplineUpsampleImageFilter::EnlargeOutputRequestedRegion cannot cast "
                    << typeid(output).name() << " to " << typeid(TOutputImage *).name());
  }
}
} // namespace itk

#endif
