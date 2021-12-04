/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_GClist_H
#define _LINUX_GClist_H

#include <linux/container_of.h>
#include <linux/types.h>
#include "gctypes.h"
#include <linux/stddef.h>
#include <linux/poison.h>
#include <linux/const.h>
#include <linux/delay.h>

#include <asm/barrier.h>

/*
 * Circular doubly linked gclist implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole gclists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

#define gclist_HEAD_INIT(name) { &(name), &(name) }

#define gclist_HEAD(name) \
	struct gclist_head name = gclist_HEAD_INIT(name)

/**
 * INIT_gclist_HEAD - Initialize a gclist_head structure
 * @gclist: gclist_head structure to be initialized.
 *
 * Initializes the gclist_head to point to itself.  If it is a gclist header,
 * the result is an empty gclist.
 */
static inline void INIT_GCLIST_HEAD(struct gclist_head *gclist)
{
	WRITE_ONCE(gclist->next, gclist);
	gclist->prev = gclist;
}

#ifdef CONFIG_DEBUG_GCLIST
extern bool __gclist_add_valid(struct gclist_head *new,
			      struct gclist_head *prev,
			      struct gclist_head *next);
extern bool __gclist_del_entry_valid(struct gclist_head *entry);
#else
static inline bool __gclist_add_valid(struct gclist_head *new,
				struct gclist_head *prev,
				struct gclist_head *next)
{
	return true;
}
static inline bool __gclist_del_entry_valid(struct gclist_head *entry)
{
	return true;
}

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
static inline bool __gclist_del_entry_valid(struct gclist_head *entry)
{
	return true;
}
#endif


// 바꿀 부분
/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal gclist manipulation where we know
 * the prev/next entries already!
 */
static inline void __gclist_add(struct gclist_head *new,
			      struct gclist_head *prev,
			      struct gclist_head *next)
{
	if (!__gclist_add_valid(new, prev, next))
		return;
	
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
	WRITE_ONCE(prev->next, new);
	__sync_lock_test_and_set(&(prev->flag), false);
	__sync_lock_test_and_set(&(next->flag), false);
}
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

//바꿀 부분
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
	WRITE_ONCE(prev->next, next);
}

/*
 * Delete a gclist entry and clear the 'prev' pointer.
 *
 * This is a special-purpose gclist clearing method used in the networking code
 * for gclists allocated as per-cpu, where we don't want to incur the extra
 * WRITE_ONCE() overhead of a regular gclist_del_init(). The code that uses this
 * needs to check the node 'prev' pointer instead of calling gclist_empty().
 */
static inline void __gclist_del_clearprev(struct gclist_head *entry)
{
	__gclist_del(entry->prev, entry->next);
	entry->prev = NULL;
}

static inline void __gclist_del_entry(struct gclist_head *entry)
{
	if (!__gclist_del_entry_valid(entry))
		return;

	__gclist_del(entry->prev, entry->next);
}

/**
 * gclist_del - deletes entry from gclist.
 * @entry: the element to delete from the gclist.
 * Note: gclist_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void gclist_del(struct gclist_head *entry)
{
	__gclist_del_entry(entry);
	entry->next = gclist_POISON1;
	entry->prev = gclist_POISON2;
}

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

/**
 * gclist_replace_init - replace old entry by new one and initialize the old one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void gclist_replace_init(struct gclist_head *old,
				     struct gclist_head *new)
{
	gclist_replace(old, new);
	INIT_gclist_HEAD(old);
}

/**
 * gclist_swap - replace entry1 with entry2 and re-add entry1 at entry2's position
 * @entry1: the location to place entry2
 * @entry2: the location to place entry1
 */
static inline void gclist_swap(struct gclist_head *entry1,
			     struct gclist_head *entry2)
{
	struct gclist_head *pos = entry2->prev;

	gclist_del(entry2);
	gclist_replace(entry1, entry2);
	if (pos == entry1)
		pos = entry2;
	gclist_add(entry1, pos);
}

/**
 * gclist_del_init - deletes entry from gclist and reinitialize it.
 * @entry: the element to delete from the gclist.
 */
