/*****************************************************************************
 *   DiscoMore: discovering polymorphism from raw unassembled NGS reads
 *   A tool from the GATB (Genome Assembly Tool Box)
 *   Copyright (C) 2014  INRIA
 *   Authors: P.Peterlongo, E.Drezen
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

/*
 *
 *  Created on: 28 oct. 2010
 *      Author: Pierre Peterlongo
 */


#include<fragment_index.h>
#include<list.h>
#include<commons.h>
#include<extension_algorithm.h>
#include<outputs.h>
#include<limits.h> // MAXINT
#include <unistd.h>  /* sleep(1) */
#include <string.h> // strdup
#include <time.h>
#include <libchash.h>

//#define OMP

#ifdef OMP
#include <omp.h>
#endif

char * getVersion(){
    return  "kissreads module 2.0.0 - Copyright INRIA - CeCILL License";
}


void print_usage_and_exit(char * name){
	fprintf (stderr, "NAME\nkissReads, version %s\n", getVersion());
#ifdef INPUT_FROM_KISSPLICE
	fprintf (stderr, "\nSYNOPSIS\n%s <toCheck.fasta> <readsC1.fasta> [<readsC2.fasta> [<readsC3.fasta] ...] [-k value] [-c value] [-d value] [-O value] [-l value] [-o name] [-u name] [-i index_stride] [-m align_file] [-p] [-s] [-f] [-h] \n", name);
#else
    fprintf (stderr, "\nSYNOPSIS\n%s <toCheck.fasta> <readsC1.fasta> [<readsC2.fasta> [<readsC3.fasta] ...] [-k value] [-c value] [-d value] [-O value] [-o name] [-u name] [-n] [-I] [-i index_stride] [-m align_file] [-p] [-s] [-f] [-h] \n", name);
#endif// INPUT_FROM_KISSPLICE
	fprintf (stderr, "\nDESCRIPTION\n");
	fprintf (stderr, "Checks for each sequence contained into the toCheck.fasta if\n");
	fprintf (stderr, "it is read coherent (each position is covered by at least \"min_coverage\" read(s)) with reads from readsA.fasta or readsB.fasta\n");
	fprintf (stderr, "A sequence s from toCheck is treated as follow:\n");
	fprintf (stderr, "  if (s coherent with at least one read set): output the sequence as follows\n");
	fprintf (stderr, "  \t >original fasta comment|C1:min<avg-corr_avg<max|C2:min<avg-cor_avg<max|C3...:\n");
	fprintf (stderr, "  \t >s\n");
	fprintf (stderr, "  With A:min<avg-cor_avg<max standing for : value of the position having minimal coverage in readsA.fasta < average coverage in readsA.fasta - R-squarred corrected average in readsA.fa < value of the position having maximal coverage in readsA.fasta\n");
	fprintf (stderr, "  The coverage is the number of reads that perfectly mapped a position\n");
	fprintf (stderr, "  Any other situation (s not coherent with any): couple non read coherent, not outputed \n");
    
	fprintf (stderr, "\nOPTIONS\n");
	fprintf (stderr, "\t -k size_seed: will use seeds of length size_seed. Default: 25.\n");
    fprintf (stderr, "\t -O minimal_read_overlap: a read can be mapped if its overlap is a least \"minimal_read_overlap\". Default: k\n");
	fprintf (stderr, "\t -c min_coverage: a sequence is covered by at least min_coverage coherent reads. Default: 2\n");
    fprintf (stderr, "\t -d max_substitutions: Maximal number of substitutions authorized between a read and a fragment. Note that no substitution is allowed on the central position while anaylizing the kissnp output. Default: 1.\n");
#ifdef INPUT_FROM_KISSPLICE
    fprintf (stderr, "\t -l min_kissplice_overlap: Kissplice min_voerlap. Default: 3\n");
    fprintf (stderr, "\t -j counting option. 0: counts are summed and represent the whole path, 1: counts are only in the junctions, 2: all the counts are shown separately   . Default: 0\n");
#endif
    fprintf (stderr, "\t -p: only print coverage, do not separate between coherent and uncoherent sequences. Not compatible with -u option.\n");
	fprintf (stderr, "\t -o file_name: write read-coherent outputs. Default: standard output \n");
	fprintf (stderr, "\t -u file_name: write unread-coherent outputs. Not compatible with -p option. Default: standard output \n");
#ifndef INPUT_FROM_KISSPLICE
	fprintf (stderr, "\t -n the input file (toCheck.fasta) is a kissnp output (incompatible with -I option) \n");
    fprintf (stderr, "\t\t in this case: 1/ only the upper characters are considered (no mapping done on the extensions) and 2/ the central position (where the SNP occurs) is strictly mapped, no subsitution is authorized on this position.\n");
	fprintf (stderr, "\t -I the input file (toCheck.fasta) is an Intl output (incompatible with -n option) \n");
#endif // not INPUT_FROM_KISSPLICE
    fprintf (stderr, "\t -i index_stride (int value). This is a heuristic for limiting the memory footprint. Instead of indexing each kmer of the sequences contained into the toCheck.fasta, kissreads indexes kmers occurring each \"index_stride\" position. Default = 1 (no heuristic)\n");
    fprintf (stderr, "\t -t max number of threads (also limited by number of input files)\n");
	fprintf (stderr, "\t -m align_file, write a file of reads mapped to sequences in file align_file\n");
	fprintf (stderr, "\t -s silent mode\n");
    fprintf (stderr, "\t -f outputs coherent events in a standard fasta file format\n");
	fprintf (stderr, "\t -h prints this message and exit\n");
	exit(0);
}


