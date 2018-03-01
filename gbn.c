#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for unidirectional or bidirectional
   data transfer protocols (bidirectional transfer of data
   is not required for project 2).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 for bidirectional data transfer */

#define PACKET_SIZE 20
#define A  0
#define B  1
#define WINDOW_SIZE 8
#define SENDER_BUFFER_SIZE 100

// If set to 1 then it will print actions performed to console with values
int DEBUG = 0;

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[PACKET_SIZE];
   };

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[PACKET_SIZE];
    };

/********* STUDENTS WRITE THE NEXT EIGHT ROUTINES *********/
/*** You do not need to implement B_output() and B_timerinterrupt() in Project 2! ***/


struct pkt *sendablePackets;
struct pkt ACK;

int base;
int next_seqNum;				// sequence number for next packet
int expect_seqNum;

/* ------- PROTOTYPES ----------------------------*/
int getCheckSum(struct pkt packet);
void tolayer3(int, struct pkt);
void tolayer5(int, char*);
void starttimer(int, float);
void stoptimer(int);
/* ------- PROTOTYPES -----------------------------*/

int getCheckSum(struct pkt packet)
{

	int S = 0;		// Default checksum to 0
	int i;

	if(packet.payload != NULL)		// Null check for good measure
	{
		// Add seqnum and acknum added to a character-by-character sum of the payload.
		// This idea was in the assignment documentation
		for(i = 0; i < PACKET_SIZE; i++)
		{
			S += packet.payload[i];
		}
			S += packet.seqnum;
			S += packet.acknum;
	}
	return S;

}

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
	printf("A_OUTPUT\n");
	printf("Data received: %s\n", message.data);
	printf("Next Seqeunce Number: %d\n", next_seqNum);

	memset(sendablePackets + next_seqNum, (sizeof(struct pkt)), 0);

	// Set the seq number of packet
	sendablePackets[next_seqNum].seqnum = next_seqNum;

	// Add checksum value to packet
	sendablePackets[next_seqNum].checksum = getCheckSum(sendablePackets[next_seqNum]);

	// Copy data from message into payload of packet
	memcpy(sendablePackets[next_seqNum].payload, message.data, strlen(sendablePackets[next_seqNum].payload) + 1);


	// Checks to see if the seqNum is in the current window
	while(next_seqNum < base + WINDOW_SIZE)
	{
//		for(int i=0;i<20;i++){
//			sendablePackets[next_seqNum].payload[i] = message.data[i];
//		}


		// Send packet to layer3
		tolayer3(A, sendablePackets[next_seqNum]);

		if(base == next_seqNum){
			starttimer(A, 30);
		}

		printf("Packet %d sent\n", sendablePackets[next_seqNum].seqnum);
		next_seqNum++;
	}
	return;
}

void B_output(message)  /* Do not implement this routine. It is needed only for bidirectional data transfer */
  struct msg message;
{

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	printf("A_INPUT\n");
	printf("Base: %d\n", base);
	printf("Next Sequence Number %d\n", next_seqNum);

	int pkt_cs;
	// Holds what the checksum of the current packet is
	pkt_cs = getCheckSum(packet);

	printf("Original checksum: %d\n", pkt_cs);
	printf("Checksum Received packet from layer3: %d\n", packet.checksum);

	// Checks if checksums are correct, if they don't packet is corrrupted
	if(pkt_cs != packet.checksum)
	{
		printf("A: Corrupted ACK\n");
		return;
	}
		// This means it is a duplicate ACK
		if(packet.acknum < base)
		{
			return;
		}
		// Out of order ACK
		else if (packet.acknum >= next_seqNum)
		{
			return;
		}
		base = packet.acknum + 1;

		// Stops timer if base and sequence number are equal
		// meaning everything should received
		if(base == next_seqNum)
		{
			stoptimer(A);
			printf("A: Timer has been stopped\n");
		}
		else
		{
			stoptimer(A);
			printf("A: Timer staretd \n");
			starttimer(A, 30);
		}
		printf("A: Ack %d received\n", packet.acknum);

	return;
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	printf("A_TIMERINTERRUPT\n");
	printf("Next Sequence Number: %d\n", next_seqNum);
	printf("Base: %d\n", base);

	int i;

	// Send all packets in current window
	for(i = base; i < (next_seqNum); i++)
	{
		printf("Retransmitting packets in window\n");
		tolayer3(A, sendablePackets[i]);
	}

