#ifndef XYREPORT_H
#define XYREPORT_H

typedef struct {
   int id;
   int step;         //Current unit of time used for the Cell Report
   double value;     //Value of the cell at time "step"
} Report;

typedef struct {
   int numInFD;
   int numOutFD;
   int allInFD[12];     //All needed "in" file descriptors
   int allOutFD[12];    //All needed "out" file descriptors
   double fixedValue;
} CellFileDescriptor;

#endif
