/********************************** (C) COPYRIGHT *******************************
 * File Name          : tiny_format.c
 * Author             : WCH / Antigravity
 * Version            : V1.0.0
 * Description        : 超轻量级嵌入式字符串格式化工具实现
 *******************************************************************************/
#include "tiny_format.h"

static void out_char(char *buf, size_t size, size_t *pos, char c)
{
    if (*pos + 1 < size)
    {
        buf[*pos] = c;
    }
    (*pos)++;
}

static void out_str(char *buf, size_t size, size_t *pos, const char *s)
{
    if (!s) s = "(null)";
    while (*s)
    {
        out_char(buf, size, pos, *s++);
    }
}

static void out_num(char *buf, size_t size, size_t *pos, uint32_t val, int is_signed, int is_neg, int base, int uppercase)
{
    char tmp[16];
    int idx = 0;

    if (is_signed && is_neg)
    {
        out_char(buf, size, pos, '-');
    }

    if (val == 0)
    {
        out_char(buf, size, pos, '0');
        return;
    }

    while (val > 0)
    {
        uint32_t rem = val % (uint32_t)base;
        val /= (uint32_t)base;
        if (rem < 10)
        {
            tmp[idx++] = (char)('0' + rem);
        }
        else
        {
            tmp[idx++] = (char)((uppercase ? 'A' : 'a') + (rem - 10));
        }
    }

    while (idx > 0)
    {
        out_char(buf, size, pos, tmp[--idx]);
    }
}

int tiny_vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    if (!buf || size == 0) return 0;

    size_t pos = 0;
    const char *p = fmt;

    while (*p)
    {
        if (*p != '%')
        {
            out_char(buf, size, &pos, *p++);
            continue;
        }

        p++; /* 跳过 '%' */
        if (*p == '\0') break;

        /* 解析修饰符 (支持 l / L) */
        int is_long = 0;
        if (*p == 'l' || *p == 'L')
        {
            is_long = 1;
            p++;
        }

        switch (*p)
        {
            case 's':
            {
                const char *s = va_arg(args, const char *);
                out_str(buf, size, &pos, s);
                break;
            }
            case 'c':
            {
                char c = (char)va_arg(args, int);
                out_char(buf, size, &pos, c);
                break;
            }
            case 'd':
            case 'i':
            {
                int32_t v = is_long ? (int32_t)va_arg(args, long) : (int32_t)va_arg(args, int);
                int is_neg = 0;
                uint32_t uv;
                if (v < 0)
                {
                    is_neg = 1;
                    uv = (uint32_t)(-v);
                }
                else
                {
                    uv = (uint32_t)v;
                }
                out_num(buf, size, &pos, uv, 1, is_neg, 10, 0);
                break;
            }
            case 'u':
            {
                uint32_t uv = is_long ? (uint32_t)va_arg(args, unsigned long) : (uint32_t)va_arg(args, unsigned int);
                out_num(buf, size, &pos, uv, 0, 0, 10, 0);
                break;
            }
            case 'x':
            {
                uint32_t uv = is_long ? (uint32_t)va_arg(args, unsigned long) : (uint32_t)va_arg(args, unsigned int);
                out_num(buf, size, &pos, uv, 0, 0, 16, 0);
                break;
            }
            case 'X':
            {
                uint32_t uv = is_long ? (uint32_t)va_arg(args, unsigned long) : (uint32_t)va_arg(args, unsigned int);
                out_num(buf, size, &pos, uv, 0, 0, 16, 1);
                break;
            }
            case '%':
            {
                out_char(buf, size, &pos, '%');
                break;
            }
            default:
            {
                out_char(buf, size, &pos, '%');
                out_char(buf, size, &pos, *p);
                break;
            }
        }
        p++;
    }

    if (pos < size)
    {
        buf[pos] = '\0';
    }
    else
    {
        buf[size - 1] = '\0';
    }

    return (int)pos;
}

int tiny_snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = tiny_vsnprintf(buf, size, fmt, args);
    va_end(args);
    return ret;
}
