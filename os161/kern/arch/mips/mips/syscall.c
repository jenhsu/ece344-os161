#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>
#include <uio.h>
#include <vnode.h>
#include <vfs.h>
#include <curthread.h>
#include <thread.h>
#include <synch.h>
#include <addrspace.h>
#include <kern/limits.h>
#include <clock.h>


/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */


void
mips_syscall(struct trapframe *tf)
{
	int callno;
	int32_t retval;
	int err;

	assert(curspl==0);

	callno = tf->tf_v0;

	/*
	 * Initialize retval to 0. Many of the system calls don't
	 * really return a value, just 0 for success and -1 on
	 * error. Since retval is the value returned on success,
	 * initialize it to 0 by default; thus it's not necessary to
	 * deal with it except for calls that return other values, 
	 * like write.
	 */

	retval = 0;

	switch (callno) {
	    case SYS_reboot:
	    	err = sys_reboot(tf->tf_a0);
	    	break;

	    case SYS_write:
	    	err = sys_write(tf->tf_a0, (userptr_t)tf->tf_a1, tf->tf_a2,  &retval);
	    	break;

	    case SYS_read:
	    	err = sys_read(tf->tf_a0, (char*)tf->tf_a1, tf->tf_a2, &retval);
	    	break;

	    case SYS_fork:
	    	err= sys_fork(tf, &retval);
	    	break;

		case SYS_getpid:
			retval = sys_getpid();
			err = 0;
			break;

		case SYS__exit:
			sys_exit(tf->tf_a0);
			break;

		case SYS_waitpid:
			err = sys_waitpid((int)tf->tf_a0, (int *)tf->tf_a1, tf->tf_a2, &retval);
			break;

		case SYS_execv:
			err = sys_execv((char*)tf->tf_a0, (char**)tf->tf_a1);
			break;

		case SYS___time:
			err = sys_time((userptr_t)tf->tf_a0, (userptr_t)tf->tf_a1, &retval);
			break;

		case SYS_sbrk:
			err = sys_sbrk((intptr_t) tf->tf_a0, &retval);
			break;

	    /* Add stuff here */

	    default:
			kprintf("Unknown syscall %d\n", callno);
			err = ENOSYS;
		break;
	}


	if (err) {
		/*
		 * Return the error code. This gets converted at
		 * userlevel to a return value of -1 and the error
		 * code in errno.
		 */
		tf->tf_v0 = err;
		tf->tf_a3 = 1;      /* signal an error */
	}
	else {
		/* Success. */
		tf->tf_v0 = retval;
		tf->tf_a3 = 0;      /* signal no error */
	}
	
	/*
	 * Now, advance the program counter, to avoid restarting
	 * the syscall over and over again.
	 */
	
	tf->tf_epc += 4;

	/* Make sure the syscall code didn't forget to lower spl */
	assert(curspl==0);
}

void
md_forkentry(void *data1, unsigned long data2)
{
	struct trapframe tf;
	struct trapframe *tf_copy = (struct trapframe *)data1;
	struct addrspace *as_copy = (struct addrspace *)data2;

	//copy the parent's tf onto the stack of the child
	memcpy(&tf,tf_copy,sizeof(struct trapframe));
	kfree(data1);

	//set return value to 0 for child, indicate system call is successful, increment the PC
	tf.tf_v0 = 0;
	tf.tf_a3 = 0;
	tf.tf_epc += 4;

	curthread->t_vmspace = as_copy;

	as_activate(curthread->t_vmspace);

	mips_usermode(&tf);
}

int sys_fork(struct trapframe *tf, int* ret)
{

	int result;
	struct addrspace *addrcopy;
	struct thread *child_thread = NULL;

	//copy parent's trapframe to pass to child
	struct trapframe *tf_copy = kmalloc(sizeof(struct trapframe));

	if (tf_copy == NULL) {
		return ENOMEM;
	}

	memcpy(tf_copy,tf,sizeof(struct trapframe));


	//copy parent's address space to pass to child
	result = as_copy(curthread->t_vmspace, &addrcopy);

	if (result) {
		kfree(tf_copy);
		//return ENOMEM
		return result;
	}

	as_activate(curthread->t_vmspace);

	//kprintf("in fork parent pid:%d\n",curthread->pid);

	result=thread_fork(curthread->t_name, (void*)tf_copy,(unsigned long)addrcopy, md_forkentry, &child_thread);

	if(result)
	{
		kfree(tf_copy);
		return result;
	}

	//Return child's PID to parent
	*ret = child_thread->pid;

    return 0;
}

int sys_getpid(void)
{
    return curthread->pid;
}

