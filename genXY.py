#!/usr/bin/python

import random
import math
import os
import sys

def getCellNumber():
   return (raw_input("Please enter a number of cells, i.e. 1, 4, 9, 16, ...," +
           " 400, 441\nNOTE: The largest possible grid is 21 x 21 cells.(441)\n"
           ),
           raw_input("Please enter a lower limit on the fixed cell values\n"),
           raw_input("Please enter an upper limit on the fixed cell values\n"))
   
def openFile(cellNumber):
   newFile = 'XY_' + str(cellNumber) + '.cell'
   return open(newFile, 'w')

def writeToFile(newFile, buildFile, g):
   for key, value in g.items():
      newFile.write(str(key[0]) + ' ' + str(key[1]))
      newFile.write(' ' + str(round(value, 3)) + '\n')
   newFile.close()

def createDict(buildFile, newFile):
   fixedCells = 0
   cellNumber = int(buildFile[0])
   lowerLimit = float(buildFile[1])
   upperLimit = float(buildFile[2])
   cellRoot = int(math.sqrt(cellNumber))
   d = {}
   for x in range(0, cellRoot):
      for y in range(0, cellRoot):
         if y < 1 or y >= cellRoot - 1:
            fixedCells += 1
            d[(x, y)] = random.uniform(lowerLimit, upperLimit)
         else:
            if x == 0 or x == cellRoot - 1:
               fixedCells += 1
               d[(x, y)] = random.uniform(lowerLimit, upperLimit)
   
   newFile.write(str(fixedCells) + '\n')
   return d

if __name__== "__main__":
   buildFile = getCellNumber()
   newFile = openFile(buildFile[0])
   cellDict = createDict(buildFile, newFile)
   writeToFile(newFile, buildFile, cellDict)
