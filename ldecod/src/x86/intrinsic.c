/*****************************************************************************
* Authors: Ronggang Wang <rgwang@pkusz.edu.cn>
*          Zhenyu Wang <wangzhenyu@pkusz.edu.cn>
*          Kui Fan <kuifan@pku.edu.cn>
*          Shenghao Zhang <1219759986@qq.com>
* Affiliate: Peking University Shenzhen Graduate School
*****************************************************************************/
#include "intrinsic.h"

#if COMPILE_FOR_8BIT

ALIGNED_16( char intrinsic_mask[15][16] ) =
{
    { -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0 },
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0 }
};

void com_init_intrinsic()
{
    // ------------------  inter filter  ----------------------
    //g_funs_handle.ipcpy = com_if_filter_cpy_sse128;

    g_funs_handle.ipflt[IPFILTER_H_4]  = com_if_filter_hor_4_sse128;
    g_funs_handle.ipflt[IPFILTER_H_6]  = com_if_filter_hor_6_sse128;
    g_funs_handle.ipflt[IPFILTER_H_10] = com_if_filter_hor_10_sse128;

    g_funs_handle.ipflt[IPFILTER_V_4]  = com_if_filter_ver_4_sse128;
    g_funs_handle.ipflt[IPFILTER_V_6]  = com_if_filter_ver_6_sse128;
    g_funs_handle.ipflt[IPFILTER_V_10] = com_if_filter_ver_10_sse128;

    g_funs_handle.ipflt_EXT[IPFILTER_EXT_4 ] = com_if_filter_hor_ver_4_sse128;
    g_funs_handle.ipflt_EXT[IPFILTER_EXT_6 ] = com_if_filter_hor_ver_6_sse128;
    g_funs_handle.ipflt_EXT[IPFILTER_EXT_10] = com_if_filter_hor_ver_10_sse128;

    g_funs_handle.ipflt_chroma_subpix_EXT[IPFILTER_EXT_4] = get_chroma_subpix_Ext_sse128;

    // ------------------  intra  ----------------------
    //g_funs_handle.intra_pred_luma_dc      = xPredIntraLumaDC_sse128;
    g_funs_handle.intra_pred_chroma_dc    = xPredIntraChromaDC_sse128;
    g_funs_handle.intra_pred_ver          = xPredIntraVer_sse128;
    g_funs_handle.intra_pred_hor          = xPredIntraHor_sse128;
    g_funs_handle.intra_pred_downright    = xPredIntraDownRight_sse128;
    g_funs_handle.intra_pred_downleft     = xPredIntraDownLeft_sse128;
    g_funs_handle.intra_pred_chroma_plane = xPredIntraChromaPlane_sse128;

    // ------------------  deblock  ----------------------
    g_funs_handle.deblock_edge[0] = EdgeLoopVer_sse128;
    g_funs_handle.deblock_edge[1] = EdgeLoopHor_sse128;

    // --------------------  dct  ------------------------
    g_funs_handle.idct_sqt[0] = idct_4x4_sse128;
    g_funs_handle.idct_sqt[1] = idct_8x8_sse128;
    g_funs_handle.idct_sqt_16 = idct_16x16_sse128;

    // --------------------  pixel  ----------------------
    g_funs_handle.add_pel_clip = add_pel_clip_sse128;
    g_funs_handle.avg_pel = avg_pel_sse128;
    g_funs_handle.padding_rows = padding_rows_sse128;
    g_funs_handle.com_cpy = com_cpy_sse128;





    //g_funs_handle.ipflt_EXT_H[IPFILTER_EXT_4 ] = com_if_filter_hor_ver_4_H_sse128;
    //g_funs_handle.ipflt_EXT_H[IPFILTER_EXT_6 ] = com_if_filter_hor_ver_6_H_sse128;
    //g_funs_handle.ipflt_EXT_H[IPFILTER_EXT_10] = com_if_filter_hor_ver_10_H;

    //g_funs_handle.ipflt_EXT_V[IPFILTER_EXT_4 ] = com_if_filter_hor_ver_4_V_sse128;
    //g_funs_handle.ipflt_EXT_V[IPFILTER_EXT_6 ] = com_if_filter_hor_ver_6_V;
    //g_funs_handle.ipflt_EXT_V[IPFILTER_EXT_10] = com_if_filter_hor_ver_10_V;

}


#endif

