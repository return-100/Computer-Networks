#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <vector>

#define sz 20
#define BIDIRECTIONAL 0
#define window_sz 7
#define buffer_sz 50

using namespace std;

struct msg
{
    char data[20];
};

struct pkt
{
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];
};

int is_time_running;
int exp_ack_a, pkt_a;
int ack_b, exp_pkt_b;

int TRACE = 1;     /* for my debugging */
int nsim = 0;      /* number of messages from 5 to 4 so far */
int nsimmax = 0;   /* number of msgs to generate, then stop */
float TIME = 0.000;
float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped */
float lambda;      /* arrival rate of messages from layer 5 */
int ntolayer3;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

queue <pkt> buffer;
vector <pkt> window;
vector <float> pkt_timer;

void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);

int get_checksum(struct pkt packet)
{
    int ret = 0;

    for (int i = 0; i < strlen(packet.payload); ++i)
        ret += (int) packet.payload[i];

    return (ret + packet.acknum + packet.seqnum);
}

void A_init(void)
{
    is_time_running = 0;
    pkt_a = 0;
    exp_ack_a = 0;
}

void A_output(struct msg message)
{
    if (window.size() == window_sz && buffer.size() == buffer_sz)
        printf("----->>> Both window and buffer are full\n");
    else
    {
        struct pkt A_pkt;
        A_pkt.seqnum = pkt_a;
        A_pkt.acknum = pkt_a;
        strncpy(A_pkt.payload, message.data, sz);
        A_pkt.checksum = get_checksum(A_pkt);
        pkt_a = (pkt_a + 1) % (window_sz + 1);

        if (window.size() < window_sz)
        {
            window.push_back(A_pkt);

            if (!is_time_running)
                starttimer(0, 20.0), is_time_running = 1;

            tolayer3(0, A_pkt);
            pkt_timer.push_back(TIME);

            printf("----->>> (A) pushed pkt%d in the window\n", A_pkt.seqnum);
        }
        else
        {
            buffer.push(A_pkt);
            printf("----->>> (A) pushed pkt%d in the buffer\n", pkt_a);
        }
    }
}

void A_input(struct pkt packet)
{
    if (packet.checksum != get_checksum(packet))
        printf("----->>> (A) has received NAK\n");
    else
    {
        int idx = -1;

        for (int i = 0; i < window.size(); ++i)
        {
            if (window[i].seqnum == packet.acknum)
            {
                idx = i;
                break;
            }
        }

        if (idx != -1)
        {
            stoptimer(0);
            is_time_running = 0;

            for (int i = 0; i <= idx; ++i)
                printf("----->>> (A) sent pkt%d successfully\n", window[i].seqnum);

            window.erase(window.begin(), window.begin() + idx + 1);
            pkt_timer.erase(pkt_timer.begin(), pkt_timer.begin() + idx + 1);

            while (!buffer.empty() && window.size() < window_sz)
            {
                window.push_back(buffer.front());
                tolayer3(0, buffer.front());
                pkt_timer.push_back(TIME);
                buffer.pop();
            }

            if (pkt_timer.size())
                starttimer(0, max(0.0, 20.0 - TIME + pkt_timer[0])), is_time_running = 1;
        }
        else
        {
            stoptimer(0);
            is_time_running = 0;
            pkt_timer.clear();

            for (int i = 0; i < window.size(); ++i)
            {
                tolayer3(0, window[i]);
                pkt_timer.push_back(TIME);
                printf("----->>> (A) resend pkt%d\n", window[i].seqnum);
            }

            starttimer(0, 20.0);
            is_time_running = 1;
        }
    }
}

void A_timerinterrupt(void)
{
    is_time_running = 0;
    pkt_timer.clear();

    for (int i = 0; i < window.size(); ++i)
    {
        printf("----->>> (A) resend pkt%d\n", window[i].seqnum);
        tolayer3(0, window[i]), pkt_timer.push_back(TIME);
    }

    if (pkt_timer.size())
        starttimer(0, 20.0), is_time_running = 1;
}

