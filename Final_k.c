//kernel space
#ifndef MODULE
#define MODULE
#endif
#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <linux/kthread.h>	// for kthreads
#include <linux/sched.h>	// for task_struct
#include <linux/time.h>		// for using jiffies 
#include <linux/timer.h>

#include <linux/fs.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

#define MSG_SIZE 50
#define CDEV_NAME "Final"	// device name

static int major; 
char msg[MSG_SIZE];

static int dummy = 0;
int ret;

//GPSET,GPSEL,GPCLR for LEDs ,GPEDS for event detect,GPAREN for rising edge detect, GPPUD for button
unsigned long *ptr;
unsigned long *GPFSEL,*GPSET,*GPCLR,*GPLEV,*GPEDS,*GPHEN,*GPLEN,*GPPUD,*GPPUDCLK0;

int mydev_id;	// variable needed to identify the handler

//user space read from kernel space
static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
	ssize_t dummy = copy_to_user(buffer, msg, length);
	msg[0] = '\0';	
	return length;
}

//kernel space read from user space
static ssize_t device_write(struct file *filp, const char __user *buff, size_t len, loff_t *off)
{
	ssize_t dummy;
	
	if(len > MSG_SIZE)
		return -EINVAL;
	
	dummy = copy_from_user(msg, buff, len);	
	if(len == MSG_SIZE)
		msg[len-1] = '\0';
	else
		msg[len] = '\0';
	//receive message and change frenquency
	switch(msg[1]){
		case 'A':
                timer_interval_ns = 6e5;
                break;
        case 'B':
                timer_interval_ns = 8e5;
                break;
        case 'C':
                timer_interval_ns = 1e6;
                break;
        case 'D':
                timer_interval_ns = 1.2e6;
                break;
        case 'E':
                timer_interval_ns = 1.4e6;
                break;
        default:
                timer_interval_ns = 1e6;
	}
	msg[0] = '\0';
	
	return len;
}

// change read and write function
static struct file_operations fops = {
	.read = device_read, 
	.write = device_write,
};


static irqreturn_t switch_isr(int irq, void *dev_id)
{

	disable_irq_nosync(79);
	timer_interval_ns=1e6;
	
	// button1
	if(*GPEDS & 0x10000){
	timer_interval_ns=6e5;
	msg[0]='@';
	msg[1]='A';
	}

	// button2
	if(*GPEDS & 0x20000){
	timer_interval_ns=8e5;
	msg[0]='@';
	msg[1]='B';
	}

	// button3
	if(*GPEDS & 0x40000){
	timer_interval_ns=1e6;
	msg[0]='@';
	msg[1]='C';
	}

	// button4
	if(*GPEDS & 0x80000){
	timer_interval_ns=1.2e6;
	msg[0]='@';
	msg[1]='D';
	}

	// button5
	if(*GPEDS & 0x100000){
	timer_interval_ns=1.4e6;
	msg[0]='@';
	msg[1]='E';
	}

	*GPEDS |=0x1F0000; //clear EDS
	
	printk("Interrupt handled\n");
	printk("%s\n",msg);	
	enable_irq(79);		// re-enable interrupt
	
    return IRQ_HANDLED;
}


int init_module()
{
	ptr=(unsigned long*)ioremap(0x3F200000,4096);
	GPFSEL=ptr;
	*GPFSEL |=0x40000; //speaker
	GPSET=ptr+7;
	GPCLR=ptr+10;
	GPLEV=ptr+13;
	GPEDS=ptr+16;
	*GPEDS |= 0x1F0000; //clear all GPEDS
	GPAREN0=ptr+31;
	*GPAREN0 |= 0x1F0000;

	//change GPPUD
	GPPUD = ptr+37;
	*GPPUD |= 01;
	udelay(100);
	GPPUDCLK0 = ptr+38;
	*GPPUDCLK0 |= 0x1F0000;

    dummy = 0;
	printk("init GPIO\n");

	//enable interrupt
	dummy = request_irq(79, switch_isr, IRQF_SHARED, "Button_handler", &mydev_id);
    printk("Button Detection enabled.\n");

	//register device
	major = register_chrdev(0, CDEV_NAME, &fops);
	if (major < 0) {
     		printk("Registering the character device failed with %d\n", major);
	     	return major;
	}
	printk("Lab6_cdev_kmod example, assigned major: %d\n", major);
	printk("Create Char Device (node) with: sudo mknod /dev/%s c %d 0\n", CDEV_NAME, major);
 	return 0;


    
	return 0;
}

void cleanup_module()
{

	//free interrupt
    free_irq(79, &mydev_id);
    printk("Button Detection disabled.\n");

	//unregister
	unregister_chrdev(major, CDEV_NAME);
	printk("Char Device /dev/%s unregistered.\n", CDEV_NAME);
    
	//clear GPPUD
	*GPPUD |= 00;
	*GPPUDCLK0 |= 0x0;
	printk("GPPUD clear");
    
}
