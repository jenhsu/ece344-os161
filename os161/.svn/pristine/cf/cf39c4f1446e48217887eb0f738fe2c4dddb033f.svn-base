#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <uio.h>
#include <elf.h>
#include <vnode.h>
#include <kern/unistd.h>
#include <kern/stat.h>

/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 */

/* under dumbvm, always have 48k of user stack */
#define STACKPAGES    1024
#define DUMBVM_STACKPAGES    12

int numpages;
int coremapsetup = 0;
paddr_t startaddr;
int coremappage = 0;

void vm_bootstrap(void) {
	int spl = splhigh();

	u_int32_t firstpaddr;
		u_int32_t lastpaddr;

		ram_getsize(&firstpaddr, &lastpaddr);

		u_int32_t ramsize = (lastpaddr - firstpaddr);

		//total number of pages
		numpages = ramsize/(PAGE_SIZE + sizeof(struct page));

		int coremapsize;
		//get coremap size
		coremapsize = numpages * sizeof(struct page);
		//get coremap page
		coremapsize = ROUNDUP(coremapsize, PAGE_SIZE);

		startaddr = firstpaddr + coremapsize;
		coremap = PADDR_TO_KVADDR(firstpaddr);

		int i;
	//	kprintf("firstpaddr is %x\n", startaddr);
		for (i = 0; i < numpages; i++){
			coremap[i].allocated = 0;
			coremap[i].length = 0;
			coremap[i].paddr = startaddr+(i*PAGE_SIZE);
			coremap[i].vaddr = PADDR_TO_KVADDR(startaddr+(i*PAGE_SIZE));
		}
		coremapsetup = 1;


	splx(spl);
}

//find page that is not allocated yet
paddr_t find_page(unsigned long npages) {
	paddr_t paddr = 1;
	int i;
	for (i = 0; i < numpages; i++) {
//		kprintf("coremap[%d].allocated:%d\n", i, coremap[i].allocated);
		if (coremap[i].allocated == 0) {
			if (npages > 1) {
				int j, k;
				//check is space at this index is enough for all pages
				for (j = 1; j < (int) npages; j++) {
					if (coremap[i + j].allocated == 1) {
						paddr = 0;
					}
				}
				//return found page address
				if (paddr != 0){
					//page with enough mem is found
					int len = npages;
				for (k = 0; k < (int) npages; k++) {
					coremap[i + k].allocated = 1;
					coremap[i + k].length = len;
					len--;
					coremap[i + k].as = curthread->t_vmspace;
					coremap[i + k].vaddr = PADDR_TO_KVADDR(coremap[i].paddr);
				}
				paddr = coremap[i].paddr;
				return paddr;
				}
			} else if (npages == 1) {
				coremap[i].allocated = 1;
				coremap[i].length = npages;
				coremap[i].as = curthread->t_vmspace;
				coremap[i].vaddr = PADDR_TO_KVADDR(coremap[i].paddr);
				paddr = coremap[i].paddr;
				return paddr;
			}
		}
	}
	//there is no page that is not yet allocated
	return 0;
}

