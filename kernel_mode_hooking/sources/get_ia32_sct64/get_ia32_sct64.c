#include <linux/module.h>

struct idt_descriptor {
    unsigned short offset_low;
    unsigned short selector;
    unsigned char zero1;
    unsigned char type_flags;
    unsigned short offset_middle;
    unsigned int offset_high;
    unsigned int zero2;
} __attribute__ ((packed));

struct idtr {
    unsigned short limit;
    void *base;
} __attribute__ ((packed));


void *get_ia32_sys_call_table(void) {
    struct idtr idtr;
    struct idt_descriptor idtd;
    void *ia32_syscall;
    unsigned char *ptr;
    int i;

    asm volatile("sidt %0" : "=m"(idtr));

    memcpy(&idtd, idtr.base + 0x80*sizeof(idtd), sizeof(idtd));

    ia32_syscall = (void*)(((long)idtd.offset_high<<32) | (idtd.offset_middle<<16) | idtd.offset_low);

    printk(KERN_INFO "ia32_syscall: 0x%p", ia32_syscall);

    for (ptr=ia32_syscall, i=0; i<500; i++) {
        if (ptr[0] == 0xff && ptr[1] == 0x14 && ptr[2] == 0xc5)
            return (void*) (0xffffffff00000000 | *((unsigned int*)(ptr+3)));
        ptr++;
    }

    return NULL;
}

static int __init sct_init(void) {
    printk(KERN_INFO "ia32_sys_call_table: 0x%p", get_ia32_sys_call_table());
    return 0;
}

static void __exit sct_exit(void) {
}

module_init(sct_init);
module_exit(sct_exit);
MODULE_LICENSE("GPL");