void B_init(void)
{
    ack_b = -1;
    exp_pkt_b = 0;
}

void B_output(struct msg message)
{
    //Bonus part
}

void B_input(struct pkt packet)
{
    pkt packet_B;

    if (packet.seqnum != exp_pkt_b || packet.checksum != get_checksum(packet))
    {
        packet_B.acknum = ack_b;
        packet_B.seqnum = exp_pkt_b;
        strncpy(packet_B.payload, "NAK", sz);
        packet_B.checksum = get_checksum(packet_B);
        tolayer3(1, packet_B);
        printf("----->>> (B) has got a corrupted packet\n");
    }
    else
    {
        tolayer5(1, packet.payload);
        packet_B.seqnum = exp_pkt_b;
        packet_B.acknum = exp_pkt_b;
        strncpy(packet_B.payload, "ACK", sz);
        packet_B.checksum = get_checksum(packet_B);
        ack_b = (ack_b + 1) % (window_sz + 1);
        exp_pkt_b = (exp_pkt_b + 1) % (window_sz + 1);
        tolayer3(1, packet_B);
        printf("----->>> (B) has received pkt%d successfully\n", packet.seqnum);
    }
}

void B_timerinterrupt(void)
{
    printf("  B_timerinterrupt: B doesn't have a timer. ignore.\n");
}

/////////////////////////////////////////emulator

struct event
{
    float evtime;
    int evtype;
    int eventity;
    struct pkt *pktptr;
    struct event *prev;
    struct event *next;
};

struct event *evlist = NULL;

/* possible events: */
#define TIMER_INTERRUPT 0
#define FROM_LAYER5 1
#define FROM_LAYER3 2

#define OFF 0
#define ON 1
#define A 0
#define B 1

void init();
void generate_next_arrival(void);
void insertevent(struct event *p);

