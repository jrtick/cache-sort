#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <time.h>
#include <omp.h>
#include <math.h>

#define NAIVE 1
#define MEM_EFFICIENT 2
#define PARALLEL 3

#define PRINT_OUT
#define WORKLOAD PARALLEL

#define PTRSUB(X,Y) ((unsigned long)(((char*)(X))-((char*)(Y))))

struct Link{
  Link *next;
  int rank;

  //sense vars
  Link* nextS[2];
  int rankS[2];
};

int main(){
  const int heap_size = 20;//100000; //double alloc so rand has less collisions
  Link* heap = (Link*) malloc(sizeof(Link)*heap_size);
  if(heap == NULL){
    printf("Segfault. Reduce heap size.\n");
    return -1;
  }

  int* mask = (int*) calloc((heap_size+31)/32,sizeof(int));

  //now, randomly insert links into heap to init linked list
  srand(time(NULL));
  int count = 0;
  Link* start = NULL;
  Link* end = NULL;
  while(count < heap_size/2){ //TODO: do it like heap adjacent insert
    const int randidx = rand() % heap_size; //TODO: avoid overalloc by randomly grabbing mask? lin search when end?
#ifdef PRINT_OUT
    printf("%d,",randidx);
#endif
    const int maskidx = randidx/32;
    const int masklane = randidx%32;
    if(mask[maskidx] & (1<<masklane)) continue;
    else{
      mask[maskidx] |= 1<<masklane; //mark used
      Link* addr = &heap[randidx];
      if(start){
        end->next = addr;
        end->nextS[0] = addr;
      }else start = addr;
      end = addr;
      count++;
    }
  }
#ifdef PRINT_OUT
  printf("\n");
#endif
  end->next = end;
  end->nextS[0] = end;

  std::clock_t begin,stop;

  //now, do page rank on the linked list!! Put rank in their data space
  printf("start.\n");
  begin = std::clock();

#if(WORKLOAD == NAIVE)
  Link* cur = start;
  int rank = 0;
  while(true){
    cur->rank = rank++;
    if(cur==end) break;
    cur = cur->next;
  }
#elif(WORKLOAD == MEM_EFFICIENT)

#elif(WORKLOAD == PARALLEL)
  #pragma omp for
  for(int i=0;i<heap_size;i++){
    Link* node = &heap[i];
    if(node->next==node) node->rankS[0] = 1;
    else node->rankS[0] = 0;
  }

  const int max_iters = log2(heap_size);
  int sense = 0;
  /*#pragma omp parallel num_threads(numthreads)
  {
    int tid = omp_get_thread_num();
  #pragma omp barrier
  }*/
  for(int iter=0;iter<max_iters;iter++){
    #pragma omp for
    for(int i=0;i<heap_size;i++){
      Link* cur = &heap[i];
      if(!cur->next) continue; //undefined c behavior

      cur->rankS[!sense] = cur->rankS[sense]+cur->nextS[sense]->rankS[sense];
      cur->nextS[!sense] = cur->nextS[sense]->nextS[sense];
    }
    sense = !sense;
  }

  //at the very end, copy back to real rank
  #pragma omp for
  for(int i=0;i<heap_size;i++){
    Link* cur = &heap[i];
    if(cur->next){
      cur->rank = cur->rankS[!sense];
    }
  }
#else
  printf("Error. Unrecognized work scheme.\n");
  goto endit;
#endif
  stop = std::clock();

#ifdef PRINT_OUT
  //print results in address-major order
  for(int i=0;i<(heap_size+31)/32;i++){
    const int curmask = mask[i];
    for(int j=0;j<32;j++){
      if(curmask & (1<<j)){
        int idx = i*32+j;
        Link* node = &heap[idx];
        int nextidx = (node->next == NULL)? -1 : PTRSUB(node->next,heap)/sizeof(Link);
        printf("%d(%d)->%d,",idx,node->rank,nextidx);
      }
    }
  }
  printf("\nStart is %lu\n",PTRSUB(start,heap)/sizeof(Link));
#endif

  printf("done in %.5fms.\n",(stop-begin)/((double)(CLOCKS_PER_SEC))*1000);

endit:
  //cleanup
  free(mask);
  free(heap);
  return 0;
}
