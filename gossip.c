#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <pthread.h>

struct peer
{
	int nodeNumber;
	char IP[16];
	int port;	
	int heartBeat;
	int timeStamp;
	int failed;
};

int done = 0, failed = 0;
int N, b, c, F, B, P, S, T;
int serverSocket;
struct sockaddr_in serverAddr;
struct peer *peerList, *neighbourList;
int nodeNumber = 0;
int localTime;
int randomSeed;
int *generatedNumbers;
char *outputFileName;
char ipAddress[16];
pthread_t serverThread;
pthread_mutex_t peerListMutex;

void setIpAddress()
{
	char ifName[] = "eth0";
	struct ifaddrs *ifAddrStruct=NULL;
	struct ifaddrs *ifa=NULL;
	void *tmpAddrPtr=NULL;
	getifaddrs(&ifAddrStruct);
	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) 
	{
		if (!ifa->ifa_addr) 
		{
			continue;
		}
		if (ifa->ifa_addr->sa_family == AF_INET) 
		{
			tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			if(strcmp(ifName, ifa->ifa_name) == 0)
			{
				strcpy(ipAddress, addressBuffer);
				break;
			}
		}
	}
}

int inNeighbourList(int neigh, int count)
{
	int i;
	for(i = 0; i<count; i++)
	{
		if(neighbourList[i].nodeNumber == neigh)
			return i;
	}
	return -1;
}

int getFailingNode()
{
	srandom(S);
	while(1)
	{
		int r = random() % N;
		if(generatedNumbers[r] == 1)
			continue;
		generatedNumbers[r] = 1;
		return r;
	}
}

void printPeerList(struct peer *list, int count)
{
	int i;
	for(i = 0; i<count; i++)
	{
		printf("%d:%d:%d\n", list[i].nodeNumber, list[i].heartBeat, list[i].timeStamp);
	}
}

void sendToNeighbours()
{
	if(done == 0)
	{
		pthread_mutex_lock(&peerListMutex);
		char *buffer = (char *)malloc(N);
		int i;
		for(i = 0; i<N; i++)
		{
			buffer[i] = peerList[i].heartBeat;
		}
		for(i = 0; i<b; i++)
		{
			int clientSocket = socket(PF_INET, SOCK_DGRAM, 0);
			struct sockaddr_in serverAddr;
		    serverAddr.sin_family = AF_INET;
		    serverAddr.sin_port = htons(neighbourList[i].port);
		    serverAddr.sin_addr.s_addr = inet_addr(neighbourList[i].IP);
		    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));  
			sendto(clientSocket, buffer, N, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
		}
		pthread_mutex_unlock(&peerListMutex);
	}
}

void replaceNeighbour(int i)
{
	int pos = inNeighbourList(i, b);
	if(pos != -1)
	{
		while(1)
		{
			int neigh = random() % N;
			if(neigh == nodeNumber || inNeighbourList(neigh, i) >= 0 || peerList[neigh].failed == 1)
			{
				continue;
			}
			neighbourList[pos] = peerList[neigh];
			break;
		}
	}
}

void *updatePeerList(void *arg)
{
	if(done == 0)
	{
		pthread_mutex_lock(&peerListMutex);
		char *buffer = (char *)arg;
		int i;
		for(i=0; i<N; i++)
		{
			char oldHB = (char)peerList[i].heartBeat;
			if(oldHB < buffer[i])
			{
				peerList[i].heartBeat = (int)buffer[i];
				peerList[i].timeStamp = localTime;
			}
			else if(localTime - oldHB >= F && peerList[i].failed == 0)
			{
				peerList[i].failed = 1;
				printf("Node %d failed.\n", i);
				replaceNeighbour(i);
			}
		}
		pthread_mutex_unlock(&peerListMutex);
	}
}

void *listenForPeerUpdates(void* arg)
{
	while(1)
	{
		char *buffer = (char *)malloc(N);
		struct sockaddr_in clientAddr;
		socklen_t addr_size;
		addr_size = sizeof(clientAddr);
		recvfrom(serverSocket, buffer, N, 0, (struct sockaddr *)&clientAddr, &addr_size);
		pthread_t updateThread;
		pthread_create(&updateThread, NULL, updatePeerList, (void *)buffer);
		if(done == 1)
		{
			pthread_join(updateThread, NULL);
			break;
		}
	}
}

void setupServerSocket()
{
    serverSocket = socket(PF_INET, SOCK_DGRAM, 0);
	int port = 10000;
	do
	{
	    serverAddr.sin_family = AF_INET;
	    serverAddr.sin_port = htons(port);
	    serverAddr.sin_addr.s_addr = inet_addr(ipAddress);
	    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
		port++;
	}
    while(bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1);
}

void waitForOk()
{
	char buffer[3];
	struct sockaddr_in clientAddr;
	socklen_t addr_size;
	addr_size = sizeof(clientAddr);
	recvfrom(serverSocket, buffer, 3, 0, (struct sockaddr *)&clientAddr, &addr_size);
}

