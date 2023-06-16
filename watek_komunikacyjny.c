#include "main.h"
#include "watek_komunikacyjny.h"

/* wątek komunikacyjny; zajmuje się odbiorem i reakcją na komunikaty */
void *startKomWatek(void *ptr)
{
    MPI_Status status;
    int is_message = FALSE;
    packet_t pakiet;
    /* Obrazuje pętlę odbierającą pakiety o różnych typach */
    
    if (rank >= size_k) {
        return NULL;
    }

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
            if (stan == InWantPortal) changeState(InRequestPortal);
            ackCount = 0;
        break;
        case JOB_REQUEST:
			pthread_mutex_lock(&jobs_lock);
            if (allLamports[pakiet.src] != -1) {
                debugLamport(lamport, "Dostałem KOLEJNY RAZ prośbę o pracę od %d z zegarem %d, mam już lamporta tego procesu: %d, zapisuję do bufora", pakiet.src, pakiet.ts, allLamports[pakiet.src]);
                if (allLamportsBuffer[pakiet.src] != -1) println("ERROR: Przepełnienie bufora, proces %d prosi o pracę po raz trzeci!", pakiet.src);
                allLamportsBuffer[pakiet.src] = pakiet.ts;
                for (int i = 0; i < 16; i++)
                    jobListsBuffer[pakiet.src][i] = pakiet.jobs[i];
            } else {
                allLamports[pakiet.src] = pakiet.ts;
                for (int i = 0; i < 16; i++) { jobLists[pakiet.src][i] = pakiet.jobs[i]; }
                debugLamport(lamport, "%d pyta o zlecenia [%d, %d, %d, %d, %d, %d, ...] (z zegarem %d)", pakiet.src, pakiet.jobs[0], pakiet.jobs[1], pakiet.jobs[2], pakiet.jobs[3], pakiet.jobs[4], pakiet.jobs[5], pakiet.ts);
            }
            pthread_mutex_unlock(&jobs_lock);
        case NEW_JOB:
            // ! There is a bug that causes receiving some NEW_JOB messages for unknown reasons (they shouldn't have been sent by skansens).
            // ! Those messages are treated them as normal jobs. There is an additional condition to prevent adding them multiple times
            debug("Dostałem nowe zlecenie: %d", pakiet.data);
			pthread_mutex_lock(&jobs_lock);
            int alreadyAssigned = 0;
            for (int i = 0; i < 100; i++) if (jobsTaken[i] == pakiet.data || jobs[i] == pakiet.data) { alreadyAssigned = 1; break; }
            if (!alreadyAssigned) {
                jobs[jobCount] = pakiet.data;
                jobCount++;
                debug("Dostałem zlecenie, teraz mam %d zleceń: [%d, %d, %d, %d, %d, %d, %d, %d, %d %d, ...]", jobCount, jobs[0], jobs[1], jobs[2], jobs[3], jobs[4], jobs[5], jobs[6], jobs[7] ,jobs[8], jobs[9]);

            } else {
                println("Dostałem zlecenie %d, ale ono zostało już wykonane", pakiet.data);
            }
			pthread_mutex_unlock(&jobs_lock);
        break;
        default:
	    break;
        }
        // pthread_mutex_lock(&lamport_lock);
        // lamport = lamport < pakiet.ts ? pakiet.ts + 1 : lamport + 1;
        // pthread_mutex_unlock(&lamport_lock);
    }
}
