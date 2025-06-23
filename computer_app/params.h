/**
 * global vars below are an idea of what the program might read
 */


/**
 * optional contention algorithm set
 */
extern char* contention_algorithm;

/**
 * optional time interval
 */
extern char* generator_time_interval;

/**
 * optional time interval kind
 * esp currently supports either linear or gauss
 */
extern char* generator_time_interval_kind;


/**
 * Function responsible for extracting arguments
 */
void extract_args(int argc, char **argv);

 
