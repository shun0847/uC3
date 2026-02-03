
#include <string.h>
#include <stdarg.h>
#include "kernel.h"
#include "shell.h"

extern void simple_vsprintf(char *str, const char *fmt, va_list ap);
extern void simple_sprintf(char *str, const char *fmt, ...);

#define SHELL_PRINTF_BUF_SZ (512)
char shell_printf_buf[SHELL_PRINTF_BUF_SZ];

void shell_printf(VP ctrl, const char *fmt, ...)
{
    va_list arg ;
    
    va_start(arg, fmt);
    simple_vsprintf(shell_printf_buf, fmt, arg);
    va_end(arg);
    
    shell_puts(ctrl, shell_printf_buf);
}

UINT hexchar2bin(char c)
{
    if ((c >= '0') && (c <= '9')) {
        return c - '0';
    } else if ((c >= 'a') && (c <= 'f')) {
        return c - 'a';
    } else if ((c >= 'A') && (c <= 'F')) {
        return c - 'A';
    } else {
        return 0;
    }
}

void hexstr2bin(UB *bin, const char *hex, INT hex_len)
{
    while (hex_len) {
        
        *bin = (hexchar2bin(*hex) << 4) | (hexchar2bin(*(hex+1))) ;
        
        bin++;
        hex += 2;
        hex_len -= 2;
    }
}

void shell_dump_buf(VP ctrl, const VB *buf, UINT len, UINT print_addr) 
{
    int i ;
    int j ;
    int len_line ;
    VB c ;
    
    for ( i = 0 ; i < len ; i += 16 ) {
        
        len_line = len - i ;
        if (len_line > 16) {
            len_line = 16 ;
        }
        
        shell_printf(ctrl, "%08x : ", print_addr + i);
        
        /* Print Hex */
        for ( j = 0 ; j < len_line ; j++ ) {
            shell_printf(ctrl, "%02x ", *(buf+i+j));
        }
        
        for ( ; j < 16 ; j++ ) {
            shell_printf(ctrl, "   ");
        }
        
        shell_printf(ctrl, "   ") ;
        
        /* Print Character */
        for ( j = 0 ; j < len_line ; j++ ) {
            
            c = *(buf+i+j) ;
            
            if ((0x20 <= c) && (c <= 0x7e)) {
                shell_printf(ctrl, "%c", c);
            } else {
                shell_printf(ctrl, "*");
            }
        }
        
        shell_printf(ctrl, "\r\n");
    }
}