int main(int argc, char **argv) {
#ifdef READ2INV
    printf("Compiled with READ2INV: output only motifs where au-vb is specific to one datasets and av'-u'b is specific to the other\n");
#endif
#ifdef CLASSICAL_SPANNING
    printf("Compiled with CLASSICAL_SPANNING: a position is considered as covered as soon as a read maps this position (else a position is considered as covered if the kmer starting at this position is fully covered by a read)\n");
#endif
#ifdef INPUT_FROM_KISSPLICE
    printf("Compiled with INPUT_FROM_KISSPLICE: dealing with kissplice output and to count separately junctions and central portions\n");
#endif
    
    
    minimal_read_overlap=0;
    char map_snps=0; // input is a kissnp output
    char map_invs=0; // input is an intl (read2sv) output.
    only_print=0; // if "only_print" == 1 do not separate between coherent and uncoherent.
    
    char input_only_upper=0; // By default: all characters (upper or lower) are read
    int number_paths_per_event=1; // By default (generic usage) each event is composed by a unique sequence.
#ifdef INPUT_FROM_KISSPLICE
    min_overlap=3; // default value.
    countingOption = 0 ; // default value
    input_only_upper=1; // don't considere the lower script characters.
    number_paths_per_event=2; // each splicing event is composed by two sequences in the fasta file.
#endif
    
    int index_stride =1;
	setvbuf(stdout,(char*)NULL,_IONBF,0); // disable buffering for printf()
	time_t before_all, after_all;
	before_all = time(NULL);
	size_seeds=25;
    
	int i;
	int nb_events_per_set;
    int max_substitutions=1;
    int max_threads = 1;
#ifdef OMP
     max_threads =  omp_get_num_procs();
#endif
    //	size_before_reads_starting=16384;
	int min_coverage=2; // minimal number of reads per positions extending the initial fragment
    
	if(argc<2){
		print_usage_and_exit(argv[0]);
	}
    
	char * toCheck_file =strdup(argv[1]);
	FILE * coherent_out = stdout;
	FILE * uncoherent_out = stdout;
	FILE * sam_out = NULL;
	silent=0; standard_fasta=0;
    
    nbits_nbseeds = 8*sizeof(uint64_t)- NBITS_OFFSET_SEED ;
    mask_nbseed  = ( 1ULL <<  (uint64_t) nbits_nbseeds  ) -1 ;
    mask_offset_seed = (1ULL << (NBITS_OFFSET_SEED)) -1 ;
    
	// GET ALL THE READ FILES
	// find the number of read sets
	number_of_read_sets=0;
	while(number_of_read_sets+2<argc && argv[number_of_read_sets+2][0]!='-') number_of_read_sets++;
	char ** reads_file_names = (char **) malloc(sizeof(char *)*number_of_read_sets);
    char * coherent_file_name="";
    char * uncoherent_file_name="";
    char * samout_file_name="";
    // copy the read set file names
	number_of_read_sets=0;
	while(number_of_read_sets+2<argc && argv[number_of_read_sets+2][0]!='-'){
		reads_file_names[number_of_read_sets]=strdup(argv[number_of_read_sets+2]);
		number_of_read_sets++;
	}
	while (1)
	{
#ifdef INPUT_FROM_KISSPLICE
        int temoin = getopt (argc-number_of_read_sets-1, &argv[number_of_read_sets+1], "c:d:k:O:o:u:q:m:i:fps-:j:l:t:");
#else
        int temoin = getopt (argc-number_of_read_sets-1, &argv[number_of_read_sets+1], "c:d:k:O:o:u:q:m:i:fpsnI-:t:");
#endif //INPUT_FROM_KISSPLICE
		if (temoin == -1){
			break;
		}
		switch (temoin)
		{
            case 'c':
                min_coverage=atoi(optarg);
                if(min_coverage<1) min_coverage=1;
                break;
            case 'p':
                only_print=1;
                break;
#ifdef INPUT_FROM_KISSPLICE
            case 'l':
                min_overlap=atoi(optarg);
                break;
            case 'j':
              countingOption  = atoi(optarg);
              break;
#endif //INPUT_FROM_KISSPLICE
            case 'o':
                coherent_file_name = strdup(optarg);
                coherent_out = fopen(optarg, "w");
                if(coherent_out == NULL){
                    fprintf(stderr,"cannot open %s for writing results, exit\n", optarg);
                    exit(1);
                }
                break;
            case 'u':
                uncoherent_file_name = strdup(optarg);
		        uncoherent_out = fopen(optarg, "w");
                if(uncoherent_out == NULL){
                    fprintf(stderr,"cannot open %s for writing results, exit\n", optarg);
                    exit(1);
                }
                break;
            case 'd':
                max_substitutions=atoi(optarg);
                break;
            case 't':
                max_threads=atoi(optarg);
                break;
            case 'i':
                index_stride=atoi(optarg);
                if(index_stride<0) index_stride=1;
                break;
                
#ifndef INPUT_FROM_KISSPLICE
            case 'n':
                map_snps=1;
                input_only_upper=1;
                number_paths_per_event=2;
                
                break;
            case 'I':
                map_invs=1;
                input_only_upper=0; // read all characters for now (still no extension pluged to intl).
                number_paths_per_event=4;
                
                break;
#endif
                
            case 'O':
                minimal_read_overlap=atoi(optarg);
                break;
                
            case 'k':
                size_seeds=atoi(optarg);
                
                if(size_seeds<10){
                    fprintf(stderr,"%d too small to be used as seed length. kisSnpCheckReads continues with seeds of length 10\n",size_seeds);
                    size_seeds=10;
                }
                break;
            case 'h':
                print_usage_and_exit(argv[0]);
                break;
            case 'f':
                standard_fasta=1;
                break;
            case 's':
                silent=1;
                break;
            case 'm':
                samout_file_name = strdup(optarg);
                sam_out = fopen(optarg, "w");
                if(sam_out == NULL){
                    fprintf(stderr,"cannot open %s for writing sam like results, exit\n", optarg);
                    exit(1);
                }
                fprintf(sam_out,"SNP_header\tmapped_read\tid_read_file\tposition_where_read_is_mapped\n");
                
                break;
                
            case '-':
                if(strcmp("version", optarg)==0){
                    printf("kissReads version %s\n", getVersion());
                    exit(0);
                }
                printf(" what next ? %s\n",optarg);
                break;
            default:
                print_usage_and_exit(argv[0]);
		}
	}
    
	if ( argc  - optind <2)
	{
		print_usage_and_exit(argv[0]);
	}
    
#ifndef INPUT_FROM_KISSPLICE
    if(map_snps && map_invs){
        fprintf(stderr, "cannot use both options -n and -I\n");
        exit(1);
    }
#endif
    
    
    if(size_seeds>32){
        fprintf(stderr,"Sorry kissreads does not accept k>32, exit\n");
        exit(1);
    }
    
    if(size_seeds>minimal_read_overlap) minimal_read_overlap=size_seeds;

    if (only_print) uncoherent_out = coherent_out;
    
    
    if(!silent){
        printf("This is kissreads, version %s\n", getVersion());
        printf("COMMAND LINE:\n");
        for(i=0;i<argc;i++) printf("%s ",argv[i]);
        printf("\n");
        printf("PARAMETERS SUM UP:\n");
        printf("\tIN\n");
        printf("\t References file: %s\n", toCheck_file);
        printf("\t Queries file%s:",number_of_read_sets>1?"s":"");
        for (i=0;i<number_of_read_sets;i++)printf(" %s", reads_file_names[i]);
        printf(".\n");
#ifndef INPUT_FROM_KISSPLICE
        if(map_invs || map_snps){
            if(map_snps) printf("\t *(-n) Input file is considered as a kissnp output\n");
            if(map_invs) printf("\t *(-I) Input file is considered as a TakeABreak output\n");
        }
        else printf("\t *Generic input file\n");
#endif
#ifdef INPUT_FROM_KISSPLICE
        printf("\t *INPUT_FROM_KISSPLICE: Input file is considered as a Kissplice output\n");
#endif
        
        
        printf("\tMAPPING PARAMETERS\n");
        printf("\t *(-k) Size seeds %d\n", size_seeds);
        printf("\t *(-i) Index Stride %d (will index a seed each %d positions)\n", index_stride, index_stride);
        printf("\t *(-O) minimal_read_overlap %d (a read is mapped on a fragment if the overlap is at least %d positions)\n", minimal_read_overlap,minimal_read_overlap);
        printf("\t \t Note that minimal_read_overlap is at least equal to size_seeds\n");
        printf("\t *(-c) Minimal coverage %d\n", min_coverage);
#ifdef KMER_SPANNING
        printf("\t \t KMER_SPANNING: Each kmer(=%d) as defined by \"minimal_read_overlap\" spanning each position should be covered by at least %d reads.\n", minimal_read_overlap, min_coverage);
#else
        printf("\t \t CLASSICAL_SPANNING: Each position should be covered by at least %d reads.\n", min_coverage);
#endif
        printf("\t *(-d) Authorize at most %d substitutions during the mapping\n", max_substitutions);
#ifdef INPUT_FROM_KISSPLICE
        printf("\t *(-l) Kissplice count uses min_overlap %d\n", min_overlap);
        printf("\t *(-j) Kissplice countingOption %d\n", countingOption);
#endif
        
        
        printf("\tOUT\n");
        if (only_print) printf("\t print all results in file: %s\n", coherent_file_name);
        else{
            printf("\t *(-o) Read-coherent results file: %s\n", coherent_file_name);
            printf("\t *(-u) Read-uncoherent results file: %s\n", uncoherent_file_name);
        }
        printf("\t *(-f) Standard fasta output mode: %s\n",standard_fasta?"yes":"no");
        if(sam_out) printf("\t *(-m) Reads mapped results file: %s\n", samout_file_name);
        
        printf("\tMISC.\n");
        printf("\t *(-t) Max %d threads.\n", max_threads);
    }
    
	init_static_variables(size_seeds);
	
	gzFile * reads_files = (gzFile*) malloc(sizeof(gzFile)*number_of_read_sets);
	p_fragment_info * results_against_set;
    
	char found;
	// test if all formats are in fastq
	quality=1;
	for (i=0;i<number_of_read_sets;i++){
        quality &= strstr(reads_file_names[i],"fastq") || strstr(reads_file_names[i],"fq") || strstr(reads_file_names[i],"txt");
	}
	if(quality) quality=1;
    
    
	for (i=0;i<number_of_read_sets;i++){
        if (quality){
            found = strstr(reads_file_names[i],"fastq") || strstr(reads_file_names[i],"fq") || strstr(reads_file_names[i],"txt");
            if (!found ){		fprintf(stderr,"\nwith q option, fastq reads files are needed, wrong %s, exit\n",reads_file_names[i]);		exit(1);	}
        }
        else
	    {
            found = strstr(reads_file_names[i],"fastq") || strstr(reads_file_names[i],"fq") || strstr(reads_file_names[i],"txt") || strstr(reads_file_names[i],"fa") || strstr(reads_file_names[i],"fasta") || strstr(reads_file_names[i],"fna");
            if (!found ){		fprintf(stderr,"\naccepted extensions are .fa[.gz], .fasta[.gz], .fna[.gz] for fasta format and .fq[.gz], .fastq[.gz], .txt[.gz] for fastq format, wrong %s, exit\n",reads_file_names[i]);		exit(1);	}
	    }
	}
	
	file=gzopen(toCheck_file,"r");
	if(file == NULL){
        fprintf(stderr,"cannot open file %s, exit\n",toCheck_file);
        exit(1);
	}
	// how many fragments do we have (fragments are in fastq format)?
    char * line =  (char *)malloc(sizeof(char)*1048576); // 1048576
	number_of_starters = number_of_sequences_in_file(file,line);
    free(line);
    
    nb_events_per_set = (number_of_starters/number_paths_per_event);
    
    
    if(!silent){
        printf("Number of sequences in %s file: %d (%d events)\n", toCheck_file, number_of_starters, nb_events_per_set);
    }
    
    results_against_set = index_starters_from_input_file (size_seeds, nb_events_per_set, number_paths_per_event, input_only_upper, index_stride);
    
     int nbthreads = number_of_read_sets;
     if (nbthreads > max_threads) nbthreads = max_threads;
    
    if(!silent){
        printf("Mapping.\n");
    }

    int silented=0;
#ifdef OMP
    if (!silent) silented=1;
    silent=1; // avoids melting messages
#endif
    

    
#ifdef OMP
#pragma omp parallel for if(nbthreads>1 && sam_out==NULL) num_threads(nbthreads) private(i)
#endif
    for (i=0;i<number_of_read_sets;i++){
        reads_files[i]=gzopen(reads_file_names[i],"r");
        if(reads_files[i] == NULL){		fprintf(stderr,"cannot open reads file %s, exit\n",reads_file_names[i]);		exit(1);	}
        
        if(!silent) printf("\nCheck read coherence... vs reads from %s (set %d)\n", reads_file_names[i], i);
        
        
        
        read_coherence(reads_files[i], reads_file_names[i], size_seeds,  min_coverage, results_against_set,  number_of_starters, i, quality, nb_events_per_set,  number_paths_per_event, sam_out, max_substitutions, minimal_read_overlap);
        
        
        gzclose(reads_files[i]);
    }
    if (silented) silent=0;
    
    if(number_paths_per_event==2)
        print_results_2_paths_per_event(coherent_out, uncoherent_out, results_against_set, number_of_read_sets, nb_events_per_set, quality);
    else if(map_invs)
        print_results_invs(coherent_out, uncoherent_out, results_against_set, number_of_read_sets, nb_events_per_set, quality);
    else   print_generic_results(coherent_out, uncoherent_out, results_against_set, number_of_read_sets, nb_events_per_set, quality); // TODO
    
    free(seed_table);
    FreeHashTable((struct HashTable*)seeds_count);
    for (i=0;i<nb_events_per_set;i++)
    {
        free(results_against_set[i]->read_coherent);
        free(results_against_set[i]->number_mapped_reads);
        int read_set_id;
        for (read_set_id=0;read_set_id<number_of_read_sets;read_set_id++)
        {
            free(results_against_set[i]->read_coherent_positions[read_set_id]);
            free(results_against_set[i]->sum_quality_per_position[read_set_id]);
        }
        free(results_against_set[i]->read_coherent_positions);
        free(results_against_set[i]->sum_quality_per_position);
        free(results_against_set[i]);
    }
    free(results_against_set);

    
	gzclose(file);
	fclose(coherent_out);
	if (!only_print) fclose(uncoherent_out);
	if(sam_out) fclose(sam_out);
	if(!silent) {
        if(only_print) printf("Results are in %s", coherent_file_name);
        else printf("Results are in %s and %s", coherent_file_name, uncoherent_file_name);
        if(sam_out) printf(" (mapped reads are in %s)", samout_file_name);
        printf(".\n");
		after_all = time(NULL);
		printf("Total time: %.0lf secs\n",  difftime(after_all, before_all));
	}
	return 0;
}
