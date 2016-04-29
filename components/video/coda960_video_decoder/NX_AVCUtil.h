/*
 *
 *	Copyright (C) 2016 Nexell Co. All Rights Reserved
 *	Nexell Co. Proprietary & Confidential
 *
 *	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
 *  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
 *  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
 *  FOR A PARTICULAR PURPOSE.
 *
 *	File		: NX_AVCUtil.h
 *	Brief		: AVC Util
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include <assert.h>

#define bswap_16(x) (((x) & 0x00ff) << 8 | ((x) & 0xff00) >> 8)

#define bswap_32(x) \
	((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
	(((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

#define be2me_16(x) bswap_16(x)
#define be2me_32(x) bswap_32(x)

#define NEG_SSR32(a,s) (int32_t)((( int32_t)(a))>>(32-(s)))
#define NEG_USR32(a,s) (uint32_t)(((uint32_t)(a))>>(32-(s)))

/* bit input */
/* buffer, buffer_end and size_in_bits must be present and used by every reader */
typedef struct GetBitContext
{
	const uint8_t *buffer, *buffer_end;
	int32_t index;
	int32_t size_in_bits;
} GetBitContext;

/* used to avoid missaligned exceptions on some archs (alpha, ...) */
#ifdef __GNUC__
static inline uint32_t unaligned32(const void *v)
{
	struct Unaligned
	{
		uint32_t i;
	} __attribute__((packed));

	return ((const struct Unaligned *) v)->i;
}
#elif defined(__DECC)
static inline uint32_t unaligned32(const void *v)
{
	return *(const __unaligned uint32_t *) v;
}
#else
static inline uint32_t unaligned32(const void *v)
{
	return *(const uint32_t *) v;
}
#endif

static inline int unaligned32_be(const void *v)
{
#ifdef CONFIG_ALIGN
	const uint8_t *p=v;
	return (((p[0]<<8) | p[1])<<16) | (p[2]<<8) | (p[3]);
#else
	return be2me_32( unaligned32(v)); //original
#endif
}

#define MIN_CACHE_BITS 25

#define OPEN_READER(name, gb)		\
	int32_t name##_index= (gb)->index;	\
	int32_t name##_cache= 0;

#define CLOSE_READER(name, gb)		\
	(gb)->index= name##_index;		\

#define UPDATE_CACHE(name, gb)		\
	name##_cache= unaligned32_be( ((const uint8_t *)(gb)->buffer)+(name##_index>>3) ) << (name##_index&0x07);	\

#define SKIP_CACHE(name, gb, num)	\
	name##_cache <<= (num);			\

/* FIXME name */
#define SKIP_COUNTER(name, gb, num)	\
	name##_index += (num);			\

#define SKIP_BITS(name, gb, num)	\
{								\
	SKIP_CACHE(name, gb, num)		\
	SKIP_COUNTER(name, gb, num)		\
}								\

#define LAST_SKIP_BITS(name, gb, num) SKIP_COUNTER(name, gb, num)
#define LAST_SKIP_CACHE(name, gb, num) ;

