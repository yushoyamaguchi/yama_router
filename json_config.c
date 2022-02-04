#include	<stdio.h>
#include	<string.h>
#include	<unistd.h>
#include	<poll.h>
#include	<errno.h>
#include	<signal.h>
#include	<stdarg.h>
#include	<sys/socket.h>
#include	<sys/types.h>
#include	<bits/types.h>
#include	<arpa/inet.h>
#include	<netinet/if_ether.h>
#include	<netinet/ip.h>
#include	<netinet/ip_icmp.h>
#include	<pthread.h>
#include	<jansson.h>

#include	"json_config.h"
#include	"tree.h"

struct node *json_to_node(json_t *table_object){
	struct node *insert_node;
	insert_node=malloc(sizeof(struct node));
	return insert_node;
}

void json_read(PARAM_new *pa,json_t *json_object,json_error_t *jerror,struct node *root){
    json_object=json_load_file("./conf.json",0,jerror);
    if(json_object==NULL){
        printf("cannot read config json\n");
        exit(1);
    }
    int i=0;
    char buf[128];
    json_t *interfaces_array;
    json_t *interfaces_object;
    interfaces_object=malloc(sizeof(json_t));
    interfaces_array=json_object_get(json_object,"interfaces");
    json_array_foreach(interfaces_array,i,interfaces_object){
        strcpy(buf,json_string_value(interfaces_object));
        pa->Device[i]=malloc(20*sizeof(char));
        strncpy(pa->Device[i],buf,20*sizeof(char));
    }
    json_t *table_array;
    json_t *table_object;
    table_object=malloc(sizeof(json_t));
    table_array=json_object_get(json_object,"routing_table");
    struct node *node;
    json_array_foreach(table_array,i,table_object){
        //テーブルに格納
        node=json_to_node(table_object);
        node_insert(node,root);
    }
}


