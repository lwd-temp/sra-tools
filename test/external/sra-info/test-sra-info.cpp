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

/**
* Unit tests for class SraInfo
*/

#include "sra-info.cpp"

#include <ktst/unit_test.hpp>

#include <klib/out.h>

using namespace std;
using namespace ncbi::NK;

TEST_SUITE(SraInfoTestSuite);

TEST_CASE(Construction)
{
    SraInfo info;
}

TEST_CASE(SetAccession)
{
    SraInfo info;
    const string Accession = "SRR000123";
    info.SetAccession(Accession);
    REQUIRE_EQ( string(Accession), info.GetAccession() );
}

TEST_CASE(ResetAccession)
{
    SraInfo info;
    const string Accession1 = "SRR000123";
    const string Accession2 = "SRR000124";
    info.SetAccession(Accession1);
    info.SetAccession(Accession2);
    REQUIRE_EQ( string(Accession2), info.GetAccession() );
}

// Platform

TEST_CASE(PlatformInInvalidAccession)
{
    SraInfo info;
    const string Accession = "i_am_groot";
    info.SetAccession(Accession);
    REQUIRE_THROW( info.GetPlatforms() );
}

TEST_CASE(NoPlatformInTable)
{
    SraInfo info;
    const string Accession = "NC_000001.10";
    info.SetAccession(Accession);
    SraInfo::Platforms p = info.GetPlatforms();
    REQUIRE_EQ( size_t(0), p.size() );
}
TEST_CASE(NoPlatformInDatabase) // ?
{//TODO
}

TEST_CASE(SinglePlatformInTable)
{
    SraInfo info;
    const string Accession = "SRR000123";
    info.SetAccession(Accession);
    SraInfo::Platforms p = info.GetPlatforms();
    REQUIRE_EQ( size_t(1), p.size() );
    REQUIRE_EQ( string("SRA_PLATFORM_454"), *p.begin() );
}
TEST_CASE(SinglePlatformInDatabase)
{
    SraInfo info;
    const string Accession = "ERR334733";
    info.SetAccession(Accession);
    SraInfo::Platforms p = info.GetPlatforms();
    REQUIRE_EQ( size_t(1), p.size() );
    REQUIRE_EQ( string("SRA_PLATFORM_ILLUMINA"), *p.begin() );
}

TEST_CASE(MultiplePlatforms)
{
    SraInfo info;
    const string Accession = "./input/MultiPlatform";
    info.SetAccession(Accession);
    SraInfo::Platforms p = info.GetPlatforms();
    REQUIRE_EQ( size_t(3), p.size() );
    // REQUIRE_NE( p.end(), p.find("SRA_PLATFORM_UNDEFINED") );
    // REQUIRE_NE( p.end(), p.find("SRA_PLATFORM_ILLUMINA") );
    // REQUIRE_NE( p.end(), p.find("SRA_PLATFORM_454") );
}

//////////////////////////////////////////// Main
#include <kapp/args.h>
#include <kfg/config.h>

extern "C"
{

ver_t CC KAppVersion ( void )
{
    return 0x1000000;
}

const char UsageDefaultName[] = "test-sra-info";

rc_t CC UsageSummary (const char * progname)
{
    return KOutMsg ( "Usage:\n" "\t%s [options] -o path\n\n", progname );
}

rc_t CC Usage( const Args* args )
{
    return 0;
}

rc_t CC KMain ( int argc, char *argv [] )
{
    KConfigDisableUserSettings();

    rc_t rc=SraInfoTestSuite(argc, argv);
    return rc;
}

}
