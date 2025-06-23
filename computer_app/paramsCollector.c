/**
 * File with code in charge of collecting parameters from command line
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "params.h"

#define CONTENTION_ALGORITHM_OPTION 'c'


// https://www.gnu.org/software/libc/manual/2.24/html_node/Getopt-Long-Option-Example.html

//extern int opterr;
//extern int optarg;

/**
 * optional contention algorithm set
 */
char* contention_algorithm;

/**
 * optional time interval
 */
char* generator_time_interval;

/**
 * optional time interval kind
 * esp currently supports either linear or gauss
 */
char* generator_time_interval_kind;

static void print_help() {
    printf("Usage: EspCommunication [OPTIONS]\n");
    printf("Options:\n");
    printf(" -h, --help           Display this help message\n");
    printf(" -a, --algorithm <selected algorithm>       Choose contention backoff algorithm\n");
	printf(" -i, --timeInterval <time interval>       Choose time interval in micro-seconds\n");
	printf(" -k, --timeIntervalKind <time interval kind>       Choose time interval kind, either gaussian or linear distribution\n");
}

void extract_args(int argc, char **argv){
	
	int option;
	
	opterr = 0;

    //to use long names and not single character, option needs to have -- before it
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"algorithm", required_argument, 0, 'a'},
        {"timeInterval", required_argument, 0, 'i'},
        {"timeIntervalKind", required_argument, 0, 'k'},
        {0, 0, 0, 0} // example has this last one, i guess its equivalent to null terminator char in char array
    };

    char* optstring = "h:a:i:k:";

	while ((option = getopt_long(argc, argv, optstring, long_options, NULL)) != -1){
		switch (option){
			case 'h':
                print_help();
				break;
			case 'a':
                printf("Target algorithm %s\n", optarg);
				contention_algorithm = optarg;
				break;
            case 'i':
                printf("Target time interval %s\n", optarg);
				generator_time_interval = optarg;
				break;
            case 'k':
                printf("Target time interval kind %s\n", optarg);
				generator_time_interval_kind = optarg;
				break;
			case ':':
				fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				
        }
    }

    //remainder code, good to use if there is one obligatory param
    /*
    if(optind < argc){
		//file_input_name = argv[optind];
		//printf("input filename: {%s}\n", file_input_name);
	}
	else{
		fprintf (stderr, "Requires Ip address\n");
		exit(0);
	}
    */
	
}