int
sys_write(int fd, userptr_t buf, size_t nbytes, int *ret)
{
	(void)fd;
	if (fd <= 0 || fd == 5){
		return EBADF;
	}

	if (buf == NULL){
		return EFAULT;
	}

	char* ptr = kmalloc((nbytes+1)*sizeof(char));

	int err = copyin(buf, ptr, nbytes);
	if (err == 0){
		ptr[nbytes]= '\0';

		int i;
		for (i=0; i < nbytes; i++){
			putch(ptr[i]);
		}


		kfree(ptr);
		*ret = nbytes;
		return 0;
	}
	else{
		kfree(ptr);
		return EFAULT;
	}
}

int
sys_read(int fd, char* buf, size_t buflen, int* ret)
{
	if (fd != 0){
		return EBADF;
	}

	if(buf == NULL){
		return EFAULT;
	}

	int err = 0;
	int count = 0;

	char *msg = kmalloc((buflen+1)*sizeof(char));

	while(count < buflen){
	  msg[count]=getch();
	  count++;
	}

	err = copyout(msg, (userptr_t)buf, count+1);

	if(err != 0){
		kfree(msg);
		return EFAULT;
	}

	kfree(msg);
	*ret = count+1;
	return 0;
}

pid_t sys_waitpid(pid_t pid, int *status, int options, int* ret)
{
	P(lock);
//	int s = splhigh();
//	kprintf("%d waiting for %d\n", curthread->pid, pid);

	struct thread *child;

	//check negative pid
	if (pid <= 0){
		V(lock);
		return EINVAL;
	}

	//check is options equal 0
	if (options != 0){
		V(lock);
		return EINVAL;
	}

	//check max pid
	if (pid > 20000){
		V(lock);
		return EINVAL;
	}

	//check if pid is self
	if (pid == curthread->pid){
		V(lock);
		return EINVAL;
	}

	//check if pid is parent
	if (pid == curthread->parent){
		V(lock);
		return EINVAL;
	}

	//check if pid process is your child
	if (curthread->pid != all_thread[pid]->parent) {
		V(lock);
		return EINVAL;
	}

	//check if status is valid
	if (status == NULL || status == 0x40000000){
		V(lock);
		return EFAULT;
	}

	if (status == 0x80000000){
		V(lock);
		return EFAULT;
	}

	//status is not aligned
	if ((unsigned int)status & 34 != 0){
		V(lock);
		return EFAULT;
	}

	// check if the parent has waited on children
	if (all_thread[pid]->haswaiting == 1){
		V(lock);
//		kprintf("\nwaitpid lock sem count %d\n", lock->count);
		return EINVAL;
	}

	//set child to have waiting parent
	all_thread[pid]->haswaiting = 1;

//	kprintf("\npid is %d has exited %d\n", pid, all_thread[pid]->exited);

	//check if child has exited already
	if (all_thread[pid]->exited == 1) {
		*status = all_thread[pid]->exitcode;
		*ret = pid;
		V(lock);
//		splx(s);
		return 0;
	}
	//child has not exited, wait on child semaphore
	else if (all_thread[pid]->exited == 0) {
		child = all_thread[pid]->thread;

		V(lock);
//		kprintf("parent %d waiting on %d sem\n", curthread->pid, child->pid);
//		kprintf("wait child %d sem count is %d\n", pid, child->wait->count);

		P(child->wait);

		//return exitcode
		*status = all_thread[pid]->exitcode;
		*ret = pid;
//		splx(s);
//		V(lock);
		return 0;
	}
}

void sys_exit(int exitcode)
{
	//set the exitcode
	all_thread[curthread->pid]-> exitcode = exitcode;
	all_thread[curthread->pid]-> exited = 1;

	thread_exit();
	return;
}

