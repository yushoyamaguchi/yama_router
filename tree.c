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
    //printb(node->daddr_full);
    //printf("%x\n",node->daddr_full);
    u_int32_t mask;
    mask=num_to_mask(node->subnet_mask);
    node->daddr_subnet=(node->daddr_full & mask);
    //printb(node->daddr_subnet);
    //printf("%x\n",node->daddr_subnet);
    //print_addr_of_binary(node->daddr_subnet);
}

void node_insert(struct node *join_node, struct node *root){
    struct node *search;
    struct node *parent_candidate;
    search=root;
    int i=0;
    u_int32_t host_order_subnet;
    host_order_subnet=ntohl(join_node->daddr_subnet);
    u_int32_t mask = (int)1 << (sizeof(host_order_subnet) * CHAR_BIT - 1);
    for(i=0;i<join_node->subnet_mask;i++){
        if(mask&host_order_subnet){
            if(search->child_one==NULL){
                //空ノード作る
                //else外すのもアリ
            }
            else{
                parent_candidate=search;
                search=search->child_one; 
            }
        }
        else{
            if(search->child_zero==NULL){
                //空ノード作る
                //else外すのもアリ
            }
            else{
                parent_candidate=search;
                search=search->child_one; 
            }
        }
        mask>>=1;
    }
    //searchが作りこむべきノード

    
    free(join_node);
}

void tree_destruct(struct node *root){
}















