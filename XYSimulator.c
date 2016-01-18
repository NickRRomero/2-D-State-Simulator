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
#define REPORT_STRING "Result from Cell (%d, %d), step %d: %.3f\n"
#define FORMAT_LENGTH 15
#define PARSE_FORMAT_STRING "X%d Y%d"
#define TEN 10
#define FIFTEEN 15
#define TWENTY_FIVE 25
#define MAX_WORD_SIZE 100
#define MIN_CELLS_FOR_PIPES 4
#define MAX_PIPES_OPEN 10
#define MAX_CELL_ARGS 100

/* The following function reads in the required command line arguments.
 * Error checking is performed on the inputted file. A .cell file is
 * assumed to be well formated.
 */
void ReadCmdArgs(int argc, char **argv, int *numberOfCells, int *simulateTime,
 int *sqrtOfTotalCells) {
   int cmd, i = 2, nFlag = 0, tFlag = 0;
   char *cmdVal, *extension;

   *numberOfCells = 0;

   extension = strrchr(argv[1], '.');

   if (!(strcmp(extension, ".cell") == 0)) {
      printf("Error: .cell file not found\n");
      printf("Usage: ./a.out XY_#.cell C S\n");
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

/* The following function parses the provided .cell file. The file
 * is assumed to be well formatted. The returned char ** contains
 * all needed fixed cell values and their coordinates.
 */ 
char **ParseXYFile(int *numFixedValueCells, char *inputFile) {
   FILE *cellFile;
   char oneWord[MAX_WORD_SIZE], **fixedCellArgs;
   int curCell = 0;

   cellFile = fopen(inputFile, "r");
   fgets(oneWord, TWENTY_FIVE, cellFile);
   oneWord[strlen(oneWord) - 1] = '\0';
   *numFixedValueCells = atoi(oneWord);

   fixedCellArgs = calloc(*numFixedValueCells + 1, sizeof(char *));

   while (curCell < *numFixedValueCells) {
      fixedCellArgs[curCell] = calloc(1, TWENTY_FIVE);
      fgets(oneWord, TWENTY_FIVE, cellFile);
      oneWord[strlen(oneWord) - 1] = '\0';
      memcpy(fixedCellArgs[curCell++], oneWord, TWENTY_FIVE);
   }
   fclose(cellFile);

   return fixedCellArgs;
}

/* This function uses the CellFileDescriptor struct and the parsed
 * XY file to assign the appropriate fixed values to their respective cells.
 */
void AssignFixedCellValues(int numberOfCells, char **parsedCellValues,
 CellFileDescriptor *cellRdWr, int numFixedValueCells,
 int sqrtOfTotalCells) {
   int cellRoot, xCord, yCord, fixedCell, i = 0;
   char *end;
   double fixedValue;

   cellRoot = sqrt(numberOfCells);

   while (numFixedValueCells--) {
      xCord = strtol(parsedCellValues[i++], &end, TEN);
      yCord = strtol(end, &end, TEN);
      fixedValue = strtod(end, &end);

      fixedCell = xCord + yCord * cellRoot;
      cellRdWr[fixedCell].fixedValue = fixedValue;
   }
}

/*This function will set up the first row of pipes. The first row of pipes
 * is determined by the square root of the number of cells. Any cells with ids
 * less than this root will be located in this row.
 */
void FirstRowPipes(int numberOfCells, int curCell, int cellRoot,
 int *downRightYAxisPipe, int *downLeftYAxisPipe, int *downYAxisPipe,
 CellFileDescriptor *cellRdWr) {
   
   //Top Left Corner Cell
   if (curCell == 0) {
      pipe(downRightYAxisPipe);

      //Future Cell Setup
      cellRdWr[cellRoot + 1].allInFD[cellRdWr[cellRoot + 1].numInFD++] =
       downRightYAxisPipe[0];
   
      //Current Cell Setup
      cellRdWr[curCell].allOutFD[cellRdWr[curCell].numOutFD++] =
       downRightYAxisPipe[1];
   }
   
   //Top Right Corner Cell
   else if (curCell == cellRoot - 1) {
      pipe(downLeftYAxisPipe);

      //Future Cell Setup
      cellRdWr[curCell + cellRoot - 1].allInFD[cellRdWr[curCell + cellRoot -
         1].numInFD++] = downLeftYAxisPipe[0];

      //Current Cell Setup
      cellRdWr[curCell].allOutFD[cellRdWr[curCell].numOutFD++] =
         downLeftYAxisPipe[1];
   }

   //All other Cells in first row
   else if (curCell < (numberOfCells - cellRoot)) {
      if (curCell < cellRoot) {
         pipe(downYAxisPipe);

         //Future Cell
         cellRdWr[curCell + cellRoot].allInFD[cellRdWr[curCell + cellRoot].
            numInFD++] = downYAxisPipe[0];

         //Current Cell
         cellRdWr[curCell].allOutFD[cellRdWr[curCell].numOutFD++] =
            downYAxisPipe[1];

         //First row cells. Does not include last two in row
         if (curCell < cellRoot - 2) {
            pipe(downRightYAxisPipe);

            //Future Cell
            cellRdWr[curCell + cellRoot + 1].allInFD[cellRdWr[curCell +
               cellRoot + 1].numInFD++] = downRightYAxisPipe[0];

            //Current Cell
            cellRdWr[curCell].allOutFD[cellRdWr[curCell].numOutFD++] =
               downRightYAxisPipe[1];
         }

         //First row cells. Does not include top elft corner
         if (curCell > 1) {
            pipe(downLeftYAxisPipe);

            //Future Cell
            cellRdWr[curCell + cellRoot - 1].allInFD[cellRdWr[curCell + 
             cellRoot - 1].numInFD++] = downLeftYAxisPipe[0];

            //Current Cell
            cellRdWr[curCell].allOutFD[cellRdWr[curCell].numOutFD++] =
               downLeftYAxisPipe[1];
         }
      }
   }
}

/* This function is responsible for setting up all interior cells, up
 * to the second to last row. Every cell will set up the adjacent row's cell
 * as well as the adjacent cells in the next row
 */
void InteriorCellsPipes(int numberOfCells, int cellRoot, int curCell,
 int *rightXAxisPipe, int *downRightYAxisPipe, int *leftXAxisPipe,
 int *upRightYAxisPipe, int *upLeftYAxisPipe, int *downLeftYAxisPipe,
 int *upYAxisPipe, int *downYAxisPipe, CellFileDescriptor *cellRdWr) { 

   //All cells in the leftmost column
   if (curCell % cellRoot == 0) {
      pipe(rightXAxisPipe);
      pipe(downRightYAxisPipe);

      //Future Cell Setup
      cellRdWr[curCell + 1].allInFD[cellRdWr[curCell + 1].numInFD++] =
         rightXAxisPipe[0];
      cellRdWr[curCell + cellRoot + 1].allInFD[cellRdWr[curCell +
         cellRoot + 1].numInFD++] = downRightYAxisPipe[0];

      //Current Cell Setup
      cellRdWr[curCell].allOutFD[cellRdWr[curCell].numOutFD++] =
         rightXAxisPipe[1];
      cellRdWr[curCell].allOutFD[cellRdWr[curCell].numOutFD++] =
         downRightYAxisPipe[1];
   }

   //All cells up to, but not indluding, the second to last column on the right
   else if (curCell % cellRoot <= cellRoot - 2) {
      if (curCell % cellRoot < cellRoot - 2) {
         pipe(rightXAxisPipe);
         pipe(leftXAxisPipe);
         pipe(upRightYAxisPipe);
         pipe(downRightYAxisPipe);
         pipe(upLeftYAxisPipe);

         //Future Cell Setup
         cellRdWr[curCell + 1].allInFD[cellRdWr[curCell + 1].numInFD++] =
            rightXAxisPipe[0];
         cellRdWr[curCell + 1].allOutFD[cellRdWr[curCell + 1].numOutFD++] =
            leftXAxisPipe[1];
         cellRdWr[curCell + cellRoot - 1].allOutFD[cellRdWr[curCell +
            cellRoot - 1].numOutFD++] = upRightYAxisPipe[1];
         cellRdWr[curCell + cellRoot + 1].allInFD[cellRdWr[curCell +
            cellRoot + 1].numInFD++] = downRightYAxisPipe[0];
         cellRdWr[curCell + cellRoot + 1].allOutFD[cellRdWr[curCell +
            cellRoot + 1].numOutFD++] = upLeftYAxisPipe[1];

         //Current Cell Setup
         cellRdWr[curCell].allOutFD[cellRdWr[curCell].numOutFD++] =
            rightXAxisPipe[1];
         cellRdWr[curCell].allInFD[cellRdWr[curCell].numInFD++] =
            leftXAxisPipe[0];
         cellRdWr[curCell].allInFD[cellRdWr[curCell].numInFD++] =
            upRightYAxisPipe[0];
         cellRdWr[curCell].allOutFD[cellRdWr[curCell].numOutFD++] =
            downRightYAxisPipe[1];
         cellRdWr[curCell].allInFD[cellRdWr[curCell].numInFD++] =
            upLeftYAxisPipe[0];
      }

      //Cells in the second to las column on the right
      else if (curCell % cellRoot == cellRoot - 2) {
         pipe(leftXAxisPipe);
         pipe(upRightYAxisPipe);
         pipe(upLeftYAxisPipe);
         cellRdWr[curCell + 1].allOutFD[cellRdWr[curCell + 1].numOutFD++] =
            leftXAxisPipe[1];
         cellRdWr[curCell + cellRoot - 1].allOutFD[cellRdWr[curCell +
            cellRoot - 1].numOutFD++] = upRightYAxisPipe[1];
         cellRdWr[curCell + cellRoot + 1].allOutFD[cellRdWr[curCell +
            cellRoot + 1].numOutFD++] = upLeftYAxisPipe[1];

         cellRdWr[curCell].allInFD[cellRdWr[curCell].numInFD++] =
            leftXAxisPipe[0];
         cellRdWr[curCell].allInFD[cellRdWr[curCell].numInFD++] =
            upRightYAxisPipe[0];
         cellRdWr[curCell].allInFD[cellRdWr[curCell].numInFD++] =
            upLeftYAxisPipe[0];
      }

      pipe(upYAxisPipe);
      pipe(downYAxisPipe);

      //Future cell setup
      cellRdWr[curCell + cellRoot].allInFD[cellRdWr[curCell + cellRoot].
         numInFD++] = downYAxisPipe[0];
      cellRdWr[curCell + cellRoot].allOutFD[cellRdWr[curCell + cellRoot].
         numOutFD++] = upYAxisPipe[1];

      //Current cell setup
      cellRdWr[curCell].allInFD[cellRdWr[curCell].numInFD++] =
         upYAxisPipe[0];
      cellRdWr[curCell].allOutFD[cellRdWr[curCell].numOutFD++] =
         downYAxisPipe[1];

      //All cells except for the first column
      if (curCell % cellRoot > 1) {
         pipe(downLeftYAxisPipe);

         cellRdWr[curCell + cellRoot - 1].allInFD[cellRdWr[curCell +
            cellRoot - 1].numInFD++] = downLeftYAxisPipe[0];
       
         cellRdWr[curCell].allOutFD[cellRdWr[curCell].numOutFD++] =
            downLeftYAxisPipe[1];
      }  
   }

   //All cells in the last column
   else if (curCell % cellRoot == cellRoot - 1) {
      pipe(downLeftYAxisPipe);

      //Future Cell Setup
      cellRdWr[curCell + cellRoot - 1].allInFD[cellRdWr[curCell +
         cellRoot - 1].numInFD++] = downLeftYAxisPipe[0];

      //Current Cell Setup
      cellRdWr[curCell].allOutFD[cellRdWr[curCell].numOutFD++] =
         downLeftYAxisPipe[1];
   }
}

/* This function will set up the second to last row's pipes. Because
 * of how the cell pipe logic works, no function is needed for the final
 * row. Once the last cell in the second to last row is finished,
 * all pipes for the last row have been created.
 */
void SecondToLastRowPipes(int numberOfCells, int cellRoot, int curCell,
 int *rightXAxisPipe, int *upRightYAxisPipe, int *upLeftYAxisPipe,
 int *upYAxisPipe, int *leftXAxisPipe, CellFileDescriptor *cellRdWr) {
   
   //Cell in the first column
   if (curCell % cellRoot == 0) {
      pipe(rightXAxisPipe);
   
      //Future Cell Setup
      cellRdWr[curCell + 1].allInFD[cellRdWr[curCell + 1].numInFD++] =
         rightXAxisPipe[0];

      //Current Cell Setup
      cellRdWr[curCell].allOutFD[cellRdWr[curCell].numOutFD++] =
         rightXAxisPipe[1];
   }

   //Cells in all columns up to, but not including, the last column
   //on the right
   else if (curCell % cellRoot != numberOfCells - cellRoot - 1) {
      if (curCell != 5) {
         if (curCell % cellRoot < cellRoot - 2) {
            pipe(rightXAxisPipe);

            //Future Cell Setup
            cellRdWr[curCell + 1].allInFD[cellRdWr[curCell + 1].numInFD++] =
               rightXAxisPipe[0];

            //Current Cell Setup
            cellRdWr[curCell].allOutFD[cellRdWr[curCell].numOutFD++] =
               rightXAxisPipe[1];
         }

         if (curCell != numberOfCells - cellRoot - 1) {
            pipe(upRightYAxisPipe);
            pipe(upLeftYAxisPipe);
            pipe(upYAxisPipe);
            pipe(leftXAxisPipe);

            //Future Cell Setup
            cellRdWr[curCell + cellRoot - 1].allOutFD[cellRdWr[curCell +
               cellRoot - 1].numOutFD++] = upRightYAxisPipe[1];
            cellRdWr[curCell + cellRoot + 1].allOutFD[cellRdWr[curCell +
               cellRoot + 1].numOutFD++] = upLeftYAxisPipe[1];
            cellRdWr[curCell + cellRoot].allOutFD[cellRdWr[curCell + cellRoot].
               numOutFD++] = upYAxisPipe[1];
            cellRdWr[curCell + 1].allOutFD[cellRdWr[curCell + 1].numOutFD++] =
               leftXAxisPipe[1];

            //Current Cell Setup
            cellRdWr[curCell].allInFD[cellRdWr[curCell].numInFD++] =
               upRightYAxisPipe[0];
            cellRdWr[curCell].allInFD[cellRdWr[curCell].numInFD++] =
               upLeftYAxisPipe[0];
            cellRdWr[curCell].allInFD[cellRdWr[curCell].numInFD++] =
               upYAxisPipe[0];
            cellRdWr[curCell].allInFD[cellRdWr[curCell].numInFD++] =
               leftXAxisPipe[0];
         }
      }
   }
}

/* Helper function for pipe creation. Depending on where we are located in
 * the grid, one of these functions are called
 */
void SetupPipes(int numberOfCells, int curCell, int *downYAxisPipe,
 int *upYAxisPipe, int *upLeftYAxisPipe, int *upRightYAxisPipe,
 int *leftXAxisPipe, int *rightXAxisPipe, int *downLeftYAxisPipe,
 int *downRightYAxisPipe, int cellRoot, CellFileDescriptor *cellRdWr) {

   if (numberOfCells > MIN_CELLS_FOR_PIPES) {
      //First row of pipes
      if (curCell < cellRoot) {
         FirstRowPipes(numberOfCells, curCell, cellRoot,
          downRightYAxisPipe, downLeftYAxisPipe, downYAxisPipe,
          cellRdWr);
      }

      //All interior cells up to the second to last row
      else if (curCell < numberOfCells - 2 * cellRoot) {
         InteriorCellsPipes(numberOfCells, cellRoot, curCell,
          rightXAxisPipe, downRightYAxisPipe, leftXAxisPipe, upRightYAxisPipe,
          upLeftYAxisPipe, downLeftYAxisPipe, upYAxisPipe, downYAxisPipe,
          cellRdWr);
      }

      //Second to last row
      else if (curCell < numberOfCells - cellRoot) {
         SecondToLastRowPipes(numberOfCells, cellRoot, curCell,
          rightXAxisPipe, upRightYAxisPipe, upLeftYAxisPipe, upYAxisPipe,
          leftXAxisPipe, cellRdWr);
      }
   }
}

/* This function will build the needed command line arguments for the cell
 * child processes. This function will only be called in the forked child
 * process.
 */
void BuildCellCmdArguments(int curCell, int reportPipeWrite,
  CellFileDescriptor *cellRdWr, char ***cellArgv, int time,
  char **parsedCellValues, int numberOfCells) {
  int tempInFD = 0, tempOutFD = 0;
  char tempArg[TWENTY_FIVE];
  
   strcpy(*(*cellArgv)++, "CELL\0");
   sprintf(tempArg, "S%i", time);
   strcpy(*(*cellArgv)++, tempArg);

   sprintf(tempArg, "D%i", curCell);
   strcpy(*(*cellArgv)++, tempArg);
   sprintf(tempArg, "O%i", reportPipeWrite);
   strcpy(*(*cellArgv)++, tempArg);
 
   while (tempInFD < cellRdWr[curCell].numInFD) {
      sprintf(tempArg, "I%i", cellRdWr[curCell].allInFD[tempInFD++]);
      strcpy(*(*cellArgv)++, tempArg);
   }

   while (tempOutFD < cellRdWr[curCell].numOutFD) {
      sprintf(tempArg, "O%i", cellRdWr[curCell].allOutFD[tempOutFD++]);
      strcpy(*(*cellArgv)++, tempArg);
   }
   
   if (cellRdWr[curCell].fixedValue) {
      tempArg[0] = 'V';
      sprintf(tempArg + 1, "%f", cellRdWr[curCell].fixedValue);
      strcpy(*(*cellArgv)++, tempArg);
   }
}

/* This function reads all data from the reportPipe. All cells will write to
 * this pipe during each of their "time" iterations. The x and y coordinates
 * are determined by the cell's id and that id's relation to the square root
 * of the total number of cells
 */
void ReportOutput(int *reportPipe, int numberOfCells, int time, int cellRoot) {
   int xCord, yCord;
   Report parentReport = { 0 };

   close(reportPipe[1]);
 
   while (numberOfCells--)
      wait(NULL);

   while (read(reportPipe[0], &parentReport, sizeof(Report))) {
      xCord = parentReport.id % cellRoot;
      yCord = parentReport.id / cellRoot;
      printf(REPORT_STRING, xCord, yCord, parentReport.step,
            parentReport.value);
   }  
}

/*Main driver for the XYSimulator.c file. In this function, the parent process
 * continually forks children that will then exec() into cells
 */
int main(int argc, char **argv) {
   int numberOfCells, simulateTime, reportPipe[2], numFixedValueCells;
   int curCell = 0, i = 0, sqrtOfTotalCells;
   int upLeftYAxisPipe[2] = { 0 }, upRightYAxisPipe[2] = { 0 };
   int upYAxisPipe[2] = { 0 }, downYAxisPipe[2] = { 0 };
   int leftXAxisPipe[2] = { 0 }, rightXAxisPipe[2] = { 0 };
   int downLeftYAxisPipe[2] = { 0 }, downRightYAxisPipe[2] = { 0 };
   char **parsedCellValues, **cellArgv, **cellArgvStart;
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

      cellArgv = calloc(MAX_CELL_ARGS, sizeof(char *));
      for (i = 0; i < MAX_CELL_ARGS; i++)
         cellArgv[i] = calloc(1, MAX_WORD_SIZE);
      
      if (!fork()) {
         close(reportPipe[0]);
         cellArgvStart = cellArgv;
         BuildCellCmdArguments(curCell, reportPipe[1], cellFileDescArgs,
          &cellArgv, simulateTime, parsedCellValues, numberOfCells);
         *cellArgv = NULL;
         cellArgv = cellArgvStart;

         execve("./Cell", cellArgv, NULL);
      }
      for (i = 0; i < MAX_CELL_ARGS; i++)
         if (cellArgv[i])
            free(cellArgv[i]);
      curCell++;     
   }

   ReportOutput(reportPipe, numberOfCells, simulateTime, sqrtOfTotalCells);

   return 0;
}
