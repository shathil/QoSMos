//
// Created by Hoque, Mohammad on 03/02/2019.
//

#ifndef QOSMOS_QOSMOS_NATIVE_H
#define QOSMOS_QOSMOS_NATIVE_H


#include <sys/types.h>
#include <linux/in.h>
#include <unordered_map>
#include "flowmaps.h"
#include "socket.h"

int sock_cssix = -1;

int ass_forward_sock31 = -1;
int ass_forward_sock33 = -1;
int ass_forward_sock41 = -1;
int ass_forward_sock43 = -1;
int exp_forward_sockEF = -1;
int class_zero_sockcs0 = -1;

struct flow{
    long first_seen;
    long last_seen;
    int uplink_bytes[10000];
    int downlink_bytes[10000];
    int up_packets;
    int down_packet;
    int flow_type;
};


#define video_ip 7 //111
#define voice_ip 3 //011
#define video_cast 6 //110
#define audio_cast 2 //010
#define video_stream 1 //001
#define time_grace 3 // pm3 seconds // look for the flows within 3 seconds gap.



struct sockaddr_in server_addr;
int vpnfd = -1;
unsigned long int lastRecvTime = 0;
bool running = true;
bool read_running = true;
bool write_running = true;
int get_dscp(char *buf);
pthread_mutex_t lock;
pthread_mutex_t media_lock;


int device_media_context;
long media_context_time; // ms


std::unordered_map<std::string, struct flow*> flow_table;
std::unordered_map<long, std::string> candid_table;

struct packet* check_modify_dscp(const char*, int);
char *address_to_string(const struct packet *packet);


void clean_flow_table(); /* If a flow does not have any update for 60 seconds remove it. Should be controlled from the main Java thread*/
void clean_candid_table(); /* When the mediacontext expires and should be controlled from Java Main thread. */


#endif //QOSMOS_QOSMOS_NATIVE_H
