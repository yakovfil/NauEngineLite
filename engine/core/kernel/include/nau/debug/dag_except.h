// Copyright 2024 N-GINN LLC. All rights reserved.
// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved 
#pragma once
#include "nau/kernel/kernel_config.h"

#if NAU_EXCEPTIONS_ENABLED
#include <osApiWrappers/dag_stackHlp.h>

//! base nau(imported from dagor) exception class
//! it has code, textual description and stack
class NauException
{
public:
  int excCode;
  const char *excDesc;

  NAU_FORCE_INLINE NauException(int code, const char *desc)
  {
    excCode = code;
    excDesc = desc;
    ::stackhlp_fill_stack(excStack, 32, 0);
  }
  NAU_FORCE_INLINE NauException(int code, const char *desc, void *ctx_ptr)
  {
    excCode = code;
    excDesc = desc;
    ::stackhlp_fill_stack_exact(excStack, 32, ctx_ptr);
  }
  virtual ~NauException() {}


  void **getStackPtr() { return excStack; }

protected:
  void *excStack[32];
};

#define NAU_TRY              try
#define NAU_THROW(x)         throw x
#define NAU_RETHROW()        throw
#define NAU_CATCH(x)         catch (x)
#define NAU_EXC_STACK_STR(e) ::stackhlp_get_call_stack_str(e.getStackPtr(), 32)

#else
class NauException
{
public:
  int excCode;
  const char *excDesc;

  NAU_FORCE_INLINE NauException(int code, const char *desc)
  {
    excCode = code;
    excDesc = desc;
  }

  void **getStackPtr() { return nullptr; }
};

#define NAU_TRY      if (1)
#define NAU_THROW(x) NAU_FAILURE("exception: " #x)
#define NAU_RETHROW()
//-V:DAGOR_CATCH:646
#define NAU_CATCH(x)         if (0)
#define NAU_EXC_STACK_STR(e) "n/a"

#endif
