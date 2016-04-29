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
 *	File		: NX_AVCUtil.c
 *	Brief		: AVC Util
 *	Author		: HyunChul Jun (hcjun@nexell.co.kr)
 *	History		: 2016.04.25 : Create
 */

#include <string.h>

#include "NX_AVCUtil.h"

#define NAL_SLICE				1
#define NAL_DPA					2
#define NAL_DPB					3
#define NAL_DPC					4
#define NAL_IDR_SLICE			5
#define NAL_SEI					6
#define NAL_SPS					7
#define NAL_PPS					8
#define NAL_PICTURE_DELIMITER	9
#define NAL_FILTER_DATA			10

const uint8_t ff_golomb_vlc_len[512]={
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

const uint8_t ff_log2_tab[256] = {
	0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};
const uint8_t ff_ue_golomb_vlc_code[512]={
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
	7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const int8_t ff_se_golomb_vlc_code[512]={
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  8, -8,  9, -9, 10,-10, 11,-11, 12,-12, 13,-13, 14,-14, 15,-15,
	4,  4,  4,  4, -4, -4, -4, -4,  5,  5,  5,  5, -5, -5, -5, -5,  6,  6,  6,  6, -6, -6, -6, -6,  7,  7,  7,  7, -7, -7, -7, -7,
	2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

static unsigned char rbsp_buffer[1024*64];
static uint8_t *decode_nal(int32_t *nal_unit_type, uint8_t *src, int32_t *dst_length, int32_t *consumed, int32_t length)
{
	int32_t i, si, di;
	uint8_t *dst;

	*nal_unit_type= src[0]&0x1F;
	src++; length--;

	for(i=0; i+1<length; i+=2)
	{
		if(src[i]) continue;
		if(i>0 && src[i-1]==0) i--;
		if(i+2<length && src[i+1]==0 && src[i+2]<=3)
		{
			if(src[i+2]!=3)
			{
				/* startcode, so we must be past the end */
				length=i;
			}
			break;
		}
	}

	if(i>=length-1)
	{ /* no escaped 0 */
		*dst_length= length;
		*consumed= length+1; /* +1 for the header */
		return src;
	}

	dst= rbsp_buffer;
	si=di=0;
	while(si<length)
	{
		/* remove escapes (very rare 1:2^22) */
		if(si+2<length && src[si]==0 && src[si+1]==0 && src[si+2]<=3)
		{
			if(src[si+2]==3)
			{ /* escape */
				dst[di++]= 0;
				dst[di++]= 0;
				si+=3;
				continue;
			}
			else /* next start code  */
				break;
		}

		dst[di++]= src[si++];
	}

	*dst_length= di;
	*consumed= si + 1; /* +1 for the header */

	return dst;
}

/*
 * identifies the exact end of the bitstream
 * @return the length of the trailing, or 0 if damaged
 */
static int32_t decode_rbsp_trailing(uint8_t *src){
	int32_t v= *src;
	int32_t r;
	for(r=1; r<9; r++)
	{
		if(v&1) return r;
		v>>=1;
	}
	return 0;
}

typedef struct SPS
{
	int32_t profile_idc;
	int32_t level_idc;

	int32_t chroma_format_idc;

	int32_t log2_max_frame_num;            /* < log2_max_frame_num_minus4 + 4  */
	int32_t poc_type;                      /* < pic_order_cnt_type */
	int32_t log2_max_poc_lsb;              /* < log2_max_pic_order_cnt_lsb_minus4 */
	int32_t delta_pic_order_always_zero_flag;
	int32_t offset_for_non_ref_pic;
	int32_t offset_for_top_to_bottom_field;
	int32_t poc_cycle_length;              /* < num_ref_frames_in_pic_order_cnt_cycle */
	int32_t ref_frame_count;               /* < num_ref_frames */
	int32_t gaps_in_frame_num_allowed_flag;
	int32_t mb_width;                      /* < frame_width_in_mbs_minus1 + 1 */
	int32_t mb_height;                     /* < frame_height_in_mbs_minus1 + 1 */
	int32_t frame_mbs_only_flag;
	int32_t mb_aff;                        /* <mb_adaptive_frame_field_flag */
	int32_t direct_8x8_inference_flag;
	int32_t crop;                   /* < frame_cropping_flag */
	int32_t crop_left;              /* < frame_cropping_rect_left_offset */
	int32_t crop_right;             /* < frame_cropping_rect_right_offset */
	int32_t crop_top;               /* < frame_cropping_rect_top_offset */
	int32_t crop_bottom;            /* < frame_cropping_rect_bottom_offset */
	int32_t vui_parameters_present_flag;
} SPS;

/* Picture parameter set */
typedef struct PPS
{
	int32_t sps_id;
	int32_t cabac;                  /* < entropy_coding_mode_flag */
	int32_t pic_order_present;      /* < pic_order_present_flag */
	int32_t slice_group_count;      /* < num_slice_groups_minus1 + 1 */
	int32_t mb_slice_group_map_type;
	int32_t ref_count[2];           /* < num_ref_idx_l0/1_active_minus1 + 1 */
	int32_t weighted_pred;          /* < weighted_pred_flag */
	int32_t weighted_bipred_idc;
	int32_t init_qp;                /* < pic_init_qp_minus26 + 26 */
	int32_t init_qs;                /* < pic_init_qs_minus26 + 26 */
	int32_t chroma_qp_index_offset;
	int32_t deblocking_filter_parameters_present; /* < deblocking_filter_parameters_present_flag */
	int32_t constrained_intra_pred; /* < constrained_intra_pred_flag */
	int32_t redundant_pic_cnt_present; /* < redundant_pic_cnt_present_flag */
} PPS;

static inline int decode_seq_parameter_set(SPS *sps,GetBitContext *gb)
{
	int32_t i, sps_id;
	sps->profile_idc= get_bits(gb,8);
	get_bits1(gb);   /* constraint_set0_flag */
	get_bits1(gb);   /* constraint_set1_flag */
	get_bits1(gb);   /* constraint_set2_flag */
	get_bits(gb,5);  /* reserved */
	sps->level_idc= get_bits(gb,8);
	sps_id= get_ue_golomb(gb);
	(void)sps_id;

	sps->chroma_format_idc = 1;
	if( sps->profile_idc == 100 ||sps->profile_idc == 110 ||
		sps->profile_idc == 122 ||sps->profile_idc == 144 )
	{
		int32_t seq_scaling_matrix_present_flag;
		sps->chroma_format_idc = get_ue_golomb(gb);
		if( sps->chroma_format_idc == 3 )
		{
			get_bits1(gb);	/* residual_colour_transform_flag */
		}
		get_ue_golomb(gb);	/* bit_depth_luma_minus8 */
		get_ue_golomb(gb);	/* bit_depth_chroma_minus8 */
		get_bits1(gb);	    /*qpprime_y_zero_transform_bypass_flag */
		seq_scaling_matrix_present_flag = get_bits1(gb);
		if( seq_scaling_matrix_present_flag )
		{
			for( i=0 ; i<8 ; i++ )
			{
				if (get_bits1(gb))	/* seq_scaling_list_present_flag[8] */
				{
					int32_t sizeOfScalingList = (i < 6) ? (16) : (64);
					int32_t lastScale = 8;
					int32_t nextScale = 8;
					int32_t j;

					for (j=0 ; j<sizeOfScalingList ; j++)
					{
						if (nextScale != 0)
						{
							int32_t delta_scale = get_se_golomb(gb);
							nextScale = ( lastScale + delta_scale + 256) % 256;
						}
						lastScale = (nextScale == 0) ? lastScale : nextScale;
					}
				}
			}
		}
	}

	sps->log2_max_frame_num= get_ue_golomb(gb) + 4;
	sps->poc_type= get_ue_golomb(gb);

	if(sps->poc_type == 0)
	{
		sps->log2_max_poc_lsb= get_ue_golomb(gb) + 4;
	}
	else if(sps->poc_type == 1)
	{
		sps->delta_pic_order_always_zero_flag= get_bits1(gb);
		sps->offset_for_non_ref_pic= get_se_golomb(gb);
		sps->offset_for_top_to_bottom_field= get_se_golomb(gb);
		sps->poc_cycle_length= get_ue_golomb(gb);

		for(i=0; i<sps->poc_cycle_length; i++)
			get_se_golomb(gb);
	}

	if(sps->poc_type > 2)
	{
		return -1;
	}

	sps->ref_frame_count= get_ue_golomb(gb);
	sps->gaps_in_frame_num_allowed_flag= get_bits1(gb);
	sps->mb_width= get_ue_golomb(gb) + 1;
	sps->mb_height= get_ue_golomb(gb) + 1;
	sps->frame_mbs_only_flag= get_bits1(gb);
	if(!sps->frame_mbs_only_flag)
		sps->mb_aff= get_bits1(gb);
	else
		sps->mb_aff= 0;

	sps->direct_8x8_inference_flag= get_bits1(gb);

	sps->crop= get_bits1(gb);
	if(sps->crop)
	{
		sps->crop_left  = get_ue_golomb(gb);
		sps->crop_right = get_ue_golomb(gb);
		sps->crop_top   = get_ue_golomb(gb);
		sps->crop_bottom= get_ue_golomb(gb);
	}
	else
	{
		sps->crop_left  =
			sps->crop_right =
			sps->crop_top   =
			sps->crop_bottom= 0;
	}

	sps->vui_parameters_present_flag = get_bits1(gb);

	return 0;
}

/*
static inline int32_t decode_picture_parameter_set(H264Context *h)
{
	MpegEncContext * const s = &h->s;
	int32_t pps_id= get_ue_golomb(&s->gb);
	PPS *pps= &h->pps_buffer[pps_id];

	pps->sps_id= get_ue_golomb(&s->gb);
	pps->cabac= get_bits1(&s->gb);
	pps->pic_order_present= get_bits1(&s->gb);
	pps->slice_group_count= get_ue_golomb(&s->gb) + 1;
	if(pps->slice_group_count > 1 )
	{
		pps->mb_slice_group_map_type= get_ue_golomb(&s->gb);
		av_log(h->s.avctx, AV_LOG_ERROR, "FMO not supported\n");
		switch(pps->mb_slice_group_map_type)
		{
			case 0:
				break;
			case 2:
				break;
			case 3:
			case 4:
			case 5:
				break;
			case 6:
				break;
		}
	}
	pps->ref_count[0]= get_ue_golomb(&s->gb) + 1;
	pps->ref_count[1]= get_ue_golomb(&s->gb) + 1;
	if(pps->ref_count[0] > 32 || pps->ref_count[1] > 32)
	{
		av_log(h->s.avctx, AV_LOG_ERROR, "reference overflow (pps)\n");
		return -1;
	}

	pps->weighted_pred= get_bits1(&s->gb);
	pps->weighted_bipred_idc= get_bits(&s->gb, 2);
	pps->init_qp= get_se_golomb(&s->gb) + 26;
	pps->init_qs= get_se_golomb(&s->gb) + 26;
	pps->chroma_qp_index_offset= get_se_golomb(&s->gb);
	pps->deblocking_filter_parameters_present= get_bits1(&s->gb);

	pps->constrained_intra_pred= get_bits1(&s->gb);
	pps->redundant_pic_cnt_present = get_bits1(&s->gb);

	return 0;
}
*/

static int32_t decode_nal_units(SPS *sps, uint8_t *buf, int32_t buf_size)
{
	GetBitContext gb;
	int32_t buf_index=0;
	int32_t nal_unit_type = 0;
	int32_t ret = 0;
	int32_t consumed;
	int32_t dst_length;
	int32_t bit_length;
	uint8_t *ptr;

	for(;;)
	{
		/* start code prefix search */
		for(; buf_index + 3 < buf_size; buf_index++)
		{
			/* this should allways succeed in the first iteration */
			if(buf[buf_index] == 0 && buf[buf_index+1] == 0 && buf[buf_index+2] == 1)
				break;
		}

		if(buf_index+3 >= buf_size) break;

		buf_index+=3;

		ptr = decode_nal(&nal_unit_type, buf + buf_index, &dst_length, &consumed, buf_size - buf_index);

		if(ptr[dst_length - 1] == 0) dst_length--;
		bit_length= 8*dst_length - decode_rbsp_trailing(ptr + dst_length - 1);

		buf_index += consumed;

		if ( nal_unit_type == NAL_SPS )
		{
			init_get_bits(&gb, ptr, bit_length);
			decode_seq_parameter_set(sps, &gb);
			ret = 1;
			break;
		}
#if 0
		switch(nal_unit_type)
		{
		case NAL_SPS:
			init_get_bits(&gb, ptr, bit_length);
			decode_seq_parameter_set(&sps, &gb);
			//				decode_seq_parameter_set(h);
			ret = 1;
			break;

		case NAL_PPS:
			init_get_bits(&gb, ptr, bit_length);
			decode_picture_parameter_set(h);
			break;
		}
#endif
	}

	return ret;
}

#if 1
int32_t avc_get_video_size(unsigned char *buf, int32_t buf_size, int32_t *width, int32_t *height)
{
	SPS sps;
	memset(&sps, 0, sizeof(SPS));

	if( buf_size > 64*1024 )
		buf_size = 64*1024;

	if ( decode_nal_units(&sps, buf, buf_size) )
	{
		*width      = sps.mb_width * 16;
		*height     = ( 2 - sps.frame_mbs_only_flag ) * sps.mb_height * 16;

		if( sps.crop )
		{
			uint32_t cropUnitX, cropUnitY;
			if (sps.chroma_format_idc == 0  /* monochrome */)
			{
				cropUnitX = 1;
				cropUnitY = 2 - sps.frame_mbs_only_flag;
			}
			else
			{
				unsigned subWidthC = (sps.chroma_format_idc == 3) ? 1 : 2;
				unsigned subHeightC = (sps.chroma_format_idc == 1) ? 2 : 1;

				cropUnitX = subWidthC;
				cropUnitY = subHeightC * (2 - sps.frame_mbs_only_flag);
			}

			*width  -= (sps.crop_left + sps.crop_right) * cropUnitX;
			*height -= (sps.crop_top + sps.crop_bottom) * cropUnitY;
		}

		return 1;
	}
	return 0;
}
#else	/* Simple find SPS */
int32_t avc_get_video_size(unsigned char *buf, int32_t buf_size, int32_t *width, int32_t *height)
{
	int32_t i=0;
	int32_t zero_count = 0;
	for( i=0 ; i<buf_size-4 ; i++ )
	{
		if( buf[i] )
		{
			zero_count = 0;
			continue;
		}
		if( (zero_count>2) && (buf[i]==1) && ((buf[i+1]&0x1f)==0x07) )
		{
			GetBitContext gb;
			SPS sps;
			init_get_bits(&gb, buf+i-2, buf_size-i+2);
			if( 0 == decode_seq_parameter_set(&sps, &gb) )
			{
				*width      = sps.mb_width * 16;
				*height     = sps.mb_height * 16;
				if( sps.crop )
				{
					uint32_t cropUnitX, cropUnitY;
					if (sps.chroma_format_idc == 0  /* monochrome */)
					{
						cropUnitX = 1;
						cropUnitY = 2 - sps.frame_mbs_only_flag;
					}
					else
					{
						unsigned subWidthC = (sps.chroma_format_idc == 3) ? 1 : 2;
						unsigned subHeightC = (sps.chroma_format_idc == 1) ? 2 : 1;

						cropUnitX = subWidthC;
						cropUnitY = subHeightC * (2 - sps.frame_mbs_only_flag);
					}
					*width  -= (sps.crop_left + sps.crop_right) * cropUnitX;
					*height -= (sps.crop_top + sps.crop_bottom) * cropUnitY;
				}
				return 1;
			}
		}
		if( buf[i] == 0 )
			zero_count ++;
	}
	return 0;
}
#endif
