// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#define OIDN_VERSION_MAJOR 1
#define OIDN_VERSION_MINOR 2
#define OIDN_VERSION_PATCH 4
#define OIDN_VERSION 10204
#define OIDN_VERSION_STRING "1.2.4"

/* #undef OIDN_STATIC_LIB */
/* #undef OIDN_API_NAMESPACE */

#if defined(OIDN_API_NAMESPACE)
  #define OIDN_API_NAMESPACE_BEGIN namespace  {
  #define OIDN_API_NAMESPACE_END }
  #define OIDN_API_NAMESPACE_USING using namespace ;
  #define OIDN_API_EXTERN_C
  #define OIDN_NAMESPACE_BEGIN namespace  {
  #define OIDN_NAMESPACE_END }
  #define OIDN_NAMESPACE_USING using namespace ;
  #undef OIDN_API_NAMESPACE
#else
  #define OIDN_API_NAMESPACE_BEGIN
  #define OIDN_API_NAMESPACE_END
  #define OIDN_API_NAMESPACE_USING
  #if defined(__cplusplus)
    #define OIDN_API_EXTERN_C extern "C"
  #else
    #define OIDN_API_EXTERN_C
  #endif
  #define OIDN_NAMESPACE_BEGIN namespace oidn {
  #define OIDN_NAMESPACE_END }
  #define OIDN_NAMESPACE_USING using namespace oidn;
#endif

#if defined(OIDN_STATIC_LIB)
  #define OIDN_API_IMPORT OIDN_API_EXTERN_C
  #define OIDN_API_EXPORT OIDN_API_EXTERN_C
#elif defined(_WIN32)
  #define OIDN_API_IMPORT OIDN_API_EXTERN_C __declspec(dllimport)
  #define OIDN_API_EXPORT OIDN_API_EXTERN_C __declspec(dllexport)
#else
  #define OIDN_API_IMPORT OIDN_API_EXTERN_C
  #define OIDN_API_EXPORT OIDN_API_EXTERN_C __attribute__ ((visibility ("default")))
#endif

#if defined(OIDN_EXPORT_API)
  #define OIDN_API OIDN_API_EXPORT
#else
  #define OIDN_API OIDN_API_IMPORT
#endif
