#include<stdarg.h>

static int proc_fmt_int(char *dest, unsigned int value, int base, unsigned int flgs, int width, int precision) ;
static int proc_fmt_string(char *dest, const char *str, unsigned int flgs, int width, int precision) ;

enum {
    /*== flags field as Format placeholder ===*/
    FLAG_MINUS = 0x01, /* - */
    FLAG_PLUS  = 0x02, /* + */
    FLAG_SPACE = 0x04, /* ' '*/
    FLAG_NUM   = 0x08, /* # */
    FLAG_ZERO  = 0x10, /* 0 */
    
    /* Not defined in flags field */
    FLAG_UPPER_CASE = 0x0100, /* output uppercase character */
    FLAG_SIGNED     = 0x0200, /* signed */
} ;

void simple_vsprintf(char *str, const char *fmt, va_list ap)
{
    const char *p = fmt ;
    char c ;
    char *dest = (char *)str;
    
    unsigned int flgs;
    int width ;
    int precision ;

    enum {
        PROC_COPY,      /* string copy */
        PROC_FLAGS,     /* flags */
        PROC_WIDTH,     /* width */
        PROC_PRECISION, /* precision */
        PROC_TYPE,      /* type  */
    } proc ;
    proc = PROC_COPY ;
    
    while ((c = *p++) != '\0')
    {
        switch(proc)
        {
            case PROC_COPY:
                if (c == '%')
                {
                    flgs  = 0 ;
                    width = 0 ;
                    precision = -1 ;
                    proc = PROC_FLAGS;
                } 
                else 
                {
                    *dest = c;
                    dest++;
                }
                break ;
            case PROC_FLAGS:
                switch(c)
                {
                    case '-':
                        flgs |= FLAG_MINUS;
                        break ;
                    case '+':
                        flgs |= FLAG_PLUS;
                        break ;
                    case ' ':
                        flgs |= FLAG_SPACE;
                        break ;
                    case '#':
                        flgs |= FLAG_NUM;
                        break ;
                    case '0':
                        flgs |= FLAG_ZERO;
                        break;
                    default:
                        proc = PROC_WIDTH;
                        p--; /* processing this character again */
                        break;
                }
                break ;
            case PROC_WIDTH:
                if (('0' <= c) && (c <= '9'))
                {
                    width = width*10 + (c - '0');
                }
                else if (c == '.')
                {
                    proc = PROC_PRECISION;
                }
                else
                {
                    proc = PROC_TYPE;
                    p--; /* processing this character again */
                }
                break ;
            case PROC_PRECISION:
                if (('0' <= c) && (c <= '9'))
                {
                    precision = precision*10 + (c - '0');
                }
                else
                {
                    proc = PROC_TYPE;
                    p--; /* processing this character again */
                }
                break ;
            case PROC_TYPE:
                switch(c)
                {
                    case 'h':
                    case 'l':
                    case 'L':
                    case 'z':
                    case 'j':
                    case 't':
                        /* Length */
                        /* ignore this character, because Length field is not support */
                        break ;
                    default:
                        switch(c)
                        {
                            /* Signed decimal integer */
                            case 'd':
                            case 'i':
                                dest += proc_fmt_int(
                                            dest, 
                                            (unsigned int)va_arg(ap, int), 
                                            10, 
                                            flgs | FLAG_SIGNED, 
                                            width, 
                                            precision);
                                break;
                            /* unsigned int as a hexadecimal number. output alphabet upper case character */
                            case 'u':
                                dest += proc_fmt_int(
                                            dest,
                                            va_arg(ap, unsigned int), 
                                            10, 
                                            flgs, 
                                            width, 
                                            precision);
                                break;
                            case 'o':
                                dest += proc_fmt_int(
                                            dest,
                                            va_arg(ap, unsigned int), 
                                            8, 
                                            flgs, 
                                            width, 
                                            precision);
                                break;
                            case 'X':
                                dest += proc_fmt_int(
                                            dest,
                                            va_arg(ap, unsigned int), 
                                            16, 
                                            flgs | FLAG_UPPER_CASE, 
                                            width, 
                                            precision);
                                break;
                            /* unsigned int as a hexadecimal number. output alphabet lower case character */
                            case 'x':
                                dest += proc_fmt_int(
                                            dest,
                                            va_arg(ap, unsigned int), 
                                            16, 
                                            flgs, 
                                            width, 
                                            precision);
                                break;
                            case 'c':
                                *dest = (char)(va_arg(ap, int));
                                dest++;
                                break ;
                            case 's':
                                dest += proc_fmt_string(
                                            dest, 
                                            va_arg(ap, const char *),
                                            flgs,
                                            width,
                                            precision);
                                break;
                            case '%':
                                *dest = '%' ;
                                dest++;
                                break;
                        }
                        proc = PROC_COPY;
                        break ;
                }
                break ;
        }
    }
    
    *dest = '\0';
}

