// engler, cs140e: client code for a simple hello ping pong.
// you should:
//  1. make a copy of the directory and change the code to do a PUT32, GET32.
//  2. rewrite so that you can handle multiple connections to the server.
//     you might have to modify some of the esp-command.c code.
//  3. add esp_command implementations that allow you to gracefully handle 
//     errors in send/receive ---- if you have a distributed system, then:
//           T(uptime) -> inf
//     and/or
//           N(nodes)->inf
//     then Pr(fail)=1.
//   4. make sure it runs on the pi.
#include "libesp.h"
#include "libunix.h"


void esp_main(lex_t *l) {
    // these are defined in ../libesp/esp-constants
    esp_t e = esp_mk(l, NETWORK, PASSWORD);

    // if you control-c the code running on unix, that has no
    // impact on the esp.  we hard reset it to try to put it
    // in a "clean" initial state.
    esp_hard_reset(&e);
    if(!esp_connect_to_wifi(&e))
        panic("cannot connect to tcp client\n");

    unsigned port = ESP_DEFAULT_PORT;
    const char *ip = ESP_SERVER_IP;
    if(!esp_start_tcp_client(&e, ip, port))
        panic("can't start esp tcp client\n");

    esp_set_verbose(0);

	char path[1024];
	char file[1024];
	for (int i  = 0; i < 2; i++) {
		while (1) {
			//first rec the path
			char buf[1024];
			esp_debug("going to receive\n");
			unsigned ch, n = esp_is_recv(&e, &ch, buf, sizeof buf);
			buf[n] = 0;
			output("client ch=%d received: <%s>, going to ack\n", ch, buf);
			if(!esp_send(&e, 0, buf, strlen(buf)))
				panic("cannot send!!\n");

			if (n && i == 0) {
				memcpy(path, buf, sizeof(buf));
				break;
			} else if (n && i  == 1) {
				memcpy(file, buf, sizeof(buf));
				break;
			}
		}
	}

	// run my-install and collect output
	char *child_argv[4] = {"my-install\0", file, 0};

	trace_output("going to fork/exec: %s ", child_argv[0]);
    for(int i = 1; child_argv[i]; i++)
        output(" argv[%d]=<%s> ", i, child_argv[i]);
    output("\n");

    int pid;
	pid = fork();
	if (!pid) {
		//child process: my-install
		//close all  file  descriptors beyond 2
		
		execvp(child_argv[0], child_argv);
	} 

    output("SUCCESS: did %d roundtrips without losing any packet\n", NTRIALS);
}
