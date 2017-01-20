//Jason Meyers
//Michael Paule
//dumbfade display driver
//builds on ti example code
//20170117


//deal with later:
//board init functions may claim pins that step on other pieces of the project
//sanity check clock speed
//debounce that shit (not actually important)

//Changes to config file:
//enable 'delete terminated tasks' in the config file
//crank up the heap - i have it at 8ish k so far

//notes:
//application deletes dead tasks when idling, and >~6 tasks will overflow the heap,
//so application must hit idle before >~6 tasks are spawned.  don't use __delay_cycles.

/*
 *  ======== displayrtos.c ========
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Types.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>

/* Example/Board Header files */
#include "Board.h"

#define TASKSTACKSIZE     1500

///////// Mike's Defines

#define ADFIRSTCOL 0x11c
#define ADFINALCOL 0x15b
#define ADFIRSTROW 0x100
#define ADFINALROW 0x13f

#define MIDDLELINETOP 0x11f
#define VERT1LEFT 0x128
#define VERT2LEFT 0x135
#define VERT3LEFT 0x142
#define VERT4LEFT 0x14f

////////////

int dpage;


void clearAll(SPI_Handle masterSpi,UShort* transmitBuffer, SPI_Transaction masterTransaction){
    bool transferOK;

    *(transmitBuffer)=0x015; //set column address
    *(transmitBuffer+1)=ADFIRSTCOL;
    *(transmitBuffer+2)=ADFINALCOL;

    *(transmitBuffer+3)=0x075; //set row address
    *(transmitBuffer+4)=ADFIRSTROW;
    *(transmitBuffer+5)=ADFINALROW;

    *(transmitBuffer+6)=0x05c;
    masterTransaction.count = 8;
    transferOK = SPI_transfer(masterSpi, &masterTransaction);

    static int tRows = 128; //total rows
    static int tCol = 64;
    int i = 0;
    int j = 0;
    for(i = 0;i < tCol;i++){ //for every row
       for(j = 0; j < tRows;j++){
           *(transmitBuffer+j)=0x100;
       }
       masterTransaction.count = tRows;
       transferOK = SPI_transfer(masterSpi, &masterTransaction);
    }
    System_printf("cleared whole thing\n");
}

void drawHorzLine(SPI_Handle masterSpi,UShort* transmitBuffer, SPI_Transaction masterTransaction, UShort startCol, UShort endCol,UShort startRow, UShort endRow){
    bool transferOK;

    *(transmitBuffer)=0x015; //set column address
    *(transmitBuffer+1)=startCol;
    *(transmitBuffer+2)=endCol;

    *(transmitBuffer+3)=0x075; //set row address
    *(transmitBuffer+4)=startRow;
    *(transmitBuffer+5)=endRow;

    *(transmitBuffer+6)=0x05c;
    masterTransaction.count = 7;
    transferOK = SPI_transfer(masterSpi, &masterTransaction);

    char i = 0;
    char j = 0;

    for(j = 0;j<endRow-startRow;j++){ //j<16
        for(i=0;i<endCol-startCol;i++){
        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1ff;  //1 0
//        *(transmitBuffer+2)=0x1ff-j-(16*j);  //7 6
//        *(transmitBuffer+3)=0x1ff-j-(16*j);  //5 4
        masterTransaction.count = 2; //4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);
        }
    }
}

