#include "buddy.h"
#include <stdlib.h>
#include <string.h>
#define NULL ((void *)0)

#define MAXRANK 16

typedef struct node_{
    void *pos;
    struct node_ *nxt,*pre;
} node;

node *head[MAXRANK + 1];
int count[MAXRANK + 1];
void* startpos; int pgcount;
int* rnk; // rank of every position, initial value is -1

int merge_page(int rank,node *cur){
    node *nxt = cur->nxt;
    if(nxt == NULL) return 0;
    if(cur->pos + (1 << (rank - 1 + LOG4K)) != nxt->pos) return 0;
    if((cur->pos - startpos) % (1 << LOG4K) != 0) return 0;
    node *nxt2 = nxt->nxt;
    if(nxt2 != NULL){
        nxt2->pre = cur->pre;
    }
    if(cur->pre != NULL){
        cur->pre->nxt = nxt2;
    }
    else{
        head[rank] = nxt2;
    }
    insert(rank + 1,cur->pos);
    free(cur);
    free(nxt);
    return 1;
}

void insert(int rank,void *pos){
    node *cur = malloc(sizeof(node));
    cur->nxt = NULL;
    cur->pos = pos;
    if(head[rank] == NULL){
        head[rank] = cur;
    }
    else{
        node *p = head[rank],*pnxt;
        while(p->nxt != NULL && p->nxt->pos < pos) p = p->nxt;
        pnxt = p->nxt;
        cur->nxt = pnxt;
        cur->pre = p;
        p->nxt = cur;
        if(pnxt != NULL) pnxt->pre = cur;
    }
}

int init_page(void *p, int pgcount){
    // supppose pgcount <= (2^15)
    rnk = malloc(sizeof(int) * pgcount);
    memset(rnk,-1,sizeof(int) * pgcount);
    int i;
    void *pos = p;
    startpos = p;
    for(i = 1;i <= MAXRANK;i ++) head[i] = NULL;
    for(i = MAXRANK;i >= 1;i --){
        if(pgcount >= (1 << (i - 1))){
            head[i] = malloc(sizeof(node));
            head[i]->nxt = NULL;
            head[i]->pos = pos;
            pos += (1 << (i - 1 + LOG4K));
        }
    }
    return OK;
}
void *alloc_pages(int rank){
    if(!(rank >= 1 && rank <= MAXRANK)) return -EINVAL;
    int i,up = -1;
    for(i = rank;i <= MAXRANK;i ++){
        if(count[i] > 0){
            up = i;
            break;
        }
    }
    if(up == -1) return -ENOSPC;
    void *nxt,*ret = head[up]->pos;
    for(i = up - 1;i > rank;i --){
        insert(i,head[up]->pos + (1 << (i - 1 + LOG4K)));
    }
    nxt = head[up]->nxt;
    free(head[up]);
    head[up] = nxt;
    rnk[(ret - startpos) / (1 << LOG4K)] = rank;
    return ret;
}
int return_pages(void *p){
    if(p < startpos || p >= startpos + (1 << LOG4K) * pgcount) return -EINVAL;
    int rank = rnk[(p - startpos) / (1 << LOG4K)];
    node *cur = head[rank];
    while(cur != NULL && cur != p) cur = cur->nxt;
    if(cur == NULL) return -EINVAL;
    if(!merge_page(rank,cur) && cur->pre != NULL) merge_page(rank,cur->pre);
    rnk[(p - startpos) / (1 << LOG4K)] = -1;
    return OK;
}
int query_ranks(void *p){
    if(p < startpos || p >= startpos + (1 << LOG4K) * pgcount) return -EINVAL;
    return rnk[(p - startpos) / (1 << LOG4K)];
}
int query_page_counts(int rank){
    int i,sum = 0;
    for(i = MAXRANK;i >= rank;i --){
        sum += count[i] * (1 << (i - rank));
    }
    return sum;
}