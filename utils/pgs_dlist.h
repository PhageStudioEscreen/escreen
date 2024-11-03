#ifndef __PGS_DLIST_H__
#define __PGS_DLIST_H__

struct _pgs_list
{
    struct _pgs_list * next;
    struct _pgs_list * prev;
};

#define pgs_dlist_init(_plist)                                                                                         \
    (_plist)->next = (_plist);                                                                                         \
    (_plist)->prev = (_plist)

#define pgs_dlist_insert_after(_plist, _pnode)                                                                         \
    (_plist)->next->prev = (_pnode);                                                                                   \
    (_pnode)->next       = (_plist)->next;                                                                             \
    (_plist)->next       = (_pnode);                                                                                   \
    (_pnode)->prev       = (_plist)

#define pgs_dlist_insert_before(_plist, _pnode)                                                                        \
    (_plist)->prev->next = (_pnode);                                                                                   \
    (_pnode)->prev       = (_plist)->prev;                                                                             \
    (_plist)->prev       = (_pnode);                                                                                   \
    (_pnode)->next       = (_plist)

#define pgs_dlist_remove(_pnode)                                                                                       \
    (_pnode)->next->prev = (_pnode)->prev;                                                                             \
    (_pnode)->prev->next = (_pnode)->next;                                                                             \
    (_pnode)->next       = (_pnode);                                                                                   \
    (_pnode)->prev       = (_pnode)

#define pgs_dlist_move_head(_plist, _pnode)                                                                            \
    pgs_dlist_remove(_pnode);                                                                                          \
    pgs_dlist_insert_after(_plist, _pnode)

#define pgs_dlist_move_tail(_plist, _pnode)                                                                            \
    pgs_dlist_remove(_pnode);                                                                                          \
    pgs_dlist_insert_before(_plist, _pnode)

#define pgs_dlist_is_empty(_plist) ((void *)((_plist)->next) == (void *)(_plist))

#define PGS_DLIST_CONTAINER_OF(_ptr, _type, _member)                                                                   \
    ((_type *)((char *)(_ptr) - (unsigned int)(&((_type *)0)->_member)))

#define PGS_DLIST_NEW(_list_name) struct _pgs_list _list_name = {&(_list_name), &(_list_name)}

#define PGS_DLIST_ENTRY(_pnode, _type, _member) PGS_DLIST_CONTAINER_OF(_pnode, _type, _member)

#define PGS_DLIST_ENTRY_FIRST(_pnode, _type, _member) PGS_DLIST_CONTAINER_OF((_pnode)->next, _type, _member)

#define PGS_DLIST_ENTRY_FIRST_ORNULL(_pnode, _type, _member)                                                           \
    (pgs_dlist_is_empty(_pnode) ? NULL : PGS_DLIST_ENTRY_FIRST(_pnode, _type, _member))

#define PGS_DLIST_FOREACH(_pos, _head) for((_pos) = (_head)->next; (_pos) != (_head); (_pos) = (_pos)->next)

#define PGS_DLIST_FOREACH_NEXT PGS_DLIST_FOREACH

#define PGS_DLIST_FOREACH_PREV(_pos, _head) for((_pos) = (_head)->prev; (_pos) != (_head); (_pos) = (_pos)->prev)

#define PGS_DLIST_FOREACH_S(_pos, _n, _head)                                                                           \
    for((_pos) = (_head)->next, (_n) = (_pos)->next; (_pos) != (_head); (_pos) = (_n), (_n) = (_pos)->next)

#define PGS_DLIST_FOREACH_NEXT_S PGS_DLIST_FOREACH_S

#define PGS_DLIST_FOREACH_PREV_S(_pos, _n, _head)                                                                      \
    for((_pos) = (_head)->prev, (_n) = (_pos)->prev; (_pos) != (_head); (_pos) = (_n), (_n) = (_pos)->prev)

#endif