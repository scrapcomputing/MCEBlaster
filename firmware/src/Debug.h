//-*- C++ -*-
//
// Copyright (C) 2024 Scrap Computing

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <config.h>


#ifdef DBGPRINT
#define DBG_PRINT(...)                                                         \
  { __VA_ARGS__ }
#else
#define DBG_PRINT(...) {}
#endif

#endif // __DEBUG_H__
