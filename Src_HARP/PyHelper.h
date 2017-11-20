// (c) 2014 Dominic Springer
// This file is licensed under the 2-Clause BSD License (see HARP_License.txt)

#pragma once

#include <Python.h>

#include "HARP_Defines.h"
#include "PyNDArray.h"
#include <stdlib.h>

//---------------------------------------------
//  HELPER FUNCTIONS
//---------------------------------------------
inline void Dict_addLong(PyObject*poDict, char *Key, int Value)
{
  PyObject* poKey   = PyString_FromString(Key);
  PyObject* poValue = PyInt_FromLong(Value);
  PyDict_SetItem(poDict, poKey, poValue);
  Py_DECREF(poKey);
  Py_DECREF(poValue);
}

inline void Dict_addFloat(PyObject*poDict, char *Key, double Value)
{
  PyObject* poKey   = PyString_FromString(Key);
  PyObject* poValue = PyFloat_FromDouble(Value);
  PyDict_SetItem(poDict, poKey, poValue);
  Py_DECREF(poKey);
  Py_DECREF(poValue);
}

inline void Dict_addObject(PyObject*poDict, char *Key, PyObject *poObject)
{
  PyObject* poKey   = PyString_FromString(Key);
  PyDict_SetItem(poDict, poKey, poObject);
  Py_DECREF(poKey);
  Py_DECREF(poObject);
}

inline void Dict_addString(PyObject*poDict, char *Key, const char *Value)
{
  PyObject* poKey   = PyString_FromString(Key);
  PyObject* poValue = PyString_FromString(Value);
  PyDict_SetItem(poDict, poKey, poValue);
  Py_DECREF(poKey);
  Py_DECREF(poValue);
}

inline PyObject* pyf_getTuple(int val0, int val1)
{
  PyObject *PoTuple = PyTuple_New(2);
  PyTuple_SetItem(PoTuple, 0, PyInt_FromLong(val0));
  PyTuple_SetItem(PoTuple, 1, PyInt_FromLong(val1));
  return PoTuple;
}

inline void Dict_addInt_asTuple(PyObject*poDict, char *Key, int Value1, int Value2)
{
  PyObject* poKey   = PyString_FromString(Key);
  PyObject *poValue = pyf_getTuple(Value1, Value2);
  PyDict_SetItem(poDict, poKey, poValue);
  Py_DECREF(poKey);
  Py_DECREF(poValue);
}


//---------------------------------------------
//  SANITY CHECKS
//---------------------------------------------
static bool pyf_checkPython()
{
  PyObject *ptype, *pvalue, *ptraceback;
  PyErr_Fetch(&ptype, &pvalue, &ptraceback);
  if(ptype != NULL)
  {
    char *pStrErrorMessage = PyString_AsString(pvalue);
    cout << pStrErrorMessage << endl;
    THROW_ERROR("Python error");
  }
  return true;
}

inline PyObject* pyf_getFuncHandle(char* FuncName, char *ModuleName)
{
  PyObject *pModuleName, *pModule, *pFunc;
  {
    //printf("%s not imported yet, importing", ModuleName);

    pModuleName = PyUnicode_FromString(ModuleName);
    pModule = PyImport_Import(pModuleName);
    if(pModule == NULL)
      THROW_ERROR_PYTHON("Error in Module ", ModuleName);
  }

  pFunc = PyObject_GetAttrString(pModule, FuncName);
  if (pFunc == NULL or PyCallable_Check(pFunc) == 0 )
    THROW_ERROR_PYTHON("Cannot find function", pFunc);

  Py_XDECREF(pModuleName);
  Py_XDECREF(pModule); //multiple imports of the same module are very fast

  return pFunc;
}

inline void pyf_callFunc_ArgsTuple(PyObject *pFunc, PyObject *pArgs)
{
  PyObject *pValue = PyObject_CallObject(pFunc, pArgs);

  if (pValue == NULL)
    THROW_ERROR_PYTHON("Call failed", "");

  printf("Result of call: %ld\n", PyLong_AsLong(pValue));
  Py_DECREF(pValue);
}

