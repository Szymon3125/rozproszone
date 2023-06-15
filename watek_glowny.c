#include "main.h"
#include "watek_glowny.h"

void mainLoop()
{
    srandom(rank);
    int tag;
    int perc;

	if (rank >= size - skanseny) {
		int nextJob = rank % (size - skanseny) + 1;
		while (1) {
			sleep(random() % 3);
		    println("Wysylam nowe zlecenie: %d", nextJob);
		    packet_t *pkt = malloc(sizeof(packet_t));
			pkt->data = nextJob;
			for (int i = 0; i < size - skanseny; i++) {
				sendPacket(pkt, i, NEW_JOB);
			}
			nextJob += skanseny;
		}
	}

    while (stan != InFinish) {
	switch (stan) {
	    case InRun: 
		perc = random()%100;
		if ( perc < 25 ) {
			// pthread_mutex_lock(&lamport_lock);
			// lamport++;
			// pthread_mutex_unlock(&lamport_lock);
		    // debug("Perc: %d", perc);
		    printlnLamport(lamport, "Ubiegam się o pracę")
		    debug("Zmieniam stan na wysyłanie");
		    packet_t *pkt = malloc(sizeof(packet_t));
		    pkt->data = perc;
			for (int i=0;i<=min(jobCount, 16);i++) { pkt->jobs[i] = jobs[i]; } // sending max 16 jobs i want to do
		    ackCount = 0;
		    changeState( InWantJob );
		    for (int i=0;i<=size-1;i++)
			if (i!=rank)
			    sendPacket( pkt, i, JOB_REQUEST);
		    free(pkt);
		}
		debug("Skończyłem myśleć");
		break;
		case InWantJob:
		debug("Chcę pracować");
		// TODO: warunek na pracę - zgoda od wszystkich pozostałych które zadanie mogę wykonać
		if ( 0 ) {
		    printlnLamport(lamport, "Ubiegam się o portal")
		    packet_t *pkt = malloc(sizeof(packet_t));
		    pkt->data = perc;
		    ackCount = 0;
		    changeState( InWantPortal );
		    for (int i=0;i<=size-1;i++)
			if (i!=rank)
			    sendPacket( pkt, i, PORTAL_REQUEST);
		    free(pkt);
		}
		break;
	    case InWantPortal:
		printlnLamport(lamport, "Czekam na wejście do sekcji krytycznej %d/%d", ackCount, size - 1)
		// tutaj zapewne jakiś muteks albo zmienna warunkowa
		// bo aktywne czekanie jest BUE
		if ( ackCount == size - 1) 
		    changeState(InSection);
		break;
	    case InSection:
		// tutaj zapewne jakiś muteks albo zmienna warunkowa
		printlnLamport(lamport, "Jestem w sekcji krytycznej")
		    sleep(5);
		//if ( perc < 25 ) {
		    // debug("Perc: %d", perc);
		    printlnLamport(lamport, "Wychodzę z sekcji krytyczneh")
		    debug("Zmieniam stan na wysyłanie");
		    packet_t *pkt = malloc(sizeof(packet_t));
		    pkt->data = perc;
		    for (int i=0;i<=size-1;i++)
			if (i!=rank)
			    sendPacket( pkt, i, PORTAL_RELEASE);
		    changeState( InRun );
		    free(pkt);
			lamport += 100;
		//}
		break;
	    default: 
		break;
            }
        sleep(SEC_IN_STATE);
    }
}
