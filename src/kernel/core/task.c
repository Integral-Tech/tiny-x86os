#include "core/task.h"
#include "comm/elf.h"
#include "core/memory.h"
#include "core/syscall.h"
#include "cpu/irq.h"
#include "cpu/mmu.h"
#include "fs/fs.h"
#include "os_cfg.h"
#include "tools/klib.h"
#include "tools/log.h"

static uint32_t idle_task_stack[IDLE_TASK_SIZE];
static task_manager_t task_manager;
static uint16_t task_cnt = 0;
static task_t task_table[TASK_NUM];
static mutex_t task_table_mutex;

static int tss_init(task_t *task, flag_t flag, uint32_t entry, uint32_t esp) {
  const int tss_selector = gdt_alloc_desc();
  if (tss_selector < 0) {
    log_printf("Allocate TSS failed.");
    return -1;
  }

  segment_desc_set(tss_selector, (uint32_t)&task->tss, sizeof(tss_t),
                   SEG_P | SEG_DPL0 | SEG_TYPE_TSS);
  kernel_memset(&task->tss, 0, sizeof(tss_t));

  const uint32_t kernel_stack = memory_alloc_page();
  if (!kernel_stack)
    goto tss_init_failed;

  int code_selector, data_selector;
  if (flag == SYSTEM) {
    code_selector = KERNEL_SELECTOR_CS;
    data_selector = KERNEL_SELECTOR_DS;
  } else {
    code_selector = task_manager.app_code_selector | SEG_CPL3;
    data_selector = task_manager.app_data_selector | SEG_CPL3;
  }

  task->tss.eip = entry;
  task->tss.esp = esp;
  task->tss.esp0 = kernel_stack + MEM_PAGE_SIZE;
  task->tss.ss = data_selector;
  task->tss.ss0 = KERNEL_SELECTOR_DS;
  task->tss.es = task->tss.ds = task->tss.fs = task->tss.gs = data_selector;
  task->tss.cs = code_selector;
  task->tss.eflags = EFLAGS_DEFAULT | EFLAGS_IF; // switch on Interrupt Flag(IF)
  const uint32_t pde = memory_create_uvm();      // user virtual memory

  if (!pde)
    goto tss_init_failed;

  task->tss.cr3 = pde;
  task->tss_selector = tss_selector;
  return 0;

tss_init_failed:
  gdt_free_selector(tss_selector);

  if (kernel_stack)
    memory_free_page(kernel_stack);

  return -1;
}

int task_init(task_t *task, const char *name, flag_t flag, uint32_t entry,
              uint32_t esp) {
  ASSERT(task != NULL);
  tss_init(task, flag, entry, esp);

  kernel_strncpy(task->name, name, TASK_NAME_SIZE);
  task->state = TASK_CREATED;
  task->time_ticks = TASK_TIME_SLICE_DEFAULT;
  task->slice_ticks = task->time_ticks;
  task->sleep_ticks = 0;
  task->parent = NULL;
  task->heap_start = task->heap_end = 0;

  list_node_init(&task->all_node);
  list_node_init(&task->run_node);
  list_node_init(&task->wait_node);

  kernel_memset(&task->file_table, 0, sizeof(task->file_table));

  const irq_state_t state = irq_protect();

  task->pid = task_cnt++;
  list_insert_last(&task_manager.task_list,
                   &task->all_node); // insert to task_list
  task->exit_status = 0;

  irq_unprotect(state);
  return 0;
}

void task_start(task_t *task) {
  const irq_state_t state = irq_protect();
  task_set_ready(task);
  irq_unprotect(state);
}

void task_uninit(task_t *task) {
  if (task->tss_selector)
    gdt_free_selector(task->tss_selector);

  if (task->tss.esp0)
    memory_free_page(task->tss.esp - MEM_PAGE_SIZE);

  if (task->tss.cr3)
    memory_destroy_uvm(task->tss.cr3);

  kernel_memset(task, 0, sizeof(task_t));
}

static void idle_task_entry() {
  while (1)
    hlt();
}

void task_switch_to(const task_t *target_task) {
  switch_to_tss(target_task->tss_selector);
}

