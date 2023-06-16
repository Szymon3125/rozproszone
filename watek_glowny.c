#include "main.h"
#include "watek_glowny.h"

int myJob = -1;

void mainLoop()
{
	// Clear allLamports & jobLists
	for (int i = 0; i < size_k; i++) {
        allLamports[i] = -1;
		allLamportsBuffer[i] = -1;
        for (int j = 0; j < 16; j++) {
            jobLists[i][j] = 0;
			jobListsBuffer[i][j] = 0;
		}
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
			if (i!=rank) {
				// println("Wysyłam JOB_REQUEST do %d", i);
			    sendPacket( pkt, i, JOB_REQUEST);
			}
		    free(pkt);
		}
		debug("Skończyłem myśleć");
break;
case InWantJob:;
		int knownLists = 0;
		for (int i = 0; i < size_k; i++) {
			if (allLamports[i] != -1) knownLists++;
			// else println("Czekam na listę od %d", i);
		}
		debug("Uzgadniam zlecenia %d/%d", knownLists, size_k);
		if (knownLists == size_k) {
			myJob = -1;
			pthread_mutex_lock(&jobs_lock);
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
					for (int i = 0; i < 100; i++) if (jobsTaken[i] != 0) { jobsTaken[i] = job; break; }
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
					// pthread_mutex_lock(&jobs_lock);
					for (int i = 0; i < jobCount; i++) {
						if (jobs[i] == job) {
							debug("Usuwam zlecenie %d.", jobs[i]);
							for (int j = i; j < jobCount + 1; j++)
								jobs[j] = jobs[j + 1];
						}
					}
					jobCount--;
					// pthread_mutex_unlock(&jobs_lock);
					for (int i = 0; i < 16; i++) {
						jobLists[minLamportId][i] = 0;
					}
					debug("%d pozostałych zleceń o których wiem: [%d, %d, %d, %d, %d, %d, ...]", jobCount, jobs[0], jobs[1], jobs[2], jobs[3], jobs[4], jobs[5]);
				}
				pthread_mutex_unlock(&jobs_lock);
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
			// Read new job lists from buffer (if any)
			debug("Czyszczę bufor");
			for (int i = 0; i < size_k; i++) {
				allLamports[i] = allLamportsBuffer[i];
				allLamportsBuffer[i] = -1;
				for (int j = 0; j < 16; j++) {
					jobLists[i][j] = jobListsBuffer[i][j];
					jobListsBuffer[i][j] = 0;
				}
			}
			debugLamport(lamport, "nowy stan list (z bufora) %d [%d, %d, %d], %d [%d, %d, %d]", allLamports[0], jobLists[0][0], jobLists[0][1], jobLists[0][2], allLamports[1], jobLists[1][0], jobLists[1][1], jobLists[1][2]);
			pthread_mutex_unlock(&jobs_lock);
			// Send the empty JOB_REQUEST to those who sent new job lists into the buffer (untested may cause issues)
			// for (int i = 0; i < size_k; i++) {
			// 	if (allLamports[i] != -1) {
			// 		packet_t* pkt = (packet_t*)malloc(sizeof(packet_t));
			// 		for (int j = 0; j < 16; j++) pkt->jobs[j] = 0;
			// 		println("Wysyłam puste JOB_REQUEST do %d (na podstawie listy skopiowanej z bufora)", i)
			// 		sendPacket(pkt, i, JOB_REQUEST);
			// 		free(pkt);
			// 	}
			// }
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
break;
case InWantPortal:
		debugLamport(lamport, "Czekam na wejście do sekcji krytycznej %d/%d", ackCount, size_k - size_p)
		// tutaj zapewne jakiś muteks albo zmienna warunkowa
		// bo aktywne czekanie jest BUE
		if ( ackCount >= size_k - size_p) 
		    changeState(InSection);
break;
case InSection:
		// tutaj zapewne jakiś muteks albo zmienna warunkowa
		//printlnLamport(lamport, "Jestem w sekcji krytycznej")
		printlnLamport(lamport, "Wykonuję zlecenie %d", myJob);
		    sleep(5);
		//if ( perc < 25 ) {
		    // debug("Perc: %d", perc);
		    printlnLamport(lamport, "Skończyłem wykonywać zlecenie %d:", myJob)
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
