#include <stdio.h>
#include <stdlib.h>

#include "converse.h"

/**
 * Structure to hold the requisites for a callback
 */
typedef struct _ccd_callback {
  CcdVoidFn fn;
  void *arg;
  int pe;			/* the pe that sets the callback */
} ccd_callback;



/**
 * An element (a single callback) in a list of callbacks
 */
typedef struct _ccd_cblist_elem {
  ccd_callback cb;
  int next;
  int prev;
} ccd_cblist_elem;



/**
 * A list of callbacks stored as an array and handled like a list
 */
typedef struct _ccd_cblist {
  unsigned int maxlen;
  unsigned int len;
  int first, last;
  int first_free;
  ccd_cblist_elem *elems;
  int flag;
} ccd_cblist;



/** Initialize a list of callbacks. Alloc memory, set counters etc. */
static void init_cblist(ccd_cblist *l, unsigned int ml)
{
  int i;
  l->elems = (ccd_cblist_elem*) malloc(ml*sizeof(ccd_cblist_elem));
  _MEMCHECK(l->elems);
  for(i=0;i<ml;i++) {
    l->elems[i].next = i+1;
    l->elems[i].prev = i-1;
  }
  l->elems[ml-1].next = -1;
  l->len = 0;
  l->maxlen = ml;
  l->first = l->last = -1;
  l->first_free = 0;
  l->flag = 0;
}



/** Expand the callback list to a max length of ml */
static void expand_cblist(ccd_cblist *l, unsigned int ml)
{
  ccd_cblist_elem *old_elems = l->elems;
  int i = 0;
  l->elems = (ccd_cblist_elem*) malloc(ml*sizeof(ccd_cblist_elem));
  _MEMCHECK(l->elems);
  for(i=0;i<(l->len);i++)
    l->elems[i] = old_elems[i];
  free(old_elems);
  for(i=l->len;i<ml;i++) {
    l->elems[i].next = i+1;
    l->elems[i].prev = i-1;
  }
  l->elems[ml-1].next = -1;
  l->elems[l->len].prev = -1;
  l->maxlen = ml;
  l->first_free = l->len;
}



/** Remove element referred to by given list index idx. */
static void remove_elem(ccd_cblist *l, int idx)
{
  ccd_cblist_elem *e = l->elems;
  /* remove lidx from the busy list */
  if(e[idx].next != (-1))
    e[e[idx].next].prev = e[idx].prev;
  if(e[idx].prev != (-1))
    e[e[idx].prev].next = e[idx].next;
  if(idx==(l->first)) 
    l->first = e[idx].next;
  if(idx==(l->last)) 
    l->last = e[idx].prev;
  /* put lidx in the free list */
  e[idx].prev = -1;
  e[idx].next = l->first_free;
  if(e[idx].next != (-1))
    e[e[idx].next].prev = idx;
  l->first_free = idx;
  l->len--;
}



/** Remove n elements from the beginning of the list. */
static void remove_n_elems(ccd_cblist *l, int n)
{
  int i;
  if(n==0 || (l->len < n))
    return;
  for(i=0;i<n;i++) {
    remove_elem(l, l->first);
  }
}



/** Append callback to the given cblist, and return the index. */
static int append_elem(ccd_cblist *l, CcdVoidFn fn, void *arg, int pe)
{
  int idx;
  ccd_cblist_elem *e;
  if(l->len == l->maxlen)
    expand_cblist(l, l->maxlen*2);
  idx = l->first_free;
  e = l->elems;
  l->first_free = e[idx].next;
  e[idx].next = -1;
  e[idx].prev = l->last;
  if(l->first == (-1))
    l->first = idx;
  if(l->last != (-1))
    e[l->last].next = idx;
  l->last = idx;
  e[idx].cb.fn = fn;
  e[idx].cb.arg = arg;
  e[idx].cb.pe = pe;
  l->len++;
  return idx;
}



/**
 * Trigger the callbacks in the provided callback list and *retain* them
 * after they are called. 
 *
 * Callbacks that are added after this function is started (e.g. callbacks 
 * registered from other callbacks) are ignored. 
 * @note: It is illegal to cancel callbacks from within ccd callbacks.
 */
static void call_cblist_keep(ccd_cblist *l,double curWallTime)
{
  int i, len = l->len, idx;
  for(i=0, idx=l->first;i<len;i++) {
    int old = CmiSwitchToPE(l->elems[idx].cb.pe);
    (*(l->elems[idx].cb.fn))(l->elems[idx].cb.arg,curWallTime);
    CmiSwitchToPE(old);
    idx = l->elems[idx].next;
  }
}