/**
 * sprintf like function
 *
 * @param  fmt format of string
 *
 * This function is support below Format placeholders.
 *  %d
 *  %0*x  (* : number *)
 *  %s
 */
void simple_sprintf(char *str, const char *fmt, ...)
{
    va_list arg ;
    
    va_start(arg, fmt);
    simple_vsprintf(str, fmt, arg);
    va_end(arg);
}

/**
 * convert integer value to string
 *
 */
static int proc_fmt_int(char *dest, unsigned int value, int base, unsigned int flgs, int width, int precision)
{
    const char upper_char_list[16] = { '0', '1', '2', '3', 
                                       '4', '5', '6', '7',
                                       '8', '9', 'A', 'B',
                                       'C', 'D', 'E', 'F' };
    const char lower_char_list[16] =  { '0', '1', '2', '3', 
                                       '4', '5', '6', '7',
                                       '8', '9', 'a', 'b',
                                       'c', 'd', 'e', 'f' };
    const char *char_list;
    
    char plus_char = '+' ;
    char pad_char = ' ';
    
    unsigned int n_data = 0 ; /* number of chars for data */
    unsigned int n_sign = 0 ; /* number of chars for sign */
    unsigned int n_pad  = 0 ; /* number of chars for padding */
    
    int minus = 0 ;
    
    unsigned int value_tmp ;
    char *p ;
    int i ;
    
    /* plus_char */
    if ((flgs & (FLAG_PLUS|FLAG_SPACE)) == FLAG_SPACE)
    {
        plus_char = ' ';
    }
    
    /* pad_char */
    if (flgs & FLAG_MINUS)
    {
        pad_char = ' ';
    }
    else if (flgs & FLAG_ZERO)
    {
        pad_char = '0';
    }
    else
    {
        pad_char = ' ';
    }
    
    if (flgs & FLAG_UPPER_CASE)
    {
        char_list = upper_char_list ;
    }
    else
    {
        char_list = lower_char_list ;
    }
    
    if (flgs & FLAG_SIGNED)
    {
        if ((int)value < 0)
        {
            value = (unsigned int)(-1*(int)value);
            minus = 1; /* 1 indicate minus value */
        }
        
        if ((flgs & FLAG_PLUS) || (minus == 1))
        {
            n_sign = 1; /* 1 indicate output sign */
        }
    }
    
    /* check number of charcters for data */
    value_tmp = value ;
    do {
        value_tmp /= base ;
        n_data++;
    } while (value_tmp != 0);
    
    if (width < (n_data+n_sign))
    {
        n_pad = 0 ;
        p = dest + (n_data + n_sign) - 1;
    }
    else
    {
        n_pad = width - (n_data + n_sign);
        if (flgs & FLAG_MINUS)
        {
            p = dest + (n_data + n_sign) - 1 ;
        }
        else 
        {
            p = dest + width - 1 ;
        }
    }
    
    /****************************************************
     * output
     ***************************************************/
    if (n_pad != 0)
    {
        for ( i = 0 ; i < (n_data + n_sign + n_pad) ; i++ )
        {
            *(dest+i) = pad_char ;
        }
    }
    
    /* convert value */
    value_tmp = value ;
    do {
        *p = *(char_list + (value_tmp % base)) ;
        p--;
        value_tmp = value_tmp / base ;
    } while (value_tmp != 0);
    
    /* sign */
    if (n_sign != 0)
    {
        if ((flgs & FLAG_MINUS) || (flgs & FLAG_ZERO))
        {
            p = dest ;
        }
        
        *p = (minus == 0) ? plus_char : '-' ;
    }
    
    /* return */
    return (n_data + n_sign + n_pad) ;
}

/**
 * this function process printf like function '%s' format
 *
 */
static int proc_fmt_string(char *dest, const char *str, unsigned int flgs, int width, int precision)
{
    int n ;
    
    n = 0 ;
    
    while (*str != '\0') 
    {
        *dest++ = *str++;
        n++ ;
    }
    
    return n ;
}

