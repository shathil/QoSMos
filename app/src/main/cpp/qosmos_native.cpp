#include <jni.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/ip.h>
#include "native.h"
#include "packet_parser.h"
#include "assert.h"
#include "checksum.h"
#include "qosmos_native.h"
#include <string.h>

#include <android/log.h>
#include <unordered_map>


void* thread_read_vpn(void* n){

    char buf[4096] = {0};
    int len = 0;
    int reader_media_context = -1;

    while(running) {
        len = read(vpnfd, buf, 4096);
        //TODO: Here we employ packet reader to find the type of service.
        if (len <= 0) {
            if (errno == EAGAIN) {
                usleep(10);
                continue;
            }
            __android_log_print(ANDROID_LOG_DEBUG, "threadOutgoing", "vpn closed: %d, %s", len, strerror(errno));
            break;
        }
        __android_log_print(ANDROID_LOG_DEBUG, "threadOutgoing", "receive %d bytes", len);

        //TODO: we need here a switch case for the selecting the tunnels. We only consider the outgoing packet


        if (reader_media_context == -1){
            pthread_mutex_lock(&media_lock);
            if(device_media_context > 0)
                reader_media_context = device_media_context;
            if(device_media_context ==  0)
                reader_media_context = -1;
            pthread_mutex_unlock(&media_lock);
        }



        struct packet *packet = check_modify_dscp(buf,0);

        // Update flow table through JNI calls




        int dscp = get_dscp(buf);

        switch(dscp) {

            case 140 :
                len = sendto(ass_forward_sock31, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                break;
            case 142 :
                len = sendto(ass_forward_sock33, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                break;
            case 143 :
                len = sendto(ass_forward_sock41, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                break;
            case 145 :
                len = sendto(ass_forward_sock43, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                break;
            case 146 :
                len = sendto(exp_forward_sockEF, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                break;
            default :
                len = sendto(class_zero_sockcs0, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        }

        //len = sendto(sock, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (len <= 0) {
            __android_log_print(ANDROID_LOG_DEBUG, "threadOutgoing", "sock closed: %d", len);
            break;
        }
        //__android_log_print(ANDROID_LOG_DEBUG, "threadOutgoing", "forward %d bytes", len);



    }

    // notify other thread
    running = false;
    return 0;
}

void* threadIncoming(void* n)
{
    char buf[4096] = {0};
    int len = 0;
    int last_recv_time = 0;

    /*
     *
     * Let as assume that we receive everything over an incoming socket class_zero_sockcs0, even the control packets.
     * Then write the incoming packets to the VPN socket except the control packets. If there is no incoming packet
     * within 15 seconds, send a controlpacket to the remote server via sock_cssix.
     *
     * */
    // write the incoming packets to
    // However we send the control packets via cs6 socket.
    while(1) {
        len = recvfrom(class_zero_sockcs0, buf, 4096, 0, NULL, NULL);
        if (len <= 0) {
            if (errno == EAGAIN) {
                usleep(10);
                continue;
            }
            __android_log_print(ANDROID_LOG_DEBUG, "threadIncoming", "sock closed: %d, %s", len, strerror(errno));
            break;
        }

        /* Comment: Dealing with the global variables */

        pthread_mutex_lock(&lock);
        if(!running) {
            pthread_mutex_unlock(&lock);
            break;
        }

        if (len> 0) {
            last_recv_time = time(NULL);
            running = true;
        }


        int idleTime = time(NULL) - last_recv_time;
        if (idleTime > 30){
            running = false;
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);

        /* Sending a control packet to the  */

        if (idleTime >= 15) {
            sendto(sock_cssix, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            __android_log_print(ANDROID_LOG_DEBUG, "threadKeepAlive", "send keep alive %lu", idleTime);
        } else {
            __android_log_print(ANDROID_LOG_DEBUG, "threadKeepAlive", "idle %lu", idleTime);
        }

        /* If the received bytes is of a control packet, do nothing, else forward it to the vpnfd */
        if (buf[0] == '\0') {
            __android_log_print(ANDROID_LOG_DEBUG, "threadIncoming", "receive control packet %d bytes", len);
        }
        else {

            /* writing incoming packets to the VPN socket*/

            __android_log_print(ANDROID_LOG_DEBUG, "threadIncoming", "receive %d bytes", len);
            len = write(vpnfd, buf, len);
            if (len <= 0) {
                __android_log_print(ANDROID_LOG_DEBUG, "threadIncoming", "vpn closed: %d, %s", len, strerror(errno));
                break;
            }

        }
    }

    // notify other thread
    running = false;
    return 0;
}




/* This function should set the mapping between the desired dscp and flow*/
int get_desired_dscp(struct ipv4* header){



    return 0;
}


char *get_flow_tuples(const struct packet *packet)
{
    char src_string[ADDR_STR_LEN];
    char dst_string[ADDR_STR_LEN];
    struct tuple tuple;

    int size = ADDR_STR_LEN*2+8;
    char s[size]={0};
    get_packet_tuple(packet, &tuple);

    sprintf(s, "%s:%u > %s:%u",
            ip_to_string(&tuple.src.ip, src_string), ntohs(tuple.src.port),
            ip_to_string(&tuple.dst.ip, dst_string), ntohs(tuple.dst.port));

    return s;
}

// get flow table from the packets from the opposite direction
char *get_oppos_flow_tuples(const struct packet *packet)
{
    char src_string[ADDR_STR_LEN];
    char dst_string[ADDR_STR_LEN];
    struct tuple tuple;

    int size = ADDR_STR_LEN*2+12;
    char s[size]={0};
    get_packet_tuple(packet, &tuple);

    sprintf(s, "%s:%u > %s:%u",
            ip_to_string(&tuple.src.ip, src_string), ntohs(tuple.src.port),
            ip_to_string(&tuple.dst.ip, dst_string), ntohs(tuple.dst.port));

    return s;
}

long get_time_second(){
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    return current_time.tv_sec;
}

struct packet *check_modify_dscp(const char *data){

    char *error = NULL;

    struct  packet *newpack = packet_new(sizeof(data));
    memcpy(newpack->buffer,data,sizeof(data));

    enum packet_parse_result_t result =
            (packet_parse_result_t)parse_packet(newpack, sizeof(data), PACKET_LAYER_3_IP,
                         &error);
    struct ipv4 *expected_ipv4 = (struct ipv4 *)(newpack->buffer);

    const char* flow_key = (const char*)get_flow_tuples(newpack);
    int remain_size = expected_ipv4->tot_len; // this reaminging size only excludes the IP header


    struct flow *newflow = flow_table[flow_key];
    if(newflow== NULL){

        newflow = (struct flow*)malloc(sizeof(flow));
        newflow->first_seen = get_time_second();
        newflow->last_seen = get_time_second();
        newflow->uplink_bytes[newflow->up_packets] = remain_size;
        newflow->flow_type = 0;
        flow_table[flow_key] = newflow;

    }else{


        // we update the flowtype here or the outgoing packet update side
        newflow->last_seen = get_time_second();
        newflow->up_packets++;
        newflow->uplink_bytes[newflow->up_packets] = remain_size;

    }



    int tos = expected_ipv4->tos;

    // get_desired_dscp should be set by the
    int desired = get_desired_dscp(expected_ipv4);
    if(tos == desired) {

        packet_free(newpack);
        return NULL;
    }
    else{
        /* We need to modify packet*/
        expected_ipv4->tos = htons(desired); // requires converting to the network
        ntohs(ipv4_checksum(expected_ipv4, sizeof(expected_ipv4)));
        return newpack;
    }

}



void* threadOutgoing(void* n)
{
    char buf[4096] = {0};
    int len = 0;

    struct ip ip_header;



    while(1) {

        /*
         *
         * First checking whethere the tunnel with the remote host is active or not
         *
         * */
        pthread_mutex_lock(&lock);
        if (!running){
            pthread_mutex_unlock(&lock);
            break;
        }

        len = read(vpnfd, buf, 4096);
        //TODO: Here we employ packet reader to find the type of service.
        if (len <= 0) {
            if (errno == EAGAIN) {
                usleep(10);
                continue;
            }
            __android_log_print(ANDROID_LOG_DEBUG, "threadOutgoing", "vpn closed: %d, %s", len, strerror(errno));
            break;
        }
        __android_log_print(ANDROID_LOG_DEBUG, "threadOutgoing", "receive %d bytes", len);

        //TODO: we need here a switch case for the selecting the tunnels. We only consider the outgoing packet

        int dscp = get_dscp(buf);
        switch(dscp) {

            case 140 :
                len = sendto(ass_forward_sock31, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                break;
            case 142 :
                len = sendto(ass_forward_sock33, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                break;
            case 143 :
                len = sendto(ass_forward_sock41, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                break;
            case 145 :
                len = sendto(ass_forward_sock43, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                break;
            case 146 :
                len = sendto(exp_forward_sockEF, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                break;
            default :
                len = sendto(class_zero_sockcs0, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        }

        //len = sendto(sock, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (len <= 0) {
            __android_log_print(ANDROID_LOG_DEBUG, "threadOutgoing", "sock closed: %d", len);
            break;
        }
        //__android_log_print(ANDROID_LOG_DEBUG, "threadOutgoing", "forward %d bytes", len);
    }

    // notify other thread
    running = false;
    return 0;
}

/*
void* threadKeepAlive(void* n)
{
    char buf[4096] = {0};
    int len = 1;

    unsigned long int nowTime, idleTime, lastChkTime;
    while(running) {
        nowTime = time(NULL);
        if (lastChkTime == nowTime) {
            usleep(100000); // 0.1 second
            continue;
        }

        idleTime = nowTime - lastRecvTime;
        if (idleTime > 30) {
            // disconnected?
            break;
        }

        if (idleTime >= 15) {
            sendto(sock_cssix, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            __android_log_print(ANDROID_LOG_DEBUG, "threadKeepAlive", "send keep alive %lu", idleTime);
        } else {
            __android_log_print(ANDROID_LOG_DEBUG, "threadKeepAlive", "idle %lu", idleTime);
        }

        lastChkTime = nowTime;
    }

    // notify other thread
    running = false;
    return 0;
}
*/


extern "C"
JNIEXPORT jstring

JNICALL
Java_nodes_helsinki_qosmos_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


extern "C"
JNIEXPORT jintArray
JNICALL
Java_nodes_helsinki_qosmos_NativeTunnels_getTunnelSock(JNIEnv *env, jobject){

    jintArray socks;
    int size = 7;
    socks = (env)->NewIntArray(size);
    if (socks == NULL) {
        return NULL; /* out of memory error thrown */
    }
    int i;
    // fill a temp structure to use to populate the java int array
    jint fill[size];


    if (-1 == sock_cssix) {
        sock_cssix = socket(AF_INET,SOCK_DGRAM,0);
        ass_forward_sock31 = socket(AF_INET,SOCK_DGRAM,0);
        ass_forward_sock33 = socket(AF_INET,SOCK_DGRAM,0);
        ass_forward_sock41 = socket(AF_INET,SOCK_DGRAM,0);
        ass_forward_sock43 = socket(AF_INET,SOCK_DGRAM,0);
        exp_forward_sockEF = socket(AF_INET,SOCK_DGRAM,0);
        class_zero_sockcs0 = socket(AF_INET,SOCK_DGRAM,0);

        int tos = 0x45;
        if(setsockopt(sock_cssix,IPPROTO_IP,IP_TOS,&tos,sizeof(tos)))
            printf("some error happened command channel - CS6 ");

        tos = 0x45;
        if(setsockopt(ass_forward_sock31,IPPROTO_IP,IP_TOS,&tos,sizeof(tos)))
            printf("some error happened");

        tos = 0x45;
        if(setsockopt(ass_forward_sock33,IPPROTO_IP,IP_TOS,&tos,sizeof(tos)))
            printf("some error happened");

        tos = 0x45;
        if(setsockopt(ass_forward_sock41,IPPROTO_IP,IP_TOS,&tos,sizeof(tos)))
            printf("some error happened");

        tos = 0x45;
        if(setsockopt(ass_forward_sock43,IPPROTO_IP,IP_TOS,&tos,sizeof(tos)))
            printf("some error happened");

        tos = 0x45;
        if(setsockopt(exp_forward_sockEF,IPPROTO_IP,IP_TOS,&tos,sizeof(tos)))
            printf("some error happened");

        tos = 0;
        if(setsockopt(class_zero_sockcs0,IPPROTO_IP,IP_TOS,&tos,sizeof(tos)))
            printf("some error happened");


    }
    fcntl(sock_cssix, F_SETFL, O_NONBLOCK);
    fcntl(ass_forward_sock31, F_SETFL, O_NONBLOCK);
    fcntl(ass_forward_sock33, F_SETFL, O_NONBLOCK);
    fcntl(ass_forward_sock41, F_SETFL, O_NONBLOCK);
    fcntl(ass_forward_sock43, F_SETFL, O_NONBLOCK);
    fcntl(exp_forward_sockEF, F_SETFL, O_NONBLOCK);
    fcntl(class_zero_sockcs0, F_SETFL, O_NONBLOCK);


    fill[0] = sock_cssix; // CS6
    fill[1] = ass_forward_sock31;
    fill[2] = ass_forward_sock33;
    fill[3] = ass_forward_sock41;
    fill[4] = ass_forward_sock43;
    fill[5] = exp_forward_sockEF;
    fill[6] = class_zero_sockcs0;
    // move from the temp structure to the java structure
    (env)->SetIntArrayRegion(socks, 0, size, fill);
    return socks;
}


extern "C"
JNIEXPORT jstring

JNICALL
Java_nodes_helsinki_qosmos_NativeTunnels_startTunnel(JNIEnv *env, jobject, jstring ip, jint port, jbyteArray secret) {


    const char *pIp = env->GetStringUTFChars(ip, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(pIp);
    server_addr.sin_port = htons(port);
    env->ReleaseStringUTFChars(ip, pIp);

    jsize secLen = env->GetArrayLength(secret);
    int ctrlPacketLen = secLen + 1;

    char ctrlPacket[1024] = {0};
    jbyte* pSecret = env->GetByteArrayElements(secret, NULL);
    memcpy(ctrlPacket + 1, pSecret, secLen);
    env->ReleaseByteArrayElements(secret, pSecret, JNI_ABORT);

    int send_num = 0;
    while (send_num++ < 2)
    {
        sendto(sock_cssix, ctrlPacket, ctrlPacketLen, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    char recvBuf[2048] = {0};
    int i = 0;
    for (i = 0; i < 50; ++i) {
        int recvLen = recvfrom(sock_cssix, recvBuf, 2048, 0, NULL, NULL);
        if (recvLen > 0 && recvBuf[0] == '\0') {
            recvBuf[recvLen] = 0;
            lastRecvTime = time(NULL);
            return env->NewStringUTF(recvBuf + 1);
        } else {
            usleep(100000);
        }
    }

    return env->NewStringUTF("");

}


extern "C"
JNIEXPORT void
JNICALL

Java_nodes_helsinki_qosmos_NativeTunnels_tunnelLoop(JNIEnv *, jobject, jint fd) {
    // let's start tunnel loop thread
    running = true;

    vpnfd = fd;
    __android_log_print(ANDROID_LOG_DEBUG, "tunnelLoop", "get fd %d", fd);

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return;
    }

    pthread_t in_thread, out_thread, keep_thread;
    pthread_create(&in_thread, NULL, &threadIncoming, NULL);
    pthread_create(&out_thread, NULL, &threadOutgoing, NULL);
    //pthread_create(&keep_thread, NULL, &threadKeepAlive, NULL);
    __android_log_print(ANDROID_LOG_DEBUG, "tunnelLoop", "thread started, looping traffic");

    pthread_join(in_thread,NULL);
    pthread_join(out_thread,NULL);
    //pthread_join(keep_thread,NULL);
    __android_log_print(ANDROID_LOG_DEBUG, "tunnelLoop", "thread joined, end loop");

    close(sock_cssix);
    close(ass_forward_sock31);
    close(ass_forward_sock33);
    close(ass_forward_sock41);
    close(ass_forward_sock43);
    close(exp_forward_sockEF);
    close(class_zero_sockcs0);
    close(vpnfd);

    sock_cssix = -1;
    ass_forward_sock31 = -1;
    ass_forward_sock33 = -1;
    ass_forward_sock41 = -1;
    ass_forward_sock43 = -1;
    exp_forward_sockEF = -1;
    class_zero_sockcs0 = -1;


}


extern "C"
JNIEXPORT void
JNICALL

Java_nodes_helsinki_qosmos_NativeTunnels_tunnelStop(JNIEnv *, jobject, jint) {


    running = false;
    __android_log_print(ANDROID_LOG_DEBUG, "tunnelStop", "stopping");
}



extern "C"
JNIEXPORT void
JNICALL

Java_nodes_helsinki_qosmos_NativeTunnels_dscpTunnelSocks(JNIEnv *, jobject) {

}



/* JNI receives the media context of the device and sets the context*/

extern "C"
JNIEXPORT void
JNICALL

Java_nodes_helsinki_mdiaproximity_MediaContext_setMediaContext(JNIEnv *, jobject, jint context) {


    pthread_mutex_lock(&media_lock);
    device_media_context = context;
    media_context_time = (get_time_second()-1)*1000;
    pthread_mutex_unlock(&media_lock);


    /* Probably we could have a pthread start here which would wait to
     * expire the timer of 7 seconds soon*/



}
