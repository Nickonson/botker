/*
    this instr mus be first cuz this code compiles to bin
    and booter pass mgmt by addr of first instr of OS kernel image binary
*/
__asm("jmp kmain");

#define VIDEO_BUF_PTR (0xb8000)

#define IDT_TYPE_INTR (0x0E)
#define IDT_TYPE_TRAP (0x0F)

// Selector of code section established by OS booter
#define GDT_CS        (0x08)

#define PIC1_PORT     (0x20)
#define CURSOR_PORT   (0x3D4)

#define VIDEO_WIDTH   (80)

struct idt_entry
{
    unsigned short base_lo;     // low bits of handler addr
    unsigned short segm_sel;    // selector of code seg
    unsigned char always0;      // this bite = 0 alw
    unsigned char flags;        // P, Dpl, types - consts of IDT_TYPE
    unsigned short base_hi;     // high bits;
}__attribute__((packed));    // alingment is forbidden, fields dont grow to be the same as others

// addr of this struct is arg to `lidt` command
struct idt_ptr
{
    unsigned short limit;
    unsigned int base;
}__attribute__((packed)); 

struct idt_entry g_idt[256];    // real IDT table
struct idt_ptr g_idtp;          // table descriptor for `lidt`

typedef void (*intr_handler)();

void intr_start();
void intr_enable();
void intr_disable();

void intr_init();
void keyb_init();

static inline unsigned char inb(unsigned short port);
static inline void outb(unsigned short port, unsigned char data);
static inline void outw(unsigned short port, unsigned short data);
void out_str(int color, const char* ptr, unsigned int strnum);
void clr_scr();
static inline void shutdown();

void default_intr_handler();

void intr_reg_handler(int num, unsigned short segm_sel, unsigned short flags, intr_handler hndlr);
void cursor_moveto(unsigned int strnum, unsigned int pos);
void keyb_handler();
void keyb_process_keys();


void default_intr_handler()
{
      asm("pusha");
      // ... (реализация обработки)
      asm("popa; leave; iret");
}

static inline unsigned char inb(unsigned short port)
{
    unsigned char data;
    asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void outb(unsigned short port, unsigned char data)
{
    asm volatile("outb %b0, %w1" : : "a" (data), "Nd" (port));
}

static inline void outw(unsigned short port, unsigned short data)
{
    asm volatile("outw %0, %1" : : "a" (data), "d" (port));
}

void intr_reg_handler(int num, unsigned short segm_sel, 
                      unsigned short flags, intr_handler hndlr)
{
    unsigned int hndlr_addr = (unsigned int) hndlr;

    g_idt[num].base_lo = (unsigned short) (hndlr_addr & 0xFFFF); 
    g_idt[num].segm_sel = segm_sel;
    g_idt[num].always0 = 0;
    g_idt[num].flags = flags;
    g_idt[num].base_hi = (unsigned short) (hndlr_addr >> 16);
}

// load g_idt with handlers addrs
void intr_init()
{
    int i;
    int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);

    for(i = 0; i < idt_count; i++)
    {
        intr_reg_handler(i, GDT_CS, 0x80 | IDT_TYPE_INTR,
                         default_intr_handler);
                         // segm_sel == 0x9, P = 1, DPL = 0, Type = Intr
    }
}

void keyb_init()
{
    // registration intr handler
    intr_reg_handler(0x09, GDT_CS, 0x80 | IDT_TYPE_INTR, keyb_handler);
    // segm_sel = 0x8, P = 1, DPL = 0, Type = Intr

    outb(PIC1_PORT + 1, 0xFF ^ 0x02);
    // 0xFF - all intrs
    // 0x02 - bit IRQ1 (keyboard)
    // only intrs with bit == 0 allowed
}

// not ready
void keyb_process_keys()
{

    // // checking if buff PS/2 keyb != empty (lo bit exists)
    // if(inb(0x64) & 0x01)
    // {
    //     unsigned char scan_code;
    //     unsigned char state;

    //     scan_code = inb(0x60);      // read symb from PS/2 keyb

    //     if(scan_code < 128)
    //         on_key(scan_code);
    // }
}

void keyb_handler()
{
    asm("pusha");

    // mgmt income data
    keyb_process_keys();

    // send 8259 control notif about itnr processed
    outb(PIC1_PORT, 0x20);
    asm("popa; leave; iret");
}

void intr_start()
{
    int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);

    g_idtp.base = (unsigned int) (&g_idt[0]);
    g_idtp.limit = (sizeof(struct idt_entry) * idt_count) - 1;

    asm("lidt %0" : : "m" (g_idtp));
}

void intr_enable()
{
    asm("sti");
}

void intr_disable()
{
    asm("cli");
}

void out_str(int color, const char* ptr, unsigned int strnum) 
{
    unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
    video_buf += 80*2 * strnum;
    while (*ptr)
    {
        video_buf[0] = (unsigned char) *ptr; // symb (code)
        video_buf[1] = color;                // colour of symb and background
        video_buf += 2;
        ptr++; 
    }
}

void clr_scr() 
{
    unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
    for(int i = 0; i < 2000; i++)
    {
        video_buf[0] = ' ';
        video_buf[1] = 0x07;
        video_buf += 2;
    }
}

void cursor_moveto(unsigned int strnum, unsigned int pos)
{
    unsigned short new_pos = (strnum * VIDEO_WIDTH) + pos;
    outb(CURSOR_PORT, 0x0F);
    outb(CURSOR_PORT + 1, (unsigned char)(new_pos & 0xFF));
    outb(CURSOR_PORT, 0x0E);
    outb(CURSOR_PORT + 1, (unsigned char)( (new_pos >> 8) & 0xFF));
}

static inline void shutdown()
{
    outw (0x604, 0x2000);
}

const char* g_test = "This is test string.";

extern "C" int kmain()
{
    intr_init();
    intr_start();

    const char* hello = "Welcome to DistrOS!";
    const char* os_info[] = 
    {
        "DistrOS: v0.1",
        "Developer: Bilan Nikita (scanel), 53/2, SPbPU, 2023",
        "Compilers: gcc 11.3.0"
    };

    // printin str out
    out_str(0x07, hello, 0);
    for(int i = 0; i < 3; i++)
        out_str(0x07, os_info[i], i + 1);

    while(1)
    {
        asm("hlt");
    }
    return 0; 
}