inline PyObject* pyf_callFunc_Arg1(PyObject *pFunc, PyObject *pArg1)
{
  PyObject *pValue = PyObject_CallFunctionObjArgs(pFunc, pArg1, NULL);
  if (pValue == NULL)
    THROW_ERROR_PYTHON("Call failed", "");

  return pValue;

}

inline PyObject* pyf_getTestArgs()
{
  PyObject *pArgs, *pValue;
  pArgs = PyTuple_New(2);
  for (int i = 0; i < 2; ++i)
  {
    pValue = PyLong_FromLong(i+2);
    if (pValue == NULL)
      THROW_ERROR_PYTHON("Cannot convert argument", "");

    /* pValue reference stolen here: */
    PyTuple_SetItem(pArgs, i, pValue);
  }
  return pArgs;
}

inline void pyf_callTestFunc()
{
  char* FuncName = "multiply";
  char *ModuleName = "a_teste_xMotionEstimation";

  // ARGUMENTS
  PyObject *pArgs = pyf_getTestArgs();

  PyObject *pFunc = pyf_getFuncHandle(FuncName, ModuleName); // LOAD MODULE + FUNCTION
  pyf_callFunc_ArgsTuple(pFunc, pArgs); // CALL FUNCTION
  Py_XDECREF(pFunc); // CLEAN UP FUNC
  Py_DECREF(pArgs);  // CLEAN UP ARGS
}

inline string getPathFromFN (const string& str)
{
  size_t found;
  cout << "Splitting: " << str << endl;
  found=str.find_last_of("/\\");
  //cout << " folder: " << str.substr(0,found) << endl;
  //cout << " file: " << str.substr(found+1) << endl;
  return str.substr(0,found);
}

inline void printPyDict_internal(PyObject *PoDict)
{
  PyObject *key, *value = NULL;
  Py_ssize_t pos = 0;
  while (PyDict_Next(PoDict, &pos, &key, &value))
  {
    PyObject *po_KeyAsString = PyObject_Str(key);
    PyObject *po_ValAsString = PyObject_Str(value);
    cout << "Key: "  << PyString_AsString(po_KeyAsString) << " | Value: " << PyString_AsString(po_ValAsString) << endl;
    Py_DECREF(po_KeyAsString);
    Py_DECREF(po_ValAsString);
  }
}

inline void printPyObject(PyObject *Po, string Title)
{
  cout << "-- Info on PyObject " << Title << ": --" << endl;
  if (Po == NULL)
  {
    cout << "is NULL!!" << endl;
    return;
  }

  cout << "Master info: ";
  PyObject_Print(Po, stdout, Py_PRINT_RAW); //same as running PyString_AsString(PyObject_Str(Po))
  cout << endl;

  //if we have a dictionary:
  if (PyDict_Check(Po))
  {
    cout << "Is a dict, contains:" << endl;
    printPyDict_internal(Po);
  }

  //if we have a module:
  if (PyModule_Check(Po))
  {
    cout << "Is a module, with namespace-dict:" << endl;
    PyObject *ModuleNamespaceDict = PyModule_GetDict(Po); //borrowed
    printPyDict_internal(ModuleNamespaceDict);
  }

  cout << endl << flush;
  fflush(stdout);
}

//#define WITH_GILSTATE_LOCKING
inline int pyf_INI_load_INT(string Section, string Option)
{
#ifdef WITH_GILSTATE_LOCKING
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
#endif

  PyObject* PoArgsTuple = PyTuple_New(2);
  PyTuple_SetItem(PoArgsTuple, 0, PyString_FromString(Section.c_str()));
  PyTuple_SetItem(PoArgsTuple, 1, PyString_FromString(Option.c_str()));

  PyObject *pFunc, *poRetVal;
  pFunc = pyf_getFuncHandle("INI_load_INT", "emb_General"); // LOAD MODULE + FUNCTION
  PY_ASSERT_RETVAL( poRetVal = PyObject_CallObject(pFunc, PoArgsTuple); )

  //printPyObject(poRetVal, "poRetVal");
  assert(PyInt_Check(poRetVal));
  int Retval = PyLong_AsLong(poRetVal);
  Py_DECREF(poRetVal);
  Py_XDECREF(pFunc); // CLEAN UP
  Py_XDECREF(PoArgsTuple);

#ifdef WITH_GILSTATE_LOCKING
  PyGILState_Release(gstate);
#endif
  return Retval;
}