/**
 * Trigger the callbacks in the provided callback list and *remove* them
 * from the list after they are called.
 *
 * Callbacks that are added after this function is started (e.g. callbacks 
 * registered from other callbacks) are ignored. 
 * @note: It is illegal to cancel callbacks from within ccd callbacks.
 */
static void call_cblist_remove(ccd_cblist *l,double curWallTime)
{
  int i, len = l->len, idx;
  /* reentrant */
  if (l->flag) return;
  l->flag = 1;
#if ! CMK_BIGSIM_CHARM
  for(i=0, idx=l->first;i<len;i++) {
    int old = CmiSwitchToPE(l->elems[idx].cb.pe);
    (*(l->elems[idx].cb.fn))(l->elems[idx].cb.arg,curWallTime);
    CmiSwitchToPE(old);
    idx = l->elems[idx].next;
  }
#else
  for(i=0, idx=l->last;i<len;i++) {
    int old = CmiSwitchToPE(l->elems[idx].cb.pe);
    (*(l->elems[idx].cb.fn))(l->elems[idx].cb.arg,curWallTime);
    CmiSwitchToPE(old);
    idx = l->elems[idx].prev;
  }
#endif
  remove_n_elems(l,len);
  l->flag = 0;
}



#define CBLIST_INIT_LEN   8
#define MAXNUMCONDS       512

/**
 * Lists of conditional callbacks that are maintained by the scheduler
 */
typedef struct {
  ccd_cblist condcb[MAXNUMCONDS];
  ccd_cblist condcb_keep[MAXNUMCONDS];
} ccd_cond_callbacks;

/***/
CpvStaticDeclare(ccd_cond_callbacks, conds);   



/*Make sure this matches the CcdPERIODIC_* list in converse.h*/
#define CCD_PERIODIC_MAX 13
const static double periodicCallInterval[CCD_PERIODIC_MAX]=
{0.001, 0.010, 0.100, 1.0, 5.0, 10.0, 60.0, 2*60.0, 5*60.0, 10*60.0, 3600.0, 12*3600.0, 24*3600.0};

/**
 * List of periodic callbacks maintained by the scheduler
 */
typedef struct {
	int nSkip;/*Number of opportunities to skip*/
	double lastCheck;/*Time of last check*/
	double nextCall[CCD_PERIODIC_MAX];
} ccd_periodic_callbacks;

/** */
CpvStaticDeclare(ccd_periodic_callbacks, pcb);
CpvDeclare(int, _ccd_numchecks);



#define MAXTIMERHEAPENTRIES       256

/**
 * Structure used to manage callbacks in a heap
 */
typedef struct {
    double time;
    ccd_callback cb;
} ccd_heap_elem;


/* Note : The heap is only stored in elements ccd_heap[0] to 
 * ccd_heap[ccd_heaplen]
 */

/** An array of time-scheduled callbacks managed as a heap */
CpvStaticDeclare(ccd_heap_elem*, ccd_heap); 
/** The length of the callback heap */
CpvStaticDeclare(int, ccd_heaplen);
/** The max allowed length of the callback heap */
CpvStaticDeclare(int, ccd_heapmaxlen);



/** Swap two elements on the heap */
static void ccd_heap_swap(int index1, int index2)
{
  ccd_heap_elem *h = CpvAccess(ccd_heap);
  ccd_heap_elem temp;
  
  temp = h[index1];
  h[index1] = h[index2];
  h[index2] = temp;
}



/**
 * Expand the ccd_heap to make more room.
 *
 * Double the heap size and copy everything over. Initial 256 is reasonably 
 * big, so expanding won't happen often.
 *
 * Had a bug previously due to late expansion, should work now - Gengbin 12/4/03
*/
static void expand_ccd_heap()
{
  int i;
  int oldlen = CpvAccess(ccd_heapmaxlen);
  int newlen = oldlen*2;
  ccd_heap_elem *newheap;

  CmiPrintf("[%d] Warning: ccd_heap expand from %d to %d\n", CmiMyPe(),oldlen, newlen);

  newheap = (ccd_heap_elem*) malloc(sizeof(ccd_heap_elem)*2*(newlen+1));
  _MEMCHECK(newheap);
  /* need to copy the second half part ??? */
  for (i=0; i<=oldlen; i++) {
    newheap[i] = CpvAccess(ccd_heap)[i];
    newheap[i+newlen] = CpvAccess(ccd_heap)[i+oldlen];
  }
  free(CpvAccess(ccd_heap));
  CpvAccess(ccd_heap) = newheap;
  CpvAccess(ccd_heapmaxlen) = newlen;
}



/**
 * Insert a new callback into the heap
 */