void drawVertLine(SPI_Handle masterSpi,UShort* transmitBuffer, SPI_Transaction masterTransaction, UShort startCol, UShort endCol,UShort startRow, UShort endRow){
    bool transferOK;

    *(transmitBuffer)=0x0a0;//Set Remap Format -  set to column writting    ///changed
    *(transmitBuffer+1)=0x111; // 0  0 A5 A4  0 A2 A1 A0 = 0 0 0 1  0 0 0 1 // A5=0 Must be zero for dual COM mode (B[4]=1) || A4=1 to scan from top row (COM127) to bottom (COM0) || A2=0 Disable Nibble Remap and A1=0 No Column Remap makes Column 0-119 map to SEG0-SEG3~SEG476-SEG479 || A0=1 Horizontal Address Increment
    *(transmitBuffer+2)=0x111; // 0  0  0 B4  0  0  0  1 = 0 0 0 1  0 0 0 1// B4=1 Dual COM mode is enabled see Figure 10-7
    masterTransaction.count = 3;
    transferOK = SPI_transfer(masterSpi, &masterTransaction);

    *(transmitBuffer)=0x015; //set column address
    *(transmitBuffer+1)=startCol;
    *(transmitBuffer+2)=endCol;

    *(transmitBuffer+3)=0x075; //set row address
    *(transmitBuffer+4)=startRow;
    *(transmitBuffer+5)=endRow;

    *(transmitBuffer+6)=0x05c;

    masterTransaction.count = 7;
    transferOK = SPI_transfer(masterSpi, &masterTransaction);

    char i = 0;
    char j = 0;

    for(j = 0;j<endCol-startCol;j++){ //j<16
        for(i=0;i<endRow-startRow;i++){
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1ff;  //1 0
//        *(transmitBuffer+2)=0x1ff-j-(16*j);  //7 6
//        *(transmitBuffer+3)=0x1ff-j-(16*j);  //5 4
        masterTransaction.count = 2; //4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);
        }
    }

    *(transmitBuffer)=0x0a0;//Set Remap Format -  set to column writting    ///changed
    *(transmitBuffer+1)=0x110; // 0  0 A5 A4  0 A2 A1 A0 = 0 0 0 1  0 0 0 1 // A5=0 Must be zero for dual COM mode (B[4]=1) || A4=1 to scan from top row (COM127) to bottom (COM0) || A2=0 Disable Nibble Remap and A1=0 No Column Remap makes Column 0-119 map to SEG0-SEG3~SEG476-SEG479 || A0=1 Horizontal Address Increment
    *(transmitBuffer+2)=0x111; // 0  0  0 B4  0  0  0  1 = 0 0 0 1  0 0 0 1// B4=1 Dual COM mode is enabled see Figure 10-7
    masterTransaction.count = 3;
    transferOK = SPI_transfer(masterSpi, &masterTransaction);


}


