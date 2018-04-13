#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <string.h>
#include <thread>

#define PLOT

#define SCRATCH_SIZE 64 //determined to be bestish (maybe 128 also)
void flush(int* untouched, int* scratch, int numElems, int& size){
  memmove(untouched+size,untouched,sizeof(int)*numElems);
  memcpy(untouched,scratch,sizeof(int)*size);
  size=0;
}
void inplace_mergesort(int* arr, int count){
  if(count <= 1) return;
  int middle = count/2;
  inplace_mergesort(arr,middle);
  inplace_mergesort(arr+middle,count-middle);
  if(arr[middle] >= arr[middle-1]) return; //already sorted!

  int i,j=middle,scratch[SCRATCH_SIZE],size=0;
  for(i=0;i<j && j<count;i++){
    if(i==j-size) flush(arr+i,scratch,(j-size)-i,size);

    if(arr[j]<(size? scratch[0] : arr[i])){
      scratch[size++] = arr[i];
      arr[i] = arr[j++];
      if(size==SCRATCH_SIZE) flush(arr+(i+1),scratch,(j-size)-(i+1),size);
    }else if(size) flush(arr+i,scratch,(j-size)-i,size);
  }
  if(size) flush(arr+i,scratch,(j-size)-i,size);
}

void mergesort(int* arr, int count){
  static int startcount = count;
  static int* scratch = (int*) malloc(sizeof(int)*count);
  if(count <= 1) return;
  int middle = count/2;
  mergesort(arr,middle);
  mergesort(arr+middle,count-middle);
  if(arr[middle] >= arr[middle-1]) return; //already sorted!

  int i=0,j=middle;
  for(int idx=0;idx<count;idx++){
    if(i<middle && j<count) scratch[idx] = (arr[i]<arr[j])? arr[i++] : arr[j++];
    else{
      if(j==count) memcpy(scratch+idx,arr+i,sizeof(int)*(middle-i));
      else          memcpy(scratch+idx,arr+j,sizeof(int)*(count-j));
      break;
    }
  }
  memcpy(arr,scratch,sizeof(int)*count);
  if(count == startcount) free(scratch);
}


void naivesort(int* arr, int count){
  for(int i=0;i<count;i++){
    int best = arr[i], best_idx = i;
    int temp = best;
    for(int j=i+1;j<count;j++){
      if(arr[j] < best){
        best = arr[j];
        best_idx = j;
      }
    }
    arr[i] = best;
    arr[best_idx] = temp;
  }
}

void quicksort(int* arr, int count){
  if(count <= 1) return;
  int pivot = arr[rand() % count];
  int bottom_idx = 0, top_idx = count;
  for(int i=0;i < top_idx;i++){
    int cur_elem = arr[i];
    if(cur_elem > pivot){
      arr[i--] = arr[--top_idx];
      arr[top_idx] = cur_elem;
    }else if(cur_elem < pivot) arr[bottom_idx++] = cur_elem;
  }
  for(int i=bottom_idx;i<top_idx;i++) arr[i] = pivot;
    quicksort(arr,bottom_idx);
    quicksort(arr+top_idx,count-top_idx);
}

int main(int argc, char** argv){
  if(argc < 2) return -1;
  const int N = atoi(argv[1]);
#ifdef PLOT
  printf("%d\t",N);
#endif

  int* nums = (int*) malloc(sizeof(int)*N);
  int* scratch1 = (int*) malloc(sizeof(int)*N);
  int* scratch2 = (int*) malloc(sizeof(int)*N);
  if(!nums || !scratch1 || !scratch2){
    printf("Couldn't malloc that much memory\n");
    if(nums) free(nums);
    if(scratch1) free(scratch1);
    return -1;
  }

  std::clock_t start,stop;

  srand(time(NULL));
  for(int i=0;i<N;i++) nums[i] = rand() % 10000;
  if(argc > 2){
    for(int i=0;i<N;i++) printf("%d,",nums[i]);
    printf("\n");
  }

/*#ifdef PLOT
  memcpy(scratch2,nums,sizeof(int)*N);
  start = std::clock();
  naivesort(scratch2,N);
  stop = std::clock();
  printf("%.5f\t",(stop-start)/((double)(CLOCKS_PER_SEC))*1000);
#endif*/

  memcpy(scratch1,nums,sizeof(int)*N);
  start = std::clock();
  mergesort(scratch1,N);
  stop = std::clock();
#ifdef PLOT
  printf("%.5f\t",(stop-start)/((double)(CLOCKS_PER_SEC))*1000);
#else
  printf(" merge: %.5fms\n",(stop-start)/((double)(CLOCKS_PER_SEC))*1000);
#endif

  /*memcpy(scratch1,nums,sizeof(int)*N);
  start = std::clock();
  inplace_mergesort(scratch1,N);
  stop = std::clock();
#ifdef PLOT
  printf("%.5f\t",(stop-start)/((double)(CLOCKS_PER_SEC))*1000);
#else
  printf("IPmerge: %.5fms\n",(stop-start)/((double)(CLOCKS_PER_SEC))*1000);
#endif*/
  fflush(stdout);

  time_t t1,t2;
  memcpy(scratch2,nums,sizeof(int)*N);
  start = std::clock();
  quicksort(scratch2,N);
  stop = std::clock();
#ifdef PLOT
  printf("%.5f\t",(stop-start)/((double)(CLOCKS_PER_SEC))*1000);
#else
  printf("quick: %.5fms\n",(stop-start)/((double)(CLOCKS_PER_SEC))*1000);
#endif

  if(argc > 2){
    for(int i=0;i<N;i++) printf("%d,",scratch1[i]);
    printf("\n");
    for(int i=0;i<N;i++) printf("%d,",scratch2[i]);
    printf("\n");
  }

  /*for(int i=0;i<N;i++){
    if(scratch2[i] != scratch1[i]){
      printf("incorrect answer.\n");
      break;
    }
  }*/

#ifdef PLOT
  printf("\n");
#endif

  free(nums);
  free(scratch1);
  free(scratch2);

  return 0;
}
