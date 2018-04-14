/*
 * catlock.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use LOCKS/CV'S to solve the cat syncronization problem in 
 * this file.
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

/*
 * 
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2

#define NLOOP 4
/*
 * 
 * Function Definitions
 * 
 */


struct lock *eat;
struct lock *bowl1;
struct lock *bowl2;
struct cv *mousewait;
struct cv *catwait;
int numcat = 0;
int nummouse = 0;
//int bowl1 = 0;
//int bowl2 = 0;

/* who should be "cat" or "mouse" */
static void lock_eat(const char *who, int num, int bowl, int iteration) {
	kprintf("%s: %d starts eating: bowl %d, iteration %d\n", who, num, bowl,
			iteration);
	clocksleep(1);
	kprintf("%s: %d ends eating: bowl %d, iteration %d\n", who, num, bowl,
			iteration);
}

/*
 * catlock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS -
 *      1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void catlock(void * unusedpointer, unsigned long catnumber) {

	(void) unusedpointer;

	int i;

	for (i = 0; i < NLOOP;) {
		lock_acquire(eat);
		while (nummouse>0 || (bowl1->held==1 && bowl2->held==1)) {
			cv_wait(catwait, eat);
		}

		//if(bowl1==1 && bowl2==1){
		//	cv_wait(catwait, eat);
		//}

		numcat++;

		if (bowl1->held==0) {
			lock_acquire(bowl1);
			lock_release(eat);

			lock_eat("cat", catnumber, 1, i);

			lock_acquire(eat);
			lock_release(bowl1);
			numcat--;
			//kprintf("numcat in cat bowl1: %d\n", numcat);
			if (numcat == 0){
				cv_broadcast(mousewait, eat);
			//	cv_signal(mousewait, eat);
			}
			else{
				cv_signal(catwait, eat);
				//kprintf("numcat in cat bowl1: %d another cat\n", numcat);

			}
			lock_release(eat);

		} else if (bowl2->held==0) {
			lock_acquire(bowl2);
			lock_release(eat);

			lock_eat("cat", catnumber, 2, i);

			lock_acquire(eat);
			lock_release(bowl2);
			numcat--;
			//kprintf("numcat in cat bowl2: %d\n", numcat);
			if (numcat == 0){
				cv_broadcast(mousewait, eat);
				//cv_signal(mousewait, eat);
			}
			else{
				cv_signal(catwait, eat);
				//kprintf("numcat in cat bowl2: %d another cat\n", numcat);
			}
			lock_release(eat);
		}
		i++;
	}

}

/*
 * mouselock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void mouselock(void * unusedpointer, unsigned long mousenumber) {
	(void) unusedpointer;

	int i;

	for (i = 0; i < NLOOP;) {
		lock_acquire(eat);
		//kprintf("numcat in mouse: %d\n", numcat);
		while (numcat>0) {
			cv_wait(mousewait, eat);
			//kprintf("numcat in mouse: %d\n", numcat);
		}
		nummouse++;
		if (bowl1->held==0) {
			lock_acquire(bowl1);
			lock_release(eat);

			lock_eat("mouse", mousenumber, 1, i);

			lock_acquire(eat);
			lock_release(bowl1);
			if (nummouse == 0){
				cv_broadcast(catwait, eat);
				//cv_signal(catwait, eat);
				//cv_signal(catwait, eat);
			}
			else
				cv_signal(mousewait, eat);
			lock_release(eat);

		} else if (bowl2->held==0) {
			lock_acquire(bowl2);
			lock_release(eat);

			lock_eat("mouse", mousenumber, 2, i);

			lock_acquire(eat);
			lock_release(bowl2);
			nummouse--;
			if (nummouse == 0){
				cv_broadcast(catwait, eat);
				//cv_signal(catwait, eat);
				//cv_signal(catwait, eat);
			}
			else
				cv_signal(mousewait, eat);
			lock_release(eat);
		}
	i++;	
	}
}

/*
 * cmlock()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catlock() and mouselock() threads.  Change
 *      this code as necessary for your solution.
 */

int catmouselock(int nargs, char ** args) {
	int index, error;

	/*
	 * Avoid unused variable warnings.
	 */

	(void) nargs;
	 (void) args;
	eat = lock_create("eat");
	bowl1 = lock_create("bowl1");
	bowl2 = lock_create("bowl2");
	mousewait = cv_create("mousewait");
	catwait = cv_create("catwait");


	/*
	 * Start NCATS catlock() threads.
	 */

	for (index = 0; index < NCATS; index++) {

		error = thread_fork("catlock thread",
		NULL, index, catlock,
		NULL);

		/*
		 * panic() on error.
		 */

		if (error) {

			panic("catlock: thread_fork failed: %s\n", strerror(error));
		}
	}

	/*
	 * Start NMICE mouselock() threads.
	 */

	for (index = 0; index < NMICE; index++) {

		error = thread_fork("mouselock thread",
		NULL, index, mouselock,
		NULL);

		/*
		 * panic() on error.
		 */

		if (error) {

			panic("mouselock: thread_fork failed: %s\n", strerror(error));
		}
	}

	return 0;
}

/*
 * End of catlock.c
 */
