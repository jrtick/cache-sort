import os
from time import sleep

#my computer has 32KB L1, 256KB L2, 4MB L3 shared
#32KB = 8192 ints
#32KB+256KB = 73,728 ints
#32KB+256KB+4MB = 1,122,304 ints


sorts = ["./samplesort","./mergesort","./quicksort","./kmergesort"]


outfile = "results.txt"
tempfile = "temp.txt"
os.system("echo 'results' > "+tempfile);

runs = [j*10000 for j in range(1,100,5)]#2**j for j in range(4,20,2)] #4,30??

for i in runs:
  print "running %d" % i
  for sort in sorts:
    os.system(sort+" "+str(i)+" 1 >> "+tempfile)
    sleep(.1)
    os.system(sort+" "+str(i)+" 0 >> "+tempfile)
    sleep(.1)

print "processing file!"

temp = open(tempfile,"r")
out = open(outfile,"w+")

out.write("time(ms) ");
for sort in sorts:
  out.write("%s " % sort[2:])
  out.write("%s(parallel) " % sort[2:])
out.write("\n")

temp.readline()
l = temp.readline().strip()

curidx = -1
counts = []
while l:
  (idx,time) = l.split("\t")
  idx = int(idx)
  time = float(time)
  if(idx != curidx):
    if(curidx != -1):
      out.write("%d " % idx)
      for i in xrange(len(counts)):
        out.write("%f " % counts[i])
      out.write("\n")
    curidx = idx
    counts = []
  counts += [time]
  l = temp.readline().strip()

out.write("%d " % idx)
for i in xrange(len(counts)):
  out.write("%f " % counts[i])
out.write("\n")

out.close()
temp.close()