int sys_execv(const char *program, char **args)
{
//	if (lock == NULL) lock = sem_create("lock", 1);
//	P(lock);

	// Copy the parameters of the function
	struct vnode *v;
	struct addresspace* as;
	vaddr_t entrypoint,stackptr;
	int result;

	if(program == NULL || program == 0x80000000 || program == 0x40000000)
		return EFAULT;

	if(program[0] == '\0')
		return EINVAL;

	if(args == NULL || args == 0x80000000 || args == 0x40000000)
		return EFAULT;

	char kprogram[PATH_MAX];

	result = copyinstr(program, kprogram, PATH_MAX, NULL);
//	kprintf("kprogram %s\n",kprogram);
	if(result){
		return result;
	}

	char** kargs = (char **)kmalloc(sizeof(char*));
		if(kargs == NULL){
			kfree(kargs);
			return ENOMEM;
		}
	int i = 0;

	while(args[i] != NULL)
	    i++;
	int argc = i;

	for (i = 0; i < argc; i++){
		if(args[i] == 0x80000000 || args[i] == 0x40000000)
			return EFAULT;
	}

	int karglength[argc];

	for (i = 0; i < argc; i++){
		int length = strlen(args[i]);
		length = length + 1;
		karglength[i] = length;
		kargs[i] = (char*)kmalloc(length*sizeof(char));
		if(kargs[i] == NULL){
			kfree(kargs[i]);
			kfree(kargs);
			return result;
		}

		result = copyinstr((userptr_t)args[i], kargs[i], length, NULL);
		//kprintf("kargs[%d] is %s addr %p\n", i, kargs[i], &kargs[i]);
		if (result){
			kfree(kargs[i]);
			kfree(kargs);
			return result;
		}
	}

	kargs[argc] = NULL;


	//Destroy the addrspace of the current thread
	as_destroy(curthread->t_vmspace);

	result = vfs_open(kprogram, O_RDONLY, &v);
	if (result) {

		return result;

	}
	curthread->t_vmspace=as_create();
	if (curthread->t_vmspace==NULL){
		vfs_close(v);
		return ENOMEM;

	}
	//* Activate it.
	as_activate(curthread->t_vmspace);

	// Load the executable.
	result = load_elf(v, &entrypoint);
	if (result) {
		curthread->t_vmspace = as;
		as_activate(as);
		vfs_close(v);
		return result;

	}

	// Done with the file now.
	vfs_close(v);

	// Define the user stack in the address space
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		//* thread_exit destroys curthread->t_vmspace
		//free the memory
		for(i = argc-1; i>=0 ; i--){
			kfree(kargs[i]);
		}
		kfree(kargs);
		return result;
	}

	//Load arguments into the stack
	int spaddr[argc];
	int shift;

	for(i = argc-1; i>=0 ; i--){
//		kprintf("length of %d is %d \n", i, karglength[i]);
		int length = karglength[i];
		shift = 4-(length%4);
		length = length + shift;
		stackptr = stackptr - length;
//		kprintf("kargs[%d] is %s\n", i, kargs[i]);

		//copy kernel arguments to stack
		result=copyoutstr(kargs[i], stackptr, length+1, NULL);
		spaddr[i] = stackptr;
		if(result){
//			kprintf("in error\n");
			//free the memory
			for(i = argc-1; i>=0 ; i--){
				kfree(kargs[i]);
			}
			kfree(kargs);

			return result;
		}
	}

	//point the stackptr to a NULL character
	stackptr = stackptr - 4;
	copyout('\0', stackptr, 4);

	//copy stackptr address to stack
	for(i=argc-1; i>=0 ;i--){
		stackptr = stackptr - 4;
		copyout(spaddr+i, stackptr, 4);
	}

	//free the memory
	for(i = argc-1; i>=0 ; i--){
		kfree(kargs[i]);
	}
	kfree(kargs);

	/* Warp to user mode. */
	md_usermode(argc /*argc*/, (userptr_t )stackptr /*userspace addr of argv*/, stackptr, entrypoint);

	panic("md_usermode returned\n");

	return EINVAL;
}

int sys_time(userptr_t seconds_ptr, userptr_t nanoseconds_ptr, int* ret)
{
	time_t seconds;
	u_int32_t nanoseconds;
	int result;

	gettime(&seconds, &nanoseconds);

	//if seconds pointer is NULL, save nanoseconds, return seconds
	if (seconds_ptr == NULL){
		result = copyout(&nanoseconds, nanoseconds_ptr, sizeof(u_int32_t));
		if (result) {
			return result;
		}
		*ret = seconds;
		return 0;
	}

	//if nanoseconds pointer is NULL, save seconds, return seconds
	if (nanoseconds_ptr == NULL){
		result = copyout(&seconds, seconds_ptr, sizeof(time_t));
		if (result) {
			return result;
		}
		*ret = seconds;
		return 0;
	}

	//invalid non-NULL pointer
	if (seconds_ptr == 0)
		return EFAULT;


	//invalid non-NULL pointer
	if (nanoseconds_ptr == 0)
		return EFAULT;

	//save seconds to seconds pointer
	result = copyout(&seconds, seconds_ptr, sizeof(time_t));
	if (result) {
		return result;
	}

	//save nanoseconds to nanoseconds pointer
	result = copyout(&nanoseconds, nanoseconds_ptr, sizeof(u_int32_t));
	if (result) {
		return result;
	}

	//return seconds
	*ret = seconds;
	return 0;
}

int sys_sbrk(intptr_t amount, int* retval){
	struct addrspace * as;
	as = curthread->t_vmspace;

	if(amount == 0){
		*retval = as->heapend;
		return 0;
	}

	//check for alignment
	if (amount%4){
		*retval = -1;
		return EINVAL;
	}

	if (amount < 0){
		// negative amount shouldn't be lager than one page
		if (amount < -PAGE_SIZE){
			*retval = -1;
			return EINVAL;
		}
		//heapend should be smaller than heapstart
		else if (as->heapend + amount < as->heapstart){
			*retval = -1;
			return EINVAL;
		}
	}

	if (amount >= 24* PAGE_SIZE){
		*retval = -1;
		return ENOMEM;
	}

	//return previous heapend
	*retval = as->heapend;


	//update heapend
	as->heapend = as->heapstart + amount;

	return 0;

}
