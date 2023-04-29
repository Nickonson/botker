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

#define COMMAND_START_POS (8)
#define COMMAND_START_STRNUM (0)

#define WORDSTAT_STRNUM (2)
#define DICTINFO_STRNUM (4)
#define INFO_STRNUM (5)
#define BADINP_STRNUM (2)
#define BADCOMMAND_STRNUM (2)



#define TOTAL_WORDS (122)

int user_color = 0x02;
int cmd_color = 0x07;

unsigned int current_pos = 0;
unsigned int current_strnum = 0;
unsigned int booter_input_length = 0;

unsigned int chosen_words = 0;

char btrinp[27];
char usrinp[40];
char tmpbuf[40];
char cmd_command[20];
char cmd_arg[30];
int lett_start_point[27];

const char* hello = "Welcome to DistrOS!";
const char* os_info[] = 
{
    "DistrOS: v0.1",
    "Developer: Bilan Nikita (scanel), 53/2, SPbPU, 2023",
    "Compilers: gcc 11.3.0"
};

const char* dict[122][2] =
{
    {"account", "tili"},
    {"add", "lisataan"},
    {"advanced", "kehittynyt"},
    {"available", "saatavilla"},
    {"because", "koska"},
    {"before", "ennen"},
    {"being", "on"},
    {"best", "paras"},
    {"black", "musta"},
    {"box", "laatikko"},
    {"buy", "ostaa"},
    {"call", "soitto"},
    {"case", "tapaus"},
    {"category", "luokka"},
    {"check", "tarkista"},
    {"conditions", "edellytykset"},
    {"copyright", "tekijanoikeudet"},
    {"credit", "luotto"},
    {"data", "tiedot"},
    {"days", "paivat"},
    {"department", "osasto"},
    {"description", "kuvaus"},
    {"detail", "yksityiskohta"},
    {"digital", "digitaalinen"},
    {"english", "englanti"},
    {"estate", "kiinteistot"},
    {"events", "tapahtumat"},
    {"file", "tiedosto"},
    {"form", "lomake"},
    {"full", "taysi"},
    {"gallery", "galleria"},
    {"games", "pelit"},
    {"gay", "homo"},
    {"general", "yleinen"},
    {"hotel", "hotelli"},
    {"hour", "tunti"},
    {"image", "kuva"},
    {"index", "indeksi"},
    {"january", "tammikuu"},
    {"join", "liity"},
    {"know", "tiedetaan"},
    {"large", "suuri"},
    {"last", "viimeinen"},
    {"left", "vasen"},
    {"level", "taso"},
    {"life", "elama"},
    {"line", "linja"},
    {"link", "linkki"},
    {"live", "elava"},
    {"local", "paikallinen"},
    {"look", "katso"},
    {"love", "rakkaus"},
    {"mail", "posti"},
    {"main", "paaasiallinen"},
    {"make", "tee"},
    {"management", "johtaminen"},
    {"map", "kartta"},
    {"member", "member"},
    {"men", "miehet"},
    {"message", "viesti"},
    {"most", "suurin osa"},
    {"much", "paljon"},
    {"music", "musiikki"},
    {"note", "huomautus"},
    {"number", "numero"},
    {"office", "toimisto"},
    {"old", "vanha"},
    {"open", "avoin"},
    {"pages", "sivut"},
    {"photos", "valokuvat"},
    {"please", "olkaa hyva ja"},
    {"point", "kohta"},
    {"policy", "politiikka"},
    {"previous", "edellinen"},
    {"prices", "hinnat"},
    {"product", "tuote"},
    {"profile", "profiili"},
    {"program", "ohjelma"},
    {"project", "hanke"},
    {"register", "rekisteri"},
    {"research", "tutkimus"},
    {"if_same", "tulos"},
    {"reviews", "arvostelut"},
    {"right", "oikeus"},
    {"sales", "myynti"},
    {"same", "sama"},
    {"science", "tiede"},
    {"select", "valitse"},
    {"send", "laheta"},
    {"set", "asetettu"},
    {"shopping", "ostokset"},
    {"should", "jos"},
    {"sign", "allekirjoitus"},
    {"site", "sivusto"},
    {"software", "ohjelmisto"},
    {"south", "etelaan"},
    {"special", "erityinen"},
    {"such", "tallaisia"},
    {"support", "tuki"},
    {"system", "jarjestelma"},
    {"table", "taulukko"},
    {"team", "tiimi"},
    {"technology", "teknologia"},
    {"terms", "termit"},
    {"them", "ne"},
    {"then", "sitten"},
    {"those", "ne"},
    {"thread", "lanka"},
    {"time", "aika"},
    {"title", "nimike"},
    {"today", "nykyaan"},
    {"type", "tyyppi"},
    {"under", "alle"},
    {"university", "yliopisto"},
    {"using", "kayttamalla"},
    {"way", "tapa"},
    {"website", "verkkosivusto"},
    {"week", "viikko"},
    {"west", "Lansi"},
    {"windows", "ikkunat"},
    {"women", "naiset"},
    {"work", "tyo"}
};

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

