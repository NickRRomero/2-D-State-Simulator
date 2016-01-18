#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "XYReport.h"
#include <unistd.h>
#include <assert.h>

#define SIMULATE 83
#define TRANSMIT 79
#define ACCEPT 73
#define FIXEDVAL 86
#define IDNUMBER 68
#define MAX_CELL_INPUTS 15

/*Following will read all Command Line Arguments passed to Cell
 * Return value signifies how many unique reads this cell will have
 */

int ReadArguments(int *inFd, int *outFd, int acceptFd[], int transmitFd[],
 Report *myReportStruct, int *time, int argc, char **argv) {
   int i, cmd, actualNumberOfInputs = 0;
   char *cmdVal;

   for (i = 1; i < argc; i++) {
      cmd = *argv[i];
      cmdVal = argv[i] + 1;
      switch (cmd) {
      case SIMULATE:
         if (atoi(cmdVal) >= 0)
            *time = atoi(cmdVal);
         break;

      case TRANSMIT:
         transmitFd[++(*outFd)] = atoi(cmdVal);
         break;

      case ACCEPT:
         actualNumberOfInputs++;
         acceptFd[++(*inFd)] = atoi(cmdVal);
         break;

      case FIXEDVAL:
         myReportStruct->value = strtod(cmdVal, NULL);
         break;

      case IDNUMBER:
         myReportStruct->id = atoi(cmdVal);
         break;
      
      default:
         assert(1);
      }
   }
   return actualNumberOfInputs;
}

/*This fuction will loop for a predetermined amount of time. The cell will read
 * all incoming data from adjacent cells. It will then average out their values
 * and adjust its own. Then the cell will write its value to all outgoing pipes
 */

void SimulateCellTime(int inFd, int outFd, int acceptFd[], int transmitFd[],
 Report *myReportStruct, int time, int actualNumberOfInputs) {
   int i, j = 0, tempInFD, tempOutFD;
   double average = 0.0;
   int bytesRead;
   Report adjacentReportCells[MAX_CELL_INPUTS];
 
   for (i = 0; i <= time; i++) {
      tempOutFD = outFd;
      while (tempOutFD != -1) {
         if (transmitFd[tempOutFD] == 4) {
            bytesRead = write(transmitFd[tempOutFD--], myReportStruct,
             sizeof(Report)); 
         }
         else
            write(transmitFd[tempOutFD--], myReportStruct, sizeof(Report));
      }
      if (inFd > -1) {
         tempInFD = inFd;
         
         j = 0;
         average = 0;
         
         while (tempInFD > -1) {
            read(acceptFd[tempInFD--], &(adjacentReportCells[j++]),
             sizeof(Report));
         }
         
         while (--j > -1)
            average += adjacentReportCells[j].value;

         myReportStruct->value = average / actualNumberOfInputs;
      }
      (myReportStruct->step)++;  
   }
}

//Ensure all file descriptors are closed when the program terminates
void CloseFileDescriptors(int inFd, int outFd, int acceptFd[],
 int transmitFd[]) {
   int i = 0, j = 0;

   while (inFd-- > -1 && i++)
      close(acceptFd[i]);

   while (outFd-- > -1 && j++)
      close(transmitFd[j]);
}


int main(int argc, char **argv) {
   int i = 0, time = 0, inFd = -1, outFd = -1, actualNumberOfInputs;
   int MAX_CELL_OUTPUTS = MAX_CELL_INPUTS;
   int acceptFd[MAX_CELL_INPUTS], transmitFd[MAX_CELL_OUTPUTS];
   Report myReportStruct = { 0 };
   char **start = argv;
   while (i < MAX_CELL_INPUTS) {
      acceptFd[i] = -1;
      transmitFd[i++] = -1;
   }
 
   actualNumberOfInputs = ReadArguments(&inFd, &outFd, acceptFd, transmitFd,
    &myReportStruct, &time, argc, argv);
   
   SimulateCellTime(inFd, outFd, acceptFd, transmitFd, &myReportStruct, time,
    actualNumberOfInputs);
 
   CloseFileDescriptors(inFd, outFd, acceptFd, transmitFd);

   return 0;
}
