#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "XYReport.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <sys/resource.h>
#include <fcntl.h>

#define CELL_NUMBER 67
#define SIMULATE 83
#define REPORT_STRING "Result from Cell (%d, %d, 0), step %d: %.3f\n"
#define FORMAT_LENGTH 15
#define PARSE_FORMAT_STRING "X%d Y%d"
#define TEN 10
#define FIFTEEN 15
#define TWENTY_FIVE 25

void ReadCmdArgs(int argc, char **argv, int *numberOfCells, int *simulateTime,
 int *sqrtOfTotalCells) {
   int cmd, i = 2, nFlag = 0, tFlag = 0, fileFound = 0;
   char inputFile[TWENTY_FIVE];
   char *cmdVal, *extension;

   *numberOfCells = 0;

   extension = strrchr(argv[1], '.');

   if (!(strcmp(extension, ".cell") == 0)) {
      printf("Error: .cell file not found\n");
      printf("Usage: ./a.out somefile.cell C S\n");
      exit(1);
   }

   for (; i < argc; i++) {
      cmd = *argv[i];
      cmdVal = argv[i] + 1;
      
      switch (cmd) {
      case CELL_NUMBER:
         if (!nFlag) {
            *numberOfCells = atoi(cmdVal);
            nFlag = 1;
         }
         break;

      case SIMULATE:
         if (!tFlag) {
            *simulateTime = atoi(cmdVal);
            tFlag = 1;
         }
         break;

      default:
         assert(1);
      }
   }
   *sqrtOfTotalCells = sqrt(*numberOfCells);
}

char **ParseXYFile(int *numFixedValueCells, char *inputFile) {
   FILE *cellFile;
   char oneWord[100], **fixedCellArgs;
   int curCell = 0;

   cellFile = fopen(inputFile, "r");
   fgets(oneWord, 30, cellFile);
   oneWord[strlen(oneWord) - 1] = '\0';
   *numFixedValueCells = atoi(oneWord);

   fixedCellArgs = calloc(*numFixedValueCells + 1, sizeof(char *));

   while (curCell < *numFixedValueCells) {
      fixedCellArgs[curCell] = calloc(1, 30);
      fgets(oneWord, 30, cellFile);
      oneWord[strlen(oneWord) - 1] = '\0';
      memcpy(fixedCellArgs[curCell++], oneWord, 30);
   }
   fclose(cellFile);

   return fixedCellArgs;
}

void AssignFixedCellValues(int numberOfCells, char **parsedCellValues,
 CellFileDescriptor *cellRdWr, int numFixedValueCells,
 int sqrtOfTotalCells) {
   int cellRoot, xCord, yCord, fixedCell, i = 0;
   char *end;
   double fixedValue;

   cellRoot = sqrt(numberOfCells);

   while (numFixedValueCells--) {
      xCord = strtol(parsedCellValues[i++], &end, 10);
      yCord = strtol(end, &end, 10);
      fixedValue = strtod(end, &end);

      fixedCell = xCord + yCord * cellRoot;
      cellRdWr[fixedCell].fixedValue = fixedValue;
   }
}
void SetupPipes(int numberOfCells, int curCell, int *downYAxisPipe,
 int *upYAxisPipe, int *upLeftYAxisPipe, int *upRightYAxisPipe,
 int *leftXAxisPipe, int *rightXAxisPipe, int *downLeftYAxisPipe,
 int *downRightYAxisPipe, int cellRoot, CellFileDescriptor *cellRdWr) {

   if (numberOfCells > 4) {
      if (curCell < cellRoot) {
         FirstRowPipes(numberOfCells, curCell, cellRoot,
          downRightYAxisPipe, downLeftYAxisPipe, downYAxisPipe,
          cellRdWr);
      }

      else if (curCell < numberOfCells - 2 * cellRoot) {
         InteriorCellsPipes(numberOfCells, cellRoot, curCell,
          rightXAxisPipe, downRightYAxisPipe, leftXAxisPipe, upRightYAxisPipe,
          upLeftYAxisPipe, downLeftYAxisPipe, upYAxisPipe, downYAxisPipe,
          cellRdWr);
      }

      else if (curCell < numberOfCells - cellRoot) {
         SecondToLastRowPipes(numberOfCells, cellRoot, curCell,
          rightXAxisPipe, upRightYAxisPipe, upLeftYAxisPipe, upYAxisPipe,
          leftXAxisPipe, cellRdWr);
      }
   }
}

int main(int argc, char **argv) {
   int numberOfCells, simulateTime, reportPipe[2], numFixedValueCells;
   int curCell = 0, i = 0, sqrtOfTotalCells;
   int upLeftYAxisPipe[2] = { 0 }, upRightYAxisPipe[2] = { 0 };
   int upYAxisPipe[2] = { 0 }, downYAxisPipe[2] = { 0 };
   int leftXAxisPipe[2] = { 0 }, rightXAxisPipe[2] = { 0 };
   int downLeftYAxisPipe[2] = { 0 }, downRightYAxisPipe[2] = { 0 };
   char **parsedCellValues, **cellArgv, **cellArgvStart;
   FILE cellFile;
   CellFileDescriptor *cellFileDescArgs;

   if (argc < 2) {
      printf("Usage: ./a.out somefile.cell C S\n");
      exit(1);
   }

   ReadCmdArgs(argc, argv, &numberOfCells, &simulateTime, &sqrtOfTotalCells);

   cellFileDescArgs = calloc(numberOfCells, sizeof(CellFileDescriptor));

   parsedCellValues = ParseXYFile(&numFixedValueCells, argv[1]);

   AssignFixedCellValues(numberOfCells, parsedCellValues, cellFileDescArgs,
    numFixedValueCells, sqrtOfTotalCells);

   pipe(reportPipe);

   while (curCell < numberOfCells) {

      SetupPipes(numberOfCells, curCell, downYAxisPipe, upYAxisPipe,
       upLeftYAxisPipe, upRightYAxisPipe, leftXAxisPipe, rightXAxisPipe,
       downLeftYAxisPipe, downRightYAxisPipe, sqrtOfTotalCells,
       cellFileDescArgs);

   }

   return 0;
}
