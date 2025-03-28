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
#ifndef itkBinaryFunctorImageFilter_hxx
#define itkBinaryFunctorImageFilter_hxx

#include "itkImageScanlineIterator.h"
#include "itkTotalProgressReporter.h"


namespace itk
{

template <typename TInputImage1, typename TInputImage2, typename TOutputImage, typename TFunction>
BinaryFunctorImageFilter<TInputImage1, TInputImage2, TOutputImage, TFunction>::BinaryFunctorImageFilter()
{
  this->SetNumberOfRequiredInputs(2);
  this->InPlaceOff();
  this->DynamicMultiThreadingOn();
  this->ThreaderUpdateProgressOff();
}

template <typename TInputImage1, typename TInputImage2, typename TOutputImage, typename TFunction>
void
BinaryFunctorImageFilter<TInputImage1, TInputImage2, TOutputImage, TFunction>::SetInput1(const TInputImage1 * image1)
{
  // Process object is not const-correct so the const casting is required.
  this->SetNthInput(0, const_cast<TInputImage1 *>(image1));
}

template <typename TInputImage1, typename TInputImage2, typename TOutputImage, typename TFunction>
void
BinaryFunctorImageFilter<TInputImage1, TInputImage2, TOutputImage, TFunction>::SetInput1(
  const DecoratedInput1ImagePixelType * input1)
{
  // Process object is not const-correct so the const casting is required.
  this->SetNthInput(0, const_cast<DecoratedInput1ImagePixelType *>(input1));
}

template <typename TInputImage1, typename TInputImage2, typename TOutputImage, typename TFunction>
void
BinaryFunctorImageFilter<TInputImage1, TInputImage2, TOutputImage, TFunction>::SetInput1(
  const Input1ImagePixelType & input1)
{
  auto newInput = DecoratedInput1ImagePixelType::New();
  newInput->Set(input1);
  this->SetInput1(newInput);
}

template <typename TInputImage1, typename TInputImage2, typename TOutputImage, typename TFunction>
void
BinaryFunctorImageFilter<TInputImage1, TInputImage2, TOutputImage, TFunction>::SetConstant1(
  const Input1ImagePixelType & input1)
{
  this->SetInput1(input1);
}

template <typename TInputImage1, typename TInputImage2, typename TOutputImage, typename TFunction>
auto
BinaryFunctorImageFilter<TInputImage1, TInputImage2, TOutputImage, TFunction>::GetConstant1() const
  -> const Input1ImagePixelType &
{
  const auto * input = dynamic_cast<const DecoratedInput1ImagePixelType *>(this->ProcessObject::GetInput(0));
  if (input == nullptr)
  {
    itkExceptionMacro("Constant 1 is not set");
  }
  return input->Get();
}

template <typename TInputImage1, typename TInputImage2, typename TOutputImage, typename TFunction>
void
BinaryFunctorImageFilter<TInputImage1, TInputImage2, TOutputImage, TFunction>::SetInput2(const TInputImage2 * image2)
{
  // Process object is not const-correct so the const casting is required.
  this->SetNthInput(1, const_cast<TInputImage2 *>(image2));
}

template <typename TInputImage1, typename TInputImage2, typename TOutputImage, typename TFunction>
void
BinaryFunctorImageFilter<TInputImage1, TInputImage2, TOutputImage, TFunction>::SetInput2(
  const DecoratedInput2ImagePixelType * input2)
{
  // Process object is not const-correct so the const casting is required.
  this->SetNthInput(1, const_cast<DecoratedInput2ImagePixelType *>(input2));
}

template <typename TInputImage1, typename TInputImage2, typename TOutputImage, typename TFunction>
void
BinaryFunctorImageFilter<TInputImage1, TInputImage2, TOutputImage, TFunction>::SetInput2(
  const Input2ImagePixelType & input2)
{
  auto newInput = DecoratedInput2ImagePixelType::New();
  newInput->Set(input2);
  this->SetInput2(newInput);
}

template <typename TInputImage1, typename TInputImage2, typename TOutputImage, typename TFunction>
void
BinaryFunctorImageFilter<TInputImage1, TInputImage2, TOutputImage, TFunction>::SetConstant2(
  const Input2ImagePixelType & input2)
{
  this->SetInput2(input2);
}

template <typename TInputImage1, typename TInputImage2, typename TOutputImage, typename TFunction>
auto
BinaryFunctorImageFilter<TInputImage1, TInputImage2, TOutputImage, TFunction>::GetConstant2() const
  -> const Input2ImagePixelType &
{
  const auto * input = dynamic_cast<const DecoratedInput2ImagePixelType *>(this->ProcessObject::GetInput(1));
  if (input == nullptr)
  {
    itkExceptionMacro("Constant 2 is not set");
  }
  return input->Get();
}

template <typename TInputImage1, typename TInputImage2, typename TOutputImage, typename TFunction>
void
BinaryFunctorImageFilter<TInputImage1, TInputImage2, TOutputImage, TFunction>::GenerateOutputInformation()
{
  const DataObject *       input = nullptr;
  const Input1ImagePointer inputPtr1 = dynamic_cast<const TInputImage1 *>(ProcessObject::GetInput(0));
  const Input2ImagePointer inputPtr2 = dynamic_cast<const TInputImage2 *>(ProcessObject::GetInput(1));

  if (this->GetNumberOfInputs() >= 2)
  {
    if (inputPtr1)
    {
      input = inputPtr1;
    }
    else if (inputPtr2)
    {
      input = inputPtr2;
    }
    else
    {
      return;
    }

    for (unsigned int idx = 0; idx < this->GetNumberOfOutputs(); ++idx)
    {
      DataObject * output = this->GetOutput(idx);
      if (output)
      {
        output->CopyInformation(input);
      }
    }
  }
}

template <typename TInputImage1, typename TInputImage2, typename TOutputImage, typename TFunction>
void
BinaryFunctorImageFilter<TInputImage1, TInputImage2, TOutputImage, TFunction>::DynamicThreadedGenerateData(
  const OutputImageRegionType & outputRegionForThread)
{
  // We use dynamic_cast since inputs are stored as DataObjects. The
  // ImageToImageFilter::GetInput(int) always returns a pointer to a
  // TInputImage1 so it cannot be used for the second input.
  const auto *   inputPtr1 = dynamic_cast<const TInputImage1 *>(ProcessObject::GetInput(0));
  const auto *   inputPtr2 = dynamic_cast<const TInputImage2 *>(ProcessObject::GetInput(1));
  TOutputImage * outputPtr = this->GetOutput(0);

  TotalProgressReporter progress(this, outputPtr->GetRequestedRegion().GetNumberOfPixels());

  if (inputPtr1 && inputPtr2)
  {
    ImageScanlineConstIterator inputIt1(inputPtr1, outputRegionForThread);
    ImageScanlineConstIterator inputIt2(inputPtr2, outputRegionForThread);
    ImageScanlineIterator      outputIt(outputPtr, outputRegionForThread);

    while (!inputIt1.IsAtEnd())
    {
      while (!inputIt1.IsAtEndOfLine())
      {
        outputIt.Set(m_Functor(inputIt1.Get(), inputIt2.Get()));
        ++inputIt2;
        ++inputIt1;
        ++outputIt;
      }

      inputIt1.NextLine();
      inputIt2.NextLine();
      outputIt.NextLine();
      progress.Completed(outputRegionForThread.GetSize()[0]);
    }
  }
  else if (inputPtr1)
  {
    ImageScanlineConstIterator inputIt1(inputPtr1, outputRegionForThread);
    ImageScanlineIterator      outputIt(outputPtr, outputRegionForThread);

    const Input2ImagePixelType & input2Value = this->GetConstant2();

    while (!inputIt1.IsAtEnd())
    {
      while (!inputIt1.IsAtEndOfLine())
      {
        outputIt.Set(m_Functor(inputIt1.Get(), input2Value));
        ++inputIt1;
        ++outputIt;
      }
      inputIt1.NextLine();
      outputIt.NextLine();
      progress.Completed(outputRegionForThread.GetSize()[0]);
    }
  }
  else if (inputPtr2)
  {
    ImageScanlineConstIterator inputIt2(inputPtr2, outputRegionForThread);
    ImageScanlineIterator      outputIt(outputPtr, outputRegionForThread);

    const Input1ImagePixelType & input1Value = this->GetConstant1();

    while (!inputIt2.IsAtEnd())
    {
      while (!inputIt2.IsAtEndOfLine())
      {
        outputIt.Set(m_Functor(input1Value, inputIt2.Get()));
        ++inputIt2;
        ++outputIt;
      }
      inputIt2.NextLine();
      outputIt.NextLine();
      progress.Completed(outputRegionForThread.GetSize()[0]);
    }
  }
  else
  {
    itkGenericExceptionMacro("At most one of the inputs can be a constant.");
  }
}
} // end namespace itk

#endif
