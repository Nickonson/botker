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
#define VIDEO_HEIGHT (25)

#define MAX_COMMAND_LENGTH (40)


int user_color = 0x02;
unsigned int current_pos = 0;
unsigned int current_strnum = 0;

char usrinp[27];

char scan_code_to_symbol[] = {
    0, 0,
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    0, 0, 0, 0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    0, 0, 0, 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    0, 0, 0, 0, 0,
    'z', 'x', 'c', 'v', 'b', 'n', 'm',
    0, 0,
    '/',
    0,
    '*',
    0,
    ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-',
    0, 0, 0,
    '+'
};

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

bool strcmp(unsigned char* str1, const char* str2);
void print_symbol(unsigned char scan_code);
void on_key(unsigned char scan_code);
void backspace_key();
void enter_key();

void command_handler();
void info_command();
void unknown_command();
static inline void shutdown_command();

void out_str(int color, const char* ptr, unsigned int strnum);
void clr_scr();

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

void print_symbol(unsigned char scan_code)
{
    char c = scan_code_to_symbol[scan_code];
    if (c == 0)
        return;
    out_str(user_color, &c, 1);
    current_pos++;
    cursor_moveto(current_strnum, current_pos);
}

bool strcmp(unsigned char* str1, const char* str2)
{
    while (*str1 == *str2 && *str1 != 0 && *str1 != ' ' && *str2 != 0)
    {
        str1 += 2;
        str2++;
    }
    if (*str1 == *str2)
        return true;
    return false;
}

void backspace_key()
{
    if (current_pos >= 3)
    {
        current_pos--;
        unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
        video_buf += 2 * (current_strnum * VIDEO_WIDTH + current_pos);
        video_buf[0] = 0;
        cursor_moveto(current_strnum, current_pos);
    }
}

void enter_key()
{
    command_handler();
    current_pos = 0;
    out_str(user_color, "# ", 0);
    current_pos = 2;
    cursor_moveto(current_strnum, 2);
}

void keyb_process_keys()
{

    // checking if buff PS/2 keyb != empty (lo bit exists)
    if(inb(0x64) & 0x01)
    {
        unsigned char scan_code;
        unsigned char state;

        scan_code = inb(0x60);      // read symb from PS/2 keyb
        if (scan_code < 79)
        {
            if (scan_code == 14)
                backspace_key();
            else if (scan_code == 28)
                enter_key();
            else if (current_pos - 2 < MAX_COMMAND_LENGTH)
                print_symbol(scan_code);
        }
    }
}

void on_key(unsigned char scan_code)
{
    
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

void command_handler()
{
    unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
    video_buf += 2 * (current_strnum * VIDEO_WIDTH + 2);

    current_strnum++;
    current_pos = 0;

    if (strcmp(video_buf, "info"))
        info_command();
    // else if (strcmp(video_buf, "expr "))
    //     expr_command(video_buf + 10);
    else if (strcmp(video_buf, "shutdown"))
        shutdown_command();
    else
        unknown_command();

    current_strnum += 2;
}

void info_command()
{

}

void unknown_command()
{
    if (current_strnum + 2 >= VIDEO_HEIGHT)
    {
        clr_scr();
        current_strnum = 0;
    }
    out_str(user_color, "Error: command not recognized", 0);
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

// void clr_scr() 
// {
//     unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
//     for(int i = 0; i < 2000; i++)
//     {
//         video_buf[0] = ' ';
//         video_buf[1] = 0x07;
//         video_buf += 2;
//     }
// }

void clr_scr()
{
    unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
    for (int i = 0; i <= VIDEO_WIDTH * VIDEO_HEIGHT * 2; i++)
        video_buf[i] = 0;
}

void cursor_moveto(unsigned int strnum, unsigned int pos)
{
    unsigned short new_pos = (strnum * VIDEO_WIDTH) + pos;
    outb(CURSOR_PORT, 0x0F);
    outb(CURSOR_PORT + 1, (unsigned char)(new_pos & 0xFF));
    outb(CURSOR_PORT, 0x0E);
    outb(CURSOR_PORT + 1, (unsigned char)( (new_pos >> 8) & 0xFF));
}

static inline void shutdown_command()
{
    outw (0x604, 0x2000);
}


extern "C" int kmain()
{
    intr_init();
    intr_start();
    keyb_init();

    char *ptr = (char *)0x9000;
    // if(*ptr == *ptr)
    //     out_str(0x07, ptr, 0);
    // else
    //     out_str(0x07, "0", 0);
    for(int i = 0; i < 26; i++)
    {
        usrinp[i] = *ptr;
        ptr++;
    }
    usrinp[26] = '\0';
    out_str(0x07, usrinp, 0);

    const char* hello = "Welcome to DistrOS!";
    const char* os_info[] = 
    {
        "DistrOS: v0.1",
        "Developer: Bilan Nikita (scanel), 53/2, SPbPU, 2023",
        "Compilers: gcc 11.3.0"
    };

    // printin str out
    // out_str(0x07, hello, 0);
    // for(int i = 0; i < 3; i++)
    //     out_str(0x07, os_info[i], i + 1);

    while(1)
    {
        asm("hlt");
    }
    return 0; 
}