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

#include "itkPyCommand.h"

namespace
{
// Wrapper to automatics obtain and release GIL
// RAII idiom
class PyGILStateEnsure
{
public:
  PyGILStateEnsure() { m_GIL = PyGILState_Ensure(); }
  ~PyGILStateEnsure() { PyGILState_Release(m_GIL); }

private:
  PyGILState_STATE m_GIL;
};
} // end anonymous namespace

namespace itk
{

PyCommand::PyCommand()
{
  this->m_Object = nullptr;
  this->m_EmptyArgumentList = Py_BuildValue("()", 0);
}


PyCommand::~PyCommand()
{
  if (this->m_Object)
  {
    const PyGILStateEnsure gil;
    Py_DECREF(this->m_Object);
  }
  this->m_Object = nullptr;

  if (this->m_EmptyArgumentList)
  {
    const PyGILStateEnsure gil;
    Py_DECREF(this->m_EmptyArgumentList);
  }
  this->m_EmptyArgumentList = nullptr;
}


void
PyCommand::SetCommandCallable(PyObject * o)
{
  if (o != this->m_Object)
  {
    const PyGILStateEnsure gil;
    if (this->m_Object)
    {
      // get rid of our reference
      Py_DECREF(this->m_Object);
    }

    // store the new object
    this->m_Object = o;

    if (this->m_Object)
    {
      // take out reference (so that the calling code doesn't
      // have to keep a binding to the callable around)
      Py_INCREF(this->m_Object);
    }
  }
}


PyObject *
PyCommand::GetCommandCallable()
{
  return this->m_Object;
}


void
PyCommand::Execute(Object *, const EventObject &)
{
  this->PyExecute();
}


void
PyCommand::Execute(const Object *, const EventObject &)
{
  this->PyExecute();
}


void
PyCommand::PyExecute()
{
  // make sure that the CommandCallable is in fact callable
  if (!PyCallable_Check(this->m_Object))
  {
    // we throw a standard ITK exception: this makes it possible for
    // our standard Swig exception handling logic to take this
    // through to the invoking Python process
    itkExceptionMacro("CommandCallable is not a callable Python object, or it has not been set.");
  }
  else
  {
    const PyGILStateEnsure gil;
    PyObject *             result = PyObject_Call(this->m_Object, this->m_EmptyArgumentList, (PyObject *)nullptr);

    if (result)
    {
      Py_DECREF(result);
    }
    else
    {
      // there was a Python error.  Clear the error by printing to stdout
      PyErr_Print();
      // make sure the invoking Python code knows there was a problem
      // by raising an exception
      itkExceptionMacro("There was an error executing the CommandCallable.");
    }
  }
}

} // end namespace itk
