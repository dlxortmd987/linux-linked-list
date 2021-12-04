#ifndef _LINUX_GCLIST_H
#define _LINUX_GCLIST_H

// #include <stdio.h>
#include <linux/delay.h>
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized gclist entries.
 */
#define GCLIST_POISON1  ((void *) 0x00100100)
#define GCLIST_POISON2  ((void *) 0x00200200)

struct gclist_head {
    bool flag;
	struct gclist_head *next, *prev;
};

struct hgclist_head {
	struct hgclist_node *first;
};

struct hgclist_node {
	struct hgclist_node *next, **pprev;
};

static inline bool __gclist_add_flag_vaild(
				struct gclist_head *prev,
				struct gclist_head *next)
{
    if(prev->flag || next->flag) {
		return false;
	} else {
		return true;
	}
}

/*
 * Simple doubly linked gclist implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole gclists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

#define GCLIST_HEAD_INIT(name) { &(name), &(name) }

#define GCLIST_HEAD(name) \
	struct gclist_head name = GCLIST_HEAD_INIT(name)

static inline void INIT_GCLIST_HEAD(struct gclist_head *gclist)
{
	gclist->next = gclist;
	gclist->prev = gclist;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal gclist manipulation where we know
 * the prev/next entries already!
 */
#ifndef CONFIG_DEBUG_GCLIST
static inline void __gclist_add(struct gclist_head *new,
			      struct gclist_head *prev,
			      struct gclist_head *next)
{
	// check flag
	while (!__gclist_add_flag_vaild(prev, next))
	{
		msleep(1);
	}
	__sync_lock_test_and_set(&(prev->flag), true);
	__sync_lock_test_and_set(&(next->flag), true);
    
    next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;

    __sync_lock_test_and_set(&(prev->flag), false);
	__sync_lock_test_and_set(&(next->flag), false);
}
#else
extern void __gclist_add(struct gclist_head *new,
			      struct gclist_head *prev,
			      struct gclist_head *next);
#endif

/**
 * gclist_add - add a new entry
 * @new: new entry to be added
 * @head: gclist head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void gclist_add(struct gclist_head *new, struct gclist_head *head)
{
	__gclist_add(new, head, head->next);
}


/**
 * gclist_add_tail - add a new entry
 * @new: new entry to be added
 * @head: gclist head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void gclist_add_tail(struct gclist_head *new, struct gclist_head *head)
{
	__gclist_add(new, head->prev, head);
}

/*
 * Delete a gclist entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal gclist manipulation where we know
 * the prev/next entries already!
 */
