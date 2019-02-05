//
// Created by Hoque, Mohammad on 03/02/2019.
//

#ifndef QOSMOS_FLOWMAPS_H
#define QOSMOS_FLOWMAPS_H



struct flowtable *create_flow_table(int size);
struct candid_table *create_candid_table(int size);
void insert_flow(struct flowtable *t,int key,int val);
int lookup_flow(struct flowtable *t,int key);
void insert_candidate(struct candidate_table *t,int key,int val);
int lookup_candidate(struct candidate_table *t,int key);

#endif //QOSMOS_FLOWMAPS_H
