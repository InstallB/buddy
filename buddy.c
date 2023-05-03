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
void* startpos; int _pgcount;
int* rnk; // rank of every position, initial value is MAXRANK

int merge_page(int rank,node *cur){
    node *nxt = cur->nxt;
    if(nxt == NULL) return 0;
    if(cur->pos + (1 << (rank - 1 + LOG4K)) != nxt->pos) return 0;
    if((cur->pos - startpos) % (1 << (rank + LOG4K)) != 0) return 0;
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
    insert(rank + 1,cur->pos); count[rank + 1] ++;
    free(cur); count[rank] --;
    free(nxt); count[rank] --;
    return 1;
}

void insert(int rank,void *pos){
    node *cur = malloc(sizeof(node));
    cur->nxt = NULL;
    cur->pre = NULL;
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
    if(!merge_page(rank,cur) && cur->pre != NULL) merge_page(rank,cur->pre);
}

int init_page(void *p, int pgcount){
    // supppose pgcount <= (2^15)
    int i;
    rnk = malloc(sizeof(int) * pgcount);
    for(i = 0;i < pgcount;i ++) rnk[i] = MAXRANK;
    _pgcount = pgcount;
    void *pos = p;
    startpos = p;
    for(i = 1;i <= MAXRANK;i ++) head[i] = NULL;
    for(i = MAXRANK;i >= 1;i --){
        if(pgcount >= (1 << (i - 1))){
            head[i] = malloc(sizeof(node));
            head[i]->nxt = NULL;
            head[i]->pos = pos;
            count[i] ++;
            pos += (1 << (i - 1 + LOG4K));
            pgcount -= (1 << (i - 1));
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
    node *nxt;
    void *ret = head[up]->pos;
    for(i = up - 1;i >= rank;i --){
        insert(i,head[up]->pos + (1 << (i - 1 + LOG4K)));
        count[i] ++;
    }
    nxt = head[up]->nxt;
    if(nxt != NULL) nxt->pre = NULL;
    free(head[up]);
    head[up] = nxt;
    count[up] --;
    rnk[(ret - startpos) / (1 << LOG4K)] = rank;
    return ret;
}

int return_pages(void *p){
    if(p < startpos || p >= startpos + (1 << LOG4K) * _pgcount) return -EINVAL;
    int rank = rnk[(p - startpos) / (1 << LOG4K)];
    insert(rank,p); count[rank] ++;
    rnk[(p - startpos) / (1 << LOG4K)] = MAXRANK;
    return OK;
}

int query_ranks(void *p){
    if(p < startpos || p >= startpos + (1 << LOG4K) * _pgcount) return -EINVAL;
    return rnk[(p - startpos) / (1 << LOG4K)];
}

int query_page_counts(int rank){
    if(!(rank >= 1 && rank <= MAXRANK)) rank = MAXRANK;
    return count[rank];
}