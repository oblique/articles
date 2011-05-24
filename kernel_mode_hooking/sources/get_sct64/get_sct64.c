#include <linux/module.h>

#define IA32_LSTAR  0xc0000082

void *get_sys_call_table(void) {
    void *system_call;
    unsigned char *ptr;
    int i, low, high;

    asm("rdmsr" : "=a" (low), "=d" (high) : "c" (IA32_LSTAR));

    system_call = (void*)(((long)high<<32) | low);

    printk(KERN_INFO "system_call: 0x%p", system_call);

    for (ptr=system_call, i=0; i<500; i++) {
        if (ptr[0] == 0xff && ptr[1] == 0x14 && ptr[2] == 0xc5)
            return (void*)(0xffffffff00000000 | *((unsigned int*)(ptr+3)));
        ptr++;
    }

    return NULL;
}

static int __init sct_init(void) {
    printk(KERN_INFO "sys_call_table: 0x%p", get_sys_call_table());
    return 0;
}

static void __exit sct_exit(void) {
}

module_init(sct_init);
module_exit(sct_exit);
MODULE_LICENSE("GPL");