void task_manager_init() {
  kernel_memset(task_table, 0, sizeof(task_table));
  mutex_init(&task_table_mutex);

  int selector = gdt_alloc_desc();
  segment_desc_set(selector, 0, 0xFFFFFFFF,
                   SEG_P | SEG_DPL3 | SEG_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW |
                       SEG_D);
  task_manager.app_data_selector = selector;

  selector = gdt_alloc_desc();
  segment_desc_set(selector, 0, 0xFFFFFFFF,
                   SEG_P | SEG_DPL3 | SEG_NORMAL | SEG_TYPE_CODE | SEG_TYPE_RW |
                       SEG_D);
  task_manager.app_code_selector = selector;

  list_init(&task_manager.ready_list);
  list_init(&task_manager.task_list);
  list_init(&task_manager.sleep_list);
  task_manager.curr_task = NULL;

  task_init(&task_manager.idle_task, "Idle Task", SYSTEM,
            (uint32_t)idle_task_entry,
            (uint32_t)(idle_task_stack + IDLE_TASK_SIZE));

  task_start(&task_manager.idle_task);
}

void task_first_init() {
  extern void first_task_entry();
  extern uint8_t s_first_task[], e_first_task[];

  const uint32_t copy_size = (uint32_t)(e_first_task - s_first_task);
  const uint32_t alloc_size = 10 * MEM_PAGE_SIZE;
  ASSERT(copy_size < alloc_size);

  task_init(&task_manager.first_task, "First Task", USER,
            (uint32_t)first_task_entry,
            (uint32_t)first_task_entry + alloc_size);
  task_manager.first_task.heap_start = (uint32_t)e_first_task;
  task_manager.first_task.heap_end = (uint32_t)e_first_task;

  write_tr(task_manager.first_task.tss_selector);
  task_manager.curr_task = &task_manager.first_task;

  mmu_set_page_dir(task_manager.first_task.tss.cr3);

  memory_alloc_page_for((uint32_t)first_task_entry, alloc_size,
                        PTE_P | PTE_W | PTE_U);
  kernel_memcpy(first_task_entry, s_first_task, copy_size);

  task_start(&task_manager.first_task);
}

task_t *get_first_task() { return &task_manager.first_task; }

void task_set_ready(task_t *task) {
  if (task != &task_manager.idle_task) {
    list_insert_last(&task_manager.ready_list, &task->run_node);
    task->state = TASK_READY;
  }
}

void task_set_block(task_t *task) {
  if (task != &task_manager.idle_task)
    list_remove(&task_manager.ready_list, &task->run_node);
}

task_t *get_curr_task() { return task_manager.curr_task; }

task_t *task_next_run() { // next task to run
  if (list_cnt(&task_manager.ready_list) == 0)
    return &task_manager.idle_task;

  const list_node_t *task_node = list_first(&task_manager.ready_list);
  return list_node_parent(task_node, task_t, run_node);
  // convert task_node to the parent task_t
}

int sys_yield() { // move the current task to the tail of list
  const irq_state_t state = irq_protect();

  if (list_cnt(&task_manager.ready_list) > 1) {
    task_t *curr_task = get_curr_task();
    task_set_block(curr_task); // remove curr_task from the list
    task_set_ready(curr_task); // insert curr_task to the tail of list
    task_dispatch();           // dispatch(assign) a new task
  }

  irq_unprotect(state);
  return 0;
}

void task_dispatch() {
  const irq_state_t state = irq_protect();

  task_t *next_task = task_next_run(); // fetch next task to run
  if (next_task != task_manager.curr_task) {
    task_manager.curr_task = next_task;
    next_task->state = TASK_RUNNING;
    task_switch_to(next_task);
  }

  irq_unprotect(state);
}

void task_time_tick() {
  task_t *curr_task = get_curr_task();

  if (--curr_task->slice_ticks == 0) {
    curr_task->slice_ticks = curr_task->time_ticks;
    task_set_block(curr_task);
    task_set_ready(curr_task); // move current task to the tail of ready queue
  }

  /*
   * scan the sleep_list at intervals (OS_TICKS_MS)
   * to check whether sleeping is due;
   * if sleeping is due, move the task to ready queue
   */
  const list_node_t *curr = list_first(&task_manager.sleep_list);
  while (curr) {
    const list_node_t *next = list_node_next(curr);
    task_t *task = list_node_parent(curr, task_t, run_node);
    if (--task->sleep_ticks == 0) {
      task_set_wakeup(task);
      task_set_ready(task);
    }

    curr = next;
  }

  task_dispatch();
}

