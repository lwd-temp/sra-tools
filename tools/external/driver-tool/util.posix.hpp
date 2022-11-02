/* ===========================================================================
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

#pragma once

#if WINDOWS
#else

#include <system_error>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <utility>
#include "opt_string.hpp"

static inline std::error_code error_code_from_errno()
{
    return std::error_code(errno, std::system_category());
}

namespace POSIX {
struct EnvironmentVariables {
    static opt_string get(char const *name) {
        return opt_string(getenv(name));
    }
    static void set(char const *name, char const *value) {
        if (value)
            setenv(name, value, 1);
        else
            unsetenv(name);
    }
};
}

using PlatformEnvironmentVariables = POSIX::EnvironmentVariables;

#endif
