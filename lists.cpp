#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <time.h>
#include <omp.h>
#include <math.h>

#define NAIVE 1
#define PARALLEL 2
//#define WORKLOAD PARALLEL //will be defined by the compiler

#define THREADS 4
//#define PRINT_OUT

#define PTRSUB(X,Y) ((unsigned long)(((char*)(X))-((char*)(Y))))

struct Link{
  Link *next;
  int rank;

  //sense vars
  Link* nextS[2];
  int rankS[2];
};

int main(int argc, char** argv){
  if(argc != 2){
    printf("Usage: ./lists num_nodes\n");
    return 1;
  }

  omp_set_dynamic(0);
  omp_set_num_threads(THREADS);


  const int heap_size = 2*atoi(argv[1]); //double alloc so rand has less collisions
  Link* heap = (Link*) calloc(heap_size,sizeof(Link));
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
  while(count < heap_size/2){ //TODO: better collision handling
    const int randidx = rand() % heap_size;
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
#elif(WORKLOAD == PARALLEL)
  int sense = 0;
  #pragma omp for
  for(int i=0;i<heap_size;i++){
    Link* node = &heap[i];
    if(node->next==node) node->rankS[sense] = 1;
    else node->rankS[sense] = 0;
  }

  const int max_iters = log2(heap_size)+1;
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