void populateNeighbourList()
{
	int i = 0;
	while(i < b)
	{
		int neigh = random() % N;
		if(neigh == nodeNumber || inNeighbourList(neigh, i) >= 0 || peerList[neigh].failed == 1)
		{
			continue;
		}
		neighbourList[i++] = peerList[neigh];
	}
}
	
void populatePeerList()
{
	int i = 0, colonFlag = 0, bufPos = 0, port = 0;
	FILE *fp = fopen("endpoints", "r");
	char c;
	while((c = fgetc(fp)) != EOF)
	{
		if(c == '\n')
		{
			peerList[i].port = port;
			colonFlag = 0;
			bufPos = 0;
			peerList[i].nodeNumber = i;
			peerList[i].heartBeat = -1;
			peerList[i].failed = 0;
			peerList[i].timeStamp = 0;
			i++;
		}
		else if(c == ':')
		{
			port = 0;
			peerList[i].IP[bufPos] = 0;
			colonFlag = 1;
		}
		else if(colonFlag == 0)
		{
			peerList[i].IP[bufPos++] = c;
		}
		else
		{
			port = port * 10 + (c - '0');
		}
	}
}

void sendOk()
{
	int clientSocket = socket(PF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in serverAddr;
	char buffer[3];
	buffer[0] = 'O';
	buffer[1] = 'K';
	buffer[2] = 0;
	int i;
	for(i = 0; i<N - 1; i++)
	{
	    serverAddr.sin_family = AF_INET;
	    serverAddr.sin_port = htons(peerList[i].port);
	    serverAddr.sin_addr.s_addr = inet_addr(peerList[i].IP);
	    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
		sendto(clientSocket, buffer, 3, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	}
}

void doFileWork()
{
	char buffer[20];
	FILE *fp = fopen("endpoints", "r");
	if(fp != NULL)
	{
		char c;
		while((c = fgetc(fp)) != EOF)
		{
			if(c == '\n')
			{
				nodeNumber++;
			}
		}
		fclose(fp);
	
		fp = fopen("endpoints", "a");
		fprintf(fp, "%s:%d\n", ipAddress, ntohs(serverAddr.sin_port));
		fclose(fp);
	}
	else
	{
		fp = fopen("endpoints", "w");
		fprintf(fp, "%s:%d\n", ipAddress, ntohs(serverAddr.sin_port));
		fclose(fp);
	}
	printf("Node number: %d\n", nodeNumber);
	if(nodeNumber + 1 == N)
	{
		populatePeerList();
		sendOk();
	}
	else
	{
		waitForOk();
		populatePeerList();
	}
}

int main(int argc, char *argv[])
{
	int i;
	if(argc != 9)
	{
		printf("Usage: ./gossip N b c F B P S T");
		return 1;
	}
	N = atoi(argv[1]);
	b = atoi(argv[2]);
	c = atoi(argv[3]);
	F = atoi(argv[4]);
	B = atoi(argv[5]);
	P = atoi(argv[6]);
	S = atoi(argv[7]);
	T = atoi(argv[8]);	
	setIpAddress();
	outputFileName = (char *)malloc(sizeof(char) * 10);
	generatedNumbers = (int *)malloc(sizeof(int) * N);
	for(i = 0; i<N; i++)
		generatedNumbers[i] = 0;
	peerList = (struct peer *)malloc(N * sizeof(struct peer));
	neighbourList = (struct peer *)malloc(b * sizeof(struct peer));
	setupServerSocket();
	doFileWork();
	sprintf(outputFileName, "list%d.txt", nodeNumber);
	localTime = 0;
	pthread_create(&serverThread, NULL, listenForPeerUpdates, NULL);
	pthread_mutex_init(&peerListMutex, NULL);
	srandom(S + nodeNumber);
	while(localTime < T)
	{
		if(localTime % c == 0)
			populateNeighbourList();
		if(localTime % P == 0 && B > 0 && localTime != 0)
		{
			int failingNode = getFailingNode();
			srandom(S + nodeNumber);
			B--;
			if(failingNode == nodeNumber)
			{
				printf("Failing node %d.\n", nodeNumber);
				done = 1;
				failed = 1;
			}
		}
		sendToNeighbours();
		sleep(1);
		if(failed == 0)
		{
			peerList[nodeNumber].heartBeat = localTime;
			peerList[nodeNumber].timeStamp = localTime;
		}
		localTime++;
	}
	done = 1;
	close(serverSocket);
	FILE *ofp;
	ofp = fopen(outputFileName, "w");
	if(failed == 0)
		fprintf(ofp, "OK\n");
	else
		fprintf(ofp, "FAIL\n");
	for(i=0; i<N; i++)
		fprintf(ofp, "%d %d\n", i, peerList[i].timeStamp);
}