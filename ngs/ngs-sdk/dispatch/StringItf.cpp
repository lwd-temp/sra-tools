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

#include <ngs/itf/StringItf.hpp>
#include <ngs/itf/ErrBlock.hpp>
#include <ngs/itf/VTable.hpp>

#include <ngs/itf/StringItf.h>

namespace ngs
{
    /*----------------------------------------------------------------------
     * metadata
     */
    extern ItfTok NGS_Refcount_v1_tok;
    static ItfTok NGS_String_v1_tok ( "NGS_String_v1", NGS_Refcount_v1_tok );

    /*----------------------------------------------------------------------
     * access vtable
     */
    static inline
    const NGS_String_v1_vt * Access ( const NGS_VTable * vt )
    {
        const NGS_String_v1_vt * out = static_cast < const NGS_String_v1_vt* >
            ( Cast ( vt, NGS_String_v1_tok ) );
        if ( out == 0 )
            throw ErrorMsg ( "object is not of type NGS_String_v1" );
        return out;
    }

    /*----------------------------------------------------------------------
     * StringItf
     *  a dynamically allocated object representing a string reference
     */

    const char * StringItf :: data () const
        NGS_NOTHROW ()
    {
        // the object is really from C
        const NGS_String_v1 * self = Self ();

        // cast vtable to our level
        try
        {
            const NGS_String_v1_vt * vt = Access ( self -> vt );

            // call through C vtable
            ErrBlock err;
            assert ( vt -> data != 0 );
            const char * ret = ( * vt -> data ) ( self, & err );

            // check for errors
            err . Check ();

            return ret;
        }
        catch ( ErrorMsg & x )
        {
            // report to console or log
        }
        catch ( ... )
        {
        }

        return "";
    }

    size_t StringItf :: size () const
        NGS_NOTHROW ()
    {
        // the object is really from C
        const NGS_String_v1 * self = Self ();

        // cast vtable to our level
        try
        {
            const NGS_String_v1_vt * vt = Access ( self -> vt );

            // call through C vtable
            ErrBlock err;
            assert ( vt -> size != 0 );
            size_t ret  = ( * vt -> size ) ( self, & err );

            // check for errors
            err . Check ();

            return ret;
        }
        catch ( ErrorMsg & x )
        {
            // report to console or log
        }
        catch ( ... )
        {
        }

        return 0;
    }

    StringItf * StringItf :: substr ( size_t offset ) const
        NGS_THROWS ( ErrorMsg )
    {
        return substr ( offset, -1 );
    }

    StringItf * StringItf :: substr ( size_t offset, size_t size ) const
        NGS_THROWS ( ErrorMsg )
    {
        // the object is really from C
        const NGS_String_v1 * self = Test ();

        // cast vtable to our level
        const NGS_String_v1_vt * vt = Access ( self -> vt );

        // call through C vtable
        ErrBlock err;
        assert ( vt -> substr != 0 );
        NGS_String_v1 * ret  = ( * vt -> substr ) ( self, & err, offset, size );

        // check for errors
        err . Check ();

        return StringItf :: Cast ( ret );
    }


} // namespace ngs
