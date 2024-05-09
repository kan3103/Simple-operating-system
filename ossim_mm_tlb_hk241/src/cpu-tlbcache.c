/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef MM_TLB
/*
 * Memory physical based TLB Cache
 * TLB cache module tlb/tlbcache.c
 *
 * TLB cache is physically memory phy
 * supports random access 
 * and runs at high speed
 */


#include "mm.h"
#include <stdlib.h>


struct tlb_entry *tlb_entries;
#define init_tlbcache(mp,sz,...) init_memphy(mp, sz, (1, ##__VA_ARGS__))

/*
 *  tlb_cache_read read TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_read(struct memphy_struct * mp, int pid, int pgnum, BYTE* value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
     // Search for the page number in the TLB
   int k= pgnum/mp->maxsz;
   int pg= pgnum % mp->maxsz;
   if(tlb_entries[pg].valid==0) return 0;
   if(tlb_entries[pg].pagenum!=k) return 0;
   if(tlb_entries[pg].pid==pid){
      TLBMEMPHY_read(mp,pg,value);
      return 1;
   };
   return 0;
}

/*
 *  tlb_cache_write write TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_write(struct memphy_struct *mp, int pid, int pgnum, BYTE value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   int k= pgnum/mp->maxsz;
   int pg= pgnum % mp->maxsz;
   TLBMEMPHY_write(mp,pg,value);
   tlb_entries[pg].pid=pid;
   tlb_entries[pg].valid=1;
   tlb_entries[pg].pagenum=k;
   return 0;
}
int tlb_cache_free(struct memphy_struct *mp, int pid, int pgnum){
   int pg= pgnum % mp->maxsz;
   TLBMEMPHY_write(mp,pg,0);
   tlb_entries[pg].pid=0;
   tlb_entries[pg].valid=-1;
   tlb_entries[pg].pagenum=-1;
   return 0;
}
/*
 *  TLBMEMPHY_read natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int TLBMEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
   
   if (mp == NULL)
     return -1;

   /* TLB cached is random access by native */
   *value = mp->storage[addr];
   return 0;
}


/*
 *  TLBMEMPHY_write natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int TLBMEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
   if (mp == NULL)
     return -1;

   /* TLB cached is random access by native */
   mp->storage[addr] = data;

   return 0;
}

/*
 *  TLBMEMPHY_format natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 */


int TLBMEMPHY_dump(struct memphy_struct * mp)
{
   /*TODO dump memphy contnt mp->storage 
    *     for tracing the memory content
    */

   return 0;
}


/*
 *  Init TLBMEMPHY struct
 */
int init_tlbmemphy(struct memphy_struct *mp, int max_size)
{
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;

   mp->rdmflg = 1;
   tlb_entries = malloc(max_size*sizeof(struct tlb_entry));
   for(int i=0;i<max_size;++i){
      tlb_entries[i].valid=0;
      tlb_entries[i].pid=-1;
      tlb_entries[i].pagenum=-1;
   }
   return 0;
}

//#endif
