/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

#ifndef _Included_py_ngs_itf_StatisticsItf
#define _Included_py_ngs_itf_StatisticsItf
#ifdef __cplusplus
extern "C" {
#endif

#include "py_ngs_defs.h"

LIB_EXPORT PY_RES_TYPE PY_NGS_StatisticsGetValueType ( void* pRef, char const* path, uint32_t* pRet, void** ppNGSStrError );
LIB_EXPORT PY_RES_TYPE PY_NGS_StatisticsGetAsString  ( void* pRef, char const* path, void** ppNGSStringBuf, void** ppNGSStrError );
LIB_EXPORT PY_RES_TYPE PY_NGS_StatisticsGetAsI64     ( void* pRef, char const* path, int64_t* pRet, void** ppNGSStrError );
LIB_EXPORT PY_RES_TYPE PY_NGS_StatisticsGetAsU64     ( void* pRef, char const* path, uint64_t* pRet, void** ppNGSStrError );
LIB_EXPORT PY_RES_TYPE PY_NGS_StatisticsGetAsDouble  ( void* pRef, char const* path, double* pRet, void** ppNGSStrError );
LIB_EXPORT PY_RES_TYPE PY_NGS_StatisticsGetNextPath  ( void* pRef, char const* path, void** ppNGSStringBuf, void** ppNGSStrError );

#ifdef __cplusplus
}
#endif
#endif /* _Included_py_ngs_itf_StatisticsItf */
