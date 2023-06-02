#include "main.h"
#include "watek_komunikacyjny.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t pakiet;
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    while ( stan!=InFinish ) {
	debug("czekam na recv");
        MPI_Recv( &pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        switch ( status.MPI_TAG ) {
	    case REQUEST: 
                debug("Ktoś coś prosi (z zegarem %d). Ja mam %d. Czy ubiegam się o dostęp? %d", pakiet.ts, lamport, stan == InWant);
                if (pakiet.ts < lamport) {
		            sendPacket( 0, status.MPI_SOURCE, ACK );
                } else {
                    
                }
	    break;
	    case ACK: 
                debug("Dostałem ACK od %d, mam już %d", status.MPI_SOURCE, ackCount);
	        ackCount++; /* czy potrzeba tutaj muteksa? Będzie wyścig, czy nie będzie? Zastanówcie się. */
	    break;
	    default:
	    break;
        }
        pthread_mutex_lock(&lamport_lock);
        lamport = lamport < pakiet.ts ? pakiet.ts + 1 : lamport + 1;
        pthread_mutex_unlock(&lamport_lock);
    }
}