static void ccd_heap_insert(double t, CcdVoidFn fnp, void *arg, int pe)
{
  int child, parent;
  ccd_heap_elem *h;
  
  if(CpvAccess(ccd_heaplen) >= CpvAccess(ccd_heapmaxlen)) {
/* CmiAbort("Heap overflow (InsertInHeap), exiting...\n"); */
    expand_ccd_heap();
  } 

  h = CpvAccess(ccd_heap);

  {
    ccd_heap_elem *e = &(h[++CpvAccess(ccd_heaplen)]);
    e->time = t;
    e->cb.fn = fnp;
    e->cb.arg = arg;
    e->cb.pe = pe;
    child  = CpvAccess(ccd_heaplen);    
    parent = child / 2;
    while((parent>0) && (h[child].time<h[parent].time)) {
	    ccd_heap_swap(child, parent);
	    child  = parent;
	    parent = parent / 2;
    }
  }
}



/**
 * Remove the top of the heap
 */
static void ccd_heap_remove(void)
{
  int parent,child;
  ccd_heap_elem *h = CpvAccess(ccd_heap);
  
  parent = 1;
  if(CpvAccess(ccd_heaplen)>0) {
    /* put deleted value at end of heap */
    ccd_heap_swap(1,CpvAccess(ccd_heaplen)); 
    CpvAccess(ccd_heaplen)--;
    if(CpvAccess(ccd_heaplen)) {
      /* if any left, then bubble up values */
	    child = 2 * parent;
	    while(child <= CpvAccess(ccd_heaplen)) {
	      if(((child + 1) <= CpvAccess(ccd_heaplen))  &&
		       (h[child].time > h[child+1].time))
                child++; /* use the smaller of the two */
	      if(h[parent].time <= h[child].time) 
		      break;
	      ccd_heap_swap(parent,child);
	      parent  = child;      /* go down the tree one more step */
	      child  = 2 * child;
      }
    }
  } 
}



/**
 * Identify any (over)due callbacks that were scheduled
 * and trigger them. 
 */
static void ccd_heap_update(double curWallTime)
{
  ccd_heap_elem *h = CpvAccess(ccd_heap);
  ccd_heap_elem *e = h+CpvAccess(ccd_heapmaxlen);
  int i,ne=0;
  /* Pull out all expired heap entries */
  while ((CpvAccess(ccd_heaplen)>0) && (h[1].time<curWallTime)) {
    e[ne++]=h[1];
    ccd_heap_remove();
  }
  /* Now execute those heap entries.  This must be
     separated from the removal phase because executing
     an entry may change the heap. 
  */
  for (i=0;i<ne;i++) {
/*
      ccd_heap_elem *h = CpvAccess(ccd_heap);
      ccd_heap_elem *e = h+CpvAccess(ccd_heapmaxlen);
*/
      int old = CmiSwitchToPE(e[i].cb.pe);
      (*(e[i].cb.fn))(e[i].cb.arg,curWallTime);
      CmiSwitchToPE(old);
  }
}



void CcdCallBacksReset(void *ignored,double curWallTime);

/**
 * Initialize the callback containers
 */
void CcdModuleInit(void)
{
   int i;
   double curTime;
   CpvInitialize(ccd_heap_elem*, ccd_heap);
   CpvInitialize(ccd_cond_callbacks, conds);
   CpvInitialize(ccd_periodic_callbacks, pcb);
   CpvInitialize(int, ccd_heaplen);
   CpvInitialize(int, ccd_heapmaxlen);
   CpvInitialize(int, _ccd_numchecks);

   CpvAccess(ccd_heaplen) = 0;
   CpvAccess(ccd_heapmaxlen) = MAXTIMERHEAPENTRIES;
   CpvAccess(ccd_heap) = 
     (ccd_heap_elem*) malloc(sizeof(ccd_heap_elem)*2*(MAXTIMERHEAPENTRIES + 1));
   _MEMCHECK(CpvAccess(ccd_heap));
   for(i=0;i<MAXNUMCONDS;i++) {
     init_cblist(&(CpvAccess(conds).condcb[i]), CBLIST_INIT_LEN);
     init_cblist(&(CpvAccess(conds).condcb_keep[i]), CBLIST_INIT_LEN);
   }
   CpvAccess(_ccd_numchecks) = 1;
   CpvAccess(pcb).nSkip = 1;
   curTime=CmiWallTimer();
   CpvAccess(pcb).lastCheck = curTime;
   for (i=0;i<CCD_PERIODIC_MAX;i++)
	   CpvAccess(pcb).nextCall[i]=curTime+periodicCallInterval[i];
   CcdCallOnConditionKeep(CcdPROCESSOR_BEGIN_IDLE,CcdCallBacksReset,0);
   CcdCallOnConditionKeep(CcdPROCESSOR_END_IDLE,CcdCallBacksReset,0);
}



