#! /usr/bin/bash

#which accession to handle
ACC1=SRR4065856
ACC2=SRR2054959
ACC3=ERR1681325
ACC4=SRR1930009
ACC5=SRR3930049
ACC6=SRR4019352
ACC7=SRR3702734

ACCESSIONS="$ACC1 $ACC2 $ACC3 $ACC4 $ACC5 $ACC6 $ACC7"

for ACC in $ACCESSIONS
do
    ./copy_diff_compare.sh $ACC
done