static inline void gclist_del_init(struct gclist_head *entry)
{
	__gclist_del_entry(entry);
	INIT_gclist_HEAD(entry);
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
 * gclist_bulk_move_tail - move a subsection of a gclist to its tail
 * @head: the head that will follow our entry
 * @first: first entry to move
 * @last: last entry to move, can be the same as first
 *
 * Move all entries between @first and including @last before @head.
 * All three entries must belong to the same linked gclist.
 */
static inline void gclist_bulk_move_tail(struct gclist_head *head,
				       struct gclist_head *first,
				       struct gclist_head *last)
{
	first->prev->next = last->next;
	last->next->prev = first->prev;

	head->prev->next = first;
	first->prev = head->prev;

	last->next = head;
	head->prev = last;
}

/**
 * gclist_is_first -- tests whether @gclist is the first entry in gclist @head
 * @gclist: the entry to test
 * @head: the head of the gclist
 */
static inline int gclist_is_first(const struct gclist_head *gclist,
					const struct gclist_head *head)
{
	return gclist->prev == head;
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
	return READ_ONCE(head->next) == head;
}

/**
 * gclist_del_init_careful - deletes entry from gclist and reinitialize it.
 * @entry: the element to delete from the gclist.
 *
 * This is the same as gclist_del_init(), except designed to be used
 * together with gclist_empty_careful() in a way to guarantee ordering
 * of other memory operations.
 *
 * Any memory operations done before a gclist_del_init_careful() are
 * guaranteed to be visible after a gclist_empty_careful() test.
 */
static inline void gclist_del_init_careful(struct gclist_head *entry)
{
	__gclist_del_entry(entry);
	entry->prev = entry;
	smp_store_release(&entry->next, entry);
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
	struct gclist_head *next = smp_load_acquire(&head->next);
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
 * gclist_rotate_to_front() - Rotate gclist to specific item.
 * @gclist: The desired new front of the gclist.
 * @head: The head of the gclist.
 *
 * Rotates gclist so that @gclist becomes the new front of the gclist.
 */
static inline void gclist_rotate_to_front(struct gclist_head *gclist,
					struct gclist_head *head)
{
	/*
	 * Deletes the gclist head from the gclist denoted by @head and
	 * places it as the tail of @gclist, this effectively rotates the
	 * gclist so that @gclist is at the front.
	 */
	gclist_move_tail(head, gclist);
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
		INIT_gclist_HEAD(gclist);
	else
		__gclist_cut_position(gclist, head, entry);
}

/**
 * gclist_cut_before - cut a gclist into two, before given entry
 * @gclist: a new gclist to add all removed entries
 * @head: a gclist with entries
 * @entry: an entry within head, could be the head itself
 *
 * This helper moves the initial part of @head, up to but
 * excluding @entry, from @head to @gclist.  You should pass
 * in @entry an element you know is on @head.  @gclist should
 * be an empty gclist or a gclist you do not care about losing
 * its data.
 * If @entry == @head, all entries on @head are moved to
 * @gclist.
 */
static inline void gclist_cut_before(struct gclist_head *gclist,
				   struct gclist_head *head,
				   struct gclist_head *entry)
{
	if (head->next == entry) {
		INIT_gclist_HEAD(gclist);
		return;
	}
	gclist->next = head->next;
	gclist->next->prev = gclist;
	gclist->prev = entry->prev;
	gclist->prev->next = gclist;
	head->next = entry;
	entry->prev = head;
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
		INIT_gclist_HEAD(gclist);
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
		INIT_gclist_HEAD(gclist);
	}
}

/**
 * gclist_entry - get the struct for this entry
 * @ptr:	the &struct gclist_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the gclist_head within the struct.
 */
#define gclist_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * gclist_first_entry - get the first element from a gclist
 * @ptr:	the gclist head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the gclist_head within the struct.
 *
 * Note, that gclist is expected to be not empty.
 */
#define gclist_first_entry(ptr, type, member) \
	gclist_entry((ptr)->next, type, member)

/**
 * gclist_last_entry - get the last element from a gclist
 * @ptr:	the gclist head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the gclist_head within the struct.
 *
 * Note, that gclist is expected to be not empty.
 */
#define gclist_last_entry(ptr, type, member) \
	gclist_entry((ptr)->prev, type, member)

/**
 * gclist_first_entry_or_null - get the first element from a gclist
 * @ptr:	the gclist head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the gclist_head within the struct.
 *
 * Note that if the gclist is empty, it returns NULL.
 */
#define gclist_first_entry_or_null(ptr, type, member) ({ \
	struct gclist_head *head__ = (ptr); \
	struct gclist_head *pos__ = READ_ONCE(head__->next); \
	pos__ != head__ ? gclist_entry(pos__, type, member) : NULL; \
})

