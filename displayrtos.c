//Jason Meyers
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

int dpage;

//FUNCTION IS OUTDATED (get update from mike)
void displayInit (UArg arg0, UArg arg1)
{

    SPI_Handle masterSpi;
    SPI_Params masterSpiParams;

    bool transferOK;

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
    System_flush();

    /* Initialize master SPI transaction structure */
    UShort transmitBuffer[100];
    UShort receiveBuffer[100];
    SPI_Transaction masterTransaction;
    masterTransaction.count = 1;
    masterTransaction.txBuf = transmitBuffer;
    masterTransaction.rxBuf = receiveBuffer;

    /* Initiate SPI transfer */
    //__delay_cycles(70000); //1ms,ish

    //example init from datasheet:
    transmitBuffer[0]=0x0fd; //unlock cmds
    transmitBuffer[1]=0x112;//turn it off

    transmitBuffer[2]=0x0ae; //maybe

    transmitBuffer[3]=0x015;
    transmitBuffer[4]=0x11c;
    transmitBuffer[5]=0x15b;

    transmitBuffer[6]=0x075;
    transmitBuffer[7]=0x100;
    transmitBuffer[8]=0x13f;

    transmitBuffer[9]=0x0bc;
    transmitBuffer[10]=0x191;

    transmitBuffer[11]=0x0ca;
    transmitBuffer[12]=0x13f;

    transmitBuffer[13]=0x0a2;
    transmitBuffer[14]=0x100;

    transmitBuffer[15]=0x0a1;
    transmitBuffer[16]=0x100;

    transmitBuffer[17]=0x0a0;  //remap: no idea
    transmitBuffer[18]=0x112;
    transmitBuffer[19]=0x111;

    transmitBuffer[20]=0x0b5;
    transmitBuffer[21]=0x100;

    transmitBuffer[22]=0x0ab;
    transmitBuffer[23]=0x101;

    transmitBuffer[24]=0x0b4;
    transmitBuffer[25]=0x1a0;
    transmitBuffer[26]=0x1fd;

    transmitBuffer[27]=0x0c1;
    transmitBuffer[28]=0x195;

    transmitBuffer[29]=0x0c7;
    transmitBuffer[30]=0x1ef;

    transmitBuffer[31]=0x0b9;

    transmitBuffer[32]=0x0b1;
    transmitBuffer[33]=0x1e2;

    transmitBuffer[34]=0x0d1;
    transmitBuffer[35]=0x120;
    transmitBuffer[36]=0x120;

    transmitBuffer[37]=0x0bb;
    transmitBuffer[38]=0x11f;

    transmitBuffer[39]=0x0b6;
    transmitBuffer[40]=0x108;
    transmitBuffer[41]=0x1cd;

    transmitBuffer[42]=0x0be;
    transmitBuffer[43]=0x107;

    transmitBuffer[44]=0x0a6;   //normal display, i think

    transmitBuffer[45]=0x0af;   //turn back on

    transmitBuffer[46]=0x0a5;

    masterTransaction.count = 47;

    transferOK = SPI_transfer(masterSpi, &masterTransaction);

    __delay_cycles(70000000); //1s,ish

    UShort transmitBuffer2[100];
    UShort receiveBuffer2[100];
    SPI_Transaction masterTransaction2;
    masterTransaction2.count = 1;
    masterTransaction2.txBuf = transmitBuffer2;
    masterTransaction2.rxBuf = receiveBuffer2;

    masterTransaction.count = 1;
    transmitBuffer2[0]=0x0a4;
    transferOK = SPI_transfer(masterSpi, &masterTransaction2);

    __delay_cycles(70000000); //1s,ish
    transmitBuffer2[0]=0x0a5;
    transferOK = SPI_transfer(masterSpi, &masterTransaction2);

    __delay_cycles(70000000); //1s,ish
    transmitBuffer2[0]=0x0a6;
    transferOK = SPI_transfer(masterSpi, &masterTransaction2);

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