inline vector<int> pyf_INI_load_INTVECTOR(string Section, string Option)
{
  PyObject* PoArgsTuple = PyTuple_New(2);
  PyTuple_SetItem(PoArgsTuple, 0, PyString_FromString(Section.c_str()));
  PyTuple_SetItem(PoArgsTuple, 1, PyString_FromString(Option.c_str()));

  PyObject *pFunc, *poRetVal;
  pFunc = pyf_getFuncHandle("INI_load_INTLIST", "emb_General"); // LOAD MODULE + FUNCTION
  PY_ASSERT_RETVAL( poRetVal = PyObject_CallObject(pFunc, PoArgsTuple); )

  //printPyObject(poRetVal, "poRetVal");
  assert (PyList_Check(poRetVal));

  vector<int> myvector;
  for(Py_ssize_t i = 0; i < PyList_Size(poRetVal); i++)
  {
    PyObject *val = PyList_GetItem(poRetVal, i);
    myvector.push_back( PyFloat_AsDouble(val) );
  }

  Py_DECREF(poRetVal);
  Py_XDECREF(pFunc); // CLEAN UP
  Py_XDECREF(PoArgsTuple);

  return myvector;
}

inline int loadINI(string Section, string Option) //quick version
{
  return pyf_INI_load_INT(Section, Option);
}

inline void pyf_INI_save_Str(string Section, string Option, string Value)
{
  PyObject* PoArgsTuple = PyTuple_New(3);
  PyTuple_SetItem(PoArgsTuple, 0, PyString_FromString(Section.c_str()));
  PyTuple_SetItem(PoArgsTuple, 1, PyString_FromString(Option.c_str()));
  PyTuple_SetItem(PoArgsTuple, 2, PyString_FromString(Value.c_str()));

  PyObject *pFunc, *poRetVal;
  pFunc = pyf_getFuncHandle("INI_save", "emb_General"); // LOAD MODULE + FUNCTION
  PY_ASSERT_RETVAL( poRetVal = PyObject_CallObject(pFunc, PoArgsTuple); )

  //printPyObject(poRetVal, "poRetVal");
  assert(PyInt_Check(poRetVal));
  int Retval = PyLong_AsLong(poRetVal);
  Py_DECREF(poRetVal);
  Py_XDECREF(pFunc); // CLEAN UP
  Py_XDECREF(PoArgsTuple);
}


inline bool pyf_is_HDT_POC(int POCIdx)
{
  PyObject* PoArgsTuple = PyTuple_New(1);
  PyTuple_SetItem(PoArgsTuple, 0, PyInt_FromLong(POCIdx));
  PyObject *pFunc, *poRetVal;
  pFunc = pyf_getFuncHandle("is_HDT_POC", "emb_General"); // LOAD MODULE + FUNCTION
  PY_ASSERT_RETVAL( poRetVal = PyObject_CallObject(pFunc, PoArgsTuple); )

  assert(PyInt_Check(poRetVal));
  bool Retval = PyLong_AsLong(poRetVal);
  Py_DECREF(poRetVal);
  Py_XDECREF(pFunc); // CLEAN UP
  Py_XDECREF(PoArgsTuple);

  return Retval;
}

inline bool pyf_is_VisRDO_CTU(int CTUIdx)
{
  PyObject* PoArgsTuple = PyTuple_New(1);
  PyTuple_SetItem(PoArgsTuple, 0, PyInt_FromLong(CTUIdx));
  PyObject *pFunc, *poRetVal;
  pFunc = pyf_getFuncHandle("is_VisRDO_CTU", "emb_General"); // LOAD MODULE + FUNCTION
  PY_ASSERT_RETVAL( poRetVal = PyObject_CallObject(pFunc, PoArgsTuple); )

  assert(PyInt_Check(poRetVal));
  bool Retval = PyLong_AsLong(poRetVal);
  Py_DECREF(poRetVal);
  Py_XDECREF(pFunc); // CLEAN UP
  Py_XDECREF(PoArgsTuple);

  return Retval;
}