#define SHOW_UBITS(name, gb, num)	\
	NEG_USR32(name##_cache, num)

#define SHOW_SBITS(name, gb, num)	\
	NEG_SSR32(name##_cache, num)

#define GET_CACHE(name, gb)			\
	((uint32_t)name##_cache)

static inline int32_t get_bits_count(GetBitContext *s)
{
	return s->index;
}

/*
 * reads 0-17 bits.
 * Note, the alt bitstream reader can read upto 25 bits, but the libmpeg2 reader cant
 */
static inline uint32_t get_bits(GetBitContext *s, int32_t n)
{
	register int32_t tmp;
	OPEN_READER(re, s)
	UPDATE_CACHE(re, s)
	tmp= SHOW_UBITS(re, s, n);
	LAST_SKIP_BITS(re, s, n)
	CLOSE_READER(re, s)
	return tmp;
}

/*
 * shows 0-17 bits.
 * Note, the alt bitstream reader can read upto 25 bits, but the libmpeg2 reader cant
 */
static inline uint32_t show_bits(GetBitContext *s, int32_t n)
{
	register int32_t tmp;
	OPEN_READER(re, s);
	UPDATE_CACHE(re, s)
	tmp= SHOW_UBITS(re, s, n);
	//CLOSE_READER(re, s)
	return tmp;
}

static inline void skip_bits(GetBitContext *s, int n)
{
	//Note gcc seems to optimize this to s->index+=n for the ALT_READER :))
	OPEN_READER(re, s)
	(void)re_cache;
	//GET_CACHE(re, s);	//	Dummy
	//UPDATE_CACHE(re, s)
	LAST_SKIP_BITS(re, s, n)
	CLOSE_READER(re, s)
}

static inline uint32_t get_bits1(GetBitContext *s)
{
	int32_t index= s->index;
	uint8_t result= s->buffer[ index>>3 ];
	result<<= (index&0x07);
	result>>= 8 - 1;
	index++;
	s->index= index;

	return result;
}

static inline uint32_t show_bits1(GetBitContext *s)
{
	return show_bits(s, 1);
}

static inline void skip_bits1(GetBitContext *s)
{
	skip_bits(s, 1);
}

/*
 * init GetBitContext.
 * @param buffer bitstream buffer, must be FF_INPUT_BUFFER_PADDING_SIZE bytes larger then the actual read bits
 * because some optimized bitstream readers read 32 or 64 bit at once and could read over the end
 * @param bit_size the size of the buffer in bits
 */
static inline void init_get_bits(GetBitContext *s, const uint8_t *buffer, int bit_size)
{
	const int32_t buffer_size= (bit_size+7)>>3;

	s->buffer= buffer;
	s->size_in_bits= bit_size;
	s->buffer_end= buffer + buffer_size;
	s->index=0;
	{
		OPEN_READER(re, s)
		(void)re_cache;
		// GET_CACHE(re,s);		//	Dummy
		//UPDATE_CACHE(re, s)
		UPDATE_CACHE(re, s)
		CLOSE_READER(re, s)
	}
}

/*
 * parses a vlc code, faster then get_vlc()
 * @param bits is the number of bits which will be read at once, must be
 *             identical to nb_bits in init_vlc()
 * @param max_depth is the number of times bits bits must be readed to completly
 *                  read the longest vlc code
 *                  = (max_vlc_length + bits - 1) / bits
 */

/* misc math functions */
extern const uint8_t ff_log2_tab[256];

static inline int av_log2(uint32_t v)
{
	int32_t n;

	n = 0;
	if (v & 0xffff0000)
	{
		v >>= 16;
		n += 16;
	}
	if (v & 0xff00)
	{
		v >>= 8;
		n += 8;
	}
	n += ff_log2_tab[v];

	return n;
}

extern const uint8_t ff_golomb_vlc_len[512];
extern const uint8_t ff_ue_golomb_vlc_code[512];
extern const  int8_t ff_se_golomb_vlc_code[512];
extern const uint8_t ff_ue_golomb_len[256];

extern const uint8_t ff_interleaved_golomb_vlc_len[256];
extern const uint8_t ff_interleaved_ue_golomb_vlc_code[256];
extern const  int8_t ff_interleaved_se_golomb_vlc_code[256];

/*
 * read unsigned exp golomb code.
 */
static inline int32_t get_ue_golomb(GetBitContext *gb)
{
	uint32_t buf;
	int32_t log;

	OPEN_READER(re, gb);
	UPDATE_CACHE(re, gb);
	buf=GET_CACHE(re, gb);

	if(buf >= (1<<27))
	{
		buf >>= 32 - 9;
		LAST_SKIP_BITS(re, gb, ff_golomb_vlc_len[buf]);
		CLOSE_READER(re, gb);

		return ff_ue_golomb_vlc_code[buf];
	}
	else
	{
		log= 2*av_log2(buf) - 31;
		buf>>= log;
		buf--;
		LAST_SKIP_BITS(re, gb, 32 - log);
		CLOSE_READER(re, gb);

		return buf;
	}
}

/*
 * read unsigned truncated exp golomb code.
 */
static inline int32_t get_te0_golomb(GetBitContext *gb, int32_t range)
{
	assert(range >= 1);

	if(range==1)      return 0;
	else if(range==2) return get_bits1(gb)^1;
	else              return get_ue_golomb(gb);
}

/*
 * read unsigned truncated exp golomb code.
 */
static inline int32_t get_te_golomb(GetBitContext *gb, int32_t range)
{
	assert(range >= 1);

	if(range==2) return get_bits1(gb)^1;
	else         return get_ue_golomb(gb);
}

/*
 * read signed exp golomb code.
 */
static inline int32_t get_se_golomb(GetBitContext *gb)
{
	uint32_t buf;
	int32_t log;

	OPEN_READER(re, gb);
	UPDATE_CACHE(re, gb);
	buf=GET_CACHE(re, gb);

	if(buf >= (1<<27))
	{
		buf >>= 32 - 9;
		LAST_SKIP_BITS(re, gb, ff_golomb_vlc_len[buf]);
		CLOSE_READER(re, gb);

		return ff_se_golomb_vlc_code[buf];
	}
	else
	{
		log= 2*av_log2(buf) - 31;
		buf>>= log;

		LAST_SKIP_BITS(re, gb, 32 - log);
		CLOSE_READER(re, gb);

		if(buf&1) buf= -((int32_t)(buf>>1));
		else      buf=  (buf>>1);

		return buf;
	}
}

/*
 * read unsigned golomb rice code (ffv1).
 */
static inline int32_t get_ur_golomb(GetBitContext *gb, int32_t k, int32_t limit, int32_t esc_len)
{
	uint32_t buf;
	int32_t log;

	OPEN_READER(re, gb);
	UPDATE_CACHE(re, gb);
	buf=GET_CACHE(re, gb);

	log= av_log2(buf);

	if(log > 31-limit)
	{
		buf >>= log - k;
		buf += (30-log)<<k;
		LAST_SKIP_BITS(re, gb, 32 + k - log);
		CLOSE_READER(re, gb);

		return buf;
	}else
	{
		buf >>= 32 - limit - esc_len;
		LAST_SKIP_BITS(re, gb, esc_len + limit);
		CLOSE_READER(re, gb);

		return buf + limit - 1;
	}
}

/*
 * read unsigned golomb rice code (jpegls).
 */
static inline int32_t get_ur_golomb_jpegls(GetBitContext *gb, int32_t k, int32_t limit, int32_t esc_len)
{
	uint32_t buf;
	int32_t log;

	OPEN_READER(re, gb);
	UPDATE_CACHE(re, gb);
	buf=GET_CACHE(re, gb);

	log= av_log2(buf);

	if(log > 31-11)
	{
		buf >>= log - k;
		buf += (30-log)<<k;
		LAST_SKIP_BITS(re, gb, 32 + k - log);
		CLOSE_READER(re, gb);

		return buf;
	}
	else
	{
		int32_t i;
		for(i=0; SHOW_UBITS(re, gb, 1) == 0; i++)
		{
			LAST_SKIP_BITS(re, gb, 1);
			UPDATE_CACHE(re, gb);
		}
		SKIP_BITS(re, gb, 1);

		if(i < limit - 1)
		{
			if(k)
			{
				buf = SHOW_UBITS(re, gb, k);
				LAST_SKIP_BITS(re, gb, k);
			}
			else
			{
				buf=0;
			}

			CLOSE_READER(re, gb);
			return buf + (i<<k);
		}
		else if(i == limit - 1)
		{
			buf = SHOW_UBITS(re, gb, esc_len);
			LAST_SKIP_BITS(re, gb, esc_len);
			CLOSE_READER(re, gb);

			return buf + 1;
		}
		else
			return -1;
	}
}

/**
* read unsigned golomb rice code (flac).
*/
static inline int32_t get_sr_golomb_flac(GetBitContext *gb, int32_t k, int32_t limit, int32_t esc_len)
{
	int32_t v= get_ur_golomb_jpegls(gb, k, limit, esc_len);
	return (v>>1) ^ -(v&1);
}

