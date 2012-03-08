#ifndef ELFUTILS_BYTESWAP_H
#define ELFUTILS_BYTESWAP_H

static inline unsigned short bswap_16(unsigned short val)
{
        return ((val & 0xff) << 8) | ((val >> 8) & 0xff);
}

static inline unsigned long bswap_32(unsigned long val)
{
        return bswap_16((unsigned short)val) << 16 |
               bswap_16((unsigned short)(val >> 16));
}

static inline unsigned long long bswap_64(unsigned long long val)
{
        return ((((unsigned long long)bswap_32(val)) << 32) |
                (((unsigned long long)bswap_32(val >> 32)) & 0xffffffffULL));
}

#endif /* ELFUTILS_BYTESWAP_H */