/**
 * Register a callback function that will be triggered when the specified
 * condition is raised the next time
 */
int CcdCallOnCondition(int condnum, CcdVoidFn fnp, void *arg)
{
  return append_elem(&(CpvAccess(conds).condcb[condnum]), fnp, arg, CcdIGNOREPE);
} 

/** 
 * Register a callback function that will be triggered on the specified PE
 * when the specified condition is raised the next time 
 */
int CcdCallOnConditionOnPE(int condnum, CcdVoidFn fnp, void *arg, int pe)
{
  return append_elem(&(CpvAccess(conds).condcb[condnum]), fnp, arg, pe);
} 

/**
 * Register a callback function that will be triggered *whenever* the specified
 * condition is raised
 */
int CcdCallOnConditionKeep(int condnum, CcdVoidFn fnp, void *arg)
{
  return append_elem(&(CpvAccess(conds).condcb_keep[condnum]), fnp, arg, CcdIGNOREPE);
} 

/**
 * Register a callback function that will be triggered on the specified PE
 * *whenever* the specified condition is raised
 */
int CcdCallOnConditionKeepOnPE(int condnum, CcdVoidFn fnp, void *arg, int pe)
{
  return append_elem(&(CpvAccess(conds).condcb_keep[condnum]), fnp, arg, pe);
} 


/**
 * Cancel a previously registered conditional callback
 */
void CcdCancelCallOnCondition(int condnum, int idx)
{
  remove_elem(&(CpvAccess(conds).condcb[condnum]), idx);
}


/**
 * Cancel a previously registered conditional callback
 */
void CcdCancelCallOnConditionKeep(int condnum, int idx)
{
  remove_elem(&(CpvAccess(conds).condcb_keep[condnum]), idx);
}


/**
 * Register a callback function that will be triggered on the specified PE
 * after a minimum delay of deltaT
 */
void CcdCallFnAfterOnPE(CcdVoidFn fnp, void *arg, double deltaT, int pe)
{
    double ctime  = CmiWallTimer();
    double tcall = ctime + deltaT/1000.0;
    ccd_heap_insert(tcall, fnp, arg, pe);
} 

/**
 * Register a callback function that will be triggered after a minimum 
 * delay of deltaT
 */
void CcdCallFnAfter(CcdVoidFn fnp, void *arg, double deltaT)
{
    CcdCallFnAfterOnPE(fnp, arg, deltaT, CcdIGNOREPE);
} 


/**
 * Raise a condition causing all registered callbacks corresponding to 
 * that condition to be triggered
 */
void CcdRaiseCondition(int condnum)
{
  double curWallTime=CmiWallTimer();
  call_cblist_remove(&(CpvAccess(conds).condcb[condnum]),curWallTime);
  call_cblist_keep(&(CpvAccess(conds).condcb_keep[condnum]),curWallTime);
}


/* 
 * Trigger callbacks periodically, and also the time-indexed
 * functions if their time has arrived
 */
void CcdCallBacks()
{
  int i;
  ccd_periodic_callbacks *o=&CpvAccess(pcb);
  
  /* Figure out how many times to skip Ccd processing */
  double curWallTime = CmiWallTimer();

  unsigned int nSkip=o->nSkip;
#if 1
/* Dynamically adjust the number of messages to skip */
  double elapsed = curWallTime - o->lastCheck;
#define targetElapsed 5.0e-3
  if (elapsed<targetElapsed) nSkip*=2; /* too short: process more */
  else /* elapsed>targetElapsed */ nSkip/=2; /* too long: process fewer */
  
/* Keep skipping within a sensible range */
#define minSkip 1u
#define maxSkip 20u
  if (nSkip<minSkip) nSkip=minSkip;
  else if (nSkip>maxSkip) nSkip=maxSkip;
#else
/* Always skip a fixed number of messages */
  nSkip=1;
#endif

  CpvAccess(_ccd_numchecks)=o->nSkip=nSkip;
  o->lastCheck=curWallTime;
  
  ccd_heap_update(curWallTime);
  
  for (i=0;i<CCD_PERIODIC_MAX;i++) 
    if (o->nextCall[i]<=curWallTime) {
      CcdRaiseCondition(CcdPERIODIC+i);
      o->nextCall[i]=curWallTime+periodicCallInterval[i];
    }
    else 
      break; /*<- because intervals are multiples of one another*/
} 



/**
 * Called when something drastic changes-- restart ccd_num_checks
 */
void CcdCallBacksReset(void *ignored,double curWallTime)
{
  ccd_periodic_callbacks *o=&CpvAccess(pcb);
  CpvAccess(_ccd_numchecks)=o->nSkip=1;
  o->lastCheck=curWallTime;
}


