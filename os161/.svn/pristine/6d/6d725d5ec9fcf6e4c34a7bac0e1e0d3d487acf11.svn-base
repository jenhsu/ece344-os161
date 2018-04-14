#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys_execv(const char *program, char **args);
int sys_write(int fd, userptr_t buf, size_t nbytes, int* retval);
int sys_read(int fd, char* buf, size_t buflen, int* retval);
int sys_fork(struct trapframe *tf, int* ret);
int sys_getpid(void);
pid_t sys_waitpid(pid_t pid, int *status, int options,  int* retval);
void sys_exit(int exitcode);
int sys_time(userptr_t seconds_ptr, userptr_t nanoseconds_ptr,  int* retval);
//int sys_sbrk(intptr_t amount, int* retval);


#endif /* _SYSCALL_H_ */
