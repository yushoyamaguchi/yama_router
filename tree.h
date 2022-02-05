#define INT_BIT_NUM 32

struct node  {
        struct node      *parent;
        struct node      *child_zero;
        struct node      *child_one;
        u_int32_t       daddr_subnet;
        u_int8_t        subnet_mask;
        u_int32_t       next_hop;
        u_int32_t       daddr_full;
        int             is_empty;
        int             is_root;
};

void init_tree_node(struct node *node);

void calc_subnet(struct node *node);


void node_insert(struct node *join_node, struct node *root);


void tree_destruct(struct node *root);