int main()
{
    freopen("input_gbn.txt", "r", stdin);
    freopen("output_gbn.doc", "w", stdout);

    struct event *eventptr;
    struct msg msg2give;
    struct pkt pkt2give;

    int i, j;
    char c;

    init();
    A_init();
    B_init();

    while (1)
    {
        eventptr = evlist; /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next; /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        if (TRACE >= 2)
        {
            printf("\nEVENT TIME: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer5 ");
            else
                printf(", fromlayer3 ");
            printf(" entity: %d\n", eventptr->eventity);
        }
        TIME = eventptr->evtime; /* update TIME to next event TIME */
        if (eventptr->evtype == FROM_LAYER5)
        {
            if (nsim < nsimmax)
            {
                if (nsim + 1 < nsimmax)
                    generate_next_arrival(); /* set up future arrival */
                /* fill in msg to give with string of same letter */
                j = nsim % 26;
                for (i = 0; i < 20; i++)
                    msg2give.data[i] = 97 + j;
                msg2give.data[19] = 0;
                if (TRACE > 2)
                {
                    printf("          MAINLOOP: data given to student: ");
                    for (i = 0; i < 20; i++)
                        printf("%c", msg2give.data[i]);
                    printf("\n");
                }
                nsim++;
                if (eventptr->eventity == A)
                    A_output(msg2give);
                else
                    B_output(msg2give);
            }
        }
        else if (eventptr->evtype == FROM_LAYER3)
        {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i = 0; i < 20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
            if (eventptr->eventity == A) /* deliver packet by calling */
                A_input(pkt2give); /* appropriate entity */
            else
                B_input(pkt2give);
            free(eventptr->pktptr); /* free the memory for packet */
        }
        else if (eventptr->evtype == TIMER_INTERRUPT)
        {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        }
        else
        {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

    terminate:
    printf(
            " Simulator terminated at TIME %f\n after sending %d msgs from layer5\n",
            TIME, nsim);
}

void init() /* initialize the simulator */
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
    printf("Enter average TIME between messages from sender's layer5 [ > 0.0]:");
    scanf("%f",&lambda);
    printf("Enter TRACE:");
    scanf("%d",&TRACE);

    srand(9999); /* init random number generator */
    sum = 0.0;   /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    if (avg < 0.25 || avg > 0.75)
    {
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(1);
    }

    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;

    TIME = 0.0;              /* initialize TIME to 0.0 */
    generate_next_arrival(); /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand(void)
{
    double mmm = RAND_MAX;
    float x;                 /* individual students may need to change mmm */
    x = rand() / mmm;        /* x should be uniform in [0,1] */
    return (x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival(void)
{
    double x, log(), ceil();
    struct event *evptr;
    float ttime;
    int tempint;

    if (TRACE > 2)
        printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
    /* having mean of lambda        */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = TIME + x;
    evptr->evtype = FROM_LAYER5;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}

void insertevent(struct event *p)
{
    struct event *q, *qold;

    if (TRACE > 2)
    {
        printf("            INSERTEVENT: TIME is %lf\n", TIME);
        printf("            INSERTEVENT: future TIME will be %lf\n", p->evtime);
    }
    q = evlist;      /* q points to header of list in which p struct inserted */
    if (q == NULL)   /* list is empty */
    {
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else
    {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL)   /* end of list */
        {
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist)     /* front of list */
        {
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else     /* middle of list */
        {
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void printevlist(void)
{
    struct event *q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next)
    {
        printf("Event TIME: %f, type: %d entity: %d\n", q->evtime, q->evtype,
               q->eventity);
    }
    printf("--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB /* A or B is trying to stop timer */)
{
    struct event *q, *qold;

    if (TRACE > 2)
        printf("          STOP TIMER: stopping timer at %f\n", TIME);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;          /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist)   /* front of list - there must be event after */
            {
                q->next->prev = NULL;
                evlist = q->next;
            }
            else     /* middle of list */
            {
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB /* A or B is trying to start timer */, float increment)
{
    struct event *q;
    struct event *evptr;

    if (TRACE > 2)
        printf("          START TIMER: starting timer at %f\n", TIME);
    /* be nice: check to see if timer is already started, if so, then  warn */
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = TIME + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}

/************************** TOLAYER3 ***************/
void tolayer3(int AorB, struct pkt packet)
{
    struct pkt *mypktptr;
    struct event *evptr, *q;
    float lastime, x;
    int i;

    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob)
    {
        nlost++;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being lost\n");
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;
    for (i = 0; i < 20; i++)
        mypktptr->payload[i] = packet.payload[i];
    if (TRACE > 2)
    {
        printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
               mypktptr->acknum, mypktptr->checksum);
        for (i = 0; i < 20; i++)
            printf("%c", mypktptr->payload[i]);
        printf("\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;      /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->pktptr = mypktptr;         /* save ptr to my copy of packet */
    /* finally, compute the arrival TIME of packet at the other end.
       medium can not reorder, so make sure packet arrives between 1 and 10
       TIME units after the latest arrival TIME of packets
       currently in the medium on their way to the destination */
    lastime = TIME;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();

    /* simulate corruption: */
    if (jimsrand() < corruptprob)
    {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr->payload[0] = 'Z'; /* corrupt payload */
        else if (x < .875)
            mypktptr->seqnum = 999999;
        else
            mypktptr->acknum = 999999;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being corrupted\n");
    }

    if (TRACE > 2)
        printf("          TOLAYER3: scheduling arrival on other side\n");
    insertevent(evptr);
}

void tolayer5(int AorB, char datasent[20])
{
    int i;
    if (TRACE > 2)
    {
        printf("          TOLAYER5: data received: ");
        for (i = 0; i < 20; i++)
            printf("%c", datasent[i]);
        printf("\n");
    }
}
