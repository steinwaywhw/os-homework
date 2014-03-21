#define COLUMNS                 80
#define LINES                   24
#define ATTRIBUTE               7
#define VIDEO_RAM               0xB8000

static unsigned int xpos;
static unsigned int ypos;
static volatile unsigned char *video;

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;

#define MAGIC 0x1BADB002

#define OFFSET_MMAP_LENGTH  44
#define OFFSET_MMAP_ADDR    48

#define MULTIBOOT_MEMORY_AVAILABLE              1
#define MULTIBOOT_MEMORY_RESERVED               2

struct mmap_entry_t
{
    u32 size;
    u32 addr_low;
    u32 addr_high;
    u32 len_low;
    u32 len_high;
    u32 type;
} __attribute__((packed));

static void cls (void)
{
    int i;

    video = (unsigned char *) VIDEO_RAM;

    for (i = 0; i < COLUMNS * LINES * 2; i++)
        *(video + i) = 0;

    xpos = 0;
    ypos = 0;
}

static void itoa (char *buf, char base, u32 d)
{
    char *p = buf;
    
    int divisor;
    divisor = (base == 'd') ? 10 : 16;  // %d and %x

    if (d < 0)
    {
        *p = '-';
        itoa (buf + 1, base, -d);
        return;
    }

    u64 target = d;

    do
    {
        int remainder = target % divisor;
        *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    } while (target /= divisor);

    *p = 0;

    char *p1, *p2;
    p1 = buf;
    p2 = p - 1;

    while (p1 < p2)
    {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
    }
}


static inline void newline ()
{
    xpos = 0;
    ypos++;
    if (ypos >= LINES)
        ypos = 0;
    return;
}



static void putchar (u8 c)
{
    if (c == '\n' || c == '\r')
    {
        newline ();
        return;
    }

    *(video + (xpos + ypos * COLUMNS) * 2) = c;
    *(video + (xpos + ypos * COLUMNS) * 2 + 1) = ATTRIBUTE;

    xpos++;
    if (xpos >= COLUMNS)
        newline ();
}

static inline void putstr (u8 *s)
{
    while (*s != 0)
        putchar (*s++);
}

void printf (const char *format, ...)
{
    
    char buf[20];

    char **arg = (char **)&format;
    arg++;

    char c;
    while ((c = *format++) != 0)
    {
        if (c != '%')
            putchar (c);
        else
        {
            char *p;
            c = *format++;

            switch (c)
            {
                case 'd':
                case 'x':
                    itoa (buf, c, *((u32 *)arg++));
                    putstr (buf);
                    break;  

                case 's':
                    p = *arg++;
                    if (!p)
                        p = "(null)";

                    putstr (p);
                    break;

                case 'c':
                default:
                    putchar (*((u8 *) arg++));
                    break;
            }
        }
    }
}


extern void kentry (unsigned long magic, unsigned long addr)
{
    u32 mmap_len = *((u32*)(((u8*)addr) + OFFSET_MMAP_LENGTH)) / sizeof (struct mmap_entry_t);
    struct mmap_entry_t *mmap = (struct mmap_entry_t *)(*((u32*)(((u8*)addr) + OFFSET_MMAP_ADDR)));

    cls ();

    printf ("Welcome to MemOS written by Steinway Wu and Ming Chen!\n");
    printf ("mmap_addr = 0x%x, mmap_length = %d\n", mmap, mmap_len);
    int i;
    for (i = 0; i < mmap_len; i++)
    {
        printf ("base_addr = 0x%x%x, length = 0x%x%x, type = 0x%x\n",
            (unsigned) mmap->addr_high,
            (unsigned) mmap->addr_low,
            (unsigned) mmap->len_high,
            (unsigned) mmap->len_low,
            (unsigned) mmap->type
        );
        mmap++;
    }
        
}