void drawLetter(SPI_Handle masterSpi,UShort* transmitBuffer, SPI_Transaction masterTransaction, char letter,char box,char columnBlock, char letterRow){ //boxes are the 5 boxes, 0 to 4, left to right,letters are Captial letters,5 letter blocks(2 columns out of 10 per letter), 3 letter rows
    bool transferOK;


    switch(box){
        case 0 :

            *(transmitBuffer)=0x015; //set column address
            *(transmitBuffer+1)=0x11D+columnBlock*2;
            *(transmitBuffer+2)=0x127-(9-columnBlock*2);

            *(transmitBuffer+3)=0x075; //set row address
            *(transmitBuffer+4)=0x120 +(10)*letterRow;
            *(transmitBuffer+5)=0x129 +(10)*letterRow;
            break;

        case 1 :

            *(transmitBuffer)=0x015; //set column address
            *(transmitBuffer+1)=0x129+columnBlock*2;
            *(transmitBuffer+2)=0x133-(9-columnBlock*2);

            *(transmitBuffer+3)=0x075; //set row address
            *(transmitBuffer+4)=0x120 +(10)*letterRow;
            *(transmitBuffer+5)=0x129 +(10)*letterRow;
             break;

        case 2 :

            *(transmitBuffer)=0x015; //set column address
            *(transmitBuffer+1)=0x136+columnBlock*2;
            *(transmitBuffer+2)=0x140-(9-columnBlock*2);

            *(transmitBuffer+3)=0x075; //set row address
            *(transmitBuffer+4)=0x120 +(10)*letterRow;
            *(transmitBuffer+5)=0x129 +(10)*letterRow;
             break;

        case 3 :

            *(transmitBuffer)=0x015; //set column address
            *(transmitBuffer+1)=0x143+columnBlock*2;
            *(transmitBuffer+2)=0x14D-(9-columnBlock*2);

            *(transmitBuffer+3)=0x075; //set row address
            *(transmitBuffer+4)=0x120 +(10)*letterRow;
            *(transmitBuffer+5)=0x129 +(10)*letterRow;
             break;

        case 4 :

            *(transmitBuffer)=0x015; //set column address
            *(transmitBuffer+1)=0x150+columnBlock*2;
            *(transmitBuffer+2)=0x15a-(9-columnBlock*2);

            *(transmitBuffer+3)=0x075; //set row address
            *(transmitBuffer+4)=0x120 +(10)*letterRow;
            *(transmitBuffer+5)=0x129 +(10)*letterRow;
             break;
        }

    *(transmitBuffer+6)=0x05c;

    masterTransaction.count = 7;
    transferOK = SPI_transfer(masterSpi, &masterTransaction);


    char i = 0;
    char j = 0;

    switch(letter){
    case ' ':

            //row 0 always

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 1

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 2

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 3

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 4

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 5

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 6

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 7

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 8

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 9 always
            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            break;

    case 'A':

        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 1
        *(transmitBuffer)=0x1f0;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 2
        *(transmitBuffer)=0x10f;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 5
        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1ff;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;


    case 'B':

        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);


        //row 1

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

         //row 2

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 5

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8
        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;

    case 'C':

        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);


        //row 1

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

         //row 2

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 5

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;


    case 'D':

        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);


        //row 1

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);
         //row 2

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 5

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8
        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;


    case 'E':

        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);


        //row 1

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1ff;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

         //row 2

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 5

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8
        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1ff;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;

    case 'F':

        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);


        //row 1

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1ff;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

         //row 2

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 5

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;


    case 'G':

        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);


        //row 1

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

         //row 2

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 5

        *(transmitBuffer)=0x1f0;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1ff;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8
        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;

    case 'H':

        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);


        //row 1

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

         //row 2

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);
        //row 5

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1ff;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;

    case 'I':
        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 1
        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1ff;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 2

        *(transmitBuffer)=0x1f0;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3

        *(transmitBuffer)=0x1f0;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4

        *(transmitBuffer)=0x1f0;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 5

        *(transmitBuffer)=0x1f0;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6

        *(transmitBuffer)=0x1f0;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7

        *(transmitBuffer)=0x1f0;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1ff;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;


    case 'J':
        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 1

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 2

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 5

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;


    case 'K':

        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);


        //row 1

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

         //row 2

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3

        *(transmitBuffer)=0x1f0;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4

        *(transmitBuffer)=0x10f;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 5

        *(transmitBuffer)=0x10f;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6

        *(transmitBuffer)=0x1f0;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x10f;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;


    case 'L':
        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 1

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 2

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 5

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1ff;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;


    case 'M':

        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);


        //row 1

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

         //row 2

        *(transmitBuffer)=0x10f;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1ff;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3

        *(transmitBuffer)=0x1f0;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 5

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;

    case 'N':

        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);


        //row 1

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

         //row 2

        *(transmitBuffer)=0x10f;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3

        *(transmitBuffer)=0x10f;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4

        *(transmitBuffer)=0x1f0;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 5

        *(transmitBuffer)=0x1f0;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1ff;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1ff;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;

    case 'O':

        //row 0 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);


        //row 1

        *(transmitBuffer)=0x1FF;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

         //row 2

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 3

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 4

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 5

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 6

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 7

        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x1f0;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 8

        *(transmitBuffer)=0x1ff;    //3 2
        *(transmitBuffer+1)=0x1f0;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        //row 9 always
        *(transmitBuffer)=0x100;    //3 2
        *(transmitBuffer+1)=0x100;  //1 0
        *(transmitBuffer+2)=0x100;  //7 6 //always
        *(transmitBuffer+3)=0x100;  //5 4
        masterTransaction.count = 4;
        transferOK = SPI_transfer(masterSpi, &masterTransaction);

        break;


    case 'P':

            //row 0 always
            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);


            //row 1

            *(transmitBuffer)=0x1ff;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x10f;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

             //row 2

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 3

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 4

            *(transmitBuffer)=0x1ff;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x10f;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 5

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 6

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 7

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 8
            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 9 always
            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            break;


    case 'Q':

            //row 0 always
            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);


            //row 1

            *(transmitBuffer)=0x1FF;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

             //row 2

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 3

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 4

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 5

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 6

            *(transmitBuffer)=0x1f0;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 7

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1ff;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 8

            *(transmitBuffer)=0x1ff;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1ff;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 9 always
            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            break;

    case 'R':

            //row 0 always
            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);


            //row 1

            *(transmitBuffer)=0x1FF;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x10f;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

             //row 2

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 3

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 4

            *(transmitBuffer)=0x1ff;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x10f;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 5

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x10f;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 6

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x10f;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 7

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 8

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 9 always
            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            break;

    case 'S':

            //row 0 always
            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);


            //row 1

            *(transmitBuffer)=0x1FF;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x10f;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

             //row 2

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 3

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 4

            *(transmitBuffer)=0x1ff;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 5

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x10f;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 6

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 7

            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1f0;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 8

            *(transmitBuffer)=0x1ff;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x10f;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 9 always
            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            break;


    case 'T':

            //row 0 always
            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);


            //row 1

            *(transmitBuffer)=0x1FF;    //3 2
            *(transmitBuffer+1)=0x1f0;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x1ff;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

             //row 2

            *(transmitBuffer)=0x10f;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 3

            *(transmitBuffer)=0x10f;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 4

            *(transmitBuffer)=0x10f;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 5

            *(transmitBuffer)=0x10f;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 6

            *(transmitBuffer)=0x10f;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 7

            *(transmitBuffer)=0x10f;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 8

            *(transmitBuffer)=0x10f;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            //row 9 always
            *(transmitBuffer)=0x100;    //3 2
            *(transmitBuffer+1)=0x100;  //1 0
            *(transmitBuffer+2)=0x100;  //7 6 //always
            *(transmitBuffer+3)=0x100;  //5 4
            masterTransaction.count = 4;
            transferOK = SPI_transfer(masterSpi, &masterTransaction);

            break;


    case 'U':

                //row 0 always
                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);


                //row 1

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                 //row 2

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 3

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 4

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 5

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 6

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 7

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 8

                *(transmitBuffer)=0x1ff;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x10f;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 9 always
                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                break;

    case 'V':

                //row 0 always
                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);


                //row 1

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                 //row 2

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 3

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 4

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 5

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 6

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 7

                *(transmitBuffer)=0x10f;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x10f;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 8

                *(transmitBuffer)=0x1f0;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 9 always
                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                break;


    case 'W':

                //row 0 always
                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);


                //row 1

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                 //row 2

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 3

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 4

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 5

                *(transmitBuffer)=0x1f0;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 6

                *(transmitBuffer)=0x10f;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1ff;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 7

                *(transmitBuffer)=0x1ff;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1ff;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 8

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 9 always
                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                break;

    case 'X':

                //row 0 always
                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);


                //row 1

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);


                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);


                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);



                *(transmitBuffer)=0x10f;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x10f;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);



                *(transmitBuffer)=0x1f0;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);



                *(transmitBuffer)=0x10f;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x10f;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 7

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 8

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 9 always
                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                break;


    case 'Y':

                //row 0 always
                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);


                //row 1

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                 //row 2

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 3

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 4

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 5

                *(transmitBuffer)=0x10f;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x10f;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 6

                *(transmitBuffer)=0x1f0;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 7

                *(transmitBuffer)=0x1f0;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 8

                *(transmitBuffer)=0x1f0;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 9 always
                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                break;


    case 'Z':

                //row 0 always
                *(transmitBuffer)=0x1ff;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1ff;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);


                //row 1

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1f0;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                 //row 2

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x10f;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 3

                *(transmitBuffer)=0x1f0;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 4

                *(transmitBuffer)=0x10f;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 5

                *(transmitBuffer)=0x10f;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 6

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 7

                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 8

                *(transmitBuffer)=0x1ff;    //3 2
                *(transmitBuffer+1)=0x1f0;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x1ff;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                //row 9 always
                *(transmitBuffer)=0x100;    //3 2
                *(transmitBuffer+1)=0x100;  //1 0
                *(transmitBuffer+2)=0x100;  //7 6 //always
                *(transmitBuffer+3)=0x100;  //5 4
                masterTransaction.count = 4;
                transferOK = SPI_transfer(masterSpi, &masterTransaction);

                break;

    }

}