paddr_t getppages(unsigned long npages) {
	int spl;
	spl = splhigh();
	paddr_t addr;

	if (coremapsetup == 0) {
		addr = ram_stealmem(npages);
	} else {

		addr = find_page(npages);
	}

	splx(spl);
	return addr;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t alloc_kpages(int npages) {
	paddr_t pa;

	pa = getppages(npages);
	if (pa == 0) {
		return 0;
	}

	return PADDR_TO_KVADDR(pa);
}

void free_kpages(vaddr_t addr) {
	int spl = splhigh();

	int i;
	//search for the correct index with specified virtual address
	for (i = 0; i < numpages; i++) {
		if (addr == coremap[i].vaddr)
			break;
	}

	//index with virtual address is found
	int npages = coremap[i].length;

	//contiguous pages
	if (npages > 1) {
		int j;
		for (j = 0; j < npages; j++) {
			coremap[i + j].allocated = 0;
			coremap[i + j].length = 0;
		}
	}
	//one page
	else {
		coremap[i].allocated = 0;
		coremap[i].length = 0;
	}

	splx(spl);
	return;
}


int vm_fault(int faulttype, vaddr_t faultaddress) {
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
	u_int32_t ehi, elo;
	struct addrspace *as;
	int spl;

	spl = splhigh();

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		panic("dumbvm: got VM_FAULT_READONLY\n");
	case VM_FAULT_READ:
	case VM_FAULT_WRITE:
		break;
	default:
		splx(spl);
		return EINVAL;
	}

	as = curthread->t_vmspace;
	if (as == NULL) {

		kprintf("vm_fault_asNULL\n");
		/*
		 * No address space set up. This is probably a kernel
		 * fault early in boot. Return EFAULT so as to panic
		 * instead of getting into an infinite faulting loop.
		 */
		return EFAULT;
	}


	/* Assert that the address space has been set up properly. */
	assert(as->as_vbase1 != 0);
	assert(as->as_npages1 != 0);
	assert(as->as_vbase2 != 0);
	assert(as->as_npages2 != 0);
	assert((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	assert((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	assert((as->as_stackvbase & PAGE_FRAME) == as->as_stackvbase);

	vaddr_t heapbase, heaptop;
	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;
	heapbase = as->heapstart;
	heaptop = as->heapend;

	int page_table_directory_index = 0;
	int page_table_index = 0;
	page_table_directory_index = faultaddress & 0xffc00000;
	page_table_directory_index = faultaddress >> 22;
	page_table_index = faultaddress & 0x003ff000;
	page_table_index = page_table_index >> 12;

	int findptable = 0;

	if (as->page_table_directory[page_table_directory_index].page_table != NULL) {
		if (as->page_table_directory[page_table_directory_index].page_table[page_table_index].ifvalid == 1) {
			paddr = as->page_table_directory[page_table_directory_index].page_table[page_table_index].paddr;
			findptable = 1;
		}
	} else {
		as->page_table_directory[page_table_directory_index].page_table = kmalloc(sizeof(struct page_table_entry) * PAGE_TABLE_SIZE);
		if(as->page_table_directory[page_table_directory_index].page_table == NULL)
			return ENOMEM;
		int j;
//		kprintf("in create page table dindex:%d, index:%d\n",page_table_directory_index,page_table_index);
		for (j = 0; j < PAGE_TABLE_SIZE; j++) {
			as->page_table_directory[page_table_directory_index].page_table[j].paddr = 0;
			as->page_table_directory[page_table_directory_index].page_table[j].ifvalid = 0;
		}
	}


if(!findptable){
	//if in text
	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		paddr = get_one_page(faultaddress);
//		kprintf("paddr in text is %x\n",paddr);
		as->page_table_directory[page_table_directory_index].page_table[page_table_index].paddr = paddr;
		as->page_table_directory[page_table_directory_index].page_table[page_table_index].ifvalid = 1;
	}
	//if in data
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		paddr = get_one_page(faultaddress);
		as->page_table_directory[page_table_directory_index].page_table[page_table_index].paddr = paddr;
		as->page_table_directory[page_table_directory_index].page_table[page_table_index].ifvalid = 1;
	}
	//if in stack
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		paddr = get_one_page(faultaddress);
		as->page_table_directory[page_table_directory_index].page_table[page_table_index].paddr = paddr;
		as->page_table_directory[page_table_directory_index].page_table[page_table_index].ifvalid = 1;
		as->as_stackvbase = as->as_stackvbase - PAGE_SIZE;
	}
	//if in heap
	else if (faultaddress >= heapbase && faultaddress < heaptop) {
		paddr = get_one_page(faultaddress);
		as->page_table_directory[page_table_directory_index].page_table[page_table_index].paddr = paddr;
		as->page_table_directory[page_table_directory_index].page_table[page_table_index].ifvalid = 1;
	} else {

		splx(spl);
		return EFAULT;
	}
}
//	TLB_load:


//	/* make sure it's page-aligned */
	assert((paddr & PAGE_FRAME)==paddr);

	for (i = 0; i < NUM_TLB; i++) {
		TLB_Read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		TLB_Write(ehi, elo, i);
		splx(spl);
		return 0;
	}

	kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	splx(spl);
	return EFAULT;
}

paddr_t get_one_page(vaddr_t vaddr) {
	paddr_t paddr;


	int i;
	for (i = coremappage; i < (int) numpages; i++) {
		if (coremap[i].allocated == 0) {
			coremap[i].allocated = 1;
			coremap[i].length = 1;
			coremap[i].as = curthread->t_vmspace;
			coremap[i].vaddr = vaddr;
			paddr = coremap[i].paddr;

			return paddr;
		}
	}
	panic("running out of free page in core map\n");

	return 0;

}

void free_coremap(struct addrspace *as) {
	int i;
	//search for the correct index with specified address space
	for (i = 0; i < numpages; i++) {
		if (coremap[i].as == as && coremap[i].allocated != 0){
			coremap[i].as = NULL;
			coremap[i].allocated = 0;
			coremap[i].length = 0;
			coremap[i].vaddr = 0;
		}
	}
}
