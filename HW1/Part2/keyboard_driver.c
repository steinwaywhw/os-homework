
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h> /* error codes */
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/interrupt.h>    /* We want an interrupt */
#include <asm/io.h>
#include <linux/irq.h>
// #include <linux/signal.h>

//#define DEBUG

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hanwen Wu steinwaywhw@gmail.com");
MODULE_DESCRIPTION("Simple Keyboard Dirver");


/***************************************************
    Data structure
 **************************************************/

// For IOCTL
struct keyboard_command_t
{
    int length;
    char* buffer;
};


// For my_get_string() or so
#define BUFFER_SIZE 256

struct keyboard_buffer_t
{
    bool is_empty;
    int head;
    int tail;
    char buffer[BUFFER_SIZE];
};

/***************************************************
    Define
 **************************************************/

// For IOCTL
#define DRIVER_TYPE     0xcc
#define CMD_GETCHAR     _IOR(DRIVER_TYPE, 0, struct keyboard_command_t)
#define CMD_GETCHARS    _IOR(DRIVER_TYPE, 1, struct keyboard_command_t)

#define PROC_ENTRY      "simple_keyboard"

/***************************************************
    Globals
 **************************************************/

static struct file_operations keyboard_operations;
static struct proc_dir_entry *proc_entry;
static struct keyboard_buffer_t buffer;

/***************************************************
    Output
 **************************************************/

// For remembering active tty pointer.
// After entering interrupt handler, I can never get an active tty,
// because there isn't one for the handler process. So
// I need a way to save it before request_irq()
struct tty_struct *tty;

// Print to active tty
// Convert \n to \n\r 
static void to_tty_string(char *string)
{
    // \010 = backspace
    if (tty != NULL)
    {
        (*tty->driver->ops->write)(tty, string, strlen(string));
    }
}

// Print to active tty
// Convert \n to \n\r 
static void to_tty_char(char ch)
{   
    #ifdef DEBUG
        to_tty_string("Printing to tty.\n\r");
    #endif

    static char string[3] = {'\0', '\0', '\0'};
    string[0] = ch;

    if (ch == '\n') 
    {
        string[1] = '\r';
    }
    else
        string[1] = '\0';

    printk(KERN_INFO "##### %c #####\n", ch);
    to_tty_string(string);
}

/***************************************************
    Handler
 **************************************************/

// IOCTL handler
// This could work for my_get_char or so. But it is not used
// since I print chars directly to tty
static long keyboard_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    // struct keyboard_command_t ioc;

    // switch (cmd)
    // {

    // case CMD_GETCHAR:
    //     copy_from_user(&ioc, (struct keyboard_command_t *)arg, sizeof(struct keyboard_command_t));
    //     printk("<1> ioctl: call to IOCTL_TEST (%d,%c)!\n",
    //            ioc.field1, ioc.field2);

    //     my_printk ("Got msg in kernel\b\n");
    //     break;

    // default:
    //     return -EINVAL;
    //     break;
    // }

    return 0;
}


/***************************************************
    Handle Scancode
 **************************************************/
#define BIT_PRESS       0x80
#define BIT_CODE        0x7f

#define SCAN_BACK       0x0e
#define SCAN_ENTER      0x1c
#define SCAN_CTRL_L     0x1d
#define SCAN_SHIFT_L    0x2a
#define SCAN_SHIFT_R    0x36
#define SCAN_ALT_L      0x38
#define SCAN_CAP        0x3a
#define SCAN_ESCAPE     0xe0

#define SCAN_CTRL_R         SCAN_CTRL_L
#define SCAN_SHIFT_FAKE_L   SCAN_SHIFT_L
#define SCAN_SHIFT_FAKE_R   SCAN_SHIFT_R
#define SCAN_ALT_R          SCAN_ALT_L