Void displayInit (UArg arg0, UArg arg1)
{

    SPI_Handle masterSpi;
    SPI_Params masterSpiParams;

    bool transferOK;
  //  unsigned int cmd[1];

    /* Initialize SPI handle as default master */
    SPI_Params_init(&masterSpiParams);
    masterSpiParams.dataSize = 9;
    masterSpi = SPI_open(Board_SPI0, &masterSpiParams);
    if (masterSpi == NULL) {
        System_abort("Error initializing SPI\n");
    }
    else {
        System_printf("SPI initialized\n");
    }


    /* Initialize master SPI transaction structure */
    UShort transmitBuffer[200];
    UShort receiveBuffer[200];
    SPI_Transaction masterTransaction;
    masterTransaction.count = 1;
    masterTransaction.txBuf = transmitBuffer;
    masterTransaction.rxBuf = receiveBuffer;

    /* Initiate SPI transfer */
    //__delay_cycles(70000); //1ms,ish

    //example init from datasheet:
    transmitBuffer[0]=0x0fd; //unlock cmds
    transmitBuffer[1]=0x112;//turn it off

    transmitBuffer[2]=0x0ae; //maybe - turn on sleep mode/ display off

    transmitBuffer[3]=0x015; //set column address
    transmitBuffer[4]=0x11c;
    transmitBuffer[5]=0x15b;

    transmitBuffer[6]=0x075; //set row address
    transmitBuffer[7]=0x100;
    transmitBuffer[8]=0x13f;

    transmitBuffer[9]=0x0b3; //set display clock 80 frames/sec
    transmitBuffer[10]=0x191;

    transmitBuffer[11]=0x0ca;//set multiplex ratio to 1/64 Duty (0x0F~0x3F)
    transmitBuffer[12]=0x13f;

    transmitBuffer[13]=0x0a2;//set display offset (Shift mapping RAM counter) to 0x00
    transmitBuffer[14]=0x100;

    transmitBuffer[15]=0x0a1;//set start line (set mapping RAM Display start line) to 0x00
    transmitBuffer[16]=0x100;

    transmitBuffer[17]=0x0a0;//Set Remap Format -       ///changed
    transmitBuffer[18]=0x110; // 0  0 A5 A4  0 A2 A1 A0 = 0 0 0 1  0 0 0 0 // A5=0 Must be zero for dual COM mode (B[4]=1) || A4=1 to scan from top row (COM127) to bottom (COM0) || A2=0 Disable Nibble Remap and A1=0 No Column Remap makes Column 0-119 map to SEG0-SEG3~SEG476-SEG479 || A0=0 Horizontal Address Increment
    transmitBuffer[19]=0x111; // 0  0  0 B4  0  0  0  1 = 0 0 0 1  0 0 0 1// B4=1 Dual COM mode is enabled see Figure 10-7

    transmitBuffer[20]=0x0b5;//Set GPIO - Disable GPIO1 and GPIO0 inputs
    transmitBuffer[21]=0x100;

    transmitBuffer[22]=0x0ab;//Set Function Selection - Enable Internal VDD regulator
    transmitBuffer[23]=0x101;

  //not in board datasheet
    transmitBuffer[24]=0x0b4;//Set Display Enhancement A - Enable External VSL
    transmitBuffer[25]=0x1a0; // 1 0 1 0 0 0 A1 A0 // A[1:0] = 00b Enables VSL
    transmitBuffer[26]=0x1fd; // B7 B6 B5 B4 B3 1 0 1 // B[7:3] = 11111b Enhanced low GS

    transmitBuffer[27]=0x0c1;//Set Contrast Current - Set Segment Output Current
    transmitBuffer[28]=0x19f;

    transmitBuffer[29]=0x0c7;//Set Master Current - Set Scale Factor of Segment Output Current Control
    transmitBuffer[30]=0x10f;

    transmitBuffer[31]=0x0b9;//Set Default Linear Gray Scale Table -

    transmitBuffer[32]=0x0b1;//Set Phase Length - Set Phase 1 as 5 clocks & Phase 2 as 14 clocks
    transmitBuffer[33]=0x1e2;

  //not in board datasheet
    transmitBuffer[34]=0x0d1;//Set Display Enhancement B - Enhance Driving Scheme Capability
    transmitBuffer[35]=0x182; // 1 0 A5 A4 0 0 1 0 = A[5:4] = 00b is reserve? i.e. enhanced driving scheme? and recommended ///changed
    transmitBuffer[36]=0x120; // 0 0 1 0 0 0 0 0

    transmitBuffer[37]=0x0bb;//Set Precharge Voltage-Set Pre-charge voltage level as 0.6*VCC
    transmitBuffer[38]=0x11f;

    transmitBuffer[39]=0x0b6;//Set Precharge Period - Set Second Pre-Charge Period as 8 Clocks
    transmitBuffer[40]=0x108;
//    transmitBuffer[41]=0x1cd; not in either datasheet controller or the specific board

    transmitBuffer[42]=0x0be;//Set VCOMH - Set common Pins Deselect Voltage Level as 0.86*VCC
    transmitBuffer[43]=0x107;

    transmitBuffer[44]=0x0a6;//Normal Display Mode    //normal display, i think //Mike agrees

    transmitBuffer[45]=0x0a9;//Exit Partial Display Mode (if it was ever on in the first place?)
    transmitBuffer[46]=0x0af;   //turn back on

    //end of routine initialization


 //   transmitBuffer[47]=0x0a5;                               //flash all on

    masterTransaction.count = 48;

    transferOK = SPI_transfer(masterSpi, &masterTransaction);

    __delay_cycles(70000000); //1s,ish

    clearAll(masterSpi,transmitBuffer,masterTransaction);
    __delay_cycles(70000000);
    //writeTesting(masterSpi,transmitBuffer,masterTransaction);

    //    drawBox(masterSpi,transmitBuffer,masterTransaction);
//    __delay_cycles(70000000);
    //outerlines
    drawHorzLine(masterSpi,transmitBuffer,masterTransaction,ADFIRSTCOL,ADFINALCOL, MIDDLELINETOP,MIDDLELINETOP+1);
    drawHorzLine(masterSpi,transmitBuffer,masterTransaction,ADFIRSTCOL,ADFINALCOL, 0x13f,0x140);
    drawVertLine(masterSpi,transmitBuffer,masterTransaction,0x11c,0x11d, MIDDLELINETOP+1,ADFINALROW);
    drawVertLine(masterSpi,transmitBuffer,masterTransaction,0x15B,0x15c, MIDDLELINETOP,ADFINALROW+1);

    //inner verticals
    drawVertLine(masterSpi,transmitBuffer,masterTransaction,VERT1LEFT,VERT1LEFT+1, MIDDLELINETOP+1,ADFINALROW);
    drawVertLine(masterSpi,transmitBuffer,masterTransaction,VERT2LEFT,VERT2LEFT+1, MIDDLELINETOP+1,ADFINALROW);
    drawVertLine(masterSpi,transmitBuffer,masterTransaction,VERT3LEFT,VERT3LEFT+1, MIDDLELINETOP+1,ADFINALROW);
    drawVertLine(masterSpi,transmitBuffer,masterTransaction,VERT4LEFT,VERT4LEFT+1, MIDDLELINETOP+1,ADFINALROW);
//   __delay_cycles(70000000);
//    drawLine(masterSpi,transmitBuffer,masterTransaction,20,0);

    //write letters

    drawLetter(masterSpi,transmitBuffer,masterTransaction,'G',0,0,0); //char letter,char box,char columnBlock, char letterRow){ //boxes are the 5 boxes, 0 to 4, left to right,letters are Captial letters,5 letter blocks(2 columns out of 10 per letter), 3 letter rows
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'R',0,1,0);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'A',0,2,0);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'N',0,3,0);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'D',0,4,0);

    drawLetter(masterSpi,transmitBuffer,masterTransaction,'M',0,0,1); //char letter,char box,char columnBlock, char letterRow){ //boxes are the 5 boxes, 0 to 4, left to right,letters are Captial letters,5 letter blocks(2 columns out of 10 per letter), 3 letter rows
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'A',0,1,1);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'S',0,2,1);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'T',0,3,1);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'R',0,4,1);

    drawLetter(masterSpi,transmitBuffer,masterTransaction,'T',1,0,0); //char letter,char box,char columnBlock, char letterRow){ //boxes are the 5 boxes, 0 to 4, left to right,letters are Captial letters,5 letter blocks(2 columns out of 10 per letter), 3 letter rows
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'R',1,1,0);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'X',1,2,0);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,' ',1,3,0);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,' ',1,4,0);

    drawLetter(masterSpi,transmitBuffer,masterTransaction,'S',1,0,1); //char letter,char box,char columnBlock, char letterRow){ //boxes are the 5 boxes, 0 to 4, left to right,letters are Captial letters,5 letter blocks(2 columns out of 10 per letter), 3 letter rows
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'U',1,1,1);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'B',1,2,1);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'M',1,3,1);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,' ',1,4,1);

    drawLetter(masterSpi,transmitBuffer,masterTransaction,'Y',1,0,2); //char letter,char box,char columnBlock, char letterRow){ //boxes are the 5 boxes, 0 to 4, left to right,letters are Captial letters,5 letter blocks(2 columns out of 10 per letter), 3 letter rows
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'Z',1,1,2);

    drawLetter(masterSpi,transmitBuffer,masterTransaction,' ',0,0,0);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'A',0,0,2);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'B',2,0,0);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'B',2,2,2);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'C',3,0,0);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'A',3,1,1);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'D',4,0,0);
    drawLetter(masterSpi,transmitBuffer,masterTransaction,'E',4,1,1);
    /* Deinitialize SPI */
    SPI_close(masterSpi);

    System_printf("Done\n");

    System_flush();
}


