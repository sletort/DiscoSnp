def X( samfile ):
	''''''
import pysam
	o_sam = pysam.AlignmentFile( samfile, "r" )
	with open( samfile, 'r' ) as o_sam:
		for line in o_sam:
			if line.startswith('@'):
				continue #We do not read headers

	def __get_quality_flag( samfile ):
		'''determine the flag based on the Q1_ presence (or not)
			in the first line after header.
		'''
		with open( samfile, 'r' ) as o_sam:
			for line in o_sam:
				if line.startswith('@'):
					continue #We do not read headers
				if "Q1_" in line : 
						return True
				return False

def CounterGenotype(fileName):
        samfile=open(fileName,'r')
        nbGeno=0
        while True:
                line=samfile.readline()
                if not line: break #End of file
                if line.startswith('@'): continue #We do not read headers
                #>SNP_higher_path_3|P_1:30_C/G|high|nb_pol_1|left_unitig_length_86|right_unitig_length_261|left_contig_length_166|right_contig_length_761|C1_124|C2_0|Q1_0|Q2_0|G1_0/0:10,378,2484|G2_1/1:2684,408,10|rank_1
                if nbGeno==0:
                        line=line.rstrip('\r').rstrip('\n').split('\t')
                        for i in line:
                                if 'SNP' or 'INDEL' in i:
                                        nomDisco=i.split('|')
                                        for k in nomDisco:
                                                if k[0]=='G':
                                                        nbGeno+=1
                                        samfile.close()
                                        return(nbGeno)
