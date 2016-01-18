#ifndef XYREPORT_H
#define XYREPORT_H
#define MAX_FD 12

typedef struct {
   int id;
   int step;         //Current unit of time used for the Cell Report
   double value;     //Value of the cell at time "step"
} Report;


/*The following struct is built specifically for each cell.
 * The struct's fields contain all the needed information for a cell
 */
typedef struct {
   int numInFD;
   int numOutFD;
   int allInFD[MAX_FD];     //All needed "in" file descriptors
   int allOutFD[MAX_FD];    //All needed "out" file descriptors
   double fixedValue;
} CellFileDescriptor;

#endif
