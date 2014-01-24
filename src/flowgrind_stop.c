/**
 * @file flowgrind_stop.c
 * @brief Utility to instruct the Flowgrind daemon to stop all flows
 */

/*
 * Copyright (C) 2013-2014 Alexander Zimmermann <alexander.zimmermann@netapp.com>
 * Copyright (C) 2010-2013 Christian Samsel <christian.samsel@rwth-aachen.de>
 * Copyright (C) 2009 Tim Kosse <tim.kosse@gmx.de>
 * Copyright (C) 2007-2008 Daniel Schaffrath <daniel.schaffrath@mac.com>
 *
 * This file is part of Flowgrind. Flowgrind is free software; you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2 as published by the Free Software Foundation.
 *
 * Flowgrind distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "common.h"

/** Name of the executable */
static char progname[50] = "flowgrind-stop";

static void usage()
{
	fprintf(stderr,
		"Usage: %1$s [OPTION]... [ADDRESS]...\n"
		"Stop all flows on the daemons running at the given addresses.\n\n"

		"Mandatory arguments to long options are mandatory for short options too.\n"
		"  -h, --help     display this help and exit\n"
		"  -v, --version  print version information and exit\n\n"

		"Example:\n"
		"   %1$s localhost 127.2.3.4:5999 example.com\n",
		progname);
	exit(EXIT_SUCCESS);
}

inline static void usage_hint(void)
{
	fprintf(stderr, "Try '%s -h' for more information\n", progname);
	exit(EXIT_FAILURE);
}

static void stop_flows(char* address)
{
	xmlrpc_env env;
	xmlrpc_client *client = 0;
	xmlrpc_value * resultP = 0;
	char* p;
	int port = DEFAULT_LISTEN_PORT;
	char host[1000], url[1000];

	if (strlen(address) > sizeof(url) - 50) {
		fprintf(stderr, "Address too long: %s\n", address);
		return;
	}

	/* Construct canonical address and URL */
	strncpy(host, address, 1000);

	p = strchr(host, ':');
	if (p) {
		if (p == host) {
			fprintf(stderr, "Error, no address given: %s\n", address);
			return;
		}
		port = atoi(p + 1);
		if (port < 1 || port > 65535) {
			fprintf(stderr, "Error, invalid port given: %s\n", address);
			return;
		}
		*p = 0;
	}
	sprintf(url, "http://%s:%d/RPC2", host, port);
	sprintf(host, "%s:%d", host, port);

	printf("Stopping all flows on %s\n", host);

	/* Stop the flows */
	xmlrpc_env_init(&env);
	xmlrpc_client_create(&env, XMLRPC_CLIENT_NO_FLAGS, "Flowgrind", FLOWGRIND_VERSION, NULL, 0, &client);
	if (env.fault_occurred)
		goto cleanup;

	xmlrpc_client_call2f(&env, client, url, "stop_flow", &resultP,
		"({s:i})", "flow_id", -1); /* -1 stops all flows */
	if (resultP)
		xmlrpc_DECREF(resultP);

cleanup:
	if (env.fault_occurred) {
		fprintf(stderr, "Could not stop flows on %s: %s (%d)\n",
			host, env.fault_string, env.fault_code);
	}
	if (client)
		xmlrpc_client_destroy(client);
	xmlrpc_env_clean(&env);

}

int main(int argc, char *argv[])
{
	/* update progname from argv[0] */
	if (argc > 0) {
		/* Strip path */
		char *tok = strrchr(argv[0], '/');
		if (tok)
			tok++;
		else
			tok = argv[0];
		strncpy(progname, tok, sizeof(progname));
		progname[sizeof(progname) - 1] = 0;
	}

	/* long options */
	static const struct option long_opt[] = {
		{"help", no_argument, 0, 'h'},
		{"version", no_argument, 0, 'v'},
		{NULL, 0, NULL, 0}
	};

	/* short options */
	static const char *short_opt = "hv";

	/* parse command line */
	int ch;
	while ((ch = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
		switch (ch) {
		case 'h':
			usage();
			break;
		case 'v':
			fprintf(stderr, "%s version: %s\n", progname,
				FLOWGRIND_VERSION);
			exit(EXIT_SUCCESS);

		/* unknown option or missing option-argument */
		case '?':
			usage_hint();
			break;
		}
	}

	xmlrpc_env rpc_env;
	xmlrpc_env_init(&rpc_env);
	xmlrpc_client_setup_global_const(&rpc_env);

	for (int i = optind; i < argc; i++)
		stop_flows(argv[i]);

	xmlrpc_env_clean(&rpc_env);
	xmlrpc_client_teardown_global_const();
}