	if (base != next_seqNum)
	{
		// Start Timer
		starttimer(A, 20.0);
	}

	return;
}  


/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	printf("A_INIT\n");
	sendablePackets = malloc( (sizeof(struct pkt)) * SENDER_BUFFER_SIZE);
	memset(sendablePackets,(sizeof(struct pkt))* SENDER_BUFFER_SIZE, 0);

	// Initialize the sequence number to 1
	base = 1;
	next_seqNum = 1;
}


/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	printf("B_INPUT\n");

	//Temporary value to hold check sum of packet
	int b_checksum;
	b_checksum = getCheckSum(packet);

	printf("Expected Sequence Number: %d\n", expect_seqNum);
	printf("Expected Sequence Number: %d\n", packet.seqnum);
	printf("Received: %s\n", packet.payload);
	printf("Original Check Sum: %d\n", b_checksum);
	printf("Checksum Received from A: %d\n", packet.checksum);

	memcpy(ACK.payload, packet.payload, strlen(ACK.payload) + 1);

	if((packet.checksum != b_checksum))
	{
		return;
	}

	if(packet.seqnum == expect_seqNum)
	{
		// Increase expeted_seqNum for next packet
		expect_seqNum++;

		// Send packet from b to layer 5
		tolayer5(B, packet.payload);
		printf("B: Sent Packet, layer 5!\n");

		// Set the acknum of ACK to number expected next in the sequence
		ACK.acknum = expect_seqNum;

		// Set the checksum of ACK to checksum of packet
		// So receiver can verify the incoming packet has the appropriate checksum
		ACK.checksum = getCheckSum(ACK);

		tolayer3(B, ACK);
		printf("B: Sent Packet, layer 3!\n");


		return;
	}
	else if (packet.seqnum < expect_seqNum)
	{
       printf("B: Duplicate Packet\n");

	}
	else
	{
       printf("B: Out of Order PACKET\n");
	}

	// Defaulting to Re-sending ACK
	 memcpy(ACK.payload, packet.payload, strlen(ACK.payload) + 1);
	 tolayer3(B, ACK);
	 printf("B: Re-sending ACK, %s\n", ACK.acknum);

	 return ;
}

/* called when B's timer goes off */
void B_timerinterrupt() /* Do not implement this routine. It is needed only for bidirectional data transfer */
{

}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void  B_init()
{
	printf("B_INIT\n");
	expect_seqNum = 1;
	ACK.acknum = 0;
	ACK.checksum = getCheckSum(ACK);

}


/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

void init();
void generate_next_arrival();


struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };

void insertevent(struct event *);

struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */ 
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */   
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

int main()
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;
   
   int i,j;
   char c; 

  
   init();
   A_init();
   B_init();
   
   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */    
            j = nsim % 26; 
            for (i=0; i<20; i++)  
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++) 
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A) 
               A_output(msg2give);  
             else
               B_output(msg2give);  
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)  
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A) 
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
   printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
}



void init()                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();

  
   printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:"); 
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");   
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   srand(9999);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(1);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time=0.0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
  double mmm = RAND_MAX;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                    
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
}  

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
 
void generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
 
   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} 


void insertevent(p)
   struct event *p;
{
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime); 
      }
   q = evlist;     /* q points to front of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q; 
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}

void printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(AorB)
int AorB;  /* A or B is trying to stop timer */
{
 struct event *q,*qold;

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
             evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
             q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(AorB,increment)
int AorB;  /* A or B is trying to stop timer */
float increment;
{

 struct event *q;
 struct event *evptr;


 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)  
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }
 
/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   
 
   evptr->eventity = AorB;
   insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
void tolayer3(AorB,packet)
int AorB;  /* A or B is trying to stop timer */
struct pkt packet;
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
 float lastime, x, jimsrand();
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)    
	printf("          TOLAYER3: packet being lost\n");
      return;
    }  

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */ 
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = time;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) ) 
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();
 


 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)    
	printf("          TOLAYER3: packet being corrupted\n");
    }  

  if (TRACE>2)  
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
} 

void tolayer5(AorB,datasent)
  int AorB;
  char datasent[20];
{
  int i;  
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)  
        printf("%c",datasent[i]);
     printf("\n");
   }
  
}
