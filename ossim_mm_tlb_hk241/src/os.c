
#include "cpu.h"
#include "timer.h"
#include "sched.h"
#include "loader.h"
#include "mm.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int time_slot;
static int num_cpus;
static int done = 0;
FILE *output_file;


#ifdef CPU_TLB
static int tlbsz;
#endif

#ifdef MM_PAGING
static int memramsz;
static int memswpsz[PAGING_MAX_MMSWP];

struct mmpaging_ld_args {
	/* A dispatched argument struct to compact many-fields passing to loader */
	struct memphy_struct *tlb;
	struct memphy_struct *mram;
	struct memphy_struct **mswp;
	struct memphy_struct *active_mswp;
	struct timer_id_t  *timer_id;
};
#endif

static struct ld_args{
	char ** path;
	unsigned long * start_time;
#ifdef MLQ_SCHED
	unsigned long * prio;
#endif
} ld_processes;
int num_processes;

struct cpu_args {
	struct timer_id_t * timer_id;
	int id;
};


static void * cpu_routine(void * args) {
	struct timer_id_t * timer_id = ((struct cpu_args*)args)->timer_id;
	int id = ((struct cpu_args*)args)->id;
	/* Check for new process in ready queue */
	int time_left = 0;
	struct pcb_t * proc = NULL;
	while (1) {
		/* Check the status of current process */
		while(timer_id->time_wait==0);
		
		while(timer_id->cpu_wait==0); //wait for load
		
		if (proc == NULL && !done) {
			/* No process is running, the we load new process from
		 	* ready queue */
			proc = get_proc();
	
			if (proc == NULL) {
                           next_slot(timer_id);
                           continue; /* First load failed. skip dummy load */
            }
		}else if (proc && proc->pc == proc->code->size) {
			/* The porcess has finish it job */
			printf("\tCPU %d: Processed %2d has finished\n",
				id ,proc->pid);
			fprintf(output_file, "\tCPU %d: Processed %2d has finished\n",
				id ,proc->pid);	
			free(proc);
			proc = get_proc();
			time_left = 0;
		}else if (proc && time_left == 0) {
			/* The process has done its job in current time slot */
			printf("\tCPU %d: Put process %2d to run queue\n",
				id, proc->pid);
			fprintf(output_file, "\tCPU %d: Put process %2d to run queue\n",
				id, proc->pid);	
			put_proc(proc);
			proc = get_proc();
		}
		
		/* Recheck process status after loading new process */
		if (proc == NULL && done) {
			/* No process to run, exit */
			printf("\tCPU %d stopped\n", id);
			fprintf(output_file, "\tCPU %d stopped\n", id);	
			break;
		}else if (proc == NULL) {
			/* There may be new processes to run in
			 * next time slots, just skip current slot */
			next_slot(timer_id);
			continue;
		}else if (time_left == 0) {
			printf("\tCPU %d: Dispatched process %2d\n",
				id, proc->pid);
			fprintf(output_file, "\tCPU %d: Dispatched process %2d\n",
				id, proc->pid);
			time_left = time_slot;
		}
		
		/* Run current process */
		run(proc);
		time_left--;
		next_slot(timer_id);
	}
	detach_event(timer_id);
	pthread_exit(NULL);
}

static void * ld_routine(void * args) {
#ifdef MM_PAGING
#ifdef CPU_TLB
	struct memphy_struct* tlb = ((struct mmpaging_ld_args *)args)->tlb;
#endif
	struct memphy_struct* mram = ((struct mmpaging_ld_args *)args)->mram;
	struct memphy_struct** mswp = ((struct mmpaging_ld_args *)args)->mswp;
	struct memphy_struct* active_mswp = ((struct mmpaging_ld_args *)args)->active_mswp;
	struct timer_id_t * timer_id = ((struct mmpaging_ld_args *)args)->timer_id;
#else
	struct timer_id_t * timer_id = (struct timer_id_t*)args;
#endif
	int i = 0;
	printf("ld_routine\n");
	fprintf(output_file, "ld_routine\n");
	while (i < num_processes) {
		while(timer_id->time_wait == 0); 
		struct pcb_t * proc = load(ld_processes.path[i]);
#ifdef MLQ_SCHED
		proc->prio = ld_processes.prio[i];
#endif
		while (current_time() < ld_processes.start_time[i]) {
			wait_cpu();
			next_slot(timer_id);
			while(timer_id->time_wait==0){};
		}
#ifdef MM_PAGING
		proc->mm = malloc(sizeof(struct mm_struct));
		init_mm(proc->mm, proc);
	#ifdef CPU_TLB
		proc->tlb = tlb;
	#endif
		proc->mram = mram;
		proc->mswp = mswp;
		proc->active_mswp = active_mswp;
#endif
		printf("\tLoaded a process at %s, PID: %d PRIO: %ld\n",
			ld_processes.path[i], proc->pid, ld_processes.prio[i]);
		fprintf(output_file,"\tLoaded a process at %s, PID: %d PRIO: %ld\n",
			ld_processes.path[i], proc->pid, ld_processes.prio[i]);
		add_proc(proc);
		free(ld_processes.path[i]);
		i++;
		wait_cpu();
		next_slot(timer_id);
	}
	free(ld_processes.path);
	free(ld_processes.start_time);
	done = 1;
	wait_cpu();
	detach_event(timer_id);
	pthread_exit(NULL);
}

