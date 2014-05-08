
#include <boot.h>
#include <multiboot.h>
#include <system.h>
#include <stdarg.h> 
#include <_printf.h> 
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <uart.h>
#include <malloc.h>
#include <ctype.h>

#ifdef _DEBUG
#include <test.h>
#endif

extern void dump_regs(void);

typedef enum tag_cmds
{
  tag_cls = 0,
  tag_time,
  tag_date,
  tag_echo,
  tag_mem,
  tag_esp,
  tag_dump,
  tag_run,
  tag_end
};

str_comm const cmds_table[COMMAND_TABLE_LEN + 1] = 
{
    {"cls",  cls},
    {"time", exec_time},
    {"date", exec_date},
    {"echo", exec_echo},
    {"mem",  exec_mem},
    {"esp",  exec_esp},
    {"dump", dump_regs},
    {"run",  exec_run},
    {NULL,   NULL}
};

volatile char command[MAX_COMM_LEN];
multiboot_info_t *mbi;

volatile bool one_sec = false;
volatile date time_run;
volatile bool exec_comm = false;
volatile bool comm_overflow = false;
bool mmap_avail = false;
register char* stack_ptr asm ("esp");
//register char* instr_ptr asm ("ip");
extern char end;              
extern int RTCtimeSecond;
extern int RTCtimeMinute;
extern int RTCtimeHour;
extern int RTCtimeDay;
extern int RTCtimeMonth;
extern int RTCtimeYear;
extern void gettime(void);

extern volatile unsigned long timer_ticks;

char *heap_start = &end;

void* sbrk (int incr)
{

  static char *heap_end = 0;
  char *prev_heap_end;
  if (heap_end == 0){
    if(mmap_avail)
      heap_end = heap_start;
    else
      heap_end = &end;
  }
  
  
  prev_heap_end = heap_end;
  if (heap_end >= stack_ptr){
      kprintf("Heap and stack collision\n");
      return NULL;
      //abort();
  }
  
  heap_end += incr;
  return prev_heap_end;
}

date *get_time_run(void)
{
  return (date*)&time_run;
}

static int kprintf_help(unsigned c, void **ptr)
{
        putch(c);
        return 0;
}

void kprintf(const char *fmt, ...)
{
        va_list args;

        va_start(args, fmt);
        (void)do_printf(fmt, args, kprintf_help, NULL);
        va_end(args);
}

unsigned short *memsetw(unsigned short *dest, unsigned short val, size_t count)
{
    
        unsigned int i;
        for(i = 0; i < count; i++)
                dest[i] = val;

        return dest;
}