void task_set_sleep(task_t *task, uint32_t ticks) {
  if (ticks <= 0)
    return;

  task->sleep_ticks = ticks;
  task->state = TASK_SLEEPING;
  list_insert_last(&task_manager.sleep_list, &task->run_node);
}

void task_set_wakeup(task_t *task) {
  list_remove(&task_manager.sleep_list, &task->run_node);
}

/*
 * move current task from ready queue to sleeping queue
 * then dispatch next task from the ready queue
 *
 *  real sleeping time should be greater than or equal to the variable
 *  "sleeping_time", which should also be a minimal multiple of 10
 *  e.g. 10 ms -> 10 ms
 *       11 ms -> 20 ms
 *       19 ms -> 20 ms
 */
void sys_sleep(uint32_t sleeping_time) {
  const irq_state_t state = irq_protect();
  task_set_block(task_manager.curr_task);
  task_set_sleep(task_manager.curr_task,
                 (sleeping_time + (OS_TICKS_MS - 1)) / OS_TICKS_MS);
  task_dispatch();

  irq_unprotect(state);
}

int sys_getpid() { return get_curr_task()->pid; }

void sys_print_msg(const char *fmt, int arg) {
  log_printf(fmt, arg);
} // for debug temporarily

static task_t *alloc_task() {
  mutex_lock(&task_table_mutex);

  task_t *task = NULL;

  for (int i = 0; i < TASK_NUM; i++) {
    task_t *curr = task_table + i;
    if (curr->name[0] == '\0') {
      task = curr;
      break;
    }
  }

  mutex_unlock(&task_table_mutex);
  return task;
}

static void free_task(task_t *task) {
  mutex_lock(&task_table_mutex);
  task->name[0] = '\0';
  mutex_unlock(&task_table_mutex);
}

int sys_fork() {
  task_t *parent_task = get_curr_task();
  task_t *child_task = alloc_task();

  if (child_task == NULL)
    goto fork_failed;

  const syscall_frame_t *frame =
      (syscall_frame_t *)(parent_task->tss.esp0 - sizeof(syscall_frame_t));
  const int err =
      task_init(child_task, parent_task->name, USER, frame->auto_push.eip,
                frame->auto_push.esp + SYSCALL_ARGC * sizeof(uint32_t));
  if (err < 0)
    goto fork_failed;

  tss_t *tss = &child_task->tss;
  tss->eax = 0;
  tss->ebx = frame->manual_push.ebx;
  tss->ecx = frame->manual_push.ecx;
  tss->edx = frame->manual_push.edx;
  tss->esi = frame->manual_push.esi;
  tss->edi = frame->manual_push.edi;
  tss->ebp = frame->manual_push.ebp;
  tss->cs = frame->auto_push.cs;
  tss->ds = frame->manual_push.ds;
  tss->es = frame->manual_push.es;
  tss->fs = frame->manual_push.fs;
  tss->gs = frame->manual_push.gs;
  tss->eflags = frame->manual_push.eflags;

  child_task->parent = parent_task;
  if (!(child_task->tss.cr3 = memory_copy_uvm(parent_task->tss.cr3)))
    goto fork_failed;

  for (int i = 0; i < TASK_FILE_NUM; i++) {
    file_t *file = parent_task->file_table[i];
    if (file) {
      file_ref_inc(file);
      child_task->file_table[i] = file;
    }
  }

  task_start(child_task);
  return child_task->pid;

fork_failed:
  if (child_task) {
    task_uninit(child_task);
    free_task(child_task);
  }

  return -1;
}