/**
 * gclist_next_entry - get the next element in gclist
 * @pos:	the type * to cursor
 * @member:	the name of the gclist_head within the struct.
 */
#define gclist_next_entry(pos, member) \
	gclist_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * gclist_prev_entry - get the prev element in gclist
 * @pos:	the type * to cursor
 * @member:	the name of the gclist_head within the struct.
 */
#define gclist_prev_entry(pos, member) \
	gclist_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * gclist_for_each	-	iterate over a gclist
 * @pos:	the &struct gclist_head to use as a loop cursor.
 * @head:	the head for your gclist.
 */
#define gclist_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * gclist_for_each_continue - continue iteration over a gclist
 * @pos:	the &struct gclist_head to use as a loop cursor.
 * @head:	the head for your gclist.
 *
 * Continue to iterate over a gclist, continuing after the current position.
 */
#define gclist_for_each_continue(pos, head) \
	for (pos = pos->next; pos != (head); pos = pos->next)

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
 * gclist_entry_is_head - test if the entry points to the head of the gclist
 * @pos:	the type * to cursor
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_head within the struct.
 */
#define gclist_entry_is_head(pos, head, member)				\
	(&pos->member == (head))

//바꿀 부분
/**
 * gclist_for_each_entry	-	iterate over gclist of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_head within the struct.
 */
#define gclist_for_each_entry(pos, head, member)				\
	for (pos = gclist_first_entry(head, typeof(*pos), member);	\
	     !gclist_entry_is_head(pos, head, member);			\
	     pos = gclist_next_entry(pos, member))

/**
 * gclist_for_each_entry_reverse - iterate backwards over gclist of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_head within the struct.
 */
#define gclist_for_each_entry_reverse(pos, head, member)			\
	for (pos = gclist_last_entry(head, typeof(*pos), member);		\
	     !gclist_entry_is_head(pos, head, member); 			\
	     pos = gclist_prev_entry(pos, member))

/**
 * gclist_prepare_entry - prepare a pos entry for use in gclist_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the gclist
 * @member:	the name of the gclist_head within the struct.
 *
 * Prepares a pos entry for use as a start point in gclist_for_each_entry_continue().
 */
#define gclist_prepare_entry(pos, head, member) \
	((pos) ? : gclist_entry(head, typeof(*pos), member))

/**
 * gclist_for_each_entry_continue - continue iteration over gclist of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_head within the struct.
 *
 * Continue to iterate over gclist of given type, continuing after
 * the current position.
 */
#define gclist_for_each_entry_continue(pos, head, member) 		\
	for (pos = gclist_next_entry(pos, member);			\
	     !gclist_entry_is_head(pos, head, member);			\
	     pos = gclist_next_entry(pos, member))

/**
 * gclist_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_head within the struct.
 *
 * Start to iterate over gclist of given type backwards, continuing after
 * the current position.
 */
#define gclist_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = gclist_prev_entry(pos, member);			\
	     !gclist_entry_is_head(pos, head, member);			\
	     pos = gclist_prev_entry(pos, member))

/**
 * gclist_for_each_entry_from - iterate over gclist of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_head within the struct.
 *
 * Iterate over gclist of given type, continuing from current position.
 */
#define gclist_for_each_entry_from(pos, head, member) 			\
	for (; !gclist_entry_is_head(pos, head, member);			\
	     pos = gclist_next_entry(pos, member))

/**
 * gclist_for_each_entry_from_reverse - iterate backwards over gclist of given type
 *                                    from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_head within the struct.
 *
 * Iterate backwards over gclist of given type, continuing from current position.
 */
#define gclist_for_each_entry_from_reverse(pos, head, member)		\
	for (; !gclist_entry_is_head(pos, head, member);			\
	     pos = gclist_prev_entry(pos, member))

/**
 * gclist_for_each_entry_safe - iterate over gclist of given type safe against removal of gclist entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_head within the struct.
 */
#define gclist_for_each_entry_safe(pos, n, head, member)			\
	for (pos = gclist_first_entry(head, typeof(*pos), member),	\
		n = gclist_next_entry(pos, member);			\
	     !gclist_entry_is_head(pos, head, member); 			\
	     pos = n, n = gclist_next_entry(n, member))

