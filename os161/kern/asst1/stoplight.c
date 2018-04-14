/*
 * stoplight.c
 *
 * 31-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: You can use any synchronization primitives available to solve
 * the stoplight problem in this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>
#include <machine/spl.h>
#include <curthread.h>
#include <queue.h>

/*
 *
 * Constants
 *
 */

/*
 * Number of cars created.
 */

#define NCARS 20


/*
 *
 * Function Definitions
 *
 */

static const char *directions[] = { "N", "E", "S", "W" };

static const char *msgs[] = {
        "approaching:",
        "region1:    ",
        "region2:    ",
        "region3:    ",
        "leaving:    "
};


//use in total 5 semaphores
struct semaphore *NW;      		//region NW
struct semaphore *NE;			//region NE
struct semaphore *SW;			//region SW
struct semaphore *SE;			//region SE
struct semaphore *TrafLock;		//entire traffic intersection that will allow max. 3 cars
struct semaphore *qNLock;
struct semaphore *qWLock;
struct semaphore *qELock;
struct semaphore *qSLock;
//struct semaphore *msgLock;


//use 4 queues for each direction
struct queue *qNorth;
struct queue *qEast;
struct queue *qSouth;
struct queue *qWest;




/* use these constants for the first parameter of message */
enum { APPROACHING, REGION1, REGION2, REGION3, LEAVING };

static void
message(int msg_nr, int carnumber, int cardirection, int destdirection)
{
	 	//int spl = splhigh();
		//P(msgLock);
        kprintf("%s car = %2d, direction = %s, destination = %s\n",
                msgs[msg_nr], carnumber,
                directions[cardirection], directions[destdirection]);
        //V(msgLock);
        //splx(spl);
}

/*
 * gostraight()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement passing straight through the
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
gostraight(unsigned long cardirection,
           unsigned long carnumber)
{

		/*
         * Avoid unused variable warnings.
         */

        (void) cardirection;
        (void) carnumber;

        //4 cases for cars coming from each direction
        if (cardirection == 0){			//North
        	message(APPROACHING, carnumber, cardirection, 2);
			P(NW);
			message(REGION1, carnumber, cardirection, 2);
			P(SW);
			message(REGION2, carnumber, cardirection, 2);
			V(NW);
			message(LEAVING, carnumber, cardirection, 2);
			V(SW);
        }
        else if (cardirection == 1){				//East
        	message(APPROACHING, carnumber, cardirection, 3);
			P(NE);
			message(REGION1, carnumber, cardirection, 3);
			P(NW);
			message(REGION2, carnumber, cardirection, 3);
			V(NE);
			message(LEAVING, carnumber, cardirection, 3);
			V(NW);
		}
        else if (cardirection == 2){			//South
        	message(APPROACHING, carnumber, cardirection, 0);
			P(SE);
			message(REGION1, carnumber, cardirection, 0);
			P(NE);
			message(REGION2, carnumber, cardirection, 0);
			V(SE);
			message(LEAVING, carnumber, cardirection, 0);
			V(NE);
		}
        else if (cardirection == 3){				//West
        	message(APPROACHING ,carnumber, cardirection, 1);
			P(SW);
			message(REGION1, carnumber, cardirection, 1);
			P(SE);
			message(REGION2, carnumber, cardirection, 1);
			V(SW);
			message(LEAVING, carnumber, cardirection, 1);
			V(SE);
		}
}


