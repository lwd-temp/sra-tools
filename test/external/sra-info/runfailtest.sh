#!/bin/sh
# ===========================================================================
#
#                            PUBLIC DOMAIN NOTICE
#               National Center for Biotechnology Information
#
#  This software/database is a "United States Government Work" under the
#  terms of the United States Copyright Act.  It was written as part of
#  the author's official duties as a United States Government employee and
#  thus cannot be copyrighted.  This software/database is freely available
#  to the public for use. The National Library of Medicine and the U.S.
#  Government have not placed any restriction on its use or reproduction.
#
#  Although all reasonable efforts have been taken to ensure the accuracy
#  and reliability of the software and data, the NLM and the U.S.
#  Government do not and cannot warrant the performance or results that
#  may be obtained by using this software or data. The NLM and the U.S.
#  Government disclaim all warranties, express or implied, including
#  warranties of performance, merchantability or fitness for any particular
#  purpose.
#
#  Please cite the author in any work or product based on this material.
#
# ===========================================================================
#echo "$0 $*"

#
# run a command line test that is expected to fail
#
# $1 - pathname of the tool; OK (return 0) if does not exist
# $2 - command line arguments
# $3 - work directory
# $4 - test case ID
#
# return codes:
# 0 - failed as expected or the executable does not exist
# 2 - succeeded (i.e. the test fails)

TOOL=$1
ARGS=$2
WORKDIR=$3
CASEID=$4
RC=0

TEMPDIR=$WORKDIR/actual/$CASEID
STDOUT=$TEMPDIR/stdout
STDERR=$TEMPDIR/stderr

EXE="${TOOL%% *}"
if ! test -f $EXE; then
    echo "$EXE does not exist. Skipping the test."
    exit 0
fi

echo "running $CASEID"
export NCBI_SETTINGS=/

mkdir -p $TEMPDIR
rm -rf $TEMPDIR/*
if [ "$?" != "0" ] ; then
    exit 1
fi
CMD="$TOOL $ARGS 1>$STDOUT 2>$STDERR"
echo $CMD
eval $CMD
rc="$?"
if [ "$rc" = "0" ] ; then
    echo "$TOOL returned $rc, did not fail as expected"
    echo "command executed:"
    echo $CMD
    cat $STDERR
    exit 2
fi

rm -rf $TEMPDIR

exit 0
