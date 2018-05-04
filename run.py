import os
from time import sleep

sorts = ["./samplesort","./mergesort","./quicksort","./kmergesort"]


outfile = "results.txt"
tempfile = "temp.txt"
os.system("echo 'results' > "+tempfile);

runs = [2**j for j in range(11,21)] #2048 to 1mil

runpar = True
runser = True

for i in runs:
  print "running %d" % i
  for sort in sorts:
    if runser:
      os.system(sort+" "+str(i)+" 1 >> "+tempfile)
      sleep(.3)
    if runpar:
      os.system(sort+" "+str(i)+" 0 >> "+tempfile)
      sleep(.3)

print "processing file!"

temp = open(tempfile,"r")
out = open(outfile,"w+")

out.write("time(ms) ");
for sort in sorts:
  if runser: out.write("%s " % sort[2:])
  if runpar: out.write("%s(parallel) " % sort[2:])
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
      out.write("%d " % curidx)
      for i in xrange(len(counts)):
        out.write("%f " % counts[i])
      out.write("\n")
    curidx = idx
    counts = []
  counts += [time]
  l = temp.readline().strip()

out.write("%d " % curidx)
for i in xrange(len(counts)):
  out.write("%f " % counts[i])
out.write("\n")

out.close()
temp.close()
