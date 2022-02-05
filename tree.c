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

int is_not_zero(u_int32_t num){
    if(num==0) return 0;
    else return 1;
}

void copy_tree_position_info(struct node *from,struct node *to){
    to->child_zero=from->child_zero;
    to->child_one=from->child_one;
    to->parent=from->parent;
    to->is_root=from->is_root;
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
    struct node *children[2];
    int zero_or_one;

    for(i=0;i<join_node->subnet_mask;i++){
        children[0]=search->child_zero;
        children[1]=search->child_one;
        zero_or_one=is_not_zero(mask&host_order_subnet);
        if(children[zero_or_one]==NULL){
            new_node=malloc(sizeof(struct node));
            init_tree_node(new_node);
            new_node->parent=search;
            children[zero_or_one]=new_node;
            make_empty_node(search,new_node,(mask&host_order_subnet));
        }
        parent_candidate=search;
        search=children[zero_or_one];
        mask>>=1;
        mask_left_one=(mask_left_one|mask);
    }
    if(search->is_empty==0){
        //元の情報をdeleteしたことを通知
    }
    search->parent=parent_candidate;
    copy_tree_position_info(search,join_node);
    memcpy(search,join_node,sizeof(struct node));

    printf("dest_addr_subnet:");
    print_addr_of_binary(search->daddr_subnet);
    printf("next_hop:");
    print_addr_of_binary(search->next_hop);    

    free(join_node);
}

void node_del(struct node *del_node){
    if((del_node->child_zero==NULL)&&(del_node->child_one==NULL)){
        tree_destruct(del_node);
    }
    else{
        int is_root_buf=del_node->is_root;
        init_tree_node(del_node);
        del_node->is_root=is_root_buf;
    }
}

void tree_destruct(struct node *root){
    if(root->child_zero!=NULL){
        tree_destruct(root->child_zero);
    }
    if(root->child_one!=NULL){
        tree_destruct(root->child_one);
    }
    free(root);
}















