#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <string.h>
#include <math.h>
//#include <thread>

#define PLOT
#define CUTOFF 32
//#define DEBUG

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

//M should be cachesize / sizeof(int). 128 on rpi?
#define K 8
#define M 512 //My laptop L1 is 32kb so (32/2)*1024/32 = 512
int offsets[K];
int counts[K];
void kmergesort(int* arr,int count,int* scratch){
  if(count<M) quicksort(arr,count); //free since fit in cache
  else{
    //do recursion
    const int merge_size = (count+K-1)/K;
    int offset = 0;
    for(int i=0;i<K-1;i++){
      kmergesort(arr+offset,merge_size,scratch);
      counts[i] = merge_size;
      offsets[i] = offset;
      offset += merge_size;
    }
    kmergesort(arr+offset,count-offset,scratch);
    counts[K-1] = count-offset;
    offsets[K-1] = offset;


    //do k-merge
    for(int idx=0;idx<count;idx++){
      int minelem;
      int minidx = -1;
      for(int check=1;check<K;check++){
        if(offsets[check]==counts[check]) continue; //saturated
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
  if(count <= CUTOFF){
    naivesort(arr,count);
    return;
  }
  const int sqrtN = int(sqrt(count)+0.5);

  //this mem scheme works only if 3ceil(sqrt(N))+N<2N and 2*ceil(sqrt(N))<N
  //which occurs when N>=12
  int* samples = heap; //sqrtN-1 samples
  int* sample_counts = heap+sqrtN; //sqrtN-1 
  int* bucket_starts = heap+2*sqrtN; //sqrtN buckets
  int* buckets = heap+3*sqrtN; //size N for space efficiency

  //pick & sort pivots
  for(int i=0;i<sqrtN-1;i++){
    bool unique;
    int sample;
    
    do{
      sample = rand()%count;
      unique = true;
      for(int j=0;j<i;j++){
        if(sample == samples[j]){
          unique = false;
          break;
        }
      }
    }while(!unique);
    
    samples[i] = arr[sample];
  }
  samplesort(samples,sqrtN-1,heap+sqrtN); //must give at least 2*sqrt(N) mem

  //fill buckets
  memset(heap+sqrtN,0,sizeof(int)*2*sqrtN); //clear bucket starts and sample_counts
  for(int i=0;i<count;i++){ //get bucket/sample counts
    int elem = arr[i];
    int bucketidx = find_bucket(elem,samples,sqrtN-1);
    if(elem == samples[bucketidx]) sample_counts[bucketidx]++;
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
    if(elem != samples[bucketidx]){
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

int main(int argc, char** argv){
  if(argc < 3) return -1;
  const int N = atoi(argv[1]);
  const int algo = atoi(argv[2]);
#ifdef PLOT
  printf("%d\t",N);
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
    start = std::clock();
    quicksort(nums,N);//samplesort(nums,N);
    stop  = std::clock();
  }else if(algo==1){
    start = std::clock();
    samplesort(nums,N,scratch);//quicksort(nums,N);
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

  printf("%.5fms\n",(stop-start)/((double)(CLOCKS_PER_SEC))*1000);

  free(nums);
  free(scratch);
  return 0;
}
