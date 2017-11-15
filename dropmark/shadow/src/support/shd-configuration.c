/*
 * The Shadow Simulator
 * Copyright (c) 2010-2011, Rob Jansen
 * See LICENSE for licensing information
 */

#include "shadow.h"

Configuration* configuration_new(gint argc, gchar* argv[]) {
    /* get memory */
    Configuration* c = g_new0(Configuration, 1);
    MAGIC_INIT(c);

    c->argstr = g_strjoinv(" ", argv);

    const gchar* required_parameters = "shadow.config.xml";
    gint nRequiredXMLFiles = 1;

    c->context = g_option_context_new(required_parameters);
    g_option_context_set_summary(c->context, "Shadow - run real applications over simulated networks");
    g_option_context_set_description(c->context,
        "Shadow is a unique discrete-event network simulator that runs real "
        "applications like Tor, and distributed systems of thousands of nodes "
        "on a single machine. Shadow combines the accuracy of emulation with the "
        "efficiency and control of simulation, achieving the best of both approaches.");

    /* set defaults */
    c->initialTCPWindow = 10;
    c->interfaceBufferSize = 1024000;
    c->interfaceBatchTime = 10;
    c->randomSeed = 1;
    c->cpuThreshold = -1;
    c->cpuPrecision = 200;
    c->heartbeatInterval = 1;

    /* set options to change defaults for the main group */
    c->mainOptionGroup = g_option_group_new("main", "Main Options", "Primary simulator options", NULL, NULL);
    const GOptionEntry mainEntries[] = {
      { "debug", 'd', 0, G_OPTION_ARG_NONE, &(c->debug), "Pause at startup for debugger attachment", NULL },
      { "heartbeat-frequency", 'h', 0, G_OPTION_ARG_INT, &(c->heartbeatInterval), "Log node statistics every N seconds [1]", "N" },
      { "heartbeat-log-level", 'j', 0, G_OPTION_ARG_STRING, &(c->heartbeatLogLevelInput), "Log LEVEL at which to print node statistics ['message']", "LEVEL" },
      { "heartbeat-log-info", 'i', 0, G_OPTION_ARG_STRING, &(c->heartbeatLogInfo), "Comma separated list of information contained in heartbeat ('node','socket','ram') ['node']", "LIST"},
      { "log-level", 'l', 0, G_OPTION_ARG_STRING, &(c->logLevelInput), "Log LEVEL above which to filter messages ('error' < 'critical' < 'warning' < 'message' < 'info' < 'debug') ['message']", "LEVEL" },
      { "preload", 'p', 0, G_OPTION_ARG_STRING, &(c->preloads), "LD_PRELOAD environment VALUE to use for function interposition (/path/to/lib:...) [None]", "VALUE" },
      { "runahead", 'r', 0, G_OPTION_ARG_INT, &(c->minRunAhead), "If set, overrides the automatically calculated minimum TIME workers may run ahead when sending events between nodes, in milliseconds [0]", "TIME" },
      { "seed", 's', 0, G_OPTION_ARG_INT, &(c->randomSeed), "Initialize randomness for each thread using seed N [1]", "N" },
      { "workers", 'w', 0, G_OPTION_ARG_INT, &(c->nWorkerThreads), "Run concurrently with N worker threads [0]", "N" },
      { "valgrind", 'x', 0, G_OPTION_ARG_NONE, &(c->runValgrind), "Run through valgrind for debugging", NULL },
      { "version", 'v', 0, G_OPTION_ARG_NONE, &(c->printSoftwareVersion), "Print software version and exit", NULL },
      { NULL },
    };

    g_option_group_add_entries(c->mainOptionGroup, mainEntries);
    g_option_context_set_main_group(c->context, c->mainOptionGroup);

    /* now fill in the default plug-in examples option group */
    c->pluginsOptionGroup = g_option_group_new("sim", "Simulation Examples", "Built-in simulation examples", NULL, NULL);
    const GOptionEntry pluginEntries[] =
    {
      { "tgen", 0, 0, G_OPTION_ARG_NONE, &(c->runTGenExample), "PLACEHOLDER - Run basic data transfer simulation", NULL },
      { NULL },
    };

    g_option_group_add_entries(c->pluginsOptionGroup, pluginEntries);
    g_option_context_add_group(c->context, c->pluginsOptionGroup);

    /* now fill in the network option group */
    GString* sockrecv = g_string_new("");
    g_string_printf(sockrecv, "Initialize the socket receive buffer to N bytes [%i]", (gint)CONFIG_RECV_BUFFER_SIZE);
    GString* socksend = g_string_new("");
    g_string_printf(socksend, "Initialize the socket send buffer to N bytes [%i]", (gint)CONFIG_SEND_BUFFER_SIZE);

    c->networkOptionGroup = g_option_group_new("sys", "System Options", "Simulated system/network behavior", NULL, NULL);
    const GOptionEntry networkEntries[] =
    {
      { "cpu-precision", 0, 0, G_OPTION_ARG_INT, &(c->cpuPrecision), "round measured CPU delays to the nearest TIME, in microseconds (negative value to disable fuzzy CPU delays) [200]", "TIME" },
      { "cpu-threshold", 0, 0, G_OPTION_ARG_INT, &(c->cpuThreshold), "TIME delay threshold after which the CPU becomes blocked, in microseconds (negative value to disable CPU delays) (experimental!) [-1]", "TIME" },
      { "interface-batch", 0, 0, G_OPTION_ARG_INT, &(c->interfaceBatchTime), "Batch TIME for network interface sends and receives, in milliseconds [10]", "TIME" },
      { "interface-buffer", 0, 0, G_OPTION_ARG_INT, &(c->interfaceBufferSize), "Size of the network interface receive buffer, in bytes [1024000]", "N" },
      { "interface-qdisc", 0, 0, G_OPTION_ARG_STRING, &(c->interfaceQueuingDiscipline), "The interface queuing discipline QDISC used to select the next sendable socket ('fifo' or 'rr') ['fifo']", "QDISC" },
      { "socket-recv-buffer", 0, 0, G_OPTION_ARG_INT, &(c->initialSocketReceiveBufferSize), sockrecv->str, "N" },
      { "socket-send-buffer", 0, 0, G_OPTION_ARG_INT, &(c->initialSocketSendBufferSize), socksend->str, "N" },
      { "tcp-congestion-control", 0, 0, G_OPTION_ARG_STRING, &(c->tcpCongestionControl), "Congestion control algorithm to use for TCP ('aimd', 'reno', 'cubic') ['cubic']", "TCPCC" },
      { "tcp-ssthresh", 0, 0, G_OPTION_ARG_INT, &(c->tcpSlowStartThreshold), "Set TCP ssthresh value instead of discovering it via packet loss or hystart [0]", "N" },
      { "tcp-windows", 0, 0, G_OPTION_ARG_INT, &(c->initialTCPWindow), "Initialize the TCP send, receive, and congestion windows to N packets [10]", "N" },
      { NULL },
    };

    g_option_group_add_entries(c->networkOptionGroup, networkEntries);
    g_option_context_add_group(c->context, c->networkOptionGroup);

    /* parse args */
    GError *error = NULL;
    if (!g_option_context_parse(c->context, &argc, &argv, &error)) {
        g_printerr("** %s **\n", error->message);
        gchar* helpString = g_option_context_get_help(c->context, TRUE, NULL);
        g_printerr("%s", helpString);
        g_free(helpString);
        configuration_free(c);
        return NULL;
    }

    /* make sure we have the required arguments. program name is first arg.
     * printing the software version requires no other args. running a
     * plug-in example also requires no other args. */
    if(!(c->printSoftwareVersion) && !(c->runTGenExample) && (argc < nRequiredXMLFiles + 1)) {
        g_printerr("** Please provide the required parameters **\n");
        gchar* helpString = g_option_context_get_help(c->context, TRUE, NULL);
        g_printerr("%s", helpString);
        g_free(helpString);
        configuration_free(c);
        return NULL;
    }

    if(c->nWorkerThreads < 0) {
        c->nWorkerThreads = 0;
    }
    if(c->logLevelInput == NULL) {
        c->logLevelInput = g_strdup("message");
    }
    if(c->heartbeatLogLevelInput == NULL) {
        c->heartbeatLogLevelInput = g_strdup("message");
    }
    if(c->heartbeatLogInfo == NULL) {
        c->heartbeatLogInfo = g_strdup("node");
    }
    if(c->heartbeatInterval < 1) {
        c->heartbeatInterval = 1;
    }
    if(c->initialTCPWindow < 1) {
        c->initialTCPWindow = 1;
    }
    if(c->interfaceBufferSize < CONFIG_MTU) {
        c->interfaceBufferSize = CONFIG_MTU;
    }
    c->interfaceBatchTime *= SIMTIME_ONE_MILLISECOND;
    if(c->interfaceBatchTime == 0) {
        /* we require at least 1 nanosecond b/c of time granularity */
        c->interfaceBatchTime = 1;
    }
    if(c->interfaceQueuingDiscipline == NULL) {
        c->interfaceQueuingDiscipline = g_strdup("fifo");
    }
    if(!c->initialSocketReceiveBufferSize) {
        c->initialSocketReceiveBufferSize = CONFIG_RECV_BUFFER_SIZE;
        c->autotuneSocketReceiveBuffer = TRUE;
    }
    if(!c->initialSocketSendBufferSize) {
        c->initialSocketSendBufferSize = CONFIG_SEND_BUFFER_SIZE;
        c->autotuneSocketSendBuffer = TRUE;
    }
    if(c->tcpCongestionControl == NULL) {
        c->tcpCongestionControl = g_strdup("cubic");
    }

    c->inputXMLFilenames = g_queue_new();
    for(gint i = 1; i < argc; i++) {
        GString* filename = g_string_new(argv[i]);
        g_queue_push_tail(c->inputXMLFilenames, filename);
    }

    if(socksend) {
        g_string_free(socksend, TRUE);
    }
    if(sockrecv) {
        g_string_free(sockrecv, TRUE);
    }

    return c;
}