//this function calls all the crap whenever the page changes
void pageChange (UArg arg0, UArg arg1)
{
    if(dpage == 1){
        GPIO_write(Board_LED0, Board_LED_OFF); //0 is blue, 1 is green, 2 is red
        GPIO_write(Board_LED1, Board_LED_OFF); //0 is blue, 1 is green, 2 is red
        GPIO_write(Board_LED2, Board_LED_ON); //0 is blue, 1 is green, 2 is red
    }
    if(dpage == 2){
        GPIO_write(Board_LED0, Board_LED_OFF); //0 is blue, 1 is green, 2 is red
        GPIO_write(Board_LED1, Board_LED_ON); //0 is blue, 1 is green, 2 is red
        GPIO_write(Board_LED2, Board_LED_OFF); //0 is blue, 1 is green, 2 is red
    }
    if(dpage == 3){
        GPIO_write(Board_LED0, Board_LED_ON); //0 is blue, 1 is green, 2 is red
        GPIO_write(Board_LED1, Board_LED_OFF); //0 is blue, 1 is green, 2 is red
        GPIO_write(Board_LED2, Board_LED_OFF); //0 is blue, 1 is green, 2 is red
    }
    if(dpage == 4){
        GPIO_write(Board_LED0, Board_LED_ON); //0 is blue, 1 is green, 2 is red
        GPIO_write(Board_LED1, Board_LED_ON); //0 is blue, 1 is green, 2 is red
        GPIO_write(Board_LED2, Board_LED_ON); //0 is blue, 1 is green, 2 is red
    }
}

