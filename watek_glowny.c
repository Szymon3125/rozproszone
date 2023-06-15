#include "main.h"
#include "watek_glowny.h"

void mainLoop()
{
	for (int i = 0; i < size_k; i++) {
        allLamports[i] = -1; // clear lamports
        for (int j = 0; j < 16; j++)
            jobLists[i][j] = 0; // clear job lists
    }
    srandom(rank);
    int tag;
    int perc;

	if (rank >= size_k) {
		int nextJob = rank % size_s + 1;
		while (1) {
			sleep(random() % 3 + 10);
		    debug("Wysylam nowe zlecenie: %d", nextJob);
		    packet_t *pkt = malloc(sizeof(packet_t));
			pkt->data = nextJob;
			for (int i = 0; i < size_k; i++) {
				sendPacket(pkt, i, NEW_JOB);
			}
			nextJob += size_s;
		}
	}

    while (stan != InFinish) {
	packet_t *pkt;
	switch (stan) {
case InRun: 
		perc = random()%100;
		if ( perc < 25 ) {
		    changeState( InWantJob );
		    printlnLamport(lamport, "Ubiegam się o %d zleceń: [%d, %d, %d, %d, %d, %d, ...]", jobCount, jobs[0], jobs[1], jobs[2], jobs[3], jobs[4], jobs[5]);
			allLamports[rank] = lamport; // ? possible race conditions between sendPacket ?
		    packet_t *pkt = malloc(sizeof(packet_t));
		    pkt->data = perc;
			pthread_mutex_lock(&jobs_lock);
			for (int i = 0; i < 16; i++) {
				pkt->jobs[i] = jobs[i];
				jobLists[rank][i] = jobs[i];
			} 
			pthread_mutex_unlock(&jobs_lock);
		    ackCount = 0;
		    for (int i=0;i<=size_k-1;i++)
			if (i!=rank)
			    sendPacket( pkt, i, JOB_REQUEST);
		    free(pkt);
		}
		debug("Skończyłem myśleć");
break;
case InWantJob:
		debug("Uzgadniam zlecenia");
		int knownLists = 0;
		for (int i = 0; i < size_k; i++) if (allLamports[i] != -1) knownLists++;
		if (knownLists == size_k) {
			int myJob = -1;
			debugLamport(lamport, "Znam listy wszystkich procesów");
			debugLamport(lamport, "%d [%d, %d, %d], %d [%d, %d, %d]", allLamports[0], jobLists[0][0], jobLists[0][1], jobLists[0][2], allLamports[1], jobLists[1][0], jobLists[1][1], jobLists[1][2]);
			for (int k = 0; k < size_k; k++) {
				int minLamportId = rank;
				for (int i = 0; i < size_k; i++) {
					if (jobLists[i][0] == 0)
						continue;
					if (jobLists[minLamportId][0] == 0				// in case of empty list, force to check next
					|| (allLamports[i] < allLamports[minLamportId]
					|| (allLamports[i] == allLamports[minLamportId] && i < minLamportId)))
							minLamportId = i;
				}
				if (jobLists[minLamportId][0] == 0) {				// nikt już nie ma zleceń
					// Reset allLamports & jobLists
					// debug("Nikt nie ma już zleceń");
					for (int i = 0; i < size_k; i++) {
						allLamports[i] = -1;
						for (int j = 0; j < 16; j++)
							jobLists[i][j] = 0;
					}
					changeState( InRun );
					break;
				} else {
					int job = jobLists[minLamportId][0];
					if (rank == minLamportId)
						myJob = job;
					// println("%d ma zlecenie %d", minLamportId, job);
					// Usunięcie zlecenia job ze wszystkich list
					for (int i = 0; i < size_k; i++) {
						for (int j = 0; j < 16; j++) {
							if (jobLists[i][j] == job) {
								// move all elements to the left
								for (int k = j; k < 15; k++)
									jobLists[i][k] = jobLists[i][k + 1];
								break;
							}
						}
					}
					// debug("%d [%d, %d, %d], %d [%d, %d, %d]", allLamports[0], jobLists[0][0], jobLists[0][1], jobLists[0][2], allLamports[1], jobLists[1][0], jobLists[1][1], jobLists[1][2]);
					pthread_mutex_lock(&jobs_lock);
					for (int i = 0; i < jobCount; i++) {
						if (jobs[i] == job)
							for (int j = i; j < jobCount + 1; j++)
								jobs[j] = jobs[j + 1];
					}
					jobCount--;
					pthread_mutex_unlock(&jobs_lock);
					for (int i = 0; i < 16; i++) {
						jobLists[minLamportId][i] = 0;
					}
					// debug("%d pozostałych zleceń o których wiem: [%d, %d, %d, %d, %d, %d, ...]", jobCount, jobs[0], jobs[1], jobs[2], jobs[3], jobs[4], jobs[5])
				}
			}
			if (myJob == -1) {
				printlnLamport(lamport, "Nie mam zleceń");
				changeState(InRun);
			} else {
				printlnLamport(lamport, "Mam zlecenie %d", myJob);
				changeState(InRequestPortal);
			}
			// Reset allLamports & jobLists
			for (int i = 0; i < size_k; i++) {
				allLamports[i] = -1;
				for (int j = 0; j < 16; j++)
					jobLists[i][j] = 0;
			}
			// TODO: copy the new job lists from buffer (if any)
			// TODO: send the empty JOB_REQUEST to those who sent new job lists into the buffer
		}
break;
case InRequestPortal:
		printlnLamport(lamport, "Ubiegam się o portal");
		pkt = malloc(sizeof(packet_t));
		pkt->data = perc;
		ackCount = 0;
		changeState( InWantPortal );
		for (int i = 0; i <= size_k; i++)
			if (i!=rank)
				sendPacket( pkt, i, PORTAL_REQUEST);
		free(pkt);
		for (int i = 0; i < size_k; i++) {
			allLamports[i] = -1; // clear lamports
			for (int j = 0; j < 16; j++)
				jobLists[i][j] = 0; // clear job lists
		}
break;
case InWantPortal:
		printlnLamport(lamport, "Czekam na wejście do sekcji krytycznej %d/%d", ackCount, size_k - size_p)
		// tutaj zapewne jakiś muteks albo zmienna warunkowa
		// bo aktywne czekanie jest BUE
		if ( ackCount >= size_k - size_p) 
		    changeState(InSection);
		break;
	    case InSection:
		// tutaj zapewne jakiś muteks albo zmienna warunkowa
		printlnLamport(lamport, "Jestem w sekcji krytycznej")
		    sleep(5);
		//if ( perc < 25 ) {
		    // debug("Perc: %d", perc);
		    printlnLamport(lamport, "Wychodzę z sekcji krytycznej")
		    debug("Zmieniam stan na wysyłanie");
		    pkt = malloc(sizeof(packet_t));
		    pkt->data = perc;
		    for (int i=0;i<=size_k-1;i++)
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