void init_system();
void intr_start();
void intr_enable();
void intr_disable();

void intr_init();
void keyb_init();
void init_usrinp();
void init_dict();

int find_str_dict(const char* to_find);

static inline unsigned char inb(unsigned short port);
static inline void outb(unsigned short port, unsigned char data);
static inline void outw(unsigned short port, unsigned short data);

void handle_booter_inp();
void put_usrinp_to_tmpbuf();

int strcmp(const char* str1, const char* str2);
int strlen(const char* str1);
void mov_to_str(char *buffer, const char* source, int index);

void print_symbol(unsigned char scan_code);
void backspace_key();
void enter_key();

void itoa(char buffer[10], int numb);

void command_handler();
void info_command();
void dictinfo_command();
void wordstat_command();
void translate_command();
void unknown_command();
void bad_input_command();
void clr_scr();
void clr_if_tigth(int strnums);
static inline void shutdown_command();

void out_str(int color, const char* ptr, unsigned int strnum);

void default_intr_handler();

void intr_reg_handler(int num, unsigned short segm_sel, unsigned short flags, intr_handler hndlr);
void cursor_moveto(unsigned int strnum, unsigned int pos);
void keyb_handler();
void keyb_process_keys();

void endless_loop();



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
    usrinp[current_pos] = c;
    current_pos++;
    usrinp[current_pos] = '\0';
    out_str(user_color, usrinp, current_strnum);
    cursor_moveto(current_strnum, current_pos);
}

void mov_to_str(char *buffer, const char* source, int index)
{
    int start_index = index;

    if(index != 0)
    {
        buffer[start_index] = ' ';
        start_index++;
    }
    int i = 0;
    while(source[i] != '\0')
    {
        buffer[start_index + i] = source[i];
        i++;
    }
    buffer[start_index + i] = '\0';
}

int strcmp(const char* str1, const char* str2)
{
    while (*str1 == *str2 && *str1 != '\0' && *str2 != '\0')
    {
        str1++;
        str2++;
    }
    if (*str1 == '\0' && *str2 == '\0')
        return 0;
    else if(*str1 < *str2)
        return 1;
    return -1;
}

int strlen(const char* str1)
{
    int if_same = 0;
    while(str1[if_same] != '\0')
        if_same++;
    return if_same;
}

