/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef CPU_TLB
/*
 * CPU TLB
 * TLB module cpu/cpu-tlb.c
 */
 
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#ifdef CPU_TLB
int tlb_change_all_page_tables_of(struct pcb_t *proc,  struct memphy_struct * mp)
{
  /* TODO update all page table directory info 
   *      in flush or wipe TLB (if needed)
   */

  return 0;
}

int tlb_flush_tlb_of(struct pcb_t *proc, struct memphy_struct * mp)
{
  /* TODO flush tlb cached*/

  return 0;
}

/*tlballoc - CPU TLB-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr, val;

  /* By default using vmaid = 0 */
  val = __alloc(proc, 0, reg_index, size, &addr);
  /* TODO update TLB CACHED frame num of the new allocated page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  int pgnum = PAGING_PGN(addr);
  int frmnum = PAGING_FPN(proc->mm->pgd[pgnum]);
  tlb_cache_write(proc->tlb, proc->pid, pgnum, frmnum);
  return val;
}

/*pgfree - CPU TLB-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlbfree_data(struct pcb_t *proc, uint32_t reg_index)
{
  __free(proc, 0, reg_index);

  /* TODO update TLB CACHED frame num of freed page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  int pgnum = PAGING_PGN(proc->mm->symrgtbl[reg_index].rg_start);
  printf("in pgnum:  %d\n",pgnum);
  tlb_cache_free(proc->tlb, proc->pid, pgnum);
  return 0;
}


/*tlbread - CPU TLB-based read a region memory
 *@proc: Process executing the instruction
 *@source: index of source register
 *@offset: source address = [source] + [offset]
 *@destination: destination storage
 */
int tlbread(struct pcb_t * proc, uint32_t source,
            uint32_t offset, 	uint32_t destination) 
{
  int val;
  BYTE data, frmnum = -1;
  if(proc->mm->symrgtbl[source].rg_start==proc->mm->symrgtbl[source].rg_end||
  proc->mm->symrgtbl[source].rg_start + offset>proc->mm->symrgtbl[source].rg_end)
  {
    printf("Segmentation fault read\n");
    return -1;
  };
	int pgnum = PAGING_PGN(proc->mm->symrgtbl[source].rg_start + offset);
  printf("pagnum nha : %d",pgnum);
  int off = PAGING_OFFST(proc->mm->symrgtbl[source].rg_start + offset);
  tlb_cache_read(proc->tlb, proc->pid, pgnum, &frmnum);
  /* TODO retrieve TLB CACHED frame num of accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  /* frmnum is return value of tlb_cache_read/write value*/
#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at read region=%d offset=%d\n", 
	         source, offset);
  else 
    printf("TLB miss at read region=%d offset=%d\n", 
	         source, offset);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif
  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  if(frmnum >= 0){
    int phyaddr = (frmnum << PAGING_ADDR_FPN_LOBIT) + off;
    MEMPHY_read(proc->mram, phyaddr, &data);
    // tlbwrite(proc,data,destination,offset);
  }
	else{
    val = __read(proc, 0, source, offset, &data);
    frmnum = PAGING_FPN(proc->mm->pgd[pgnum]);
    tlb_cache_write(proc->tlb, proc->pid, pgnum, frmnum);
    // tlbwrite(proc,data,destination,offset);
  }
  TLBMEMPHY_dump(proc->tlb);
 // destination = (uint32_t) data;
  printf("data readed : %d\n", data);
  return val;
}


int tlbwrite(struct pcb_t * proc, BYTE data,
             uint32_t destination, uint32_t offset)
{
  int val;
  BYTE frmnum = -1;
  int pgnum = PAGING_PGN(proc->mm->symrgtbl[destination].rg_start + offset);
  int off = PAGING_OFFST(proc->mm->symrgtbl[destination].rg_start + offset);
  if(proc->mm->symrgtbl[destination].rg_start==proc->mm->symrgtbl[destination].rg_end||
    proc->mm->symrgtbl[destination].rg_start + offset>proc->mm->symrgtbl[destination].rg_end)
  {
    printf("Segmentation fault write\n");
    return -1;
  };
  tlb_cache_read(proc->tlb, proc->pid, pgnum, &frmnum);
  /* TODO retrieve TLB CACHED frame num of accessing page(s))*/
  /* by using tlb_cache_read()/tlb_cache_write()
  frmnum is return value of tlb_cache_read/write value*/
  
#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at write region=%d offset=%d value=%d\n",
	          destination, offset, data);
	else
    printf("TLB miss at write region=%d offset=%d value=%d\n",
            destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif
  
  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  if(frmnum >= 0){//ghi truc tiep vao bo nho
    int phyaddr = (frmnum << PAGING_ADDR_FPN_LOBIT) + off;
    MEMPHY_write(proc->mram, phyaddr, data);
  }
  else{
    val = __write(proc, 0, destination, offset, data);
    uint32_t pte = proc->mm->pgd[pgnum];
    frmnum = PAGING_FPN(pte);
    tlb_cache_write(proc->tlb, proc->pid, pgnum, frmnum);
  }
  return val;
}
#endif
