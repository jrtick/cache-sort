#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <string.h>
#include <math.h>
#include <omp.h>


#define MERGESORT
//#define KMERGESORT
//#define SELECTIONSORT
//#define QUICKSORT


//#define PLOT
#define CUTOFF 32
#define M 512 //My laptop L1 is 32kb so (32/2)*1024/32 = 512
#define DEBUG

#define THREADS 4 //should only be 1,2, or 4

void naivesort(int* arr,int count);
void quicksort(int* arr,int count);

#define SCRATCH_SIZE 64 //determined to be bestish (maybe 128 also)
int statScratch[SCRATCH_SIZE];
void flush(int* untouched, int* scratch, int numElems, int& size){
  memmove(untouched+size,untouched,sizeof(int)*numElems);
  memcpy(untouched,scratch,sizeof(int)*size);
  size=0;
}
void inplace_mergesort(int* arr, int count){
  if(count <= CUTOFF){
    naivesort(arr,count);
  }else{
    int middle = count/2;
    inplace_mergesort(arr,middle);
    inplace_mergesort(arr+middle,count-middle);
    if(arr[middle] >= arr[middle-1]) return; //already sorted!

    int i,j=middle,size=0;
    for(i=0;i<j && j<count;i++){
      if(i==j-size) flush(arr+i,statScratch,(j-size)-i,size);

      if(arr[j]<(size? statScratch[0] : arr[i])){
        statScratch[size++] = arr[i];
        arr[i] = arr[j++];
        if(size==SCRATCH_SIZE) flush(arr+(i+1),statScratch,(j-size)-(i+1),size);
      }else if(size) flush(arr+i,statScratch,(j-size)-i,size);
    }
    if(size) flush(arr+i,statScratch,(j-size)-i,size);
  }
}

void mergesort(int* arr, int count,int* scratch){
  if(count <= CUTOFF) naivesort(arr,count);
  else{
    int middle = count/2;

    mergesort(arr,middle,scratch);
    mergesort(arr+middle,count-middle,scratch); //same scratch for cache eff

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
  }
}
void mergesort_parallel(int* arr, int count,int* scratch){
  int middle = count/2;
  #pragma omp parallel
  {
    int tid = omp_get_thread_num();
    if(tid==0) mergesort(arr,middle,scratch);
    if(tid==1) mergesort(arr+middle,count-middle,scratch+middle);
  }

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
}

//M should be cachesize / sizeof(int). 128 on rpi?
#define K 8 //MUST be greater than 4 for parallel code to work right
void kmergesort(int* arr,int count,int* scratch){
  if(count<M) quicksort(arr,count); //free since fit in cache
  else{
    //do recursion
    int offsets[K];
    int stops[K];
    const int merge_size = (count+K-1)/K;
    int offset = 0;
    for(int i=0;i<K-1;i++){
      kmergesort(arr+offset,merge_size,scratch);
      offsets[i] = offset;
      offset += merge_size;
      stops[i] = offset;
    }
    kmergesort(arr+offset,count-offset,scratch);
    stops[K-1] = count;
    offsets[K-1] = offset;

    //do k-merge
    for(int idx=0;idx<count;idx++){
      int minelem;
      int minidx = -1;
      for(int check=0;check<K;check++){
        if(offsets[check]==stops[check]) continue; //saturated
        int curelem = arr[offsets[check]];
        if((minidx < 0) || (curelem < minelem)){
          minelem = curelem;
          minidx = check;
        }
      }
      scratch[idx] = minelem;
      offsets[minidx]++;
    }
    memcpy(arr,scratch,sizeof(int)*count);
  }
}
void kmergesort_parallel(int* arr,int count,int* scratch){ //split merges onto independent threads
  const int merge_size = (count+THREADS-1)/THREADS;
  int offsets[K];
  int stops[K];
  #pragma omp parallel
  {
    int tid = omp_get_thread_num();
    int start = merge_size*tid;
    int stop = (tid==THREADS-1)? count : start+merge_size;
    kmergesort(arr+start,stop-start,scratch+start);
    stops[tid] = stop;
    offsets[tid] = start;
  }

  //do k-merge
  for(int idx=0;idx<count;idx++){
    int minelem;
    int minidx = -1;
    for(int check=0;check<THREADS;check++){
      if(offsets[check]==stops[check]) continue; //saturated
      int curelem = arr[offsets[check]];
      if((minidx < 0) || (curelem < minelem)){
        minelem = curelem;
        minidx = check;
      }
    }
    scratch[idx] = minelem;
    offsets[minidx]++;
  }

  memcpy(arr,scratch,sizeof(int)*count);
}

//won't parallelize this since it should be more of a helper func
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
    if(i!=best_idx){
      arr[i] = best;
      arr[best_idx] = temp;
    }
  }
}

