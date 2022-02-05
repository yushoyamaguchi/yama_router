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
#include    <limits.h>
#include	"netutil.h"
#include	"base.h"
#include	"ip2mac.h"
#include	"sendBuf.h"
#include	"json_config.h"
#include	"tree.h"


void init_tree_node(struct node *node){
        node->parent=NULL;
        node->child_one=NULL;
        node->child_zero=NULL;
        node->daddr_subnet=0;
        node->subnet_mask=0;
        node->next_hop=0;
        node->daddr_full=0;
        node->is_empty=1;
        node->is_root=0;
}


void printb(u_int32_t v) {
  unsigned int mask = (int)1 << (sizeof(v) * CHAR_BIT - 1);
  do putchar(mask & v ? '1' : '0');
  while (mask >>= 1);

  printf("\n");
}

u_int32_t num_to_mask(int n){
    int i=0;
    u_int32_t buf=0;
    for(i=0;i<n;i++){
        buf=2*buf+1;
    }
    return buf;
}

void print_addr_of_binary(u_int32_t addr_binary){
    struct in_addr addr;
    addr.s_addr=addr_binary;
    char buf[128];
    strcpy(buf,inet_ntoa(addr));
    printf("%s\n",buf);
}

void calc_subnet(struct node *node){
    u_int32_t mask;
    mask=num_to_mask(node->subnet_mask);
    node->daddr_subnet=(node->daddr_full & mask);
}

void make_empty_node(struct node *parent,struct node *myself,u_int32_t mask){
    myself->subnet_mask=parent->subnet_mask+1;
    myself->daddr_subnet=(parent->daddr_subnet|htonl(mask));
    myself->is_empty=1;
}

void node_insert(struct node *join_node, struct node *root){
    struct node *search;
    struct node *parent_candidate;
    struct node *new_node;
    search=root;
    u_int8_t i=0;
    u_int32_t host_order_subnet;
    host_order_subnet=ntohl(join_node->daddr_subnet);
    u_int32_t mask = (int)1 << (sizeof(u_int32_t) * CHAR_BIT - 1);
    u_int32_t mask_left_one = (int)1 << (sizeof(u_int32_t) * CHAR_BIT - 1);

    for(i=0;i<join_node->subnet_mask;i++){
        if(mask&host_order_subnet){
            if(search->child_one==NULL){
                new_node=malloc(sizeof(struct node));
                init_tree_node(new_node);
                new_node->parent=search;
                search->child_one=new_node;
                make_empty_node(search,new_node,(mask&host_order_subnet));
            }
            parent_candidate=search;
            search=search->child_one; 
        }
        else{
            if(search->child_zero==NULL){
                new_node=malloc(sizeof(struct node));
                init_tree_node(new_node);
                new_node->parent=search;
                search->child_zero=new_node;
                make_empty_node(search,new_node,(mask&host_order_subnet));
            }
            parent_candidate=search;
            search=search->child_zero; 
        }
        mask>>=1;
        mask_left_one=(mask_left_one|mask);
    }
    //searchが作りこむべきノード
    if(search->is_empty==0){
        //元の情報をdeleteしたことを通知
    }
    memcpy(search,join_node,sizeof(struct node));
    search->parent=parent_candidate;
    //print_addr_of_binary(search->next_hop);    
    free(join_node);
}

void tree_destruct(struct node *root){
}















