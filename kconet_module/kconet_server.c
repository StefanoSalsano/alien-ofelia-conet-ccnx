#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>

#include <linux/errno.h>
#include <linux/types.h>

#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/in.h>

#include <linux/delay.h>
#include "kconet.h"

#define DEFAULT_PORT 9698
#define CONNECT_PORT 9698
#define MODULE_NAME "kconet_mod"


struct kthread_t{
	struct task_struct *thread;
	struct socket *sock;
	struct sockaddr_in addr;
	struct socket *sock_send;
	struct sockaddr_in addr_send;
	int running;
};

struct kthread_t *kthread = NULL;

/* function prototypes */
int ksocket_receive(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len);
int ksocket_send(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len);

static void ksocket_start(void)
{
	int size, err;
	int bufsize = INTEREST_CP_MAX_SIZE;//d
	unsigned char buf[bufsize+1];
	
	//forse possono andare dentro conet.c
	//robba che serve per lanciare la process interest
	unsigned short ll = 0;
	unsigned short c = 1; //default: cache
	unsigned short test_res;
	unsigned char CIUflags;
	char *conet_payload ;
	int conet_payload_size = 0;
	// unsigned char pppp;


/* kernel thread initialization */
// lock_kernel(); //rimossi i lock perche questi non esistono piu
	kthread->running = 1;
	current->flags |= PF_NOFREEZE;

/* daemonize (take care with signals, after daemonize() they are disabled) */
	daemonize(MODULE_NAME);
	allow_signal(SIGKILL);
// unlock_kernel();

/* create a socket */
	if ( ( (err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &kthread->sock)) < 0) ||
		( (err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &kthread->sock_send)) < 0 ))
	{
		KDEBUG("[kCONET]: Could not create a datagram socket, error = %d\n", -ENXIO);
		goto out;
	}

	memset(&kthread->addr, 0, sizeof(struct sockaddr));
	memset(&kthread->addr_send, 0, sizeof(struct sockaddr));
	kthread->addr.sin_family      = AF_INET;
	// kthread->addr_send.sin_family = AF_INET;

	kthread->addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// kthread->addr_send.sin_addr.s_addr = inet_addr(INADDR_SEND);

	kthread->addr.sin_port = htons(DEFAULT_PORT);
	// kthread->addr_send.sin_port = htons(CONNECT_PORT);

	if ( ( (err = kthread->sock->ops->bind(kthread->sock, (struct sockaddr *)&kthread->addr, sizeof(struct sockaddr) ) ) < 0))
	{
		KDEBUG("[kCONET]: Could not connect to socket, error = %d\n", -err);
		goto close_and_out;
	}

	KDEBUG("[kCONET]: listening on port %d\n", DEFAULT_PORT);

/* main loop */
	for (;;)
	{
		memset(&buf, 0, bufsize+1);
		size = ksocket_receive(kthread->sock, &kthread->addr_send, buf, bufsize);
		kthread->addr_send.sin_port = htons(CONNECT_PORT);

		if (signal_pending(current))
			break;

		if (size < 0)
			KDEBUG("[kCONET]: error getting datagram, sock_recvmsg error = %d\n", size);
		else 
		{
			KDEBUG("[kCONET]: received %d bytes\n", size);
			
			/* data processing */
			//robba che serve per lanciare la process interest
			CIUflags = buf[0]; //da rivedere con udp
			// pppp = CIUflags >> 4;
			if ((test_res = (CIUflags | 243)) == 243) //243 = 11110011, LL='00'
				ll = 0;
			else if (test_res == 251) //251 = 11111011, LL='10'
				ll = 2;
			else {
				printk("[kCONET]: Using reserved ll-flag in input message. Not yet supported\n");
			}
			if ((CIUflags | 253) == 253) //253 = 11111101, c='0' else c='1'
				c = 0;

			conet_payload = conet_process_interest_cp(/*"192.168.0.143",*/buf, &conet_payload_size,ll,c,0);
			// printk("\n data: %s\n", buf);
			KDEBUG("[kCONET]: data cp size %d\n",conet_payload_size);
			// KDEBUG("[kCONET]: arifile??? %s\n",conet_payload);

			/* sending */
			if (conet_payload > 0)
				ksocket_send(kthread->sock_send, &kthread->addr_send, conet_payload, conet_payload_size);
			kfree(conet_payload);
		}
	}

	close_and_out:
	sock_release(kthread->sock);
	sock_release(kthread->sock_send);
	kthread->sock = NULL;
	kthread->sock_send = NULL;

	out:
	kthread->thread = NULL;
	kthread->running = 0;
}

int ksocket_send(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len)
{
	struct msghdr msg;
	struct iovec iov;
	mm_segment_t oldfs;
	int size = 0;

	if (sock->sk==NULL)
		return 0;

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_flags = 0;
	msg.msg_name = addr;
	msg.msg_namelen  = sizeof(struct sockaddr_in);
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	size = sock_sendmsg(sock,&msg,len);
	set_fs(oldfs);

	return size;
}

int ksocket_receive(struct socket* sock, struct sockaddr_in* addr, unsigned char* buf, int len)
{
	struct msghdr msg;
	struct iovec iov;
	mm_segment_t oldfs;
	int size = 0;

	if (sock->sk==NULL) return 0;

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_flags = 0;
	msg.msg_name = addr;
	msg.msg_namelen  = sizeof(struct sockaddr_in);
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	size = sock_recvmsg(sock,&msg,len,msg.msg_flags);
	set_fs(oldfs);

	return size;
}

int __init ksocket_init(void) 
{
	kthread = kmalloc(sizeof(struct kthread_t), GFP_KERNEL);
	memset(kthread, 0, sizeof(struct kthread_t));

/* start kernel thread */
	kthread->thread = kthread_run((void *)ksocket_start, NULL, MODULE_NAME);
	if (IS_ERR(kthread->thread)) {
		printk( KERN_INFO "[kCONET]: unable to start kernel thread\n");
		kfree(kthread);
		kthread = NULL;
		return -ENOMEM;
	}

	return 0;

}

void __exit ksocket_exit(void) 
{
	int err;

	if (kthread->thread==NULL){
		KDEBUG("[kCONET]: no kernel thread to kill\n");
	} else {
// lock_kernel();
		err = send_sig(SIGKILL, kthread->thread, 0); //da kill_proc a kill_pid		
// unlock_kernel();

/* wait for kernel thread to die */
		if (err < 0){
			KDEBUG("[kCONET]: unknown error %d while trying to terminate kernel thread\n",-err);
		}else {
			while (kthread->running == 1)
				msleep(10);
			KDEBUG("[kCONET]: succesfully killed kernel thread!\n");
		}
	}

/* free allocated resources before exit */
	if (kthread->sock != NULL) {
		sock_release(kthread->sock);
		kthread->sock = NULL;
	}

	kfree(kthread);
	kthread = NULL;

	printk( KERN_INFO "[kCONET]: module unloaded\n");
}

/* init and cleanup functions */
module_init(ksocket_init);
module_exit(ksocket_exit);

/* module information */
MODULE_DESCRIPTION("un seplice server Conet");
MODULE_AUTHOR("whr da pezzi di codice di Alessandro Rubini e Toni Garcia-Navarro");
MODULE_LICENSE("GPL");