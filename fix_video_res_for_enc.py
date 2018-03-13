#!/usr/bin/python
import os
import sys

INPUT_FN = sys.argv[1]
OUTPUT_FN = sys.argv[2]
W = int(sys.argv[3])
H = int(sys.argv[4])
CU_SIZE = int(sys.argv[5])
FIX_W = W - (W % CU_SIZE)
FIX_H = H - (H % CU_SIZE)
COMMAND = 'ffmpeg -i ' + INPUT_FN + ' -filter:v "crop=' + str(FIX_W) + ':' + str(FIX_H) + ':0:0" ' + OUTPUT_FN
print(COMMAND)
os.system(COMMAND)
