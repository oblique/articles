#include <linux/module.h>

struct idt_descriptor {
    unsigned short offset_low;
    unsigned short selector;
    unsigned char zero;
    unsigned char type_flags;
    unsigned short offset_high;
} __attribute__ ((packed));

struct idtr {
    unsigned short limit;
    void *base;
} __attribute__ ((packed));


void *get_sys_call_table(void) {
    struct idtr idtr;
    struct idt_descriptor idtd;
    void *system_call;
    unsigned char *ptr;
    int i;

    asm volatile("sidt %0" : "=m"(idtr));

    memcpy(&idtd, idtr.base + 0x80*sizeof(idtd), sizeof(idtd));

    system_call = (void*)((idtd.offset_high<<16) | idtd.offset_low);

    printk(KERN_INFO "system_call: 0x%p", system_call);

    for (ptr=system_call, i=0; i<500; i++) {
        if (ptr[0] == 0xff && ptr[1] == 0x14 && ptr[2] == 0x85)
            return *((void**)(ptr+3));
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