char plain_map[] = {
    0x00, // 00 = Error
    0x1b, // 01 = ESC
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    '\010', // 0E = BACK // ?0x7f
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 
    0x00, // 1D = LEFT CTRL 
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',  ';', '\'', '`',
    0x00, // 2A = LEFT SHIFT
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0x00, // 36 = RIGHT SHIFT
    0x00, // 37 = NOT SUPPORT
    0x00, // 38 = LEFT ALT
    ' ', 

    0x00, // 3A = CAPS LOCK
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 3B~44 = F1~F10
    
    // NOT SUPPORTED
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

char shift_map[] = {
    // ASCII: SCANCODE = KEY
    0x00, // 00 = Error
    0x00, // 01 = ESC
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
    0x00, // 0E = BACK // ?0x7f
    0x00, 
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 
    0x00, // 1C = ENTER // ?0x01
    0x00, // 1D = LEFT CTRL 
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', // 1E~26 = 'asdfghjkl'
    ':', // 27 = ';'
    '\"', // 28 = '
    '~', // 29 = `
    0x00, // 2A = LEFT SHIFT
    '|', 
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '\?',
    0x00, // 36 = RIGHT SHIFT
    0x00, // 37 = NOT SUPPORT
    0x00, // 38 = LEFT ALT
    ' ', 

    0x00, // 3A = CAPS LOCK
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 3B~44 = F1~F10
    
    // NOT SUPPORTED
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

char scancode_to_ascii(char scancode, bool shift) 
{

    char data = scancode & BIT_CODE;
    char ch = shift ? shift_map[data] : plain_map[data];

    printk(KERN_INFO "Get scancode %02x, shift %d, ascii %c\n", data, (int)shift, ch);

    return ch;
}

// For remembering keyboard status
static struct keyboard_status_t
{
    char last_scancode;
    char current_scancode;
    char current_ascii;

    bool press;
    bool shift;
    bool ctrl;
    bool caps;
    bool alt;
} keyboard_status;

// To decide if the scancode is supported 
// by my very simple driver. If supported, return true
bool inline is_data_scancode(char scancode_7, bool shift) 
{
    if (scancode_7 <= 0 || scancode_7 >= 128)
        return false;
    if (!shift && plain_map[(int)scancode_7] == 0)
        return false;
    if (shift && shift_map[(int)scancode_7] == 0)
        return false;

    return true;
}

bool inline is_escape_scancode(char scancode_7) {return scancode_7 == SCAN_ESCAPE;}
bool inline is_press(char scancode_8) {return !(scancode_8 & BIT_PRESS);}
char inline get_scancode_7(char scancode_8) {return scancode_8 & BIT_CODE;}

// To decide if the scancode is supported 
// by my very simple driver. If not supported, return true
bool inline should_ignore(char scancode_8)
{   
    #ifdef DEBUG
    to_tty_string("[simple_keyboard.c] Entering should_ignore.\n\r");
    #endif

    char scancode_7 = get_scancode_7(scancode_8);

    switch (scancode_7) {
        case SCAN_CAP:
        case SCAN_BACK:
        case SCAN_ALT_L:
        case SCAN_CTRL_L:
        case SCAN_SHIFT_L:
        case SCAN_SHIFT_R:
            return true;
        default:
            break;
    }

    // // repeat, not supported yet
    // if (scancode_7 == keyboard_status.last_scancode) 
    // {    #ifdef DEBUG
    //     to_tty_string("[simple_keyboard.c] Repeat code.\n\r");
    //     return true;
    // }

    // escaped scancode, wait for next
    if (is_escape_scancode(scancode_7))
    {   
        #ifdef DEBUG
        to_tty_string("[simple_keyboard.c] Escape code.\n\r");
        #endif
        return true;
    }

    // not supported scancode, ignore
    if (!is_data_scancode(scancode_7, keyboard_status.shift))
    {   
        #ifdef DEBUG
        to_tty_string("[keyboard_driver.c]: Not supported code.\n\r");
        #endif
        return true;
    }

    #ifdef DEBUG
    to_tty_string("[keyboard_driver.c]: Not ignore.\n\r");
    #endif
    return false;
}


// Handle scancode
static void handle_scancode(char scancode_8) 
{
    //printk(KERN_INFO "Got scancode %02x in handle_scancode.\n", scancode_8);

    if (should_ignore(scancode_8))
        return;

    #ifdef DEBUG
    to_tty_string("[simple_keyboard.c] Handling scancode.\n\r");
    #endif

    // save last scancode
    keyboard_status.last_scancode = keyboard_status.current_scancode;

    // update press status
    keyboard_status.press = is_press(scancode_8);

    // update current scancode
    char scancode_7 = get_scancode_7(scancode_8);


    keyboard_status.current_scancode = scancode_7;

    // update control scancode
    if (scancode_7 == SCAN_CAP && keyboard_status.press) 
    {   
        #ifdef DEBUG
        to_tty_string("[simple_keyboard.c] CAPSLOCK.\n\r");
        #endif
        keyboard_status.caps = !keyboard_status.caps;
    }
        

    if (scancode_7 == SCAN_ALT_L || scancode_7 == SCAN_ALT_R)
    {   
        #ifdef DEBUG
        to_tty_string("[simple_keyboard.c] ALT.\n\r");
        #endif

        if (keyboard_status.press)
            keyboard_status.alt = true;
        else
            keyboard_status.alt = false;
        return;
    }

    if (scancode_7 == SCAN_CTRL_L || scancode_7 == SCAN_CTRL_R)
    {   
        #ifdef DEBUG
        to_tty_string("[simple_keyboard.c] CTLR.\n\r");
        #endif

        if (keyboard_status.press)
            keyboard_status.ctrl = true;
        else
            keyboard_status.ctrl = false;
        return;
    }

    if (scancode_7 == SCAN_SHIFT_L || scancode_7 == SCAN_SHIFT_R)
    {   
        #ifdef DEBUG
        to_tty_string("[simple_keyboard.c] SHIFT.\n\r");
        #endif

        if (keyboard_status.press)
            keyboard_status.shift = true;
        else
            keyboard_status.shift = false;
        return;
    }

    #ifdef DEBUG
    to_tty_string("[simple_keyboard.c] Normal Key.\n\r");
    #endif

    if (!keyboard_status.press)
        return;

    // update keycode
    keyboard_status.current_ascii = scancode_to_ascii(scancode_7, keyboard_status.shift);
    to_tty_char(keyboard_status.current_ascii);
}




/***************************************************
    Interrupt Handler
 **************************************************/

//#define I8042_COMMAND_REG 0x64
#define I8042_STATUS_REG    0x64
#define I8042_DATA_REG      0x60
#define I8042_KEYBOARD_IRQ  1

#define FLAG_ERR_KEYS   0x00 //00h: Keyboard error, too many keys are being pressed at once
#define FLAG_BAT_END    0xaa //aah: Basic Assurance Test (BAT) end
#define FLAG_MFII_1     0xab //abh 41h: The result of requesting keyboard ID on a MF II keyboard
#define FLAG_MFII_2     0x41 
#define FLAG_ECHO       0xee //eeh: The result of the echo command
#define FLAG_ACK        0xfa //fah: ACK(noledge). Sent by every command, except eeh and feh
#define FLAG_BAT_FAIL   0xfc //fch: BAT failed
#define FLAG_RESEND     0xfe //feh: Resend your data please
#define FLAG_ERR        0xff //ffh: Keyboard error

#define BIT_PRESS_RELEASE   0x80 //1000 0000
#define BIT_SCANCODE        0x7F //0111 1111

/***************************************************
    i8042 Hacking Helpers
 **************************************************/

// It's an unexported symbol in the kernel
// For hacking usage
typedef struct irq_desc *(*irq_to_desc_t) (unsigned int irq);
#define IRQ_TO_DESC_ENTRY 0xc10be5a0    //THIS IS THE ENTRY POINT FOR irq_to_desc(); Get it by cat System.map | grep irq_to_desc.

static irq_handler_t    i8042_handler;  // Keep it for recovery use
static void            *i8042_dev_id;   // Keep it for recovery use

// When free_irq(), I need a dev_id of the original i8042 handler
// Then I will need the hacked symbol to get irq_desc->irq_action->dev_id
void remove_18042_irq()
{
    printk(KERN_INFO "Removing i8042 IRQ handler.\n");

    struct irq_desc *ptr     = (*((irq_to_desc_t)IRQ_TO_DESC_ENTRY))  (I8042_KEYBOARD_IRQ);
    struct irqaction *action = ptr->action;

    i8042_handler = action->handler;
    i8042_dev_id = action->dev_id;
    free_irq(I8042_KEYBOARD_IRQ, i8042_dev_id);
    
    printk(KERN_INFO "i8042 IRQ handler removed.\n");
    printk(KERN_INFO "Handler: %x\t\tDevID: %x\n", (unsigned int)i8042_handler, (unsigned int)i8042_dev_id);
}

void inline restore_i8042_irq() {request_irq(I8042_KEYBOARD_IRQ, i8042_handler, IRQF_SHARED, "i8042", i8042_dev_id);}

/***************************************************
    Buffer Helpers
 **************************************************/
// static void enqueue(char data) 
// {
//     buffer.buffer[buffer.tail] = data;
//     buffer.tail++;

//     buffer.is_empty = false;

//     if (buffer.tail >= BUFFER_SIZE)
//         buffer.tail = 0;

//     if (buffer.tail == buffer.head)
//         buffer.head++;

//     if (buffer.head >= BUFFER_SIZE)
//         buffer.head -= 0;

// }

// static char dequeue()
// {
//     if (buffer.is_empty)
//         return '\0';

//     char ret = buffer.buffer[head];
//     head++;

//     if (buffer.head >= BUFFER_SIZE)
//         buffer.head = 0;

//     if (buffer.head == buffer.tail)
//         buffer.is_empty = true;

//     return ret;
// }

// static void initqueue() 
// {
//     buffer.is_empty = true;
//     buffer.head = 0;
//     buffer.tail = 0;
// }


/***************************************************
    i8042 Interrupt Handlers
 **************************************************/

static inline char i8042_read_data(void)    {return inb(I8042_DATA_REG);}
static inline char i8042_read_status(void)  {return inb(I8042_STATUS_REG);}

static void bottom_half(unsigned long data);
static irqreturn_t top_half(int irq, void *dev_id);

static struct tasklet_data_t    
{
    char scancode;
    char status;
} tasklet_data;

DECLARE_TASKLET
(
    simple_keyboard_tasklet, 
    bottom_half, 
    (unsigned long)&tasklet_data
);


static void init_driver(void)
{
    remove_18042_irq();
    request_irq( I8042_KEYBOARD_IRQ, 
                 top_half,   
                 IRQF_SHARED, 
                 "simple_keyboard_driver",
                 (void *)(top_half));
 
    to_tty_string("[simple_keyboard.c] Removing i8024 IRQ.\n\r");


    /*
        The fifth param should be a random value to 
        tell this handler from other handlers who
        share the same IRQ. It is required when using
        IRQF_SHARED flag.
    */
}


static void exit_driver(void)
{
    to_tty_string("[simple_keyboard.c] Restoring i8024 IRQ.\n\r");

    free_irq(I8042_KEYBOARD_IRQ, (void *)(top_half));
    tasklet_kill(&simple_keyboard_tasklet);

    restore_i8042_irq();
}

static void bottom_half(unsigned long data) 
{
    #ifdef DEBUG
    to_tty_string("[simple_keyboard.c] Entring bottom_half.\n\r");
    #endif

    struct tasklet_data_t *keyboard_data = (struct tasklet_data_t *)data;

    //printk(KERN_INFO "Got scancode %02x in bottom_half.\n", keyboard_data->scancode);

    handle_scancode(keyboard_data->scancode);
}

static irqreturn_t top_half(int irq, void *dev_id)
{
    #ifdef DEBUG
    to_tty_string("[simple_keyboard.c] Entring top_half.\n\r");
    #endif

    tasklet_data.status = i8042_read_status();
    tasklet_data.scancode = i8042_read_data(); 

    #ifdef DEBUG
    to_tty_string("[simple_keyboard.c] Got scancode\n\r");
    #endif
    printk(KERN_INFO "Got scancode %02x in top_half.\n", tasklet_data.scancode);

    if ((tasklet_data.scancode & BIT_SCANCODE) == 0x01)
        exit_driver();

    tasklet_schedule(&simple_keyboard_tasklet);

    return IRQ_HANDLED;
}




/***************************************************
    Init and Exit
 **************************************************/

static int __init init(void)
{
    tty = current->signal->tty;

    //printk(KERN_INFO "[simple_keyboard.c] Loading module.\n");
    to_tty_string("[simple_keyboard.c] Loading module.\n\r");

    //keyboard_operations.compat_ioctl = keyboard_ioctl;
    keyboard_operations.unlocked_ioctl = keyboard_ioctl;

    // create proc entry
    proc_entry = create_proc_entry(PROC_ENTRY, 0444, NULL);
    if (!proc_entry)
    {
        // printk(KERN_INFO "[simple_keyboard.c] Error creating /proc entry.\n");
        to_tty_string("[simple_keyboard.c] Error creating /proc entry.\n\r");
        return 1;
    }

    proc_entry->proc_fops = &keyboard_operations;

    init_driver();

    return 0;
}


static void __exit exit(void)
{
    //printk(KERN_INFO "[simple_keyboard.c] Removing module.\n");
    to_tty_string("[simple_keyboard.c] Removing module.\n\r");

    remove_proc_entry(PROC_ENTRY, NULL);

    exit_driver();

    return;
}

module_init(init);
module_exit(exit);





// void alarm_handler( void )
// {
//     exit_driver();
// }

// void set_alarm(int seconds)
// {
//     signal(SIGALRM, alarm_handler);
//     alarm(SIGALRM, seconds);
// }









// void *sidt(void) {
//     unsigned int ptr[2];
//     asm volatile ("sidt (%0)": :"r" (((char*)ptr)+2));
//     return (void *)ptr[1];
// }

// int segment_selector(void *entry_base) 
// {
//     char offset[2];
//     offset[0] = *((char *)entry_base + 2);
//     offset[1] = *((char *)entry_base + 3);
//     return *((int *)offset);
// }

// int segment_offest(void *entry_base) 
// {
//     char offset[4];
//     offset[0] = *((char *)entry_base + 0);
//     offset[1] = *((char *)entry_base + 1);
//     offset[2] = *((char *)entry_base + 6);
//     offset[3] = *((char *)entry_base + 7);
//     return *((int *)offset);
// }

// void *next_entry(void *entry_base)
// {
//     return (unsigned int*)entry_base + 2;
// }