/*
 * turnleft()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a left turn through the
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnleft(unsigned long cardirection,
         unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) cardirection;
        (void) carnumber;


        //4 cases for cars coming from each direction
        if (cardirection == 0){		//North
        	message(APPROACHING, carnumber, cardirection, 1);
			P(NW);
			message(REGION1, carnumber, cardirection, 1);
			P(SW);
			message(REGION2, carnumber, cardirection, 1);
			V(NW);
			P(SE);
			message(REGION3, carnumber, cardirection, 1);
			V(SW);
			message(LEAVING, carnumber, cardirection, 1);
			V(SE);
        }
        else if (cardirection == 1){		//East
        	message(APPROACHING, carnumber, cardirection, 2);
			P(NE);
			message(REGION1, carnumber, cardirection, 2);
			P(NW);
			message(REGION2, carnumber, cardirection, 2);
			V(NE);
			P(SW);
			message(REGION3, carnumber, cardirection, 2);
			V(NW);
			message(LEAVING, carnumber, cardirection, 2);
			V(SW);
		}
        else if (cardirection == 2) {		//South
        	message(APPROACHING, carnumber, cardirection, 3);
			P(SE);
			message(REGION1, carnumber, cardirection, 3);
			P(NE);
			message(REGION2, carnumber, cardirection, 3);
			V(SE);
			P(NW);
			message(REGION3, carnumber, cardirection, 3);
			V(NE);
			message(LEAVING, carnumber, cardirection, 3);
			V(NW);
		}
        else if (cardirection == 3) {			//West
        	message(APPROACHING, carnumber, cardirection, 0);
			P(SW);
			message(REGION1, carnumber, cardirection, 0);
			P(SE);
			message(REGION2, carnumber, cardirection, 0);
			V(SW);
			P(NE);
			message(REGION3, carnumber, cardirection, 0);
			V(SE);
			message(LEAVING, carnumber, cardirection, 0);
			V(NE);
        }

}


/*
 * turnright()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a right turn through the
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnright(unsigned long cardirection,
          unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) cardirection;
        (void) carnumber;


        //4 cases for cars coming from each direction
        if (cardirection == 0) {		//North
        	message(APPROACHING, carnumber, cardirection, 3);
			P(NW);
			message(REGION1, carnumber, cardirection, 3);
			message(LEAVING, carnumber, cardirection, 3);
			V(NW);
        }
        else if (cardirection == 1) {		//East
        	message(APPROACHING, carnumber, cardirection, 0);
			P(NE);
			message(REGION1, carnumber, cardirection, 0);
			message(LEAVING, carnumber, cardirection, 0);
			V(NE);
        }
        else if (cardirection == 2){			//South
        	message(APPROACHING, carnumber, cardirection, 1);
			P(SE);
			message(REGION1, carnumber, cardirection, 1);
			message(LEAVING, carnumber, cardirection, 1);
			V(SE);
		}
        else if (cardirection == 3) {		//West
        	message(APPROACHING, carnumber, cardirection, 2);
			P(SW);
			message(REGION1, carnumber, cardirection, 2);
			message(LEAVING, carnumber, cardirection, 2);
			V(SW);
        }

}


/*
 * approachintersection()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Change this function as necessary to implement your solution. These
 *      threads are created by createcars().  Each one must choose a direction
 *      randomly, approach the intersection, choose a turn randomly, and then
 *      complete that turn.  The code to choose a direction randomly is
 *      provided, the rest is left to you to implement.  Making a turn
 *      or going straight should be done by calling one of the functions
 *      above.
 */

static
void
approachintersection(void * unusedpointer,
                     unsigned long carnumber)
{
        int cardirection, turndirection;
        struct queue *selfqueue;
        struct semaphore *qLock;
        /*
         * Avoid unused variable and function warnings.
         */

        (void) unusedpointer;
        (void) carnumber;
		(void) gostraight;
		(void) turnleft;
		(void) turnright;
        int spl;


        /*
         * cardirection is set randomly.
         */

        cardirection = random() % 4;

        //turndirection is set randomly to determine where the car is going
        turndirection = random() % 3;

        /*
         * 0 = go straight
         * 1 = turn left
         * 2 = turn right
         */
        if (cardirection == 0){ 		//North
        	qLock = qNLock;
        	selfqueue = qNorth;
        }
        else if (cardirection == 1){		//East
        	qLock = qELock;
        	selfqueue = qEast;
        }
        else if (cardirection == 2){		//South
        	qLock = qSLock;
        	selfqueue = qSouth;
        }
        else if (cardirection == 3){		//West
        	qLock = qWLock;
        	selfqueue = qWest;
        }

		//add current thread to queue
        P(qLock);
        q_addtail(selfqueue, &curthread);
        V(qLock);

		//sleep all the threads that is not the first thread of queue
        while (q_getguy(selfqueue, 0) != &curthread){
            spl = splhigh();
            thread_sleep(curthread);
            splx(spl);
        }

        P(TrafLock);

		if (turndirection == 0) {
			gostraight(cardirection, carnumber);
		}
		else if (turndirection == 1) {
			turnleft(cardirection, carnumber);
		}
		else if (turndirection == 2) {
			turnright(cardirection, carnumber);
		}

		P(qLock);
		q_remhead(selfqueue);
		V(qLock);

		if (!q_empty(selfqueue)){
			//kprintf("queue not empty\n");
            spl = splhigh();
            thread_wakeup(q_getguy(selfqueue, 0));
            //kprintf("thread woke up\n");
            splx(spl);
		}

        V(TrafLock);
}

/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.  You are
 *      free to modiy this code as necessary for your solution.
 */

int
createcars(int nargs,
           char ** args)
{
        int index, error;

        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;

        //create the semaphores
        NW = sem_create("NW", 1);
        NE = sem_create("NE", 1);
        SW = sem_create("SW", 1);
        SE = sem_create("SE", 1);
        //msgLock = sem_create("msgLock", 1);
        TrafLock = sem_create("TrafLock", 3);
		qNLock = sem_create("qNLock", 1);
		qELock = sem_create("qELock", 1);
		qSLock = sem_create("qSLock", 1);
		qWLock = sem_create("qWLock", 1);


		//create the queues for each direction
		qNorth = q_create(20);
		qEast = q_create(20);
		qSouth = q_create(20);
		qWest = q_create(20);


        /*
         * Start NCARS approachintersection() threads.
         */

        for (index = 0; index < NCARS; index++) {
                error = thread_fork("approachintersection thread",
                                    NULL,
                                    index,
                                    approachintersection,
                                    NULL
                                    );

                /*
                 * panic() on error.
                 */

                if (error) {

                        panic("approachintersection: thread_fork failed: %s\n",
                              strerror(error)
                              );
                }
        }
        return 0;
}
