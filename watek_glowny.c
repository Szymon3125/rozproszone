#include "main.h"
#include "watek_glowny.h"

void mainLoop()
{
    srandom(rank);
    int tag;
    int perc;

    while (stan != InFinish) {
	switch (stan) {
	    case InRun: 
		perc = random()%100;
		if ( perc < 25 ) {
		    changeState( InWantJob );
		    printlnLamport(lamport, "Ubiegam się o zlecenie")
			allLamports[rank] = lamport; // ? possible race conditions between sendPacket ?
		    packet_t *pkt = malloc(sizeof(packet_t));
		    pkt->data = perc;
			for (int i = 0; i < 16; i++) {
				pkt->jobs[i] = jobs[i];
				jobLists[rank][i] = jobs[i];
			} 
		    ackCount = 0;
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
		// if ( ackCount == size - 1) 
		//     changeState( InRun );
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
			for (int i = 0; i < size; i++) {
				allLamports[i] = -1; // clear lamports
				for (int j = 0; j < 16; j++)
					jobLists[i][j] = 0; // clear job lists
			}
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
