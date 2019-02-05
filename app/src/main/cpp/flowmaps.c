//
// Created by Hoque, Mohammad on 03/02/2019.
//

#include <stdio.h>
#include <stdlib.h>

struct flow{
    long key; // flow time
    long last_update;
    struct ipv4 *val; // flow
    struct flow *next;
};

struct candidate{
    long key; // flow time
    struct ipv4 *val; // flow
    struct flow *next;
};

struct flowtable{
    int size;
    struct flow **list;
};

struct candid_table{
    int size;
    struct candidate **list;
};

struct flowtable *create_flow_table(int size){
    struct flowtable *t = (struct flowtable*)malloc(sizeof(struct flowtable));
    t->size = size;
    t->list = (struct flow**)malloc(sizeof(struct flow*)*size);
    int i;
    for(i=0;i<size;i++)
        t->list[i] = NULL;
    return t;
}


/* This table should store values only when a media context is initialized. */
struct candid_table *create_candid_table(int size){
    struct candid_table *t = (struct candid_table*)malloc(sizeof(struct candid_table));
    t->size = size;
    t->list = (struct candidate**)malloc(sizeof(struct candidate*)*size);
    int i;
    for(i=0;i<size;i++)
        t->list[i] = NULL;
    return t;
}


int hashCode(struct flowtable *t,int key){
    if(key<0)
        return -(key%t->size);
    return key%t->size;
}
void insert_flow(struct flowtable *t,int key,int val){
    int pos = hashCode(t,key);
    struct flow *list = t->list[pos];

    struct flow *temp = list;
    while(temp){
        if(temp->key==key){
            temp->val = val;
            return;
        }
        temp = temp->next;
    }
    struct flow *newNode = (struct node*)malloc(sizeof(struct flow));
    newNode->key = key;
    newNode->val = val;
    newNode->next = list;
    t->list[pos] = newNode;
}


int lookup_flow(struct flowtable *t,int key){
    int pos = hashCode(t,key);
    struct flow *list = t->list[pos];
    struct flow *temp = list;
    while(temp){
        if(temp->key==key){
            return temp->val;
        }
        temp = temp->next;
    }
    return -1;
}

void insert_candidate(struct candidate_table *t,int key,int val){
    int pos = hashCode(t,key);
    struct candidate *list = t->list[pos];

    struct candidate *temp = list;
    while(temp){
        if(temp->key==key){
            temp->val = val;
            return;
        }
        temp = temp->next;
    }
    struct candidate *newNode = (struct candudate*)malloc(sizeof(struct candidate));
    newNode->key = key;
    newNode->val = val;
    newNode->next = list;
    t->list[pos] = newNode;
}


int lookup_candidate(struct candidate_table *t,int key){
    int pos = hashCode(t,key);
    struct candidate *list = t->list[pos];
    struct candidate *temp = list;
    while(temp){
        if(temp->key==key){
            return temp->val;
        }
        temp = temp->next;
    }
    return -1;
}


/*

int test_methods(){
    struct table *t = create_flow_table(5);
    insert(t,2,3);
    insert(t,5,4);
    printf("%d",lookup(t,5));
    return 0;
}*/