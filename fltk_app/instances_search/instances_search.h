//size from avahi example
#define IP_ADDRESS_MAX_SIZE 40

#define SERVICE_TYPE "_http._tcp"

/**
 * note: for now parameter address is not to be freed
 */
typedef void (instance_search_event )(char address[IP_ADDRESS_MAX_SIZE]);

/**
 * Start esp instance search. As long nothing
 * goes wrong, this is an infinite loop and should
 * be started on its own thread
 * 
 * Todo: decide how to report any error that compromises the
 * loop. 
 */
void start_instance_search();

/**
 * end search loop
 */
void end_instance_search();

/**
 * @param[in] action sets the action that should happen
 * on esp instance found. Should be set before starting
 */
void on_instance_found_event(instance_search_event action);

/**
 * @param[in] action sets the action that should happen
 * on esp instance dissapears. Should be set before starting
 */
void on_instance_left_event(instance_search_event action);