inline void pyf_Wrapper_save_CPickle_FN(PyObject *poObject, char* FN)
{
  PyObject* PoArgsTuple = PyTuple_New(2);
  PyTuple_SetItem(PoArgsTuple, 0, poObject);
  PyTuple_SetItem(PoArgsTuple, 1, PyString_FromString(FN));

  // CAREFULL!! We need to increment the ref count of poObject, since
  // decref'ing the Tuple will decref the poObject too!
  // Note that in pure Python, this would be handled in the background automatically
  //Py_INCREF(poObject);

  PyObject *pFunc, *poRetVal;
  pFunc = pyf_getFuncHandle("save_CPickle_FN", "General"); // LOAD MODULE + FUNCTION
  PY_ASSERT_RETVAL( poRetVal = PyObject_CallObject(pFunc, PoArgsTuple); )

  Py_DECREF(poRetVal);
  Py_XDECREF(pFunc); // CLEAN UP
  Py_XDECREF(PoArgsTuple);
}

inline void pyf_Wrapper_postprocess_POC(char* HDT_FN)
{
  PyObject* PoArgsTuple = PyTuple_New(1);
  PyTuple_SetItem(PoArgsTuple, 0, PyString_FromString(HDT_FN));

  //  PyObject* PoArgsTuple = PyTuple_New(1);
  //  PyTuple_SetItem(PoArgsTuple, 0, PyString_FromString(HDT_FN));

  // CAREFULL!! We need to increment the ref count of poObject, since
  // decref'ing the Tuple will decref the poObject too!
  // Note that in pure Python, this would be handled in the background automatically
  //Py_INCREF(poObject);

  PyObject *pFunc, *poRetVal;
  pFunc = pyf_getFuncHandle("postprocess_POC", "emb_General"); // LOAD MODULE + FUNCTION
  PY_ASSERT_RETVAL( poRetVal = PyObject_CallObject(pFunc, PoArgsTuple); )

  Py_DECREF(poRetVal);
  Py_XDECREF(pFunc); // CLEAN UP
  Py_XDECREF(PoArgsTuple);
}

inline void pyf_Wrapper_close_HARP()
{
  PyObject* PoArgsTuple = PyTuple_New(0);

  // CAREFULL!! We need to increment the ref count of poObject, since
  // decref'ing the Tuple will decref the poObject too!
  // Note that in pure Python, this would be handled in the background automatically
  //Py_INCREF(poObject);

  PyObject *pFunc, *poRetVal;
  pFunc = pyf_getFuncHandle("close_HARP", "emb_General"); // LOAD MODULE + FUNCTION
  PY_ASSERT_RETVAL( poRetVal = PyObject_CallObject(pFunc, PoArgsTuple); )

  Py_DECREF(poRetVal);
  Py_XDECREF(pFunc); // CLEAN UP
  Py_XDECREF(PoArgsTuple);
}

#define HARP_CORE_DIR "/../HARP_Core"