static int load_phdr(int file, const Elf32_Phdr *phdr, uint32_t page_dir) {
  const int err = memory_alloc_for_page_dir(
      page_dir, phdr->p_vaddr, phdr->p_memsz, PTE_P | PTE_U | PTE_W);
  if (err < 0) {
    log_printf("Memory is insufficient!");
    return -1;
  }

  if (sys_lseek(file, phdr->p_offset, 0) < 0) {
    log_printf("Read file failed!");
    return -1;
  }

  uint32_t vaddr = phdr->p_vaddr;
  uint32_t size = phdr->p_filesz;

  while (size > 0) {
    const int curr_size = size > MEM_PAGE_SIZE ? MEM_PAGE_SIZE : size;
    const uint32_t paddr = memory_get_paddr(page_dir, vaddr);

    if (sys_read(file, (char *)paddr, curr_size) < curr_size) {
      log_printf("Read file failed.");
      return -1;
    }

    size -= curr_size;
    vaddr += curr_size;
  }

  return 0;
}

static uint32_t load_elf_file(task_t *task, const char *name,
                              uint32_t page_dir) {
  const Elf32_Ehdr elf_hdr;
  const Elf32_Phdr elf_phdr;

  const int file = sys_open(name, USER);
  if (file < 0) {
    log_printf("Open file %s failed.", name);
    goto load_failed;
  }

  size_t size = sys_read(file, (char *)&elf_hdr, sizeof(Elf32_Ehdr));
  if (size < sizeof(Elf32_Ehdr)) {
    log_printf("ELF header is too small! (Size = %d)", size);
    goto load_failed;
  }

  if (elf_hdr.e_ident[0] != ELF_MAGIC || elf_hdr.e_ident[1] != 'E' ||
      elf_hdr.e_ident[2] != 'L' || elf_hdr.e_ident[3] != 'F') {
    log_printf("ELF File is invalid!");
    goto load_failed;
  }

  uint32_t e_phoff = elf_hdr.e_phoff;
  for (int i = 0; i < elf_hdr.e_phnum; i++, e_phoff += elf_hdr.e_phentsize) {
    if (sys_lseek(file, e_phoff, 0) < 0) {
      log_printf("Read file failed!");
      goto load_failed;
    }

    size = sys_read(file, (char *)&elf_phdr, sizeof(Elf32_Phdr));
    if (size < sizeof(Elf32_Phdr)) {
      log_printf("Read file failed!");
      goto load_failed;
    }

    if ((elf_phdr.p_type != PT_LOAD) || (elf_phdr.p_vaddr < MEM_TASK_BASE))
      continue;

    if ((load_phdr(file, &elf_phdr, page_dir)) < 0) {
      log_printf("Load program failed!");
      goto load_failed;
    }
    /*
     * Set the heap_start and heap_end to the end of the last program header,
     * which is also the end of .bss section.
     * .text .rodata .data .bss [heap ->] [<- stack]
     */
    task->heap_start = task->heap_end = elf_phdr.p_vaddr + elf_phdr.p_memsz;
  }

  sys_close(file);
  return elf_hdr.e_entry;

load_failed:
  if (file)
    sys_close(file);

  return 0;
}

static int copy_args(const char *target, uint32_t page_dir, int argc,
                     char *const *argv) {
  const task_args_t task_args = {.argc = argc,
                                 .argv =
                                     (char **)(target + sizeof(task_args_t)),
                                 .ret_addr = 0};

  const char *dest_arg = target + sizeof(task_args_t) + sizeof(char *) * argc;
  const char **dest_arg_ptr = (const char **)memory_get_paddr(
      page_dir, (uint32_t)(target + sizeof(task_args_t)));

  for (int i = 0; i < argc; i++) {
    const char *src = argv[i];
    const int len = kernel_strlen(src) + 1;
    ASSERT(memory_copy_uvm_data((uint32_t)dest_arg, page_dir, (uint32_t)src,
                                len) >= 0);

    dest_arg_ptr[i] = dest_arg;
    dest_arg += len;
  }

  return memory_copy_uvm_data((uint32_t)target, page_dir, (uint32_t)&task_args,
                              sizeof(task_args));
}

