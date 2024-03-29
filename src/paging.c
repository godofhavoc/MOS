#include "paging.h"
#include "kheap.h"

page_directory_t *kernel_directory = 0;

page_directory_t *current_directory = 0;

u32int *frames;
u32int nframes;

extern u32int placement_address;
extern heap_t *kheap;

#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

static void set_frame(u32int frame_addr)
{
    u32int frame = frame_addr/0x1000;
    u32int idx = INDEX_FROM_BIT(frame);
    u32int off = OFFSET_FROM_BIT(frame);
    frames[idx] |= (0x1 << off);
}

static void clear_frame(u32int frame_addr)
{
    u32int frame = frame_addr/0x1000;
    u32int idx = INDEX_FROM_BIT(frame);
    u32int off = OFFSET_FROM_BIT(frame);
    frames[idx] &= ~(0x1 << off);
}

static u32int test_frame(u32int frame_addr)
{
    u32int frame = frame_addr/0x1000;
    u32int idx = INDEX_FROM_BIT(frame);
    u32int off = OFFSET_FROM_BIT(frame);
    return (frames[idx] & (0x1 << off));
}

static u32int first_frame()
{
    u32int i, j;
    for (i = 0; i < INDEX_FROM_BIT(nframes); i++)
    {
        if (frames[i] != 0xFFFFFFFF)
        {
            for (j = 0; j < 32; j++)
            {
                u32int toTest = 0x1 << j;
                if ( !(frames[i]&toTest) )
                {
                    return i*4*8+j;
                }
            }
        }
    }
}

void alloc_frame(page_t *page, int is_kernel, int is_writeable)
{
    if (page->frame != 0)
    {
        return;
    }
    else
    {
        u32int idx = first_frame();
        if (idx == (u32int)-1)
        {
            //no free frames
        }
        set_frame(idx*0x1000);
        page->present = 1;
        page->rw = (is_writeable)?1:0;
        page->user = (is_kernel)?0:1;
        page->frame = idx;
    }
}

void free_frame(page_t *page)
{
    u32int frame;
    if (!(frame=page->frame))
    {
        return;
    }
    else
    {
        clear_frame(frame);
        page->frame = 0x0;
    }
}

void initialise_paging()
{
    u32int mem_end_page = 0x1000000;
    
    nframes = mem_end_page / 0x1000;
    frames = (u32int*)kmalloc(INDEX_FROM_BIT(nframes));
    memset(frames, 0, INDEX_FROM_BIT(nframes));

    u32int phys;
    frames = (u32int*)kmalloc(INDEX_FROM_BIT(nframes));
    memset(frames, 0, INDEX_FROM_BIT(nframes));

    u32int phys;
    kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    memset(kernel_directory, 0, sizeof(page_directory_t));
    kernel_directory->physicalAddr = (u32int)kernel_directory->tablesPhysical;

    int i = 0;
    for(i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += 0x1000)
    	get_page(i, 1, kernel_directory);

    i = 0;
    while(i < placement_address+0x1000){
    	alloc_frame(get_page(i, 1, kernel_directory), 0, 0);
    	i += 0x1000;
    }

    for (i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += 0x1000)
       alloc_frame( get_page(i, 1, kernel_directory), 0, 0);

   register_interrupt_handler(14, page_fault);

   switch_page_directory(kernel_directory);
   kheap = create_heap(KHEAP_START, KHEAP_START+KHEAP_INITIAL_SIZE, 0xCFFFF000, 0, 0);
   
   current_directory = clone_directory(kernel_directory);
   switch_page_directory(current_directory);
}

void switch_page_directory(page_directory_t *dir)
{
    current_directory = dir;
    asm volatile("mov %0, %%cr3":: "r"(&dir->tablesPhysical));
    u32int cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; 
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

page_t *get_page(u32int address, int make, page_directory_t *dir)
{
    
    address /= 0x1000;
    
    u32int table_idx = address / 1024;
    if (dir->tables[table_idx]) 
    {
        return &dir->tables[table_idx]->pages[address%1024];
    }
    else if(make)
    {
        u32int tmp;
        dir->tables[table_idx] = (page_table_t*)kmalloc_ap(sizeof(page_table_t), &tmp);
        dir->tablesPhysical[table_idx] = tmp | 0x7; 
        return &dir->tables[table_idx]->pages[address%1024];
    }
    else
    {
        return 0;
    }
}


void page_fault(registers_t regs)
{
    
    
    u32int faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
    
    
    int present   = !(regs.err_code & 0x1); 
    int rw = regs.err_code & 0x2;           
    int us = regs.err_code & 0x4;           
    int reserved = regs.err_code & 0x8;     
    int id = regs.err_code & 0x10;          

    monitor_write("Page fault! ( ");
    if (present) {monitor_write("present ");}
    if (rw) {monitor_write("read-only ");}
    if (us) {monitor_write("user-mode ");}
    if (reserved) {monitor_write("reserved ");}
    monitor_write(") at 0x");
    monitor_write_hex(faulting_address);
    monitor_write(" - EIP: ");
    monitor_write_hex(regs->eip);
    monitor_write("\n");
    PANIC("Page fault");
}

static page_table_t *clone_table(page_table_t *src, u32int *physAddr){
	page_table_t *table = (page_table_t*)kmalloc_ap(sizeof(page_table_t), physAddr);

	memset(table, 0, sizeof(page_directory_t));

	int i;
	for(i=0; i<1024; i++){
		if(src->pages[i].frame){
			alloc_frame(&table->pages[i], 0, 0);

			if (src->pages[i].present) table->pages[i].present = 1;
            if (src->pages[i].rw) table->pages[i].rw = 1;
            if (src->pages[i].user) table->pages[i].user = 1;
            if (src->pages[i].accessed) table->pages[i].accessed = 1;
            if (src->pages[i].dirty) table->pages[i].dirty = 1;

            copy_page_physical(src->pages[i].frame*0x1000, tables->pages[i].frame*0x1000);
		}
	}
	return table;
}

page_directory_t *clone_directory(page_directory_t *src){
	u32int phys;

	page_directory_t *dir = (page_directory_t*)kmalloc_ap(sizeof(page_directory_t), &phys);

	memset(dir, 0, sizeof(page_directory_t));

	u32int offset = (u32int)dir->tablesPhysical - (u32int)dir;

	dir->physicalAddr = phys + offset;

	int i;
	for(i=0; i<1024; i++){
		if(!src->tables[i])
			continue;

		if(kernel_directory->tables[i] == src->tables[i]){
			dir->tables[i] = src->tables[i];
			dir->tablesPhysical[i] = src->tablesPhysical[i];
		}
		else{
			u32int phys;
			dir->tables[i] = clone_table(src->tables[i], &phys);
			dir->tablesPhysical[i] = phys | 0x07;
		}
	}
	return dir;
}
