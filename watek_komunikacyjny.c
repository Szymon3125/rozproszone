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
	    case PORTAL_REQUEST: 
            pthread_mutex_lock(&lamport_lock);
            pthread_mutex_lock(&stateMut);
            debug("%d prosi o dostęp (z zegarem %d). Ja mam %d. Czy ubiegam się o dostęp? %d", pakiet.src, pakiet.ts, lamport, stan == InWantPortal);
            int agree = (stan != InSection && (stan != InWantPortal || pakiet.ts < lamport || (pakiet.ts == lamport && pakiet.src < rank)));
            if (stan != InWantPortal) lamport = lamport < pakiet.ts ? pakiet.ts + 1 : lamport + 1; // update lamport if process does not care about the critical section
            pthread_mutex_unlock(&stateMut);
            pthread_mutex_unlock(&lamport_lock);
            if (agree) {
                sendPacket( 0, status.MPI_SOURCE, PORTAL_ACK );
            } else {
                debug("Nie odsyłam PORTAL_ACK do %d!", pakiet.src);
            }
            
	    break;
	    case PORTAL_ACK: 
	        ackCount++; /* czy potrzeba tutaj muteksa? Będzie wyścig, czy nie będzie? Zastanówcie się. */
                debug("Dostałem PORTAL_ACK od %d, mam już %d", status.MPI_SOURCE, ackCount);
	    break;
        case PORTAL_RELEASE:
            if (stan == InWantPortal) changeState(InRun);
            ackCount = 0;
        break;
        case JOB_REQUEST:
            debug("%d prosi o pracę (z zegarem %d). Ja mam %d. Czy ubiegam się o pracę? %d", pakiet.src, pakiet.ts, lamport, stan == InWantJob);
            // TODO: porównać jobs[100] z pakiet.jobs[16]
	    default:
	    break;
        }
        // pthread_mutex_lock(&lamport_lock);
        // lamport = lamport < pakiet.ts ? pakiet.ts + 1 : lamport + 1;
        // pthread_mutex_unlock(&lamport_lock);
    }
}
