#include "main.h"
#include "watek_glowny.h"

void mainLoop()
{
	for (int i = 0; i < size; i++) {
        allLamports[i] = -1; // clear lamports
        for (int j = 0; j < 16; j++)
            jobLists[i][j] = 0; // clear job lists
    }
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
		int knownLists = 0;
		for (int i = 0; i < size; i++) if (allLamports[i] != -1) knownLists++;
		if (knownLists == size) {
			int myJob = -1;
			debugLamport(lamport, "Znam listy wszystkich procesów");
			debugLamport(lamport, "%d [%d, %d, %d], %d [%d, %d, %d]", allLamports[0], jobLists[0][0], jobLists[0][1], jobLists[0][2], allLamports[1], jobLists[1][0], jobLists[1][1], jobLists[1][2]);
			for (int k = 0; k < size; k++) {
				int minLamportId = rank;
				for (int i = 0; i < size; i++) {
					if (jobLists[i][0] == 0)
						continue;
					if (jobLists[minLamportId][0] == 0				// in case of empty list, force to check next
					|| (allLamports[i] < allLamports[minLamportId]
					|| (allLamports[i] == allLamports[minLamportId] && i < minLamportId)))
							minLamportId = i;
				}
				if (jobLists[minLamportId][0] == 0) {				// nikt już nie ma zleceń
					// TODO: reset allLamports & jobLists
					break;
				} else {
					if (rank == minLamportId)
						myJob = jobLists[minLamportId][0];
					
					// TODO: usunąć zlecenie jobLists[minLamportId][0] ze wszystkich list
					for (int i = 0; i < 16; i++) {
						jobLists[minLamportId][i] = 0;
					}
				}
			}
			if (myJob == -1) {
				debugLamport(lamport, "Nie mam zleceń");
				changeState(InRun);
			} else {
				debugLamport(lamport, "Mam zlecenie %d", myJob);

				printlnLamport(lamport, "Ubiegam się o portal");
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
		}
		if ( 0 ) {
		    
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