/**
 * gclist_for_each_entry_safe_continue - continue gclist iteration safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_head within the struct.
 *
 * Iterate over gclist of given type, continuing after current point,
 * safe against removal of gclist entry.
 */
#define gclist_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = gclist_next_entry(pos, member), 				\
		n = gclist_next_entry(pos, member);				\
	     !gclist_entry_is_head(pos, head, member);				\
	     pos = n, n = gclist_next_entry(n, member))

/**
 * gclist_for_each_entry_safe_from - iterate over gclist from current point safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_head within the struct.
 *
 * Iterate over gclist of given type from current point, safe against
 * removal of gclist entry.
 */
#define gclist_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = gclist_next_entry(pos, member);					\
	     !gclist_entry_is_head(pos, head, member);				\
	     pos = n, n = gclist_next_entry(n, member))

/**
 * gclist_for_each_entry_safe_reverse - iterate backwards over gclist safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your gclist.
 * @member:	the name of the gclist_head within the struct.
 *
 * Iterate backwards over gclist of given type, safe against removal
 * of gclist entry.
 */
#define gclist_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = gclist_last_entry(head, typeof(*pos), member),		\
		n = gclist_prev_entry(pos, member);			\
	     !gclist_entry_is_head(pos, head, member); 			\
	     pos = n, n = gclist_prev_entry(n, member))

/**
 * gclist_safe_reset_next - reset a stale gclist_for_each_entry_safe loop
 * @pos:	the loop cursor used in the gclist_for_each_entry_safe loop
 * @n:		temporary storage used in gclist_for_each_entry_safe
 * @member:	the name of the gclist_head within the struct.
 *
 * gclist_safe_reset_next is not safe to use in general if the gclist may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the gclist,
 * and gclist_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define gclist_safe_reset_next(pos, n, member)				\
	n = gclist_next_entry(pos, member)

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

/**
 * hgclist_unhashed - Has node been removed from gclist and reinitialized?
 * @h: Node to be checked
 *
 * Not that not all removal functions will leave a node in unhashed
 * state.  For example, hgclist_nulls_del_init_rcu() does leave the
 * node in unhashed state, but hgclist_nulls_del() does not.
 */
static inline int hgclist_unhashed(const struct hgclist_node *h)
{
	return !h->pprev;
}

/**
 * hgclist_unhashed_lockless - Version of hgclist_unhashed for lockless use
 * @h: Node to be checked
 *
 * This variant of hgclist_unhashed() must be used in lockless contexts
 * to avoid potential load-tearing.  The READ_ONCE() is paired with the
 * various WRITE_ONCE() in hgclist helpers that are defined below.
 */
static inline int hgclist_unhashed_lockless(const struct hgclist_node *h)
{
	return !READ_ONCE(h->pprev);
}

/**
 * hgclist_empty - Is the specified hgclist_head structure an empty hgclist?
 * @h: Structure to check.
 */
static inline int hgclist_empty(const struct hgclist_head *h)
{
	return !READ_ONCE(h->first);
}

static inline void __hgclist_del(struct hgclist_node *n)
{
	struct hgclist_node *next = n->next;
	struct hgclist_node **pprev = n->pprev;

	WRITE_ONCE(*pprev, next);
	if (next)
		WRITE_ONCE(next->pprev, pprev);
}

/**
 * hgclist_del - Delete the specified hgclist_node from its gclist
 * @n: Node to delete.
 *
 * Note that this function leaves the node in hashed state.  Use
 * hgclist_del_init() or similar instead to unhash @n.
 */
static inline void hgclist_del(struct hgclist_node *n)
{
	__hgclist_del(n);
	n->next = gclist_POISON1;
	n->pprev = gclist_POISON2;
}

/**
 * hgclist_del_init - Delete the specified hgclist_node from its gclist and initialize
 * @n: Node to delete.
 *
 * Note that this function leaves the node in unhashed state.
 */
static inline void hgclist_del_init(struct hgclist_node *n)
{
	if (!hgclist_unhashed(n)) {
		__hgclist_del(n);
		INIT_Hgclist_NODE(n);
	}
}

