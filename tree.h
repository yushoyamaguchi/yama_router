struct node  {
        struct node      *parent;
        struct node      *child_left;
        struct node      *child_right;
        u_int32_t       daddr_subnet;
        u_int8_t        subnet_mask;
        u_int32_t       next_hop;
        u_int32_t       daddr_full;
};

void calc_subnet(struct node *node);

void node_insert(struct node *join_node, struct node *root);


void tree_destruct(struct node *root);