int sys_execve(const char *name, char *const argv[], char *const envp[]) {
  task_t *task = get_curr_task();
  kernel_strncpy(task->name, kernel_basename(name), TASK_NAME_SIZE);

  const uint32_t old_page_dir = task->tss.cr3;
  const uint32_t new_page_dir = memory_create_uvm();
  if (!new_page_dir)
    goto exec_failed;

  const uint32_t entry = load_elf_file(task, name, new_page_dir);
  if (!entry)
    goto exec_failed;

  const uint32_t stack_top = MEM_TASK_STACK_TOP - MEM_TASK_ARG_SIZE;
  const int err = memory_alloc_for_page_dir(
      new_page_dir, MEM_TASK_STACK_TOP - MEM_TASK_STACK_SIZE,
      MEM_TASK_STACK_SIZE, PTE_P | PTE_U | PTE_W);
  if (err < 0)
    goto exec_failed;

  const int argc = strings_cnt(argv);
  if ((copy_args((char *)stack_top, new_page_dir, argc, argv)) < 0)
    goto exec_failed;

  syscall_frame_t *frame =
      (syscall_frame_t *)(task->tss.esp0 - sizeof(syscall_frame_t));
  frame->auto_push.eip = entry;
  frame->manual_push.eax = frame->manual_push.ebx = frame->manual_push.ecx =
      frame->manual_push.edx = 0;
  frame->manual_push.esi = frame->manual_push.edi = frame->manual_push.ebp = 0;
  frame->manual_push.eflags = EFLAGS_IF | EFLAGS_DEFAULT;
  frame->auto_push.esp = stack_top - sizeof(uint32_t) * SYSCALL_ARGC;

  task->tss.cr3 = new_page_dir;
  mmu_set_page_dir(new_page_dir);
  memory_destroy_uvm(old_page_dir);
  return 0;

exec_failed:
  if (!new_page_dir)
    memory_destroy_uvm(new_page_dir);

  return -1;
}

int task_alloc_fd(file_t *file) {
  task_t *task = get_curr_task();
  for (int i = 0; i < TASK_FILE_NUM; i++) {
    if (task->file_table[i] == NULL) {
      task->file_table[i] = file;
      return i;
    }
  }

  return -1;
}

int task_remove_fd(int fd) {
  if (fd < 0 || fd >= TASK_FILE_NUM)
    return -1;

  get_curr_task()->file_table[fd] = NULL;
  return 0;
}

file_t *task_file(int fd) {
  if (fd < 0 || fd >= TASK_FILE_NUM)
    return NULL;

  return get_curr_task()->file_table[fd];
}

void sys_exit(int status) {
  task_t *curr_task = get_curr_task();
  for (int fd = 0; fd < TASK_FILE_NUM; fd++) {
    const file_t *file = curr_task->file_table[fd];
    if (file) {
      sys_close(fd);
      curr_task->file_table[fd] = NULL;
    }
  }

  _Bool child_zombie = FALSE;
  mutex_lock(&task_table_mutex);

  for (int i = 0; i < TASK_FILE_NUM; i++) {
    task_t *task = task_table + i;
    if (task->parent == curr_task)
      task->parent = &task_manager.first_task;

    if (task->state == TASK_ZOMBIE)
      child_zombie = TRUE;
  }

  mutex_unlock(&task_table_mutex);

  const irq_state_t state = irq_protect();

  task_t *parent = curr_task->parent;

  if (child_zombie && parent != &task_manager.first_task &&
      task_manager.first_task.state == TASK_WAITING)
    task_set_ready(&task_manager.first_task);

  if (parent->state == TASK_WAITING)
    task_set_ready(parent);

  curr_task->exit_status = status;
  curr_task->state = TASK_ZOMBIE;
  task_set_block(curr_task);
  task_dispatch();

  irq_unprotect(state);
}

int sys_wait(int *status) {
  task_t *curr_task = get_curr_task();
  while (1) {
    mutex_lock(&task_table_mutex);

    for (int i = 0; i < TASK_NUM; i++) {
      task_t *task = task_table + i;
      if (task->parent != curr_task)
        continue;

      if (task->state == TASK_ZOMBIE) {
        *status = task->exit_status;

        memory_destroy_uvm(task->tss.cr3);
        memory_free_page(task->tss.esp0 - MEM_PAGE_SIZE);
        kernel_memset(task, 0, sizeof(task_t));

        mutex_unlock(&task_table_mutex);
        return task->pid;
      }
    }

    mutex_unlock(&task_table_mutex);

    const irq_state_t state = irq_protect();

    task_set_block(curr_task);
    curr_task->state = TASK_WAITING;
    task_dispatch();

    irq_unprotect(state);
  }

  return 0;
}
