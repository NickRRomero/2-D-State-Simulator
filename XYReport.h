#ifndef XYREPORT_H
#define XYREPORT_H

typedef struct {
   int id;
   int step;         //Current unit of time used for the Cell Report
   double value;     //Value of the cell at time "step"
                     //Following Coordinates give a cell's unique "id"
   int x_coordinate; //X Coordinate. Used if Program Dimension >= 1
   int y_coordinate; //Y Coordinate. Used if Program Dimension >= 2
   int z_coordinate; //Z Coordinate. Used if Program Dimension >= 3
} Report;

typedef struct {
   int numInFD;
   int numOutFD;
   int allInFD[12];     //All needed "in" file descriptors
   int allOutFD[12];    //All needed "out" file descriptors
   double fixedValue;
} CellFileDescriptor;

#endif