//==============================================
// PYTHON INIT
//==============================================
#include <sys/auxv.h> //required for getting exe path
inline void pyf_initialize()
{
  cout << "============== Initializing HARP's PYTHON Interface ============== " << endl;

  Py_Initialize();
  char *str = "testargv";
  char* argv[1];
  argv[0] = str;
  PySys_SetArgvEx(1, argv, 0);


  PyObject *main_module, *main_dict;

  //-------------------------------
  // GET IMPORTANT HARP DIRS
  //-------------------------------
  //system("readlink /proc/curproc/file");
  cout << "PID " << ::getpid() << endl;
  printf("%s\n", (char *)getauxval(AT_EXECFN));
  char* CurrentBin = (char *)getauxval(AT_EXECFN);
  string folder = getPathFromFN(CurrentBin);
  QString CWD =  QDir::currentPath();

  //fixme: remove canonicalPath and only apply later
  //problem: of dir doesn't exists, canonicalPath returns an empty string!!!

  //will get builtin-variables
//  QString HARP_ProjectDir  = QDir(QFileInfo(CurrentBin).canonicalPath() + "/../../HARP_Kit").canonicalPath();  //Fixme: change to KitDir
//  QString ProjectDir       = QDir(QFileInfo(CurrentBin).canonicalPath() + "/../../HARP_Core").canonicalPath(); //Fixme: change to CoreDir
//  QString CoreTmpDir       = QDir(QFileInfo(CurrentBin).canonicalPath() + "/../../HARP_Core").canonicalPath();
//  QString TmpDir      = QDir(HARP_ProjectDir + "/tmp").canonicalPath(); //Fixme: change to KitTmpDir
//  //QString LibDir      = ProjectDir + HARP_CORE_DIR + "";

  //DEPRECATED!!
  QString ProjectDir       = QFileInfo(CurrentBin).absolutePath()  + "/../../HARP_Core"; //Fixme: change to CoreDir
  QString HARP_ProjectDir  = QFileInfo(CurrentBin).absolutePath() + "/../../HARP_Kit";  //Fixme: change to KitDir
  QString TmpDir           = HARP_ProjectDir + "/tmp"; //Fixme: change to KitTmpDir

  //NEW, USE THESE!
  QString CoreDir          = QFileInfo(CurrentBin).absolutePath()  + "/../../HARP_Core";
  QString CoreTmpDir       = CoreDir + "/tmp";
  QString KitDir           = QFileInfo(CurrentBin).absolutePath()  + "/../../HARP_Kit";
  QString KitTmpDir        = KitDir + "/tmp";

  system((string ("mkdir -p ") + CoreTmpDir.toStdString()).c_str());
  system((string ("mkdir -p ") + KitTmpDir.toStdString()).c_str());

  cout << "==== C++ calculated pathes ====" << endl;
  cout << "HARP_ProjectDir=" << HARP_ProjectDir.toStdString() << endl;
  cout << "ProjectDir=" << ProjectDir.toStdString() << endl;
  //cout << "LibDir=" << LibDir.toStdString() << endl;
  cout << "TmpDir=" << TmpDir.toStdString() << endl << endl;

  assert(QDir(CoreDir).exists());
  assert(QDir(CoreTmpDir).exists());
  assert(QDir(KitDir).exists());
  assert(QDir(KitTmpDir).exists());


  ProjectDir = QDir(ProjectDir).absolutePath();
  HARP_ProjectDir = QDir(HARP_ProjectDir).absolutePath();
  TmpDir = QDir(TmpDir).absolutePath();
  HARP_ProjectDir = QDir(HARP_ProjectDir).absolutePath();

  cout << "==== C++ calculated pathes (absolute) ====" << endl;
  cout << "HARP_ProjectDir=" << HARP_ProjectDir.toStdString() << endl;
  cout << "ProjectDir=" << ProjectDir.toStdString() << endl;
  //cout << "LibDir=" << LibDir.toStdString() << endl;
  cout << "TmpDir=" << TmpDir.toStdString() << endl << endl;

  //used for adding to path
  //QString EmbeddedDir = ProjectDir + HARP_CORE_DIR + "/Embedded";
  //QString MotionDir   = ProjectDir + HARP_CORE_DIR + "/Motion";
  //QString NextDir     = ProjectDir + "/Next";

  //-------------------------------
  // SETTING HARP GLOBALS AS BUILTINS
  //-------------------------------
  //since we use PyObject_CallFunctionObjArgs, we are not running unter "main" context
  //but always in the context of the module(=file) where the function is defined in
  //hence, we use __builtins__ for storing HARP path infos
  PyObject* GlobalsDict = PyEval_GetGlobals(); //why is this one empty?
  PyObject* BuiltinsDict = PyEval_GetBuiltins();

  PyDict_SetItemString(BuiltinsDict, "ProjectDir", PyString_FromString(ProjectDir.C_STR));
  PyDict_SetItemString(BuiltinsDict, "HARP_ProjectDir", PyString_FromString(HARP_ProjectDir.C_STR));
  //PyDict_SetItemString(BuiltinsDict, "LibDir", PyString_FromString(LibDir.C_STR));
  PyDict_SetItemString(BuiltinsDict, "TmpDir", PyString_FromString(TmpDir.C_STR));

  //PyDict_SetItemString(BuiltinsDict, "Cnt1", PyString_FromString("Hello World"));
  PyDict_SetItemString(BuiltinsDict, "Cnt1", PyInt_FromLong(0));
  PyDict_SetItemString(BuiltinsDict, "isLiveEncoding", PyInt_FromLong(1));
  PyDict_SetItemString(BuiltinsDict, "InputYUV", PyString_FromString(Global.FN_InputYUV.C_STR));

  //-------------------------------
  // SETTING PYTHON PATH
  //-------------------------------
  PyObject *sys = PyImport_ImportModule("sys");
  PyObject *path = PyObject_GetAttrString(sys, "path");
  PyList_Append(path, PyString_FromString("."));

  PyList_Append(path, PyString_FromString(ProjectDir.C_STR));
  PyList_Append(path, PyString_FromString((ProjectDir + "/Embedded").C_STR));
  //PyList_Append(path, PyString_FromString(NextDir.C_STR));
  //PyList_Append(path, PyString_FromString(MotionDir.C_STR));



  //-------------------------------
  // SETUP THE MAIN MODULE
  //-------------------------------
  //this is NOT NECESSARY: the code has only effect on PyRun_SimpleString() calls
  //PyRun_SimpleString() is defined in run in the context of the "main" module"
  main_module = PyImport_ImportModule("__main__");

  PyObject_SetAttrString(main_module, "TestString1", PyString_FromString("Test successfull: Adding attribute to Main-Module (thus global)"));
  main_dict   = PyModule_GetDict(main_module);
  PyDict_SetItemString(main_dict, "TestString2", PyString_FromString("Test successfull: Adding item to Main-Module-Dict (thus global)")); //same effect as above!

  //-------------------------------
  // TEST ENVIRONMENT: C++ SIDE
  //-------------------------------
  //printPyObject(main_module, "main_module");
  //printPyObject(GlobalsDict, "GlobalsDict");
  //printPyObject(BuiltinsDict, "BuiltinsDict");


  //-------------------------------
  // TEST ENVIRONMENT: PYTHON SIDE
  //-------------------------------
  PY_ASSERT(  PyRun_SimpleString("print TestString1 \n"))
  PY_ASSERT(  PyRun_SimpleString("print TestString2 \n"))

  PyObject *pFunc, *poRetVal;
  pFunc = pyf_getFuncHandle("testfunc", "emb_General"); // LOAD MODULE + FUNCTION
  PY_ASSERT_RETVAL( poRetVal = PyObject_CallFunctionObjArgs(pFunc, NULL); )
  PyRun_SimpleString("print 'Test successful: testfunc called'");
  Py_DECREF(poRetVal);
  Py_XDECREF(pFunc); // CLEAN UP

  //-------------------------------
  // FOR THE COMPLEX STUFF AND TO FINISH UP: CALL FUNCTION ON PYTHON SIDE
  //-------------------------------
  PY_ASSERT( PyRun_SimpleString("print 'C++ side: initializing HARP... '") );
  pFunc = pyf_getFuncHandle("init_HARP", "emb_General"); // LOAD MODULE + FUNCTION
  PY_ASSERT_RETVAL( poRetVal = PyObject_CallFunctionObjArgs(pFunc, NULL); )
  PyRun_SimpleString("print 'HARP successfully initialized'");
  Py_DECREF(poRetVal);
  Py_XDECREF(pFunc); // CLEAN UP

  //-------------------------------
  // NUMPY PREPARATION
  //-------------------------------
  cout << "Setting up NUMPY support" << endl;
  cout << "Importing numpy.core.multiarray" << endl;
  cout << "Checking ARRAY_API, ABI/API, endianess" << endl;
  import_array(); //for numpy support

  PY_ASSERT(  PyRun_SimpleString("print 'isLiveEncoding=', isLiveEncoding \n"))
  cout << "================== End of PYTHON Initialization ================== " << endl;
}

//---------------------------------------------
// PYTHON FINALIZE
//---------------------------------------------
inline void pyf_finalize()
{
  Py_Finalize();

}
