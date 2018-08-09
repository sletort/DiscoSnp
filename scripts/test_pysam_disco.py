#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import pysam

samfile = "test.sam"
vcf_file = "test.out.vcf"

o_header = pysam.VariantHeader()
o_header.add_meta( "date", '"20180401"' )
o_header.add_meta( "source", '"test"' )
o_vcf = pysam.VariantFile( vcf_file, 'w', header=o_header )
