struct gclist_head {
    bool flag;
	struct gclist_head *next, *prev;
};

struct gchlist_head {
	struct gchlist_node *first;
};

struct gchlist_node {
	struct gchlist_node *next, **pprev;
};