//this is the down button.  both button functions just change the dpage variable
//and then queue a task to do everything else
void gpioButtonFxn0(unsigned int index)
{
    if(dpage<=1)
        return;

    dpage--;

    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.priority = 3; //higher is more important - page up/down is 3
    Task_create(pageChange, &taskParams, NULL);

}


//this is the up button
void gpioButtonFxn1(unsigned int index)
{
   if(dpage>=4)
        return;

    dpage++;

    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.priority = 3; //higher is more important - page up/down is 3
    Task_create(pageChange, &taskParams, NULL);

}

int main(void)
{
    // Construct BIOS objects
    Task_Params taskParams;

    // Call board init functions.
    Board_initGeneral();
    Board_initGPIO();
    Board_initSPI();

    // Construct display init thread
    Task_Params_init(&taskParams);
    taskParams.priority = 2; //higher is more important - everything display related is 2 so that they execute in order
    taskParams.stackSize = TASKSTACKSIZE;
    Task_create(displayInit, &taskParams, NULL);


    //page down/up buttons
    dpage = 1;

    // install Button callback
    GPIO_setCallback(Board_BUTTON0, gpioButtonFxn0);//dn - world explodes if you change this name
    GPIO_enableInt(Board_BUTTON0);

    GPIO_setCallback(Board_BUTTON1, gpioButtonFxn1);//up - world explodes if you change this name
    GPIO_enableInt(Board_BUTTON1);

    taskParams.priority = 3; //higher is more important - page up/down is 3
    Task_create(pageChange, &taskParams, NULL);

    System_printf("here goes nothin!\n");
    System_flush(); // SysMin will only print to the console when you call flush or exit


    BIOS_start(); //Go!

    return (0);
}


//TI copyright nonsense:
/*
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