unsigned char inportb (unsigned short _port)
{
    unsigned char rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

void outportb (unsigned short _port, unsigned char _data)
{
    __asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

unsigned int peekl(unsigned int addr)
{
  unsigned int *p =(void*) addr;
  
  return (unsigned int)*p;
}
void pokel(unsigned int addr, unsigned int data)
{
  unsigned int *p = (void*) addr;
  (*p) = data;
}

void print_prompt(void)
{
  kprintf("[%d:%d:%d]#", time_run.hr, time_run.min, time_run.sec);
}

unsigned long bin2bcd(unsigned int n)
{
  unsigned int i, u;
  unsigned int ret = 0;
  char buf[MAX_YEAR_LEN];
  
  memset(buf, NULL, MAX_YEAR_LEN);
  sprintf(buf, "%x:", n);
  //putstr(buf);
  u = 0;
  for(i = 1; i <= n; i += i*10){
    ret = ret * 10;
    ret += buf[u++] - '0';
    if(u >= MAX_YEAR_LEN)
      break;
  }
   
  return ret;
}

struct date* getdatetime(void){

  gettime();
    
#ifndef USE_GRUB
  time_run.day = peekl(SEC_DAYF) & 0xff;
  time_run.hr = ((((peekl(HRS_MIN) & 0xff00) >> 8)) * 24) / 100;
  time_run.min = ((peekl(HRS_MIN) & 0xff) * 60) / 100;
  time_run.sec = (((peekl(SEC_DAYF) & 0xff00) >> 8) * 60) / 100;
#else
  time_run.year = bin2bcd(RTCtimeYear);
  time_run.month = bin2bcd(RTCtimeMonth);
  time_run.day = bin2bcd(RTCtimeDay);
  time_run.hr = bin2bcd(RTCtimeHour);
  time_run.min = bin2bcd(RTCtimeMinute);
  time_run.sec = bin2bcd(RTCtimeSecond);
#endif
  
  return &time_run;
}

void parse_command_line(void)
{
  char *pch;
  void *param = NULL;
  char *cmd;
  char *line = NULL;
  int idx = 0;
  bool found = false;

  pch = strchr((char*)command, '\n');
  *pch = '\0';
  if(strlen((char*)command) == 0){
    return;
  }
  
  
  line = malloc(strlen((char*)command) + 1);
  if(line == NULL){
    kprintf("Error: out of memory (malloc failed).\n");
    return;
  }
    
  memset(line, NULL, strlen((char*)command));
  strcpy(line, (char*)command);

  while(line[0] == ' ')
    line++;
  
  if(strlen(line) == 0){
    free(line);
    return;
  }
  pch = strtok(line, " ");
  
  if(pch != NULL){
    cmd = pch;
  }
  else{
    kprintf("Error: invalid command \"%s\"\n", command);
    free(line);
    return;
  }
  
  pch = strtok(NULL, " ");
  
  if(pch != NULL)
    param = pch;

  for(idx = 0; cmds_table[idx].name != NULL; idx++){
    if(strcmp(cmds_table[idx].name, cmd) == 0){
      found = true;
      break;
    }
  }
  
  if(found){
    (*cmds_table[idx].func)(param);
  }else{
    kprintf("%s: not found.\n", command);
  }
  
  free(line);
  return;
}



void exec_time(void){
  kprintf("time: %d:%d:%d\n", time_run.hr, time_run.min, time_run.sec);
}


void exec_date(void){
  kprintf("date: %d/%d/%d\n", time_run.day, time_run.month, time_run.year);
}

void exec_mem(void){
#ifdef USE_GRUB
  multiboot_info_t *mbinfo = NULL;
  mbinfo = mbi;
  if(mbinfo != NULL){  
    if (CHECK_FLAG (mbinfo->flags, 6)){
      memory_map_t *mmap;
    
      kprintf ("mmap_addr = 0x%x, mmap_length = 0x%x\n",
              (unsigned) mbinfo->mmap_addr, (unsigned) mbinfo->mmap_length);
      for (mmap = (memory_map_t *) mbinfo->mmap_addr; 
          (unsigned long) mmap < mbinfo->mmap_addr + mbinfo->mmap_length;
           mmap = (memory_map_t *) ((unsigned long) mmap + mmap->size + sizeof (mmap->size)))
        kprintf (" size = 0x%x, base_addr = 0x%x%x, length = 0x%x%x, type = 0x%x\n",
                (unsigned) mmap->size,
                (unsigned) mmap->base_addr_high,
                (unsigned) mmap->base_addr_low,
                (unsigned) mmap->length_high,
                (unsigned) mmap->length_low,
                (unsigned) mmap->type);
    }  
  }else{
    kprintf("Invalid parameter passed.\n");
  }
#else          
          kprintf("Total memory: %dKB\n", peekl(EXT_MEM1) + (peekl(EXT_MEM2) * 0x40));
          kprintf("Configured memory: %dKB\n", peekl(CFG_MEM1) + (peekl(CFG_MEM2) * 0x40));
#endif          
}
  
void exec_esp(void){
  kprintf("::esp:: 0x%x\n", stack_ptr);
  kprintf("::end:: 0x%x\n", &end);
  kprintf("::heap:: 0x%x\n", heap_start);
//  kprintf("::eip:: 0x%x\n", instr_ptr);
}


void exec_dump(struct regs *r){
  kprintf("Regs dump:\n eax=0x%x\n ebx=0x%x\n ecx=0x%x\n edx=0x%x\n esp=0x%x\n ebp=0x%x\n esi=0x%x\n edi=0x%x\n",\
          r->eax, r->ebx, r->ecx, r->edx, r->esp, r->ebp, r->esi, r->edi);
  kprintf(" gs=0x%x\n fs=0x%x\n es=0x%x\n ds=0x%x\n err_code=0x%x\n eip=0x%x\n cs=0x%x\n eflags=0x%x\n useresp=0x%x\n ss=0x%x\n",\
          r->gs, r->fs, r->es, r->ds, r->err_code, r->eip, r->cs, r->eflags, r->useresp, r->ss); 
  return;
}

void exec_echo(char *str){
  if(str != NULL)
    kprintf("%s\n", str);
}

void exec_run(void){
  kprintf("time elapsed: %d sec\n", timer_ticks/18 );
}


* MAIN
oid main (unsigned long magic, unsigned long addr)
{
  mbi = (multiboot_info_t *)addr;
  
  gdt_install();
  idt_install();
  isrs_install();
  irq_install();
  init_video();
  timer_install();
  keyboard_install();
  uart_init();

  __asm__ __volatile__ ("sti");

  memset((void*)command, NULL, sizeof(command));

  kprintf("kernel loaded\n");
#ifndef USE_GRUB
  kprintf("Total memory: %dKB\n", peekl(EXT_MEM1) + (peekl(EXT_MEM2) * 0x40));
#else
  if(magic != MULTIBOOT_BOOTLOADER_MAGIC)
    kprintf("ERROR: Invalid magic number!\n");
  kprintf("Magic number: 0x%x\n", magic);
  kprintf("Addr: 0x%x\n", addr);

  getdatetime();

  kprintf("time: %d:%d:%d\n", time_run.hr, time_run.min, time_run.sec);
  kprintf("date: %d/%d/20%d\n", time_run.day, time_run.month, time_run.year);
  
  
  kprintf ("flags = 0x%x\n", (unsigned) mbi->flags);

  
  if (CHECK_FLAG (mbi->flags, 0))
    kprintf ("mem_lower = %dKB, mem_upper = %dKB\n",
            (unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

  
  if (CHECK_FLAG (mbi->flags, 1))
    kprintf ("boot_device = 0x%x\n", (unsigned) mbi->boot_device);

  
  if (CHECK_FLAG (mbi->flags, 2))
    kprintf ("cmdline = %s\n", (char *) mbi->cmdline);

  
  if (CHECK_FLAG (mbi->flags, 3)){
      module_t *mod;
      unsigned int i;

      kprintf ("mods_count = %d, mods_addr = 0x%x\n",
              (int) mbi->mods_count, (int) mbi->mods_addr);
      for (i = 0, mod = (module_t *) mbi->mods_addr; i < mbi->mods_count; i++, mod++)
        kprintf (" mod_start = 0x%x, mod_end = 0x%x, string = %s\n",
                (unsigned) mod->mod_start,
                (unsigned) mod->mod_end,
                (char *) mod->string);
  }

  
  if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5)){
      kprintf ("ERROR: Both bits 4 and 5 are set.\n");
      //return;
  }

  
  if (CHECK_FLAG (mbi->flags, 4)){
      aout_symbol_table_t *aout_sym = &(mbi->u.aout_sym);

      kprintf ("aout_symbol_table: tabsize = 0x%0x, "
              "strsize = 0x%x, addr = 0x%x\n",
              (unsigned) aout_sym->tabsize,
              (unsigned) aout_sym->strsize,
              (unsigned) aout_sym->addr);
  }
  

  
  if (CHECK_FLAG (mbi->flags, 5)){
      elf_section_header_table_t *elf_sec = &(mbi->u.elf_sec);

      kprintf ("elf_sec: num = %u, size = 0x%x,"
              " addr = 0x%x, shndx = 0x%x\n",
              (unsigned) elf_sec->num, (unsigned) elf_sec->size,
              (unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
  }

  
  if (CHECK_FLAG (mbi->flags, 6)){
      memory_map_t *mmap;

      kprintf ("mmap_addr = 0x%x, mmap_length = 0x%x\n",
              (unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
      for (mmap = (memory_map_t *) mbi->mmap_addr;
           (unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
           mmap = (memory_map_t *) ((unsigned long) mmap
                                    + mmap->size + sizeof (mmap->size))){
        kprintf (" size = 0x%x, base_addr = 0x%x%x,"
                " length = 0x%x%x, type = 0x%x\n",
                (unsigned) mmap->size,
                (unsigned) mmap->base_addr_high,
                (unsigned) mmap->base_addr_low,
                (unsigned) mmap->length_high,
                (unsigned) mmap->length_low,
                (unsigned) mmap->type);
        if(mmap->type == 1){
          kprintf("Setting Stack and heap...\n");
          stack_ptr = (mmap->base_addr_high | mmap->base_addr_low) + 
                       (mmap->length_high | mmap->length_low) - 16;
          mmap_avail = true;
        }
      }
  }  
#endif //USE_GRUB
  
  print_prompt();

  while (true) {
    if(exec_comm){
      if(!comm_overflow){
        parse_command_line();
      }else{
        kprintf("command buffer overflow\n");
        comm_overflow = false;
      }
      print_prompt();
      exec_comm = false;
      memset((void*)command, NULL, sizeof(command));
    }
  }
}
