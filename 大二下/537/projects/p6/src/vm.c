#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"

extern char data[];  // defined by kernel.ld
pde_t *kpgdir;  // for use in scheduler()

pte_t *walkpgdir(pde_t *pgdir, const void *va, int alloc);

// Removing a page forcefully is tricky because you need to
// shift things around.
// This happens at page deallocation.
int
clk_remove(char *virtual_address)
{
    struct proc *curproc = myproc();
    int prev_tail = curproc->clk_hand;
    int match_idx = -1;

    // Search for the matching element.
    for (int i = 0; i < CLOCKSIZE; ++i) {
        int idx = (curproc->clk_hand + i) % CLOCKSIZE;
        if (curproc->clk_queue[idx] == virtual_address) {
            match_idx = idx;
            break;
        }
    }

    if (match_idx == -1)
        return -1;

    // Shift everything from match_idx+1 to prev_tail to
    // one slot to the left.
    for (int idx = match_idx;
         idx != prev_tail;
         idx = (idx + 1) % CLOCKSIZE) {
        int next_idx = (idx + 1) % CLOCKSIZE;
        curproc->clk_queue[idx] = curproc->clk_queue[next_idx];
    }

    // Clear the element at prev_tail. Set clk_hand to
    // one entry to the left.
    curproc->clk_queue[prev_tail] = (char*)-1;
    // CHECKING: HOW THIS WORKS?
    curproc->clk_hand = curproc->clk_hand == 0 ? CLOCKSIZE - 1
                                               : curproc->clk_hand - 1;

    return 0;
}

// Initialize the clock queue to an empty state.
void
clk_clear(void)
{
    struct proc *curproc = myproc();
    for (int i = 0; i < CLOCKSIZE; ++i)
        curproc->clk_queue[i] = (char*)-1;
    curproc->clk_hand = -1;
}

// Print the clock queue in head->tail orderr, so starting
// from hand+1.
void
clk_print(void)
{
    struct proc *curproc = myproc();
    
    int print_idx = curproc->clk_hand;
    cprintf("CLK queue: | ");
    for (int i = 0; i < CLOCKSIZE; ++i) {
        print_idx = (print_idx + 1) % CLOCKSIZE;
        pte_t * pte = walkpgdir(curproc->pgdir, curproc->clk_queue[print_idx], 0);
        if (curproc->clk_queue[print_idx] != (char*)-1) {
            cprintf("\nVirtual Address %x Encrypted %d Referenced %d| ",
                curproc->clk_queue[print_idx], ((*pte) & PTE_E) > 0,
                ((*pte) & PTE_A) > 0);
        }
    }
    cprintf("\n\n");
}

// Traverse through the queue to see if there's a match of virtual address
// Return the index if there's such a value, return -1 if there's not.
int
clk_search(char *virtual_address)
{
    struct proc *curproc = myproc();
    // Search for the matching element.
    for (int i = 0; i < CLOCKSIZE; ++i) {
        if (curproc->clk_queue[i] != (char*)-1) {
          if (curproc->clk_queue[i] == virtual_address) {
              return i;
          }
        }
    }
    return -1;
}

// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void
seginit(void)
{
  struct cpu *c;

  // Map "logical" addresses to virtual addresses using identity map.
  // Cannot share a CODE descriptor for both kernel and user
  // because it would have to have DPL_USR, but the CPU forbids
  // an interrupt from CPL=0 to DPL=3.
  c = &cpus[cpuid()];
  c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
  c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
  c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
  c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
  lgdt(c->gdt, sizeof(c->gdt));
}

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){//No need to check PTE_E here.
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
  } else {
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table
    // entries, if necessary.
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
static int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
  char *a, *last;
  pte_t *pte;

  a = (char*)PGROUNDDOWN((uint)va);
  last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
  for(;;){
    if((pte = walkpgdir(pgdir, a, 1)) == 0)
      return -1;
    if(*pte & (PTE_P | PTE_E))
      panic("remap");
    
    //"perm" is just the lower 12 bits of the PTE
    //if encrypted, then ensure that PTE_P is not set
    //This is somewhat redundant. If our code is correct,
    //we should just be able to say pa | perm
    if (perm & PTE_E)
      *pte = (pa | perm | PTE_E) & ~PTE_P;
    else
      *pte = pa | perm | PTE_P;


    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// There is one page table per process, plus one that's used when
// a CPU is not running any process (kpgdir). The kernel uses the
// current process's page table during system calls and interrupts;
// page protection bits prevent user code from using the kernel's
// mappings.
//
// setupkvm() and exec() set up every page table like this:
//
//   0..KERNBASE: user memory (text+data+stack+heap), mapped to
//                phys memory allocated by the kernel
//   KERNBASE..KERNBASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
//   KERNBASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
//                for the kernel's instructions and r/o data
//   data..KERNBASE+PHYSTOP: mapped to V2P(data)..PHYSTOP,
//                                  rw data + free physical memory
//   0xfe000000..0: mapped direct (devices such as ioapic)
//
// The kernel allocates physical memory for its heap and for user memory
// between V2P(end) and the end of physical memory (PHYSTOP)
// (directly addressable from end..P2V(PHYSTOP)).

// This table defines the kernel's mappings, which are present in
// every process's page table.
static struct kmap {
  void *virt;
  uint phys_start;
  uint phys_end;
  int perm;
} kmap[] = {
 { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
 { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
 { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
 { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
};

// Set up kernel part of a page table.
pde_t*
setupkvm(void)
{
  pde_t *pgdir;
  struct kmap *k;

  if((pgdir = (pde_t*)kalloc()) == 0)
    return 0;
  memset(pgdir, 0, PGSIZE);
  if (P2V(PHYSTOP) > (void*)DEVSPACE)
    panic("PHYSTOP too high");
  for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
    if(mappages(pgdir, k->virt, k->phys_end - k->phys_start,
                (uint)k->phys_start, k->perm) < 0) {
      freevm(pgdir);
      return 0;
    }
  return pgdir;
}

// Allocate one page table for the machine for the kernel address
// space for scheduler processes.
void
kvmalloc(void)
{
  kpgdir = setupkvm();
  switchkvm();
}

// Switch h/w page table register to the kernel-only page table,
// for when no process is running.
void
switchkvm(void)
{
  lcr3(V2P(kpgdir));   // switch to the kernel page table
}

// Switch TSS and h/w page table to correspond to process p.
void
switchuvm(struct proc *p)
{
  if(p == 0)
    panic("switchuvm: no process");
  if(p->kstack == 0)
    panic("switchuvm: no kstack");
  if(p->pgdir == 0)
    panic("switchuvm: no pgdir");

  pushcli();
  mycpu()->gdt[SEG_TSS] = SEG16(STS_T32A, &mycpu()->ts,
                                sizeof(mycpu()->ts)-1, 0);
  mycpu()->gdt[SEG_TSS].s = 0;
  mycpu()->ts.ss0 = SEG_KDATA << 3;
  mycpu()->ts.esp0 = (uint)p->kstack + KSTACKSIZE;
  // setting IOPL=0 in eflags *and* iomb beyond the tss segment limit
  // forbids I/O instructions (e.g., inb and outb) from user space
  mycpu()->ts.iomb = (ushort) 0xFFFF;
  ltr(SEG_TSS << 3);
  lcr3(V2P(p->pgdir));  // switch to process's address space
  popcli();
}

// Load the initcode into address 0 of pgdir.
// sz must be less than a page.
void
inituvm(pde_t *pgdir, char *init, uint sz)
{
  char *mem;

  if(sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W|PTE_U);
  memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int
loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
{
  uint i, pa, n;
  pte_t *pte;

  if((uint) addr % PGSIZE != 0)
    panic("loaduvm: addr must be page aligned");
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, addr+i, 0)) == 0)
      panic("loaduvm: address should exist");
    pa = PTE_ADDR(*pte);
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if(readi(ip, P2V(pa), offset+i, n) != n)
      return -1;
  }
  return 0;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int
allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  char *mem;
  uint a;

  if(newsz >= KERNBASE)
    return 0;
  if(newsz < oldsz)
    return oldsz;

  a = PGROUNDUP(oldsz);
  for(; a < newsz; a += PGSIZE){
    mem = kalloc();
    if(mem == 0){
      cprintf("allocuvm out of memory\n");
      deallocuvm(pgdir, newsz, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if(mappages(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
      cprintf("allocuvm out of memory (2)\n");
      deallocuvm(pgdir, newsz, oldsz);
      kfree(mem);
      return 0;
    }
  }
  return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int
deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  pte_t *pte;
  uint a, pa;

  if(newsz >= oldsz)
    return oldsz;

  a = PGROUNDUP(newsz);
  for(; a  < oldsz; a += PGSIZE){
    pte = walkpgdir(pgdir, (char*)a, 0);
    if(!pte)
      a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
    else if((*pte & (PTE_P | PTE_E)) != 0){
      pa = PTE_ADDR(*pte);
      if(pa == 0)
        panic("kfree");
      char *v = P2V(pa);
      kfree(v);
      *pte = 0;
    }
  }
  return newsz;
}

// Free a page table and all the physical memory pages
// in the user part.
void
freevm(pde_t *pgdir)
{
  uint i;

  if(pgdir == 0)
    panic("freevm: no pgdir");
  deallocuvm(pgdir, KERNBASE, 0);
  for(i = 0; i < NPDENTRIES; i++){
    //you don't need to check for PTE_E here because
    //this is a pde_t, where PTE_E doesn't get set
    if(pgdir[i] & PTE_P){
      char * v = P2V(PTE_ADDR(pgdir[i]));
      kfree(v);
    }
  }
  kfree((char*)pgdir);
}

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void
clearpteu(pde_t *pgdir, char *uva)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  if(pte == 0)
    panic("clearpteu");
  *pte &= ~PTE_U;
}

// Given a parent process's page table, create a copy
// of it for a child.
pde_t*
copyuvm(pde_t *pgdir, uint sz)
{
  pde_t *d;
  pte_t *pte;
  uint pa, i, flags;
  char *mem;

  if((d = setupkvm()) == 0)
    return 0;
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if(!(*pte & (PTE_P | PTE_E)))
      panic("copyuvm: page not present");
    pa = PTE_ADDR(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto bad;
    memmove(mem, (char*)P2V(pa), PGSIZE);
    if(mappages(d, (void*)i, PGSIZE, V2P(mem), flags) < 0) {
      kfree(mem);
      goto bad;
    }
  }
  return d;

bad:
  freevm(d);
  return 0;
}

//PAGEBREAK!
// Map user virtual address to kernel address.
// UVA -> PA
// KVA -> PA
// PA -> KVA
// KVA = PA + KERNBASE
char*
uva2ka(pde_t *pgdir, char *uva)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  //TODO: uva2ka says not present if PTE_P is 0
  if(((*pte & PTE_P) | (*pte & PTE_E)) == 0)
    return 0;
  if((*pte & PTE_U) == 0)
    return 0;
  return (char*)P2V(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int
copyout(pde_t *pgdir, uint va, void *p, uint len)
{
  char *buf, *pa0;
  uint n, va0;

  buf = (char*)p;
  while(len > 0){
    va0 = (uint)PGROUNDDOWN(va);
    //TODO: what happens if you copyout to an encrypted page?
    pa0 = uva2ka(pgdir, (char*)va0);
    if(pa0 == 0) {
      return -1;
    }
    n = PGSIZE - (va - va0);
    if(n > len)
      n = len;
    memmove(pa0 + (va - va0), buf, n);
    len -= n;
    buf += n;
    va = va0 + PGSIZE;
  }
  return 0;
}


//returns 0 on success
int mdecrypt(char *virtual_addr) {
  //cprintf("mdecrypt: VPN %d, %p, pid %d\n", PPN(virtual_addr), virtual_addr, myproc()->pid);
  //the given pointer is a virtual address in this pid's userspace
  struct proc * p = myproc();
  pde_t* mypd = p->pgdir;

  if (virtual_addr >= (char*) KERNBASE) {
    return -1;
  }
  //set the present bit to true and encrypt bit to false
  pte_t * pte = walkpgdir(mypd, virtual_addr, 0);
  if (!pte || (*pte == 0)) {
    return -1;
  }

  *pte = *pte & ~PTE_E;
  *pte = *pte | PTE_P;

  virtual_addr = (char *)PGROUNDDOWN((uint)virtual_addr);

  char * slider = virtual_addr;
  for (int offset = 0; offset < PGSIZE; offset++) {
    *slider = ~*slider;
    slider++;
  }
  
  pte_t * pte_slot;
  char* va_slot;

  for (;;) {
    // First advance the hand.
    p->clk_hand = (p->clk_hand + 1) % CLOCKSIZE;
    va_slot = (char*)PGROUNDDOWN((uint)p->clk_queue[p->clk_hand]);
    pte_slot = walkpgdir(mypd, va_slot, 0);

    // Found an empty slot.
    if (p->clk_queue[p->clk_hand] == (char*)-1) {
        p->clk_queue[p->clk_hand] = virtual_addr;
        break;
    
    // Else if the page in this slot does not have its ref
    // bit set, evict this one.
    } else if (!((*pte_slot) & PTE_A)) {
        // Encrypt the evicted page.
        if(mencrypt(p->clk_queue[p->clk_hand], 1) != 0) {
          return -1;
        }
        // Put in the new page.
        p->clk_queue[p->clk_hand] = virtual_addr;
        break;

    // Else, clear the ref bit of the page in slot.
    } else {
        *pte_slot = *pte_slot & ~PTE_A;
    }
  }

  return 0;
}

int mencrypt(char *virtual_addr, int len) {
  //the given pointer is a virtual address in this pid's userspace
  struct proc * p = myproc();
  pde_t* mypd = p->pgdir;

  virtual_addr = (char *)PGROUNDDOWN((uint)virtual_addr);

  //error checking first. all or nothing.
  char * slider = virtual_addr;
  for (int i = 0; i < len; i++) { 
    //check page table for each translation first
    char * kvp = uva2ka(mypd, slider);
    if (!kvp) {
      cprintf("mencrypt: Could not access address\n");
      return -1;
    }
    slider = slider + PGSIZE;
  }

  //encrypt stage. Have to do this before setting flag 
  //or else we'll page fault
  slider = virtual_addr;
  for (int i = 0; i < len; i++) { 
    //we get the page table entry that corresponds to this VA
    pte_t * mypte = walkpgdir(mypd, slider, 0);
    if (*mypte & PTE_E) {//already encrypted
      slider += PGSIZE;
      continue;
    }
    for (int offset = 0; offset < PGSIZE; offset++) {
      *slider = ~*slider;
      slider++;
    }
    *mypte = *mypte & ~PTE_P;
    *mypte = *mypte | PTE_E;
    *mypte = *mypte & ~PTE_A;
  }

  switchuvm(myproc());
  return 0;
}

int getpgtable(struct pt_entry* entries, int num, int wsetOnly) {
  if(wsetOnly != 0 && wsetOnly != 1) {
    return -1;
  }
  struct proc * me = myproc();

  int index = 0;
  pte_t * curr_pte;
  //reverse order

  for (void * i = (void*) PGROUNDDOWN(((int)me->sz)); i >= 0 && index < num; i-=PGSIZE) {
    //walk through the page table and read the entries
    //Those entries contain the physical page number + flags
    curr_pte = walkpgdir(me->pgdir, i, 0);

    if(wsetOnly == 1) {
      //currPage is 0 if page is not allocated
      //see deallocuvm
      if (curr_pte && *curr_pte && (clk_search(i) != -1)) {//this page is allocated
        //this is the same for all pt_entries... right?
        entries[index].pdx = PDX(i); 
        entries[index].ptx = PTX(i);
        //convert to physical addr then shift to get PPN 
        entries[index].ppage = PPN(*curr_pte);
        //have to set it like this because these are 1 bit wide fields
        entries[index].present = (*curr_pte & PTE_P) ? 1 : 0;
        entries[index].writable = (*curr_pte & PTE_W) ? 1 : 0;
        entries[index].user = (*curr_pte & PTE_U) ? 1 : 0;
        entries[index].encrypted = (*curr_pte & PTE_E) ? 1 : 0;
        entries[index].ref = (*curr_pte & PTE_A) ? 1 : 0;
        index++;
      }
    } else {
      //currPage is 0 if page is not allocated
      //see deallocuvm
      if (curr_pte && *curr_pte) {//this page is allocated
        //this is the same for all pt_entries... right?
        entries[index].pdx = PDX(i); 
        entries[index].ptx = PTX(i);
        //convert to physical addr then shift to get PPN 
        entries[index].ppage = PPN(*curr_pte);
        //have to set it like this because these are 1 bit wide fields
        entries[index].present = (*curr_pte & PTE_P) ? 1 : 0;
        entries[index].writable = (*curr_pte & PTE_W) ? 1 : 0;
        entries[index].user = (*curr_pte & PTE_U) ? 1 : 0;
        entries[index].encrypted = (*curr_pte & PTE_E) ? 1 : 0;
        entries[index].ref = (*curr_pte & PTE_A) ? 1 : 0;
        index++;
      }
    }
    if (i == 0) {
      break;
    }
  }
  //index is the number of ptes copied
  return index;
}


int dump_rawphymem(uint physical_addr, char * buffer) {
  //note that copyout converts buffer to a kva and then copies
  //which means that if buffer is encrypted, it won't trigger a decryption request
  *buffer = *buffer;
  int retval = copyout(myproc()->pgdir, (uint) buffer, (void *) P2V(physical_addr), PGSIZE);
  if (retval)
    return -1;
  return 0;
}


//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.