void configuration_free(Configuration* config) {
    MAGIC_ASSERT(config);

    if(config->inputXMLFilenames) {
        g_queue_free(config->inputXMLFilenames);
    }
    g_free(config->logLevelInput);
    g_free(config->heartbeatLogLevelInput);
    g_free(config->heartbeatLogInfo);
    g_free(config->interfaceQueuingDiscipline);
    if(config->argstr) {
        g_free(config->argstr);
    }
    if(config->preloads) {
        g_free(config->preloads);
    }

    /* groups are freed with the context */
    g_option_context_free(config->context);

    MAGIC_CLEAR(config);
    g_free(config);
}

GLogLevelFlags configuration_getLevel(Configuration* config, const gchar* input) {
    MAGIC_ASSERT(config);
    if (g_ascii_strcasecmp(input, "error") == 0) {
        return G_LOG_LEVEL_ERROR;
    } else if (g_ascii_strcasecmp(input, "critical") == 0) {
        return G_LOG_LEVEL_CRITICAL;
    } else if (g_ascii_strcasecmp(input, "warning") == 0) {
        return G_LOG_LEVEL_WARNING;
    } else if (g_ascii_strcasecmp(input, "message") == 0) {
        return G_LOG_LEVEL_MESSAGE;
    } else if (g_ascii_strcasecmp(input, "info") == 0) {
        return G_LOG_LEVEL_INFO;
    } else if (g_ascii_strcasecmp(input, "debug") == 0) {
        return G_LOG_LEVEL_DEBUG;
    } else {
        return G_LOG_LEVEL_MESSAGE;
    }
}

GLogLevelFlags configuration_getLogLevel(Configuration* config) {
    MAGIC_ASSERT(config);
    const gchar* l = (const gchar*) config->logLevelInput;
    return configuration_getLevel(config, l);
}

GLogLevelFlags configuration_getHeartbeatLogLevel(Configuration* config) {
    MAGIC_ASSERT(config);
    const gchar* l = (const gchar*) config->heartbeatLogLevelInput;
    return configuration_getLevel(config, l);
}

SimulationTime configuration_getHearbeatInterval(Configuration* config) {
    MAGIC_ASSERT(config);
    return config->heartbeatInterval * SIMTIME_ONE_SECOND;
}

gchar* configuration_getQueuingDiscipline(Configuration* config) {
    MAGIC_ASSERT(config);
    return config->interfaceQueuingDiscipline;
}

gint configuration_getNWorkerThreads(Configuration* config) {
    MAGIC_ASSERT(config);
    return config->nWorkerThreads;
}