void backspace_key()
{
    if (current_pos > COMMAND_START_POS)
    {
        usrinp[current_pos] = '\0';
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
    current_pos = COMMAND_START_POS;
    usrinp[current_pos] = '\0';
    out_str(user_color, usrinp, current_strnum);
    cursor_moveto(current_strnum, current_pos);
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

void keyb_handler()
{
    asm("pusha");

    // mgmt income data
    keyb_process_keys();

    // send 8259 control notif about itnr processed
    outb(PIC1_PORT, 0x20);
    asm("popa; leave; iret");
}

void put_usrinp_to_tmpbuf()
{
    int i = COMMAND_START_POS;
    int command_start;
    
    while(i < current_pos && usrinp[i] == ' ')
        i++;

    command_start = i;
    while(i < current_pos && usrinp[i] != ' ')
    {
        cmd_command[i - command_start] = usrinp[i];
        i++;
    }
    cmd_command[i - command_start] = '\0';
        
    while(i < current_pos && usrinp[i] == ' ')
        i++;
    command_start = i;
    while(i < current_pos && usrinp[i] != ' ')
    {
        cmd_arg[i - command_start] = usrinp[i];
        i++;
    }
    cmd_arg[i - command_start] = '\0';
}

void itoa(char buffer[10], int numb)
{
    if(numb != 0)
    {
        char tmpbuf[10];
        int i = 0, j = 0;
        while(numb > 0)
        {
            tmpbuf[i] = numb % 10 + '0';
            numb /= 10;
            i++;
        }
        i--;
        while(i >= 0)
        {
            buffer[j] = tmpbuf[i];
            j++;
            i--;
        }
        buffer[j] = '\0';
    }
    else
    {
        buffer[0] = '0';
        buffer[1] = '\0';
    }
}

void command_handler()
{
    unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
    video_buf += 2 * (current_strnum * VIDEO_WIDTH + 2);

    put_usrinp_to_tmpbuf();

    current_strnum++;
    current_pos = 0;

    if(strcmp(cmd_command, "clear\0") == 0)
        clr_scr();
    else if (strcmp(cmd_command, "translate\0") == 0)
        translate_command();
    else if (strcmp(cmd_command, "info\0") == 0)
        info_command();
    else if(strcmp(cmd_command, "dictinfo\0") == 0)
        dictinfo_command();
    else if(strcmp(cmd_command, "wordstat\0") == 0)
        wordstat_command();
    else if (strcmp(cmd_command, "shutdown\0") == 0)
        shutdown_command();
    else
        unknown_command();
}

void info_command()
{
    clr_if_tigth(INFO_STRNUM);

    for(int i = 0; i < 3; i++, current_strnum++)
        out_str(cmd_color, os_info[i], current_strnum);

    mov_to_str(tmpbuf, "Chosen letters:", 0);
    mov_to_str(tmpbuf, btrinp, strlen("Chosen letters:"));
    out_str(cmd_color, tmpbuf, current_strnum++);
}

void unknown_command()
{
    clr_if_tigth(BADCOMMAND_STRNUM);

    out_str(cmd_color, "Error: command not recognized", current_strnum++);
}

void dictinfo_command()
{
    clr_if_tigth(DICTINFO_STRNUM);

    char temp_string[30];
    char buffer[10];
    out_str(cmd_color, "Dictionary: en -> fi", current_strnum++);

    mov_to_str(temp_string, "Number of words", 0);
    itoa(buffer, 122);
    mov_to_str(temp_string, buffer, strlen("Number of words"));
    out_str(cmd_color, temp_string, current_strnum++);

    mov_to_str(temp_string, "Number of loaded words", 0);
    itoa(buffer, chosen_words);
    mov_to_str(temp_string, buffer, strlen("Number of loaded words"));
    out_str(cmd_color, temp_string, current_strnum++);
}

void wordstat_command()
{
    if(strlen(cmd_arg) != 1)
    {
        bad_input_command();
        return;
    }
    clr_if_tigth(WORDSTAT_STRNUM);

    int if_same = 0;
    char temp_string[30];
    char buffer[10];

    char char_to_find = cmd_arg[0];
 
    if(!(lett_start_point[char_to_find - 'a'] == -1 || 
        lett_start_point[char_to_find - 'a'] == lett_start_point[char_to_find - 'a' + 1] ||
        lett_start_point[char_to_find - 'a' + 1] == -1))
        if_same = lett_start_point[char_to_find - 'a' + 1] - lett_start_point[char_to_find - 'a'];
    
    mov_to_str(temp_string, "Chosen words:", 0);
    itoa(buffer, if_same);
    mov_to_str(temp_string, buffer, strlen("Chosen words:"));
    out_str(cmd_color, temp_string, current_strnum++);
}

void translate_command()
{
    clr_if_tigth(2);
    int strind = find_str_dict(cmd_arg);

    if(strind == -1)
    {
        out_str(cmd_color, "Such word doesnt exist", current_strnum++);
    }
    else
    {
        mov_to_str(tmpbuf, cmd_arg, 0);
        mov_to_str(tmpbuf, ":", strlen(tmpbuf));
        mov_to_str(tmpbuf, dict[strind][1], strlen(tmpbuf));
        out_str(cmd_color, tmpbuf, current_strnum++);
    }
}

void bad_input_command()
{
    clr_if_tigth(BADINP_STRNUM);

    out_str(cmd_color, "Error: bad input", current_strnum++);
}

static inline void shutdown_command()
{
    outw (0x604, 0x2000);
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
    unsigned char* video_buf = (unsigned char*) VIDEO_BUF_PTR;
    for (int i = 0; i <= VIDEO_WIDTH * VIDEO_HEIGHT * 2; i++)
        video_buf[i] = 0;
    current_pos = COMMAND_START_POS;
    current_strnum = COMMAND_START_STRNUM;
}

void clr_if_tigth(int strnums)
{
    if(current_strnum + strnums >= VIDEO_HEIGHT)
    {
        clr_scr();
        current_strnum = 0;
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

void handle_booter_inp()
{
    char *ptr = (char *)0x9000;
    
    for(int i = 0; i < 26; i++)
    {
        if(*ptr != '_')
        {
            btrinp[booter_input_length] = *ptr;
            booter_input_length++;
        }
        ptr++;
    }
    btrinp[booter_input_length] = '\0';
}

void init_usrinp()
{
    usrinp[0] = 's';
    usrinp[1] = 'c';
    usrinp[2] = 'a';
    usrinp[3] = 'n';
    usrinp[4] = 'e';
    usrinp[5] = 'l';
    usrinp[6] = '#';
    usrinp[7] = ' ';
    current_pos = COMMAND_START_POS;
    current_strnum = COMMAND_START_STRNUM;
    out_str(user_color, usrinp, current_strnum);
    cursor_moveto(current_strnum, current_pos);
}

void init_dict()
{
    for(int i = 0; i < 27; i++)
        lett_start_point[i] = -1;

    for(int i = 0; i < booter_input_length; i++)
    {
        int l, r, mid, to_find;
        l = 0;
        r = TOTAL_WORDS;
        mid =  l / 2 + r / 2;
        to_find = btrinp[i];

        while(dict[l][0][0] != to_find)
        {
            if(r <= l)
            {
                l = -1;
                break;
            }
            if(dict[mid][0][0] < to_find)
                l = mid + 1;
            else
                r = mid;
            mid = l / 2 + r / 2;
        }
        
        //init hash_table for alphabet symbols
        if(l == -1)
        {
            lett_start_point[to_find - 'a'] = -1;
        }
        else
        {
            while(l > 0 && dict[l-1][0][0] == to_find)
                l--;
            lett_start_point[to_find - 'a'] = l;

            while(l < TOTAL_WORDS - 1 && dict[l+1][0][0] == to_find)
                l++;
            lett_start_point[to_find - 'a' + 1] = l + 1;
            chosen_words += lett_start_point[to_find - 'a' + 1] - lett_start_point[to_find - 'a'];
        }
    }

}

int find_str_dict(const char* to_find)
{
    int l, r, mid, result;
    l = 0;
    r = TOTAL_WORDS;
    mid =  l / 2 + r / 2;
    int if_same = strcmp(dict[mid][0], to_find);
    while(strcmp(dict[l][0], to_find) != 0)
    {
        if(r <= l)
        {
            result = -1;
            return result;
        }
        else if(if_same == 1)
            l = mid + 1;
        else
            r = mid;
        mid = l / 2 + r / 2;
        if_same = strcmp(dict[mid][0], to_find);
    }
    result = l;
    return result;
}

void init_system()
{
    intr_disable();

    intr_init();
    keyb_init();
    handle_booter_inp();  
    init_dict();
    init_usrinp();  

    intr_start();
    intr_enable();
}

void endless_loop()
{
    while(1)
    {
        asm("hlt");
    }
}

extern "C" int kmain()
{
    init_system();

    endless_loop();
    return 0; 
}