void quicksort(int* arr, int count){
  if(count <= CUTOFF) naivesort(arr,count);
  else{
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
}

void quicksort_parallel(int* arr, int count){
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

  #pragma omp parallel
  {
    int tid = omp_get_thread_num();
    if(tid==0) quicksort(arr,bottom_idx);
    if(tid==1) quicksort(arr+top_idx,count-top_idx);
  }
}

int find_bucket(int elem, int* samples, int count){
  if(elem>samples[count-1]) return count;
  else if(elem<=samples[0]) return 0;
  else{
    int minidx = 0;
    int maxidx = count;
    while(maxidx-minidx>1){ //bin search
      int mid = (maxidx+minidx)/2;
      int sample = samples[mid];
      if(elem == sample){
        minidx = mid;
        maxidx = mid;
      }else if(elem < sample) maxidx = mid;
      else                    minidx = mid;
    }
    return maxidx;
  }
}
void samplesort(int* arr, int count, int* heap){
  if(count <= M){
    quicksort(arr,count);
    return;
  }
  const int sqrtN = sqrt(count);
  const int logN = log2(count);

  //this mem scheme works only if 3ceil(sqrt(N))+N<2N and 2*ceil(sqrt(N))<N
  //which occurs when N>=12
  int* samples = heap; //sqrtN-1 samples
  int* sample_counts = heap+sqrtN; //sqrtN-1 
  int* bucket_starts = heap+2*sqrtN; //sqrtN buckets
  int* buckets = heap+3*sqrtN; //size N for space efficiency

  //pick & sort pivots
  //for(int i=0;i<sqrtN-1;i++) samples[i] = arr[rand()%count];
  //samplesort(samples,sqrtN-1,heap+sqrtN); //must give at least 2*sqrt(N) mem

  int samplecount = sqrtN*logN;
  for(int i=0;i<samplecount;i++) samples[i] = arr[rand()%count];
  samplesort(samples,samplecount,heap+samplecount); //must give at least 4*sqrt(N) mem
  for(int i=0;i<sqrtN-1;i++) samples[i] = samples[i*logN];


  //fill buckets
  memset(heap+sqrtN,0,sizeof(int)*2*sqrtN); //clear bucket starts and sample_counts
  for(int i=0;i<count;i++){ //get bucket/sample counts
    int elem = arr[i];
    int bucketidx = find_bucket(elem,samples,sqrtN-1);
    if(bucketidx<sqrtN-1 && elem == samples[bucketidx]) sample_counts[bucketidx]++;
    else bucket_starts[bucketidx]++;
  }
  //prefix sum exclusive
  int running_tot = 0;
  for(int i=0;i<sqrtN;i++){
    int cur = bucket_starts[i];
    bucket_starts[i] = running_tot;
    running_tot += cur;
  }

  //add to buckets (will update prefix sum to incl)
  for(int i=0;i<count;i++){
    int elem = arr[i];
    int bucketidx = find_bucket(elem,samples,sqrtN-1);
    if(!(bucketidx<sqrtN-1 && elem == samples[bucketidx])){
      buckets[bucket_starts[bucketidx]++] = elem;
    }
  }

  //concatenate buckets before you sort!!
  //this frees up bucket mem to be used in recursive calls
  int offset = 0;
  int prev = 0,cur;
  for(int i=0;i<sqrtN-1;i++){
    cur = bucket_starts[i];
    memcpy(&arr[offset],&buckets[prev],sizeof(int)*(cur-prev));
    offset += (cur-prev);
    for(int j=0;j<sample_counts[i];j++) arr[offset+j] = samples[i];
    offset += sample_counts[i];
    prev = cur;
  }
  cur = bucket_starts[sqrtN-1];
  memcpy(&arr[offset],&buckets[prev],sizeof(int)*(cur-prev));

  //sort 'buckets' recursively
  offset = 0;
  prev = 0;
  for(int i=0;i<sqrtN;i++){
    cur = bucket_starts[i];
    int curcount = cur-prev;
    if(curcount>=2){
      samplesort(&arr[offset],curcount,heap+3*sqrtN);
    }
    offset += curcount+sample_counts[i];
    prev = cur;
  }
}

#define SAMPLE_SIZE 16
void samplesort_parallel(int* arr, int count, int* heap){
  int samples[SAMPLE_SIZE];
  int sample_counts[THREADS-1]={0}; 
  int bucket_starts[THREADS]={0};
  int* buckets = heap;

  for(int i=0;i<SAMPLE_SIZE;i++) samples[i] = arr[rand()%count];
  naivesort(samples,SAMPLE_SIZE);
  for(int i=0;i<THREADS-1;i++) samples[i] = samples[SAMPLE_SIZE/THREADS*(i+1)];

  //fill buckets
  //#pragma omp for nowait schedule(static)
  for(int i=0;i<count;i++){ //get bucket/sample counts
    int elem = arr[i];
    int bucketidx = THREADS-1;
    for(int j=0;j<THREADS-1;j++){
      if(elem <= samples[j]){
        bucketidx = j;
        break;
      }
    }
    if(bucketidx<THREADS-1 && elem == samples[bucketidx]){
      //#pragma omp atomic
      sample_counts[bucketidx]++;
    }else{
      //#pragma omp atomic
      bucket_starts[bucketidx]++;
    }
  }
  //prefix sum exclusive
  int running_tot = 0;
  for(int i=0;i<THREADS;i++){
    int cur = bucket_starts[i];
    bucket_starts[i] = running_tot;
    running_tot += cur;
  }

  //add to buckets (will update prefix sum to incl)
  //#pragma omp for nowait schedule(static)
  for(int i=0;i<count;i++){
    int elem = arr[i];
    int bucketidx = THREADS-1;
    for(int j=0;j<THREADS-1;j++){
      if(elem <= samples[j]){
        bucketidx = j;
        break;
      }
    }
    if(!(bucketidx<THREADS-1 && elem == samples[bucketidx])){
      int idx = bucket_starts[bucketidx];
      //#pragma omp atomic
      bucket_starts[bucketidx]++;
      buckets[idx] = elem;
    }
  }

  //concatenate buckets before you sort!!
  //this frees up bucket mem to be used in recursive calls
  int offset = 0;
  int prev = 0,cur;
  for(int i=0;i<THREADS-1;i++){
    cur = bucket_starts[i];
    memcpy(&arr[offset],&buckets[prev],sizeof(int)*(cur-prev));
    offset += (cur-prev);
    for(int j=0;j<sample_counts[i];j++) arr[offset+j] = samples[i];
    offset += sample_counts[i];
    prev = cur;
  }
  cur = bucket_starts[THREADS-1];
  memcpy(&arr[offset],&buckets[prev],sizeof(int)*(cur-prev));

  //sort 'buckets' recursively
  #pragma omp parallel
  {
    int tid = omp_get_thread_num();
    int start = tid? bucket_starts[tid-1] : 0;
    int end = bucket_starts[tid];
    int offset = 0;
    for(int i=0;i<tid;i++) offset += sample_counts[i];
    samplesort(arr+offset+start,(end-start),heap+start*2);
  }
}



int main(int argc, char** argv){
  if(argc < 3) return -1;
  const int N = atoi(argv[1]);
  const int algo = atoi(argv[2]);
#ifdef PLOT
  printf("%d\t",N);
#else
#ifdef MERGESORT
  printf("running mergesort.\n");
#endif

#ifdef QUICKSORT
  printf("running quicksort.\n");
#endif

#ifdef KMERGESORT
  printf("running kmergesort.\n");
#endif

#ifdef SAMPLESORT
  printf("running selectionsort.\n");
#endif
#endif

  int* nums    = (int*) malloc(sizeof(int)*N);
  int* scratch = (int*) malloc(2*sizeof(int)*N);
#ifdef DEBUG
  int* answers = (int*) malloc(sizeof(int)*N);
  if(!answers ||
#else
  if(
#endif
      !nums || !scratch){
    printf("Couldn't malloc that much memory\n");
    if(scratch) free(scratch);
    if(nums) free(nums);
    return -1;
  }

  std::clock_t start,stop;

  srand(time(NULL));
  for(int i=0;i<N;i++) nums[i] = (rand() % 200)-100;

#ifdef DEBUG
  //for(int i=0;i<N;i++) printf("%d,",nums[i]);printf("\n");

  memcpy(answers,nums,sizeof(int)*N);
  quicksort(answers,N);
#endif

  if(algo==0){
    omp_set_dynamic(0);
    omp_set_num_threads(THREADS);
    start = std::clock();
#ifdef MERGESORT
    mergesort_parallel(nums,N,scratch);
#endif

#ifdef QUICKSORT
    quicksort_parallel(nums,N);
#endif

#ifdef KMERGESORT
    kmergesort_parallel(nums,N,scratch);
#endif

#ifdef SAMPLESORT
    samplesort_parallel(nums,N,scratch);
#endif
    stop  = std::clock();
  }else if(algo==1){
    start = std::clock();
#ifdef MERGESORT
    mergesort(nums,N,scratch);
#endif

#ifdef QUICKSORT
    quicksort(nums,N);
#endif

#ifdef KMERGESORT
    kmergesort(nums,N,scratch);
#endif

#ifdef SAMPLESORT
    samplesort(nums,N,scratch);
#endif
    stop  = std::clock();
  }

#ifdef DEBUG
  //for(int i=0;i<N;i++) printf("%d,",nums[i]);printf("\n");
  //for(int i=0;i<N;i++) printf("%d,",answers[i]);printf("\n");

  bool match = true;
  for(int i=0;i<N;i++){
    if(nums[i] != answers[i]){
      match = false;
      break;
    }
  }
  printf("%s\n",match? "Match!" : "Don't match.");
#endif

  printf("%.5f\n",(stop-start)/((double)(CLOCKS_PER_SEC))*1000);

  free(nums);
  free(scratch);
  return 0;
}
