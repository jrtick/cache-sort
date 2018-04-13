import os

#my computer has 32KB L1, 256KB L2, 4MB L3 shared
#32KB = 8192 ints
#32KB+256KB = 73,728 ints
#32KB+256KB+4MB = 1,122,304 ints

for i in [2**j for j in range(4,30)]:
  os.system("./sort "+str(i))