void merge(unsigned long *start_time, unsigned long *prio, char **path, int left, int mid, int right) {
    int i, j, k;
    int n1 = mid - left + 1;
    int n2 = right - mid;


    unsigned long L_start_time[n1], R_start_time[n2];
    unsigned long L_prio[n1], R_prio[n2];
    char *L_path[n1], *R_path[n2];


    for (i = 0; i < n1; i++) {
        L_start_time[i] = start_time[left + i];
        L_prio[i] = prio[left + i];
        L_path[i] = path[left + i];
    }
    for (j = 0; j < n2; j++) {
        R_start_time[j] = start_time[mid + 1 + j];
        R_prio[j] = prio[mid + 1 + j];
        R_path[j] = path[mid + 1 + j];
    }


    i = 0;
    j = 0;
    k = left;
    while (i < n1 && j < n2) {
        if (L_start_time[i] < R_start_time[j] || (L_start_time[i] == R_start_time[j] && L_prio[i] < R_prio[j])) {
            start_time[k] = L_start_time[i];
            prio[k] = L_prio[i];
            path[k] = L_path[i];
            i++;
        } else {
            start_time[k] = R_start_time[j];
            prio[k] = R_prio[j];
            path[k] = R_path[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        start_time[k] = L_start_time[i];
        prio[k] = L_prio[i];
        path[k] = L_path[i];
        i++;
        k++;
    }

    while (j < n2) {
        start_time[k] = R_start_time[j];
        prio[k] = R_prio[j];
        path[k] = R_path[j];
        j++;
        k++;
    }
}

void mergeSort(unsigned long *start_time, unsigned long *prio, char **path, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;

        mergeSort(start_time, prio, path, left, mid);
        mergeSort(start_time, prio, path, mid + 1, right);

        merge(start_time, prio, path, left, mid, right);
    }
}
static void read_config(const char * path) {
	FILE * file;
	if ((file = fopen(path, "r")) == NULL) {
		printf("Cannot find configure file at %s\n", path);
		exit(1);
	}
	fscanf(file, "%d %d %d\n", &time_slot, &num_cpus, &num_processes);
	ld_processes.path = (char**)malloc(sizeof(char*) * num_processes);
	ld_processes.start_time = (unsigned long*)
		malloc(sizeof(unsigned long) * num_processes);

#ifdef CPU_TLB
#ifdef CPUTLB_FIXED_TLBSZ
	/* We provide here a back compatible with legacy OS simulatiom config file
	 * In which, it have no addition config line for CPU_TLB
	 */
	tlbsz = 0x10000;
#else
	/* Read input config of TLB size:
	 * Format:
	 *        CPU_TLBSZ
	*/
	fscanf(file, "%d\n", &tlbsz);
#endif
#endif

#ifdef MM_PAGING
	int sit;
#ifdef MM_FIXED_MEMSZ
	/* We provide here a back compatible with legacy OS simulatiom config file
	 * In which, it have no addition config line for Mema, keep only one line
	 * for legacy info 
	 *  [time slice] [N = Number of CPU] [M = Number of Processes to be run]
	 */
	memramsz    =  0x100000;
	memswpsz[0] = 0x1000000;
	for(sit = 1; sit < PAGING_MAX_MMSWP; sit++)
		memswpsz[sit] = 0;
#else
	/* Read input config of memory size: MEMRAM and upto 4 MEMSWP (mem swap)
	 * Format: (size=0 result non-used memswap, must have RAM and at least 1 SWAP)
	 *        MEM_RAM_SZ MEM_SWP0_SZ MEM_SWP1_SZ MEM_SWP2_SZ MEM_SWP3_SZ
	*/
	fscanf(file, "%d\n", &memramsz);
	for(sit = 0; sit < PAGING_MAX_MMSWP; sit++)
		fscanf(file, "%d", &(memswpsz[sit])); 

	fscanf(file, "\n"); /* Final character */
#endif
#endif

#ifdef MLQ_SCHED
	ld_processes.prio = (unsigned long*)
		malloc(sizeof(unsigned long) * num_processes);
#endif
	int i;
	for (i = 0; i < num_processes; i++) {
		ld_processes.path[i] = (char*)malloc(sizeof(char) * 100);
		ld_processes.path[i][0] = '\0';
		strcat(ld_processes.path[i], "input/proc/");
		char proc[100];
#ifdef MLQ_SCHED
		fscanf(file, "%lu %s %lu\n", &ld_processes.start_time[i], proc, &ld_processes.prio[i]);
#else
		fscanf(file, "%lu %s\n", &ld_processes.start_time[i], proc);
#endif

		strcat(ld_processes.path[i], proc);
	}
	mergeSort(ld_processes.start_time, ld_processes.prio, ld_processes.path, 0, num_processes-1); // Fix nếu thứ tự time_start nhập ko theo thứ tự
}

int main(int argc, char * argv[]) {
	/* Read config */
	if (argc != 2) {
		printf("Usage: os [path to configure file]\n");
		return 1;
	}
	char path[100];
	path[0] = '\0';
	char pathout[100];
	pathout[0] = '\0';
	strcat(path, "input/");
	strcat(pathout, "output/");
	strcat(path, argv[1]);
	strcat(pathout, argv[1]);
	strcat(pathout, ".output");
	output_file = fopen(pathout, "w+");

	read_config(path);

	pthread_t * cpu = (pthread_t*)malloc(num_cpus * sizeof(pthread_t));
	struct cpu_args * args =
		(struct cpu_args*)malloc(sizeof(struct cpu_args) * num_cpus);
	pthread_t ld;
	
	/* Init timer */
	int i;
	for (i = 0; i < num_cpus; i++) {
		args[i].timer_id = attach_event();
		args[i].id = i;
	}
	struct timer_id_t * ld_event = attach_event();
	start_timer();
#ifdef CPU_TLB
	struct memphy_struct tlb;

	init_tlbmemphy(&tlb, tlbsz);
#endif

#ifdef MM_PAGING
	/* Init all MEMPHY include 1 MEMRAM and n of MEMSWP */
	int rdmflag = 1; /* By default memphy is RANDOM ACCESS MEMORY */

	struct memphy_struct mram;
	struct memphy_struct mswp[PAGING_MAX_MMSWP];
	

	/* Create MEM RAM */
	init_memphy(&mram, memramsz, rdmflag);
	/* Create all MEM SWAP */ 
	int sit;
	for(sit = 0; sit < PAGING_MAX_MMSWP; sit++)
	       init_memphy(&mswp[sit], memswpsz[sit], rdmflag);

	/* In Paging mode, it needs passing the system mem to each PCB through loader*/
	struct mmpaging_ld_args *mm_ld_args = malloc(sizeof(struct mmpaging_ld_args));

	mm_ld_args->timer_id = ld_event;
	mm_ld_args->mram = (struct memphy_struct *) &mram;
	mm_ld_args->mswp = (struct memphy_struct**) &mswp;
	mm_ld_args->active_mswp = (struct memphy_struct *) &mswp[0];
#endif

#ifdef CPU_TLB
#ifdef MM_PAGING
	/* In MM_PAGING employ CPU_TLB mode, it needs passing
	 * the system tlb to each PCB through loader
	*/
	mm_ld_args->tlb = (struct memphy_struct *) &tlb;
#endif
#endif

	/* Init scheduler */
	init_scheduler();
	/* Run CPU and loader */
#ifdef MM_PAGING
	pthread_create(&ld, NULL, ld_routine, (void*)mm_ld_args);
#else
	pthread_create(&ld, NULL, ld_routine, (void*)ld_event);
#endif
	for (i = 0; i < num_cpus; i++) {
		pthread_create(&cpu[i], NULL,
			cpu_routine, (void*)&args[i]);
	}

	/* Wait for CPU and loader finishing */
	for (i = 0; i < num_cpus; i++) {
		pthread_join(cpu[i], NULL);
	}
	pthread_join(ld, NULL);

	/* Stop timer */
	stop_timer();
	fclose(output_file);
	return 0;

}