static inline void __gclist_del(struct gclist_head * prev, struct gclist_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * gclist_del - deletes entry from gclist.
 * @entry: the element to delete from the gclist.
 * Note: gclist_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
#ifndef CONFIG_DEBUG_GCLIST
static inline void __gclist_del_entry(struct gclist_head *entry)
{
	__gclist_del(entry->prev, entry->next);
}

static inline void gclist_del(struct gclist_head *entry)
{
	__gclist_del(entry->prev, entry->next);
	entry->next = GCLIST_POISON1;
	entry->prev = GCLIST_POISON2;
}
#else
extern void __gclist_del_entry(struct gclist_head *entry);
extern void gclist_del(struct gclist_head *entry);
#endif

/**
 * gclist_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void gclist_replace(struct gclist_head *old,
				struct gclist_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void gclist_replace_init(struct gclist_head *old,
					struct gclist_head *new)
{
	gclist_replace(old, new);
	INIT_GCLIST_HEAD(old);
}

/**
 * gclist_del_init - deletes entry from gclist and reinitialize it.
 * @entry: the element to delete from the gclist.
 */
static inline void gclist_del_init(struct gclist_head *entry)
{
	__gclist_del_entry(entry);
	INIT_GCLIST_HEAD(entry);
}

/**
 * gclist_move - delete from one gclist and add as another's head
 * @gclist: the entry to move
 * @head: the head that will precede our entry
 */
static inline void gclist_move(struct gclist_head *gclist, struct gclist_head *head)
{
	__gclist_del_entry(gclist);
	gclist_add(gclist, head);
}

/**
 * gclist_move_tail - delete from one gclist and add as another's tail
 * @gclist: the entry to move
 * @head: the head that will follow our entry
 */
static inline void gclist_move_tail(struct gclist_head *gclist,
				  struct gclist_head *head)
{
	__gclist_del_entry(gclist);
	gclist_add_tail(gclist, head);
}

/**
 * gclist_is_last - tests whether @gclist is the last entry in gclist @head
 * @gclist: the entry to test
 * @head: the head of the gclist
 */
static inline int gclist_is_last(const struct gclist_head *gclist,
				const struct gclist_head *head)
{
	return gclist->next == head;
}

/**
 * gclist_empty - tests whether a gclist is empty
 * @head: the gclist to test.
 */
static inline int gclist_empty(const struct gclist_head *head)
{
	return head->next == head;
}

/**
 * gclist_empty_careful - tests whether a gclist is empty and not being modified
 * @head: the gclist to test
 *
 * Description:
 * tests whether a gclist is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using gclist_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the gclist entry is gclist_del_init(). Eg. it cannot be used
 * if another CPU could re-gclist_add() it.
 */
static inline int gclist_empty_careful(const struct gclist_head *head)
{
	struct gclist_head *next = head->next;
	return (next == head) && (next == head->prev);
}

/**
 * gclist_rotate_left - rotate the gclist to the left
 * @head: the head of the gclist
 */
static inline void gclist_rotate_left(struct gclist_head *head)
{
	struct gclist_head *first;

	if (!gclist_empty(head)) {
		first = head->next;
		gclist_move_tail(first, head);
	}
}

/**
 * gclist_is_singular - tests whether a gclist has just one entry.
 * @head: the gclist to test.
 */
static inline int gclist_is_singular(const struct gclist_head *head)
{
	return !gclist_empty(head) && (head->next == head->prev);
}

static inline void __gclist_cut_position(struct gclist_head *gclist,
		struct gclist_head *head, struct gclist_head *entry)
{
	struct gclist_head *new_first = entry->next;
	gclist->next = head->next;
	gclist->next->prev = gclist;
	gclist->prev = entry;
	entry->next = gclist;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * gclist_cut_position - cut a gclist into two
 * @gclist: a new gclist to add all removed entries
 * @head: a gclist with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the gclist
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @gclist. You should
 * pass on @entry an element you know is on @head. @gclist
 * should be an empty gclist or a gclist you do not care about
 * losing its data.
 *
 */
static inline void gclist_cut_position(struct gclist_head *gclist,
		struct gclist_head *head, struct gclist_head *entry)
{
	if (gclist_empty(head))
		return;
	if (gclist_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		INIT_GCLIST_HEAD(gclist);
	else
		__gclist_cut_position(gclist, head, entry);
}

static inline void __gclist_splice(const struct gclist_head *gclist,
				 struct gclist_head *prev,
				 struct gclist_head *next)
{
	struct gclist_head *first = gclist->next;
	struct gclist_head *last = gclist->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * gclist_splice - join two gclists, this is designed for stacks
 * @gclist: the new gclist to add.
 * @head: the place to add it in the first gclist.
 */
static inline void gclist_splice(const struct gclist_head *gclist,
				struct gclist_head *head)
{
	if (!gclist_empty(gclist))
		__gclist_splice(gclist, head, head->next);
}

/**
 * gclist_splice_tail - join two gclists, each gclist being a queue
 * @gclist: the new gclist to add.
 * @head: the place to add it in the first gclist.
 */
static inline void gclist_splice_tail(struct gclist_head *gclist,
				struct gclist_head *head)
{
	if (!gclist_empty(gclist))
		__gclist_splice(gclist, head->prev, head);
}

/**
 * gclist_splice_init - join two gclists and reinitialise the emptied gclist.
 * @gclist: the new gclist to add.
 * @head: the place to add it in the first gclist.
 *
 * The gclist at @gclist is reinitialised
 */
static inline void gclist_splice_init(struct gclist_head *gclist,
				    struct gclist_head *head)
{
	if (!gclist_empty(gclist)) {
		__gclist_splice(gclist, head, head->next);
		INIT_GCLIST_HEAD(gclist);
	}
}

/**
 * gclist_splice_tail_init - join two gclists and reinitialise the emptied gclist
 * @gclist: the new gclist to add.
 * @head: the place to add it in the first gclist.
 *
 * Each of the gclists is a queue.
 * The gclist at @gclist is reinitialised
 */
static inline void gclist_splice_tail_init(struct gclist_head *gclist,
					 struct gclist_head *head)
{
	if (!gclist_empty(gclist)) {
		__gclist_splice(gclist, head->prev, head);
		INIT_GCLIST_HEAD(gclist);
	}
}

/**
 * gclist_entry - get the struct for this entry
 * @ptr:	the &struct gclist_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the gclist_struct within the struct.
 */
#define gclist_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * gclist_first_entry - get the first element from a gclist
 * @ptr:	the gclist head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the gclist_struct within the struct.
 *
 * Note, that gclist is expected to be not empty.
 */
#define gclist_first_entry(ptr, type, member) \
	gclist_entry((ptr)->next, type, member)

/**
 * gclist_for_each	-	iterate over a gclist
 * @pos:	the &struct gclist_head to use as a loop cursor.
 * @head:	the head for your gclist.
 */
#define gclist_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * __gclist_for_each	-	iterate over a gclist
 * @pos:	the &struct gclist_head to use as a loop cursor.
 * @head:	the head for your gclist.
 *
 * This variant doesn't differ from gclist_for_each() any more.
 * We don't do prefetching in either case.
 */
#define __gclist_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * gclist_for_each_prev	-	iterate over a gclist backwards
 * @pos:	the &struct gclist_head to use as a loop cursor.
 * @head:	the head for your gclist.
 */
#define gclist_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * gclist_for_each_safe - iterate over a gclist safe against removal of gclist entry
 * @pos:	the &struct gclist_head to use as a loop cursor.
 * @n:		another &struct gclist_head to use as temporary storage
 * @head:	the head for your gclist.
 */
#define gclist_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * gclist_for_each_prev_safe - iterate over a gclist backwards safe against removal of gclist entry
 * @pos:	the &struct gclist_head to use as a loop cursor.
 * @n:		another &struct gclist_head to use as temporary storage
 * @head:	the head for your gclist.
 */
#define gclist_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)

/**
 * gclist_for_each_entry	-	iterate over gclist of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_struct within the struct.
 */
#define gclist_for_each_entry(pos, head, member)				\
	for (pos = gclist_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = gclist_entry(pos->member.next, typeof(*pos), member))

/**
 * gclist_for_each_entry_reverse - iterate backwards over gclist of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_struct within the struct.
 */
#define gclist_for_each_entry_reverse(pos, head, member)			\
	for (pos = gclist_entry((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = gclist_entry(pos->member.prev, typeof(*pos), member))

/**
 * gclist_prepare_entry - prepare a pos entry for use in gclist_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the gclist
 * @member:	the name of the gclist_struct within the struct.
 *
 * Prepares a pos entry for use as a start point in gclist_for_each_entry_continue().
 */
#define gclist_prepare_entry(pos, head, member) \
	((pos) ? : gclist_entry(head, typeof(*pos), member))

/**
 * gclist_for_each_entry_continue - continue iteration over gclist of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_struct within the struct.
 *
 * Continue to iterate over gclist of given type, continuing after
 * the current position.
 */
#define gclist_for_each_entry_continue(pos, head, member) 		\
	for (pos = gclist_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head);	\
	     pos = gclist_entry(pos->member.next, typeof(*pos), member))

/**
 * gclist_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_struct within the struct.
 *
 * Start to iterate over gclist of given type backwards, continuing after
 * the current position.
 */
#define gclist_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = gclist_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head);	\
	     pos = gclist_entry(pos->member.prev, typeof(*pos), member))

/**
 * gclist_for_each_entry_from - iterate over gclist of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_struct within the struct.
 *
 * Iterate over gclist of given type, continuing from current position.
 */
#define gclist_for_each_entry_from(pos, head, member) 			\
	for (; &pos->member != (head);	\
	     pos = gclist_entry(pos->member.next, typeof(*pos), member))

/**
 * gclist_for_each_entry_safe - iterate over gclist of given type safe against removal of gclist entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_struct within the struct.
 */
#define gclist_for_each_entry_safe(pos, n, head, member)			\
	for (pos = gclist_entry((head)->next, typeof(*pos), member),	\
		n = gclist_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = gclist_entry(n->member.next, typeof(*n), member))

/**
 * gclist_for_each_entry_safe_continue - continue gclist iteration safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_struct within the struct.
 *
 * Iterate over gclist of given type, continuing after current point,
 * safe against removal of gclist entry.
 */
#define gclist_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = gclist_entry(pos->member.next, typeof(*pos), member), 		\
		n = gclist_entry(pos->member.next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = gclist_entry(n->member.next, typeof(*n), member))

/**
 * gclist_for_each_entry_safe_from - iterate over gclist from current point safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_struct within the struct.
 *
 * Iterate over gclist of given type from current point, safe against
 * removal of gclist entry.
 */
#define gclist_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = gclist_entry(pos->member.next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = gclist_entry(n->member.next, typeof(*n), member))

/**
 * gclist_for_each_entry_safe_reverse - iterate backwards over gclist safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_struct within the struct.
 *
 * Iterate backwards over gclist of given type, safe against removal
 * of gclist entry.
 */
#define gclist_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = gclist_entry((head)->prev, typeof(*pos), member),	\
		n = gclist_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = gclist_entry(n->member.prev, typeof(*n), member))

/**
 * gclist_safe_reset_next - reset a stale gclist_for_each_entry_safe loop
 * @pos:	the loop cursor used in the gclist_for_each_entry_safe loop
 * @n:		temporary storage used in gclist_for_each_entry_safe
 * @member:	the name of the gclist_struct within the struct.
 *
 * gclist_safe_reset_next is not safe to use in general if the gclist may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the gclist,
 * and gclist_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define gclist_safe_reset_next(pos, n, member)				\
	n = gclist_entry(pos->member.next, typeof(*pos), member)

/*
 * Double linked gclists with a single pointer gclist head.
 * Mostly useful for hash tables where the two pointer gclist head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */

#define HGCLIST_HEAD_INIT { .first = NULL }
#define HGCLIST_HEAD(name) struct hgclist_head name = {  .first = NULL }
#define INIT_HGCLIST_HEAD(ptr) ((ptr)->first = NULL)
static inline void INIT_HGCLIST_NODE(struct hgclist_node *h)
{
	h->next = NULL;
	h->pprev = NULL;
}

static inline int hgclist_unhashed(const struct hgclist_node *h)
{
	return !h->pprev;
}

static inline int hgclist_empty(const struct hgclist_head *h)
{
	return !h->first;
}

static inline void __hgclist_del(struct hgclist_node *n)
{
	struct hgclist_node *next = n->next;
	struct hgclist_node **pprev = n->pprev;
	*pprev = next;
	if (next)
		next->pprev = pprev;
}

static inline void hgclist_del(struct hgclist_node *n)
{
	__hgclist_del(n);
	n->next = GCLIST_POISON1;
	n->pprev = GCLIST_POISON2;
}

static inline void hgclist_del_init(struct hgclist_node *n)
{
	if (!hgclist_unhashed(n)) {
		__hgclist_del(n);
		INIT_HGCLIST_NODE(n);
	}
}

static inline void hgclist_add_head(struct hgclist_node *n, struct hgclist_head *h)
{
	struct hgclist_node *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

/* next must be != NULL */
static inline void hgclist_add_before(struct hgclist_node *n,
					struct hgclist_node *next)
{
	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	*(n->pprev) = n;
}

static inline void hgclist_add_after(struct hgclist_node *n,
					struct hgclist_node *next)
{
	next->next = n->next;
	n->next = next;
	next->pprev = &n->next;

	if(next->next)
		next->next->pprev  = &next->next;
}

/* after that we'll appear to be on some hgclist and hgclist_del will work */
static inline void hgclist_add_fake(struct hgclist_node *n)
{
	n->pprev = &n->next;
}

/*
 * Move a gclist from one gclist head to another. Fixup the pprev
 * reference of the first entry if it exists.
 */
static inline void hgclist_move_gclist(struct hgclist_head *old,
				   struct hgclist_head *new)
{
	new->first = old->first;
	if (new->first)
		new->first->pprev = &new->first;
	old->first = NULL;
}

#define hgclist_entry(ptr, type, member) container_of(ptr,type,member)

#define hgclist_for_each(pos, head) \
	for (pos = (head)->first; pos ; pos = pos->next)

#define hgclist_for_each_safe(pos, n, head) \
	for (pos = (head)->first; pos && ({ n = pos->next; 1; }); \
	     pos = n)

/**
 * hgclist_for_each_entry	- iterate over gclist of given type
 * @tpos:	the type * to use as a loop cursor.
 * @pos:	the &struct hgclist_node to use as a loop cursor.
 * @head:	the head for your gclist.
 * @member:	the name of the hgclist_node within the struct.
 */
#define hgclist_for_each_entry(tpos, pos, head, member)			 \
	for (pos = (head)->first;					 \
	     pos &&							 \
		({ tpos = hgclist_entry(pos, typeof(*tpos), member); 1;}); \
	     pos = pos->next)

/**
 * hgclist_for_each_entry_continue - iterate over a hgclist continuing after current point
 * @tpos:	the type * to use as a loop cursor.
 * @pos:	the &struct hgclist_node to use as a loop cursor.
 * @member:	the name of the hgclist_node within the struct.
 */
#define hgclist_for_each_entry_continue(tpos, pos, member)		 \
	for (pos = (pos)->next;						 \
	     pos &&							 \
		({ tpos = hgclist_entry(pos, typeof(*tpos), member); 1;}); \
	     pos = pos->next)

/**
 * hgclist_for_each_entry_from - iterate over a hgclist continuing from current point
 * @tpos:	the type * to use as a loop cursor.
 * @pos:	the &struct hgclist_node to use as a loop cursor.
 * @member:	the name of the hgclist_node within the struct.
 */
#define hgclist_for_each_entry_from(tpos, pos, member)			 \
	for (; pos &&							 \
		({ tpos = hgclist_entry(pos, typeof(*tpos), member); 1;}); \
	     pos = pos->next)

/**
 * hgclist_for_each_entry_safe - iterate over gclist of given type safe against removal of gclist entry
 * @tpos:	the type * to use as a loop cursor.
 * @pos:	the &struct hgclist_node to use as a loop cursor.
 * @n:		another &struct hgclist_node to use as temporary storage
 * @head:	the head for your gclist.
 * @member:	the name of the hgclist_node within the struct.
 */
#define hgclist_for_each_entry_safe(tpos, pos, n, head, member) 		 \
	for (pos = (head)->first;					 \
	     pos && ({ n = pos->next; 1; }) && 				 \
		({ tpos = hgclist_entry(pos, typeof(*tpos), member); 1;}); \
	     pos = n)

#endif