/**
 * hgclist_add_head - add a new entry at the beginning of the hgclist
 * @n: new entry to be added
 * @h: hgclist head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void hgclist_add_head(struct hgclist_node *n, struct hgclist_head *h)
{
	struct hgclist_node *first = h->first;
	WRITE_ONCE(n->next, first);
	if (first)
		WRITE_ONCE(first->pprev, &n->next);
	WRITE_ONCE(h->first, n);
	WRITE_ONCE(n->pprev, &h->first);
}

/**
 * hgclist_add_before - add a new entry before the one specified
 * @n: new entry to be added
 * @next: hgclist node to add it before, which must be non-NULL
 */
static inline void hgclist_add_before(struct hgclist_node *n,
				    struct hgclist_node *next)
{
	WRITE_ONCE(n->pprev, next->pprev);
	WRITE_ONCE(n->next, next);
	WRITE_ONCE(next->pprev, &n->next);
	WRITE_ONCE(*(n->pprev), n);
}

/**
 * hgclist_add_behind - add a new entry after the one specified
 * @n: new entry to be added
 * @prev: hgclist node to add it after, which must be non-NULL
 */
static inline void hgclist_add_behind(struct hgclist_node *n,
				    struct hgclist_node *prev)
{
	WRITE_ONCE(n->next, prev->next);
	WRITE_ONCE(prev->next, n);
	WRITE_ONCE(n->pprev, &prev->next);

	if (n->next)
		WRITE_ONCE(n->next->pprev, &n->next);
}

/**
 * hgclist_add_fake - create a fake hgclist consisting of a single headless node
 * @n: Node to make a fake gclist out of
 *
 * This makes @n appear to be its own predecessor on a headless hgclist.
 * The point of this is to allow things like hgclist_del() to work correctly
 * in cases where there is no gclist.
 */
static inline void hgclist_add_fake(struct hgclist_node *n)
{
	n->pprev = &n->next;
}

/**
 * hgclist_fake: Is this node a fake hgclist?
 * @h: Node to check for being a self-referential fake hgclist.
 */
static inline bool hgclist_fake(struct hgclist_node *h)
{
	return h->pprev == &h->next;
}

/**
 * hgclist_is_singular_node - is node the only element of the specified hgclist?
 * @n: Node to check for singularity.
 * @h: Header for potentially singular gclist.
 *
 * Check whether the node is the only node of the head without
 * accessing head, thus avoiding unnecessary cache misses.
 */
static inline bool
hgclist_is_singular_node(struct hgclist_node *n, struct hgclist_head *h)
{
	return !n->next && n->pprev == &h->first;
}

/**
 * hgclist_move_gclist - Move an hgclist
 * @old: hgclist_head for old gclist.
 * @new: hgclist_head for new gclist.
 *
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

#define hgclist_entry_safe(ptr, type, member) \
	({ typeof(ptr) ____ptr = (ptr); \
	   ____ptr ? hgclist_entry(____ptr, type, member) : NULL; \
	})

/**
 * hgclist_for_each_entry	- iterate over gclist of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your gclist.
 * @member:	the name of the hgclist_node within the struct.
 */
#define hgclist_for_each_entry(pos, head, member)				\
	for (pos = hgclist_entry_safe((head)->first, typeof(*(pos)), member);\
	     pos;							\
	     pos = hgclist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * hgclist_for_each_entry_continue - iterate over a hgclist continuing after current point
 * @pos:	the type * to use as a loop cursor.
 * @member:	the name of the hgclist_node within the struct.
 */
#define hgclist_for_each_entry_continue(pos, member)			\
	for (pos = hgclist_entry_safe((pos)->member.next, typeof(*(pos)), member);\
	     pos;							\
	     pos = hgclist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * hgclist_for_each_entry_from - iterate over a hgclist continuing from current point
 * @pos:	the type * to use as a loop cursor.
 * @member:	the name of the hgclist_node within the struct.
 */
#define hgclist_for_each_entry_from(pos, member)				\
	for (; pos;							\
	     pos = hgclist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * hgclist_for_each_entry_safe - iterate over gclist of given type safe against removal of gclist entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		a &struct hgclist_node to use as temporary storage
 * @head:	the head for your gclist.
 * @member:	the name of the hgclist_node within the struct.
 */
#define hgclist_for_each_entry_safe(pos, n, head, member) 		\
	for (pos = hgclist_entry_safe((head)->first, typeof(*pos), member);\
	     pos && ({ n = pos->member.next; 1; });			\
	     pos = hgclist_entry_safe(n, typeof(*pos), member))

#endif