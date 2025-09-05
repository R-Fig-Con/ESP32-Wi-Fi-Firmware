// source
//  https://github.com/avahi/avahi/blob/master/examples/client-browse-services.c

#include "instances_search.h"

#include <string.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include <avahi-client/lookup.h>       // for lots
#include <avahi-common/simple-watch.h> // for lots

#include <avahi-common/malloc.h> // avahi_free
#include <avahi-common/error.h>  // for error strings

static AvahiSimplePoll *simple_poll = NULL;
static AvahiClient *client = NULL;
static AvahiServiceBrowser *esp = NULL;

static instance_search_event *found = NULL;
static instance_search_event *left = NULL;

// address values given by function are probably not in malloc indefinitely
// so saving in node structure may be necessary

typedef struct node
{
    char *address; //[IP_ADDRESS_MAX_SIZE];
    struct node *next;
} address_node;

static address_node *head = NULL;

static void add_node(char *value)
{
    address_node *current = head;

    if (head == NULL)
    {
        current = (address_node *)malloc(sizeof(address_node));
        current->address = value;
        head = current;
        return;
    }

    address_node *next = head->next;

    while (1)
    {
        if (next == NULL)
        {
            next = (address_node *)malloc(sizeof(address_node));
            next->address = value;
            current->next = next;
            return;
        }

        current = current->next;
        next = next->next;
    }
}

static void remove_node(char *value)
{
    address_node *prev = head;

    address_node *current = head->next;

    while (1)
    {
        if (!strcmp(current->address, value))
        {
            prev->next = current->next;
            free(current->address);
            free(current);
            return;
        }

        current = current->next;
        prev = prev->next;
    }
}

void on_instance_found_event(instance_search_event action)
{
    found = action;
}

void on_instance_left_event(instance_search_event action)
{
    left = action;
}

// as service name is ip for now this function may not be necessary
// consider cutting it and leaving it to browser callback
static void resolve_callback(
    AvahiServiceResolver *r,
    AVAHI_GCC_UNUSED AvahiIfIndex interface,
    AVAHI_GCC_UNUSED AvahiProtocol protocol,
    AvahiResolverEvent event,
    const char *name,
    const char *type,
    const char *domain,
    const char *host_name,
    const AvahiAddress *address,
    uint16_t port,
    AvahiStringList *txt,
    AvahiLookupResultFlags flags,
    AVAHI_GCC_UNUSED void *userdata)
{

    assert(r);

    /* Called whenever a service has been resolved successfully or timed out */

    switch (event)
    {
    case AVAHI_RESOLVER_FAILURE:
        fprintf(stderr, "(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
        break;

    case AVAHI_RESOLVER_FOUND:
    {
        char a[AVAHI_ADDRESS_STR_MAX], *t;

        avahi_address_snprint(a, sizeof(a), address);

        fprintf(stderr, "Service '%s' of type '%s' in domain '%s with address'%s:\n", name, type, domain, a);

        char *address = (char *)malloc(IP_ADDRESS_MAX_SIZE);
        memcpy(address, name, strlen(name) + 1);
        add_node(address);

        printf("Calling callback\n");

        found(address);

        t = avahi_string_list_to_string(txt);
        fprintf(stderr,
                "\t%s:%u (%s)\n"
                "\tTXT=%s\n"
                "\tcookie is %u\n"
                "\tis_local: %i\n"
                "\tour_own: %i\n"
                "\twide_area: %i\n"
                "\tmulticast: %i\n"
                "\tcached: %i\n",
                host_name, port, a,
                t,
                avahi_string_list_get_service_cookie(txt),
                !!(flags & AVAHI_LOOKUP_RESULT_LOCAL),
                !!(flags & AVAHI_LOOKUP_RESULT_OUR_OWN),
                !!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA),
                !!(flags & AVAHI_LOOKUP_RESULT_MULTICAST),
                !!(flags & AVAHI_LOOKUP_RESULT_CACHED));

        avahi_free(t);
    }
    }

    avahi_service_resolver_free(r);
}

static void browse_callback(
    AvahiServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name, // name is unique
    const char *type,
    const char *domain,
    AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
    void *userdata)
{

    AvahiClient *c = (AvahiClient *)userdata;
    assert(b);

    /* Called whenever a new services becomes available on the LAN or is removed from the LAN */

    switch (event)
    {
    case AVAHI_BROWSER_FAILURE: // Error case

        fprintf(stderr, "(Browser) %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
        avahi_simple_poll_quit(simple_poll);
        return;

    case AVAHI_BROWSER_NEW:
        fprintf(stderr, "(Browser) NEW: service '%s' of type '%s' in domain '%s'\n", name, type, domain);

        /* We ignore the returned resolver object. In the callback
           function we free it. If the server is terminated before
           the callback function is called the server will free
           the resolver for us. */

        if (!(avahi_service_resolver_new(c, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, (AvahiLookupFlags)0, resolve_callback, c)))
            fprintf(stderr, "Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(c)));

        break;

    case AVAHI_BROWSER_REMOVE:

        left((char *)name);
        remove_node((char *)name);

        fprintf(stderr, "(Browser) REMOVE: service '%s' of type '%s' in domain '%s'\n", name, type, domain);
        break;

    case AVAHI_BROWSER_ALL_FOR_NOW:
    case AVAHI_BROWSER_CACHE_EXHAUSTED:
        fprintf(stderr, "(Browser) %s\n", event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW");
        break;
    }
}

static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void *userdata)
{
    assert(c);

    /* Called whenever the client or server state changes */

    if (state == AVAHI_CLIENT_FAILURE)
    {
        fprintf(stderr, "Server connection failure: %s\n", avahi_strerror(avahi_client_errno(c)));
        avahi_simple_poll_quit(simple_poll);
    }
}

// Todo decide if it should wait for search end
void end_instance_search()
{
    avahi_simple_poll_quit(simple_poll); // avahi_simple_poll_iterate();
}

void start_instance_search()
{
    int error;

    if (!(simple_poll = avahi_simple_poll_new()))
    {
        fprintf(stderr, "Failed to create simple poll object.\n");
        goto fail;
    }

    client = avahi_client_new(avahi_simple_poll_get(simple_poll), (AvahiClientFlags)0, client_callback, NULL, &error);

    if (!client)
    {
        fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));
        goto fail;
    }

    if (!(esp = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, SERVICE_TYPE, NULL, (AvahiLookupFlags)0, browse_callback, client)))
    {
        fprintf(stderr, "Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(client)));
        goto fail;
    }

    avahi_simple_poll_loop(simple_poll); // loop

fail:

    /* Cleanup things */
    if (esp)
        avahi_service_browser_free(esp);

    if (client)
        avahi_client_free(client);

    if (simple_poll)
        avahi_simple_poll_free(simple_poll);

    // todo add error communication
}