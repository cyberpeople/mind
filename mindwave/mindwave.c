#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <winsock2.h>

#include "mindwave.h"


char *comPortName = NULL;
int   dllVersion = 0;
int   connectionId = 0;
int   packetsRead = 0;
int   errCode = 0;
int   blink = 0;
int   att = 0;
FILE *dataFile;
SOCKET helen;


/**
 * Prompts and waits for the user to press ENTER.
 */
void wait() {
    printf( "\n" );
    printf( "Press the ENTER key...\n" );
    fflush( stdout );
    getc( stdin );
}

/**
 * Program which prints ThinkGear EEG_POWERS values to stdout.
 */
int main( void ) {

	opensocket();
	dataFile = fopen("dataStore.txt", "a+");

    /* Print driver version number */
    dllVersion = TG_GetDriverVersion();
    printf( "ThinkGear DLL version: %d\n", dllVersion );

    /* Get a connection ID handle to ThinkGear */
    connectionId = TG_GetNewConnectionId();
    if( connectionId < 0 ) {
        fprintf( stderr, "ERROR: TG_GetNewConnectionId() returned %d.\n", 
                 connectionId );
        wait();
        exit( EXIT_FAILURE );
    }

    /* Attempt to connect the connection ID handle to serial port "COM5" */
    //comPortName = "\\\\.\\COM19";
	comPortName = "COM7:";
    errCode = TG_Connect( connectionId, 
                          comPortName, 
                          TG_BAUD_57600, 
                          TG_STREAM_PACKETS );

	printf("TG_Connect struct: %d, %s, %d \n", connectionId, comPortName, TG_BAUD_57600);
    if( errCode < 0 ) {
        fprintf( stderr, "ERROR: TG_Connect() returned %d.\n", errCode );
        wait();
        exit( EXIT_FAILURE );
    }

    errCode = TG_EnableBlinkDetection( connectionId, 1 );
    if( errCode < 0 ) {
        fprintf( stderr, "ERROR: TG_EnableBlinkDetection() returned %d.\n", errCode );
        wait();
        exit( EXIT_FAILURE );
    }


	do
	{
		readdata();
		if(blink>=80)
			trncmd();
		else if(att>=68)
			fwdcmd();
		//wait();
		 
	}while(1);

    /* Clean up */
    TG_FreeConnection( connectionId );

    /* End program */
    wait();
    return( EXIT_SUCCESS );
}

int readdata()
{
	/* Attempt to read a Packet of data from the connection */
        errCode = TG_ReadPackets( connectionId, 1 );

		/* If TG_ReadPackets() was able to read a complete Packet of data... */
        if( errCode == 1 ) 
		{
            /* If attention value has been updated by TG_ReadPackets()... */
            if( TG_GetValueStatus(connectionId, TG_DATA_ATTENTION) != 0 ) 
			{
				/* Get and print out the updated attention value */
                att = (int)TG_GetValue(connectionId, TG_DATA_ATTENTION);
				printf( "New ATT value: %d\n", att );
				fprintf( dataFile, "ATT Value: %d\n", att);
                fflush( dataFile );

            } /* end "If attention value has been updated..." */

			/* If blink value has been updated by TG_ReadPackets()... */
            if( TG_GetValueStatus(connectionId, TG_DATA_BLINK_STRENGTH) != 0 ) 
			{
				/* Get and print out the updated attention value */
                blink = (int)TG_GetValue(connectionId, TG_DATA_BLINK_STRENGTH);
				printf( "New BLINK value: %d\n", blink );
				fprintf( dataFile, "BLINK Value: %d\n\n\n", blink);
                fflush( dataFile );

            } /* end "If blink value has been updated..." */


        } /* end "If a Packet of data was read..." */

		//wait();
		return;
}

int trncmd()
{
	printf("Turning around\n");
	//send command to 10.30.200.190 command=RIGHT
	wait();
	blink=0;
	return 1;
}

int fwdcmd()
{
	printf("Going forward\n");
	//send command to 10.30.200.190 command=FORWARD
	wait();
	att=0;
	return 1;
}

int opensocket()
{
	//open a connect to 10.30.200.190 //helen's netbook
	helen = EstablishConnection();
}
