/*****************************************************************************
* Authors: Ronggang Wang <rgwang@pkusz.edu.cn>
*          Zhenyu Wang <wangzhenyu@pkusz.edu.cn>
*          Kui Fan <kuifan@pku.edu.cn>
*          Shenghao Zhang <1219759986@qq.com>
* Affiliate: Peking University Shenzhen Graduate School
*****************************************************************************/

#include "intrinsic.h"
#include "global.h"

#if COMPILE_FOR_8BIT

void EdgeLoopVer_sse128( uchar_t* SrcPtr, int istride, bool bChroma, int edge )
{
    pel_t *pTmp = SrcPtr - 4;

    __m128i mAxxxelta, mLeftD, mRightD, mflatedge, medge_th;
    __m128i TL0, TL1, TL2, TL3;
    __m128i TR0, TR1, TR2, TR3;
    __m128i TL0l, TL1l, TL2l;
    __m128i TR0l, TR1l, TR2l;
    __m128i V0, V1, V2, V3, V4, V5;
    __m128i T0, T1, T2, T3, T4, T5, T6, T7;
    __m128i M0, M1, M2, M3;
    __m128i FS, FS_TMP;
    __m128i mTrue, mFalse;
    __m128i mbChroma, medge, mAlpha, mBeta;
    __m128i c_0, c_1, c_2, c_3, c_4, c_5, c_6, c_7, c_8, c_9, c_16, c_17;

    if ( edge )
    {
        medge = _mm_set1_epi8( 0xff );
    }
    else
    {
        medge = _mm_set1_epi8( 0 );
    }

    if ( bChroma )
    {
        mbChroma = _mm_set1_epi8( 0xff );
    }
    else
    {
        mbChroma = _mm_set1_epi8( 0 );
    }

    mTrue  = _mm_set1_epi8( 0xff );
    mFalse = _mm_set1_epi8( 0 );

    mAlpha = _mm_set1_epi16( ( i16s_t )input->Alpha );
    mBeta = _mm_set1_epi16( ( i16s_t )input->Beta );

    c_0  = _mm_set1_epi16( 0 );
    c_1  = _mm_set1_epi16( 1 );
    c_2  = _mm_set1_epi16( 2 );
    c_3  = _mm_set1_epi16( 3 );
    c_4  = _mm_set1_epi16( 4 );
    c_5  = _mm_set1_epi16( 5 );
    c_6  = _mm_set1_epi16( 6 );
    c_7  = _mm_set1_epi16( 7 );
    c_8  = _mm_set1_epi16( 8 );
    c_9  = _mm_set1_epi16( 9 );
    c_16 = _mm_set1_epi16( 16 );
    c_17 = _mm_set1_epi16( 17 );

    // there are 8 circulations when bChroma is false, otherwise there are 16 circulations.
    // ------------------- eight data set -------------------
    T0 = _mm_loadl_epi64( ( __m128i* )( pTmp ) );
    T1 = _mm_loadl_epi64( ( __m128i* )( pTmp + istride ) );
    T2 = _mm_loadl_epi64( ( __m128i* )( pTmp + istride * 2 ) );
    T3 = _mm_loadl_epi64( ( __m128i* )( pTmp + istride * 3 ) );
    T4 = _mm_loadl_epi64( ( __m128i* )( pTmp + istride * 4 ) );
    T5 = _mm_loadl_epi64( ( __m128i* )( pTmp + istride * 5 ) );
    T6 = _mm_loadl_epi64( ( __m128i* )( pTmp + istride * 6 ) );
    T7 = _mm_loadl_epi64( ( __m128i* )( pTmp + istride * 7 ) );

    T0 = _mm_unpacklo_epi8( T0, T1 );
    T1 = _mm_unpacklo_epi8( T2, T3 );
    T2 = _mm_unpacklo_epi8( T4, T5 );
    T3 = _mm_unpacklo_epi8( T6, T7 );

    T4 = _mm_unpacklo_epi16( T0, T1 );
    T5 = _mm_unpacklo_epi16( T2, T3 );
    T6 = _mm_unpackhi_epi16( T0, T1 );
    T7 = _mm_unpackhi_epi16( T2, T3 );

    T0 = _mm_unpacklo_epi32( T4, T5 );
    T1 = _mm_unpackhi_epi32( T4, T5 );
    T2 = _mm_unpacklo_epi32( T6, T7 );
    T3 = _mm_unpackhi_epi32( T6, T7 );

    TL3 = _mm_unpacklo_epi8( T0, c_0 ); // 0 data
    TL2 = _mm_unpackhi_epi8( T0, c_0 ); // 1 data
    TL1 = _mm_unpacklo_epi8( T1, c_0 ); // 2 data
    TL0 = _mm_unpackhi_epi8( T1, c_0 ); // 3 data

    TR0 = _mm_unpacklo_epi8( T2, c_0 ); // 4 data
    TR1 = _mm_unpackhi_epi8( T2, c_0 ); // 5 data
    TR2 = _mm_unpacklo_epi8( T3, c_0 ); // 6 data
    TR3 = _mm_unpackhi_epi8( T3, c_0 ); // 7 data

    // ------------------- eight data set -------------------
#define _mm_subabs_epu16(a, b) _mm_abs_epi16(_mm_subs_epi16(a, b))

    mAxxxelta = _mm_subabs_epu16( TR0, TL0 ); // mAxxxelta = abs(R0 - L0);
    mLeftD    = _mm_subabs_epu16( TL0, TL1 ); // LeftD = abs(L0 - L1);
    mRightD   = _mm_subabs_epu16( TR0, TR1 ); // RightD = abs(R0 - R1);

    medge_th = _mm_min_epi16( c_3, mBeta );

    // flatedge = LeftD < edge_th && RightD < edge_th; [16 bit]
    T1 = _mm_cmplt_epi16( mLeftD, medge_th );
    T2 = _mm_cmplt_epi16( mRightD, medge_th );
    mflatedge = _mm_and_si128( T1, T2 );   // "&&" operatiLon

    // prepare for assigning value to fs
    T1 = _mm_cmplt_epi16( mLeftD, mBeta );
    T2 = _mm_cmplt_epi16( mRightD, mBeta );
    T3 = _mm_cmpgt_epi16( mAxxxelta, mLeftD );
    T4 = _mm_cmpgt_epi16( mAxxxelta, mRightD );
    T5 = _mm_cmplt_epi16( mAxxxelta, mAlpha );

    M0 = _mm_and_si128( T1, T2 );
    M1 = _mm_and_si128( M0, T3 );
    M2 = _mm_and_si128( M1, T4 );
    M3 = _mm_and_si128( M2, T5 );  // M3 is the 1st if's final result

    T1 = _mm_subabs_epu16( TL2, TL0 );
    T2 = _mm_subabs_epu16( TR2, TR0 );
    T3 = _mm_cmplt_epi16( T1, mBeta );
    T4 = _mm_cmplt_epi16( T2, mBeta );
    M0 = _mm_and_si128( T3, T4 ); // M0 is the 2nd if's final result

    // two nested if
    FS = _mm_blendv_epi8( _mm_blendv_epi8( c_2, c_1, _mm_cmpeq_epi16( M0, c_0 ) ), c_0, _mm_cmpeq_epi16( M3, c_0 ) );

    // if   !!!
    T1 = _mm_andnot_si128( mflatedge, mbChroma );
    FS_TMP = _mm_blendv_epi8( mTrue, mFalse, _mm_cmpeq_epi16( FS, c_0 ) );
    T2 = _mm_and_si128( T1, FS_TMP );
    FS = _mm_blendv_epi8( FS, c_1, T2 );

    // if
    T1 = _mm_cmpeq_epi16( FS, c_2 ); //
    T2 = _mm_subabs_epu16( TR3, TR0 );
    T3 = _mm_cmplt_epi16( T2, mBeta ); //
    T4 = _mm_subabs_epu16( TL3, TL0 );
    T5 = _mm_cmplt_epi16( T4, mBeta ); //

    T2 = _mm_andnot_si128( medge, mflatedge ); //
    T4 = _mm_andnot_si128( mbChroma, T1 );  //
    T6 = _mm_and_si128( T2, T4 );
    T7 = _mm_and_si128( T3, T5 );

    T1 = _mm_and_si128( T6, T7 ); // result
    FS = _mm_blendv_epi8( FS, c_3, T1 );

#undef _mm_subabs_epu16

    // 16bit
    TL0l = TL0;
    TL1l = TL1;
    TL2l = TL2;
    TR0l = TR0;
    TR1l = TR1;
    TR2l = TR2;

    /* fs == 1 */
    FS_TMP = _mm_cmpeq_epi16( FS, c_1 );

    if ( FS_TMP.m128i_u64[0] || FS_TMP.m128i_u64[1] )
    {
        T2 = _mm_add_epi16( _mm_sub_epi16( TL0l, TR0l ), c_2 ); // L0 - R0 + 2
        V0 = _mm_add_epi16( _mm_srai_epi16( T2, 2 ), TR0l );

        T2 = _mm_add_epi16( _mm_sub_epi16( TR0l, TL0l ), c_2 ); // R0 - L0 + 2
        V1 = _mm_add_epi16( _mm_srai_epi16( T2, 2 ), TL0l );

        TL0 = _mm_blendv_epi8( TL0, V1, _mm_cmpeq_epi16( FS, c_1 ) );
        TR0 = _mm_blendv_epi8( TR0, V0, _mm_cmpeq_epi16( FS, c_1 ) );
    }

    /* fs == 2 */
    FS_TMP = _mm_cmpeq_epi16( FS, c_2 );

    if ( FS_TMP.m128i_u64[0] || FS_TMP.m128i_u64[1] )
    {
        // -2 1
        T2 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_3 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL1l ), c_8 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_4 );
        T5 = _mm_add_epi16( T2, T3 );
        T6 = _mm_add_epi16( T4, c_8 );
        T7 = _mm_add_epi16( T5, T6 );
        T0 = _mm_srai_epi16( T7, 4 );

        V0 = _mm_add_epi16( T0, TL1l );

        T2 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_3 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR1l ), c_8 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_4 );
        T5 = _mm_add_epi16( T2, T3 );
        T6 = _mm_add_epi16( T4, c_8 );
        T7 = _mm_add_epi16( T5, T6 );
        T0 = _mm_srai_epi16( T7, 4 );

        V1 = _mm_add_epi16( T0, TR1l );

        TL1 = _mm_blendv_epi8( TL1, V0, _mm_cmpeq_epi16( FS, c_2 ) );
        TR1 = _mm_blendv_epi8( TR1, V1, _mm_cmpeq_epi16( FS, c_2 ) );

        // -1 0
        T1 = _mm_sub_epi16( TL2l, TR0l );
        T2 = _mm_sub_epi16( TR1l, TL0l );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TL1l, TR0l ), c_4 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_9 );
        T5 = _mm_add_epi16( T1, T2 );
        T6 = _mm_add_epi16( T3, T4 );
        T7 = _mm_add_epi16( T5, T6 );
        T7 = _mm_add_epi16( T7, c_8 );
        T0 = _mm_srai_epi16( T7, 4 );

        V2 = _mm_add_epi16( T0, TL0l );

        T1 = _mm_sub_epi16( TR2l, TL0l );
        T2 = _mm_sub_epi16( TL1l, TR0l );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TR1l, TL0l ), c_4 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_9 );
        T5 = _mm_add_epi16( T1, T2 );
        T6 = _mm_add_epi16( T3, T4 );
        T7 = _mm_add_epi16( T5, T6 );
        T7 = _mm_add_epi16( T7, c_8 );
        T0 = _mm_srai_epi16( T7, 4 );

        V3 = _mm_add_epi16( T0, TR0l );

        TL0 = _mm_blendv_epi8( TL0, V2, _mm_cmpeq_epi16( FS, c_2 ) ); // ѡV2
        TR0 = _mm_blendv_epi8( TR0, V3, _mm_cmpeq_epi16( FS, c_2 ) );
    }

    /* fs == 3 */
    FS_TMP = _mm_cmpeq_epi16( FS, c_3 );

    if ( FS_TMP.m128i_u64[0] || FS_TMP.m128i_u64[1] )
    {
        // -3 2
        T2 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_4 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL2l ), c_5 );
        T4 = _mm_add_epi16( T2, T3 );
        T4 = _mm_add_epi16( T4, c_4 );
        T5 = _mm_srai_epi16( T4, 3 );
        V0 = _mm_add_epi16( T5, TL2l );

        T2 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_4 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR2l ), c_5 );
        T4 = _mm_add_epi16( T2, T3 );
        T4 = _mm_add_epi16( T4, c_4 );
        T5 = _mm_srai_epi16( T4, 3 );
        V1 = _mm_add_epi16( T5, TR2l );

        TL2 = _mm_blendv_epi8( TL2, V0, _mm_cmpeq_epi16( FS, c_3 ) );
        TR2 = _mm_blendv_epi8( TR2, V1, _mm_cmpeq_epi16( FS, c_3 ) );

        // -2 1
        T2 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL1l ), c_16 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_6 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_7 );
        T5 = _mm_add_epi16( T2, T3 );
        T6 = _mm_add_epi16( T4, c_8 );
        T7 = _mm_add_epi16( T5, T6 );
        T0 = _mm_srai_epi16( T7, 4 );
        V2 = _mm_add_epi16( T0, TL1l );

        T2 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR1l ), c_16 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_6 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_7 );
        T5 = _mm_add_epi16( T2, T3 );
        T6 = _mm_add_epi16( T4, c_8 );
        T7 = _mm_add_epi16( T5, T6 );
        T0 = _mm_srai_epi16( T7, 4 );
        V3 = _mm_add_epi16( T0, TR1l );

        TL1 = _mm_blendv_epi8( TL1, V2, _mm_cmpeq_epi16( FS, c_3 ) );
        TR1 = _mm_blendv_epi8( TR1, V3, _mm_cmpeq_epi16( FS, c_3 ) );

        // -1 0
        T2 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_9 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_6 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_17 );
        T5 = _mm_add_epi16( T2, T3 );
        T6 = _mm_add_epi16( T4, c_16 );
        T7 = _mm_add_epi16( T5, T6 );
        T0 = _mm_srai_epi16( T7, 5 );
        V4 = _mm_add_epi16( T0, TL0l );

        T2 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_9 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_6 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_17 );
        T5 = _mm_add_epi16( T2, T3 );
        T6 = _mm_add_epi16( T4, c_16 );
        T7 = _mm_add_epi16( T5, T6 );
        T0 = _mm_srai_epi16( T7, 5 );
        V5 = _mm_add_epi16( T0, TR0l );

        TL0 = _mm_blendv_epi8( TL0, V4, _mm_cmpeq_epi16( FS, c_3 ) );
        TR0 = _mm_blendv_epi8( TR0, V5, _mm_cmpeq_epi16( FS, c_3 ) );
    }

    /* stroe result */
    T0 = _mm_packus_epi16( TL3, TR0 );
    T1 = _mm_packus_epi16( TL2, TR1 );
    T2 = _mm_packus_epi16( TL1, TR2 );
    T3 = _mm_packus_epi16( TL0, TR3 );

    T4 = _mm_unpacklo_epi8( T0, T1 );
    T5 = _mm_unpacklo_epi8( T2, T3 );
    T6 = _mm_unpackhi_epi8( T0, T1 );
    T7 = _mm_unpackhi_epi8( T2, T3 );

    V0 = _mm_unpacklo_epi16( T4, T5 );
    V1 = _mm_unpacklo_epi16( T6, T7 );
    V2 = _mm_unpackhi_epi16( T4, T5 );
    V3 = _mm_unpackhi_epi16( T6, T7 );

    T0 = _mm_unpacklo_epi32( V0, V1 );
    T1 = _mm_unpackhi_epi32( V0, V1 );
    T2 = _mm_unpacklo_epi32( V2, V3 );
    T3 = _mm_unpackhi_epi32( V2, V3 );

    //pTmp = SrcPtr - 4;
    _mm_storel_epi64( ( __m128i* )( pTmp ), T0 );
    pTmp += istride;
    _mm_storel_epi64( ( __m128i* )( pTmp ), _mm_srli_si128( T0, 8 ) );
    pTmp += istride;
    _mm_storel_epi64( ( __m128i* )( pTmp ), T1 );
    pTmp += istride;
    _mm_storel_epi64( ( __m128i* )( pTmp ), _mm_srli_si128( T1, 8 ) );
    pTmp += istride;
    _mm_storel_epi64( ( __m128i* )( pTmp ), T2 );
    pTmp += istride;
    _mm_storel_epi64( ( __m128i* )( pTmp ), _mm_srli_si128( T2, 8 ) );
    pTmp += istride;
    _mm_storel_epi64( ( __m128i* )( pTmp ), T3 );
    pTmp += istride;
    _mm_storel_epi64( ( __m128i* )( pTmp ), _mm_srli_si128( T3, 8 ) );


    // -----------------  bChroma is flase  --------------------
    if ( !bChroma )
    {
        pTmp = SrcPtr + istride * 8 - 4;

        T0 = _mm_loadl_epi64( ( __m128i* )( pTmp ) );
        T1 = _mm_loadl_epi64( ( __m128i* )( pTmp + istride ) );
        T2 = _mm_loadl_epi64( ( __m128i* )( pTmp + istride * 2 ) );
        T3 = _mm_loadl_epi64( ( __m128i* )( pTmp + istride * 3 ) );
        T4 = _mm_loadl_epi64( ( __m128i* )( pTmp + istride * 4 ) );
        T5 = _mm_loadl_epi64( ( __m128i* )( pTmp + istride * 5 ) );
        T6 = _mm_loadl_epi64( ( __m128i* )( pTmp + istride * 6 ) );
        T7 = _mm_loadl_epi64( ( __m128i* )( pTmp + istride * 7 ) );

        T0 = _mm_unpacklo_epi8( T0, T1 );
        T1 = _mm_unpacklo_epi8( T2, T3 );
        T2 = _mm_unpacklo_epi8( T4, T5 );
        T3 = _mm_unpacklo_epi8( T6, T7 );

        T4 = _mm_unpacklo_epi16( T0, T1 );
        T5 = _mm_unpacklo_epi16( T2, T3 );
        T6 = _mm_unpackhi_epi16( T0, T1 );
        T7 = _mm_unpackhi_epi16( T2, T3 );

        T0 = _mm_unpacklo_epi32( T4, T5 );
        T1 = _mm_unpackhi_epi32( T4, T5 );
        T2 = _mm_unpacklo_epi32( T6, T7 );
        T3 = _mm_unpackhi_epi32( T6, T7 );

        TL3 = _mm_unpacklo_epi8( T0, c_0 ); // 0 data
        TL2 = _mm_unpackhi_epi8( T0, c_0 ); // 1 data
        TL1 = _mm_unpacklo_epi8( T1, c_0 ); // 2 data
        TL0 = _mm_unpackhi_epi8( T1, c_0 ); // 3 data

        TR0 = _mm_unpacklo_epi8( T2, c_0 ); // 4 data
        TR1 = _mm_unpackhi_epi8( T2, c_0 ); // 5 data
        TR2 = _mm_unpacklo_epi8( T3, c_0 ); // 6 data
        TR3 = _mm_unpackhi_epi8( T3, c_0 ); // 7 data

        // ------------------- eight data set -------------------
#define _mm_subabs_epu16(a, b) _mm_abs_epi16(_mm_subs_epi16(a, b))

        mAxxxelta = _mm_subabs_epu16( TR0, TL0 ); // mAxxxelta = abs(R0 - L0);
        mLeftD = _mm_subabs_epu16( TL0, TL1 ); // LeftD = abs(L0 - L1);
        mRightD = _mm_subabs_epu16( TR0, TR1 ); // RightD = abs(R0 - R1);

        medge_th = _mm_min_epi16( c_3, mBeta );

        // flatedge = LeftD < edge_th && RightD < edge_th; [16 bit]
        T1 = _mm_cmplt_epi16( mLeftD, medge_th );
        T2 = _mm_cmplt_epi16( mRightD, medge_th );
        mflatedge = _mm_and_si128( T1, T2 );   // "&&" operatiLon

        // prepare for assigning value to fs
        T1 = _mm_cmplt_epi16( mLeftD, mBeta );
        T2 = _mm_cmplt_epi16( mRightD, mBeta );
        T3 = _mm_cmpgt_epi16( mAxxxelta, mLeftD );
        T4 = _mm_cmpgt_epi16( mAxxxelta, mRightD );
        T5 = _mm_cmplt_epi16( mAxxxelta, mAlpha );

        M0 = _mm_and_si128( T1, T2 );
        M1 = _mm_and_si128( M0, T3 );
        M2 = _mm_and_si128( M1, T4 );
        M3 = _mm_and_si128( M2, T5 );  // M3 is the 1st if's final result

        T1 = _mm_subabs_epu16( TL2, TL0 );
        T2 = _mm_subabs_epu16( TR2, TR0 );
        T3 = _mm_cmplt_epi16( T1, mBeta );
        T4 = _mm_cmplt_epi16( T2, mBeta );
        M0 = _mm_and_si128( T3, T4 ); // M0 is the 2nd if's final result

        // two nested if
        FS = _mm_blendv_epi8( _mm_blendv_epi8( c_2, c_1, _mm_cmpeq_epi16( M0, c_0 ) ), c_0, _mm_cmpeq_epi16( M3, c_0 ) );

        // if   !!!
        T1 = _mm_andnot_si128( mflatedge, mbChroma );
        FS_TMP = _mm_blendv_epi8( mTrue, mFalse, _mm_cmpeq_epi16( FS, c_0 ) );
        T2 = _mm_and_si128( T1, FS_TMP );
        FS = _mm_blendv_epi8( FS, c_1, T2 );

        // if
        T1 = _mm_cmpeq_epi16( FS, c_2 ); //
        T2 = _mm_subabs_epu16( TR3, TR0 );
        T3 = _mm_cmplt_epi16( T2, mBeta ); //
        T4 = _mm_subabs_epu16( TL3, TL0 );
        T5 = _mm_cmplt_epi16( T4, mBeta ); //

        T2 = _mm_andnot_si128( medge, mflatedge ); //
        T4 = _mm_andnot_si128( mbChroma, T1 );  //
        T6 = _mm_and_si128( T2, T4 );
        T7 = _mm_and_si128( T3, T5 );

        T1 = _mm_and_si128( T6, T7 ); // result
        FS = _mm_blendv_epi8( FS, c_3, T1 );

#undef _mm_subabs_epu16

        // 16bit
        TL0l = TL0;
        TL1l = TL1;
        TL2l = TL2;
        TR0l = TR0;
        TR1l = TR1;
        TR2l = TR2;

        /* fs == 1 */
        FS_TMP = _mm_cmpeq_epi16( FS, c_1 );

        if ( FS_TMP.m128i_u64[0] || FS_TMP.m128i_u64[1] )
        {
            T2 = _mm_add_epi16( _mm_sub_epi16( TL0l, TR0l ), c_2 ); // L0 - R0 + 2
            V0 = _mm_add_epi16( _mm_srai_epi16( T2, 2 ), TR0l );

            T2 = _mm_add_epi16( _mm_sub_epi16( TR0l, TL0l ), c_2 ); // R0 - L0 + 2
            V1 = _mm_add_epi16( _mm_srai_epi16( T2, 2 ), TL0l );

            TL0 = _mm_blendv_epi8( TL0, V1, _mm_cmpeq_epi16( FS, c_1 ) );
            TR0 = _mm_blendv_epi8( TR0, V0, _mm_cmpeq_epi16( FS, c_1 ) );
        }

        /* fs == 2 */
        FS_TMP = _mm_cmpeq_epi16( FS, c_2 );

        if ( FS_TMP.m128i_u64[0] || FS_TMP.m128i_u64[1] )
        {
            // -2 1
            T2 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_3 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL1l ), c_8 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_4 );
            T5 = _mm_add_epi16( T2, T3 );
            T6 = _mm_add_epi16( T4, c_8 );
            T7 = _mm_add_epi16( T5, T6 );
            T0 = _mm_srai_epi16( T7, 4 );

            V0 = _mm_add_epi16( T0, TL1l );

            T2 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_3 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR1l ), c_8 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_4 );
            T5 = _mm_add_epi16( T2, T3 );
            T6 = _mm_add_epi16( T4, c_8 );
            T7 = _mm_add_epi16( T5, T6 );
            T0 = _mm_srai_epi16( T7, 4 );

            V1 = _mm_add_epi16( T0, TR1l );

            TL1 = _mm_blendv_epi8( TL1, V0, _mm_cmpeq_epi16( FS, c_2 ) );
            TR1 = _mm_blendv_epi8( TR1, V1, _mm_cmpeq_epi16( FS, c_2 ) );

            // -1 0
            T1 = _mm_sub_epi16( TL2l, TR0l );
            T2 = _mm_sub_epi16( TR1l, TL0l );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TL1l, TR0l ), c_4 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_9 );
            T5 = _mm_add_epi16( T1, T2 );
            T6 = _mm_add_epi16( T3, T4 );
            T7 = _mm_add_epi16( T5, T6 );
            T7 = _mm_add_epi16( T7, c_8 );
            T0 = _mm_srai_epi16( T7, 4 );

            V2 = _mm_add_epi16( T0, TL0l );

            T1 = _mm_sub_epi16( TR2l, TL0l );
            T2 = _mm_sub_epi16( TL1l, TR0l );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TR1l, TL0l ), c_4 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_9 );
            T5 = _mm_add_epi16( T1, T2 );
            T6 = _mm_add_epi16( T3, T4 );
            T7 = _mm_add_epi16( T5, T6 );
            T7 = _mm_add_epi16( T7, c_8 );
            T0 = _mm_srai_epi16( T7, 4 );

            V3 = _mm_add_epi16( T0, TR0l );

            TL0 = _mm_blendv_epi8( TL0, V2, _mm_cmpeq_epi16( FS, c_2 ) ); // ѡV2
            TR0 = _mm_blendv_epi8( TR0, V3, _mm_cmpeq_epi16( FS, c_2 ) );
        }

        /* fs == 3 */
        FS_TMP = _mm_cmpeq_epi16( FS, c_3 );

        if ( FS_TMP.m128i_u64[0] || FS_TMP.m128i_u64[1] )
        {
            // -3 2
            T2 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_4 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL2l ), c_5 );
            T4 = _mm_add_epi16( T2, T3 );
            T4 = _mm_add_epi16( T4, c_4 );
            T5 = _mm_srai_epi16( T4, 3 );
            V0 = _mm_add_epi16( T5, TL2l );

            T2 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_4 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR2l ), c_5 );
            T4 = _mm_add_epi16( T2, T3 );
            T4 = _mm_add_epi16( T4, c_4 );
            T5 = _mm_srai_epi16( T4, 3 );
            V1 = _mm_add_epi16( T5, TR2l );

            TL2 = _mm_blendv_epi8( TL2, V0, _mm_cmpeq_epi16( FS, c_3 ) );
            TR2 = _mm_blendv_epi8( TR2, V1, _mm_cmpeq_epi16( FS, c_3 ) );

            // -2 1
            T2 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL1l ), c_16 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_6 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_7 );
            T5 = _mm_add_epi16( T2, T3 );
            T6 = _mm_add_epi16( T4, c_8 );
            T7 = _mm_add_epi16( T5, T6 );
            T0 = _mm_srai_epi16( T7, 4 );
            V2 = _mm_add_epi16( T0, TL1l );

            T2 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR1l ), c_16 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_6 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_7 );
            T5 = _mm_add_epi16( T2, T3 );
            T6 = _mm_add_epi16( T4, c_8 );
            T7 = _mm_add_epi16( T5, T6 );
            T0 = _mm_srai_epi16( T7, 4 );
            V3 = _mm_add_epi16( T0, TR1l );

            TL1 = _mm_blendv_epi8( TL1, V2, _mm_cmpeq_epi16( FS, c_3 ) );
            TR1 = _mm_blendv_epi8( TR1, V3, _mm_cmpeq_epi16( FS, c_3 ) );

            // -1 0
            T2 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_9 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_6 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_17 );
            T5 = _mm_add_epi16( T2, T3 );
            T6 = _mm_add_epi16( T4, c_16 );
            T7 = _mm_add_epi16( T5, T6 );
            T0 = _mm_srai_epi16( T7, 5 );
            V4 = _mm_add_epi16( T0, TL0l );

            T2 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_9 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_6 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_17 );
            T5 = _mm_add_epi16( T2, T3 );
            T6 = _mm_add_epi16( T4, c_16 );
            T7 = _mm_add_epi16( T5, T6 );
            T0 = _mm_srai_epi16( T7, 5 );
            V5 = _mm_add_epi16( T0, TR0l );

            TL0 = _mm_blendv_epi8( TL0, V4, _mm_cmpeq_epi16( FS, c_3 ) );
            TR0 = _mm_blendv_epi8( TR0, V5, _mm_cmpeq_epi16( FS, c_3 ) );
        }

        /* stroe result */
        T0 = _mm_packus_epi16( TL3, TR0 );
        T1 = _mm_packus_epi16( TL2, TR1 );
        T2 = _mm_packus_epi16( TL1, TR2 );
        T3 = _mm_packus_epi16( TL0, TR3 );

        T4 = _mm_unpacklo_epi8( T0, T1 );
        T5 = _mm_unpacklo_epi8( T2, T3 );
        T6 = _mm_unpackhi_epi8( T0, T1 );
        T7 = _mm_unpackhi_epi8( T2, T3 );

        V0 = _mm_unpacklo_epi16( T4, T5 );
        V1 = _mm_unpacklo_epi16( T6, T7 );
        V2 = _mm_unpackhi_epi16( T4, T5 );
        V3 = _mm_unpackhi_epi16( T6, T7 );

        T0 = _mm_unpacklo_epi32( V0, V1 );
        T1 = _mm_unpackhi_epi32( V0, V1 );
        T2 = _mm_unpacklo_epi32( V2, V3 );
        T3 = _mm_unpackhi_epi32( V2, V3 );

        pTmp = SrcPtr + istride * 8 - 4;
        _mm_storel_epi64( ( __m128i* )( pTmp ), T0 );
        pTmp += istride;
        _mm_storel_epi64( ( __m128i* )( pTmp ), _mm_srli_si128( T0, 8 ) );
        pTmp += istride;
        _mm_storel_epi64( ( __m128i* )( pTmp ), T1 );
        pTmp += istride;
        _mm_storel_epi64( ( __m128i* )( pTmp ), _mm_srli_si128( T1, 8 ) );
        pTmp += istride;
        _mm_storel_epi64( ( __m128i* )( pTmp ), T2 );
        pTmp += istride;
        _mm_storel_epi64( ( __m128i* )( pTmp ), _mm_srli_si128( T2, 8 ) );
        pTmp += istride;
        _mm_storel_epi64( ( __m128i* )( pTmp ), T3 );
        pTmp += istride;
        _mm_storel_epi64( ( __m128i* )( pTmp ), _mm_srli_si128( T3, 8 ) );
    }

}


void EdgeLoopHor_sse128( uchar_t* SrcPtr, int istride, bool bChroma, int edge )
{
    int inc = istride;
    int inc2 = istride << 1;
    int inc3 = istride * 3;
    int inc4 = istride << 2;

    __m128i mAxxxelta, mLeftD, mRightD, mflatedge, medge_th;
    __m128i TL0, TL1, TL2, TL3;
    __m128i TR0, TR1, TR2, TR3;
    __m128i TL0l, TL1l, TL2l;
    __m128i TR0l, TR1l, TR2l;
    __m128i V0, V1, V2, V3, V4, V5;
    __m128i T0, T1, T2, T3, T4, T5, T6, T7;
    __m128i M0, M1, M2, M3;
    __m128i FS, FS_TMP;
    __m128i mTrue, mFalse;
    __m128i mbChroma, medge, mAlpha, mBeta;
    __m128i c_0, c_1, c_2, c_3, c_4, c_5, c_6, c_7, c_8, c_9, c_16, c_17;

    if ( edge )
    {
        medge = _mm_set1_epi8( 0xff );
    }
    else
    {
        medge = _mm_set1_epi8( 0 );
    }

    if ( bChroma )
    {
        mbChroma = _mm_set1_epi8( 0xff );
    }
    else
    {
        mbChroma = _mm_set1_epi8( 0 );
    }

    mTrue = _mm_set1_epi8( 0xff );
    mFalse = _mm_set1_epi8( 0 );

    mAlpha = _mm_set1_epi16( ( i16s_t )input->Alpha );
    mBeta = _mm_set1_epi16( ( i16s_t )input->Beta );

    c_0 = _mm_set1_epi16( 0 );
    c_1 = _mm_set1_epi16( 1 );
    c_2 = _mm_set1_epi16( 2 );
    c_3 = _mm_set1_epi16( 3 );
    c_4 = _mm_set1_epi16( 4 );
    c_5 = _mm_set1_epi16( 5 );
    c_6 = _mm_set1_epi16( 6 );
    c_7 = _mm_set1_epi16( 7 );
    c_8 = _mm_set1_epi16( 8 );
    c_9 = _mm_set1_epi16( 9 );
    c_16 = _mm_set1_epi16( 16 );
    c_17 = _mm_set1_epi16( 17 );

    // there are 8 circulations when bChroma is false, otherwise there are 16 circulations.
    // ------------------- eight data set -------------------
    TL3 = _mm_loadl_epi64( ( __m128i* )( SrcPtr - inc4 ) );
    TL2 = _mm_loadl_epi64( ( __m128i* )( SrcPtr - inc3 ) );
    TL1 = _mm_loadl_epi64( ( __m128i* )( SrcPtr - inc2 ) );
    TL0 = _mm_loadl_epi64( ( __m128i* )( SrcPtr - inc ) );
    TR0 = _mm_loadl_epi64( ( __m128i* )( SrcPtr + 0 ) );
    TR1 = _mm_loadl_epi64( ( __m128i* )( SrcPtr + inc ) );
    TR2 = _mm_loadl_epi64( ( __m128i* )( SrcPtr + inc2 ) );
    TR3 = _mm_loadl_epi64( ( __m128i* )( SrcPtr + inc3 ) );

    TL3 = _mm_unpacklo_epi8( TL3, c_0 );
    TL2 = _mm_unpacklo_epi8( TL2, c_0 );
    TL1 = _mm_unpacklo_epi8( TL1, c_0 );
    TL0 = _mm_unpacklo_epi8( TL0, c_0 );
    TR0 = _mm_unpacklo_epi8( TR0, c_0 );
    TR1 = _mm_unpacklo_epi8( TR1, c_0 );
    TR2 = _mm_unpacklo_epi8( TR2, c_0 );
    TR3 = _mm_unpacklo_epi8( TR3, c_0 );

    // ------------------- eight data set -------------------
#define _mm_subabs_epu16(a, b) _mm_abs_epi16(_mm_subs_epi16(a, b))

    mAxxxelta = _mm_subabs_epu16( TR0, TL0 ); // mAxxxelta = abs(R0 - L0);
    mLeftD = _mm_subabs_epu16( TL0, TL1 ); // LeftD = abs(L0 - L1);
    mRightD = _mm_subabs_epu16( TR0, TR1 ); // RightD = abs(R0 - R1);

    medge_th = _mm_min_epi16( c_3, mBeta );

    // flatedge = LeftD < edge_th && RightD < edge_th; [16 bit]
    T1 = _mm_cmplt_epi16( mLeftD, medge_th );
    T2 = _mm_cmplt_epi16( mRightD, medge_th );
    mflatedge = _mm_and_si128( T1, T2 );   // "&&" operatiLon

    // prepare for assigning value to fs
    T1 = _mm_cmplt_epi16( mLeftD, mBeta );
    T2 = _mm_cmplt_epi16( mRightD, mBeta );
    T3 = _mm_cmpgt_epi16( mAxxxelta, mLeftD );
    T4 = _mm_cmpgt_epi16( mAxxxelta, mRightD );
    T5 = _mm_cmplt_epi16( mAxxxelta, mAlpha );

    M0 = _mm_and_si128( T1, T2 );
    M1 = _mm_and_si128( M0, T3 );
    M2 = _mm_and_si128( M1, T4 );
    M3 = _mm_and_si128( M2, T5 );  // M3 is the 1st if's final result

    T1 = _mm_subabs_epu16( TL2, TL0 );
    T2 = _mm_subabs_epu16( TR2, TR0 );
    T3 = _mm_cmplt_epi16( T1, mBeta );
    T4 = _mm_cmplt_epi16( T2, mBeta );
    M0 = _mm_and_si128( T3, T4 ); // M0 is the 2nd if's final result

    // two nested if
    FS = _mm_blendv_epi8( _mm_blendv_epi8( c_2, c_1, _mm_cmpeq_epi16( M0, c_0 ) ), c_0, _mm_cmpeq_epi16( M3, c_0 ) );

    // if   !!!
    T1 = _mm_andnot_si128( mflatedge, mbChroma );
    FS_TMP = _mm_blendv_epi8( mTrue, mFalse, _mm_cmpeq_epi16( FS, c_0 ) );
    T2 = _mm_and_si128( T1, FS_TMP );
    FS = _mm_blendv_epi8( FS, c_1, T2 );

    // if
    T1 = _mm_cmpeq_epi16( FS, c_2 ); //
    T2 = _mm_subabs_epu16( TR3, TR0 );
    T3 = _mm_cmplt_epi16( T2, mBeta ); //
    T4 = _mm_subabs_epu16( TL3, TL0 );
    T5 = _mm_cmplt_epi16( T4, mBeta ); //

    T2 = _mm_andnot_si128( medge, mflatedge ); //
    T4 = _mm_andnot_si128( mbChroma, T1 );  //
    T6 = _mm_and_si128( T2, T4 );
    T7 = _mm_and_si128( T3, T5 );

    T1 = _mm_and_si128( T6, T7 ); // result
    FS = _mm_blendv_epi8( FS, c_3, T1 );

#undef _mm_subabs_epu16

    // 16bit
    TL0l = TL0;
    TL1l = TL1;
    TL2l = TL2;
    TR0l = TR0;
    TR1l = TR1;
    TR2l = TR2;

    /* fs == 1 */
    FS_TMP = _mm_cmpeq_epi16( FS, c_1 );

    if ( FS_TMP.m128i_u64[0] || FS_TMP.m128i_u64[1] )
    {
        T2 = _mm_add_epi16( _mm_sub_epi16( TL0l, TR0l ), c_2 ); // L0 - R0 + 2
        V0 = _mm_add_epi16( _mm_srai_epi16( T2, 2 ), TR0l );

        T2 = _mm_add_epi16( _mm_sub_epi16( TR0l, TL0l ), c_2 ); // R0 - L0 + 2
        V1 = _mm_add_epi16( _mm_srai_epi16( T2, 2 ), TL0l );

        TL0 = _mm_blendv_epi8( TL0, V1, _mm_cmpeq_epi16( FS, c_1 ) );
        TR0 = _mm_blendv_epi8( TR0, V0, _mm_cmpeq_epi16( FS, c_1 ) );
    }

    /* fs == 2 */
    FS_TMP = _mm_cmpeq_epi16( FS, c_2 );

    if ( FS_TMP.m128i_u64[0] || FS_TMP.m128i_u64[1] )
    {
        // -2 1
        T2 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_3 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL1l ), c_8 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_4 );
        T5 = _mm_add_epi16( T2, T3 );
        T6 = _mm_add_epi16( T4, c_8 );
        T7 = _mm_add_epi16( T5, T6 );
        T0 = _mm_srai_epi16( T7, 4 );

        V0 = _mm_add_epi16( T0, TL1l );

        T2 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_3 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR1l ), c_8 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_4 );
        T5 = _mm_add_epi16( T2, T3 );
        T6 = _mm_add_epi16( T4, c_8 );
        T7 = _mm_add_epi16( T5, T6 );
        T0 = _mm_srai_epi16( T7, 4 );

        V1 = _mm_add_epi16( T0, TR1l );

        TL1 = _mm_blendv_epi8( TL1, V0, _mm_cmpeq_epi16( FS, c_2 ) );
        TR1 = _mm_blendv_epi8( TR1, V1, _mm_cmpeq_epi16( FS, c_2 ) );

        // -1 0
        T1 = _mm_sub_epi16( TL2l, TR0l );
        T2 = _mm_sub_epi16( TR1l, TL0l );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TL1l, TR0l ), c_4 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_9 );
        T5 = _mm_add_epi16( T1, T2 );
        T6 = _mm_add_epi16( T3, T4 );
        T7 = _mm_add_epi16( T5, T6 );
        T7 = _mm_add_epi16( T7, c_8 );
        T0 = _mm_srai_epi16( T7, 4 );

        V2 = _mm_add_epi16( T0, TL0l );

        T1 = _mm_sub_epi16( TR2l, TL0l );
        T2 = _mm_sub_epi16( TL1l, TR0l );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TR1l, TL0l ), c_4 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_9 );
        T5 = _mm_add_epi16( T1, T2 );
        T6 = _mm_add_epi16( T3, T4 );
        T7 = _mm_add_epi16( T5, T6 );
        T7 = _mm_add_epi16( T7, c_8 );
        T0 = _mm_srai_epi16( T7, 4 );

        V3 = _mm_add_epi16( T0, TR0l );

        TL0 = _mm_blendv_epi8( TL0, V2, _mm_cmpeq_epi16( FS, c_2 ) ); // ѡV2
        TR0 = _mm_blendv_epi8( TR0, V3, _mm_cmpeq_epi16( FS, c_2 ) );
    }

    /* fs == 3 */
    FS_TMP = _mm_cmpeq_epi16( FS, c_3 );

    if ( FS_TMP.m128i_u64[0] || FS_TMP.m128i_u64[1] )
    {
        // -3 2
        T2 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_4 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL2l ), c_5 );
        T4 = _mm_add_epi16( T2, T3 );
        T4 = _mm_add_epi16( T4, c_4 );
        T5 = _mm_srai_epi16( T4, 3 );
        V0 = _mm_add_epi16( T5, TL2l );

        T2 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_4 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR2l ), c_5 );
        T4 = _mm_add_epi16( T2, T3 );
        T4 = _mm_add_epi16( T4, c_4 );
        T5 = _mm_srai_epi16( T4, 3 );
        V1 = _mm_add_epi16( T5, TR2l );

        TL2 = _mm_blendv_epi8( TL2, V0, _mm_cmpeq_epi16( FS, c_3 ) );
        TR2 = _mm_blendv_epi8( TR2, V1, _mm_cmpeq_epi16( FS, c_3 ) );

        // -2 1
        T2 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL1l ), c_16 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_6 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_7 );
        T5 = _mm_add_epi16( T2, T3 );
        T6 = _mm_add_epi16( T4, c_8 );
        T7 = _mm_add_epi16( T5, T6 );
        T0 = _mm_srai_epi16( T7, 4 );
        V2 = _mm_add_epi16( T0, TL1l );

        T2 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR1l ), c_16 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_6 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_7 );
        T5 = _mm_add_epi16( T2, T3 );
        T6 = _mm_add_epi16( T4, c_8 );
        T7 = _mm_add_epi16( T5, T6 );
        T0 = _mm_srai_epi16( T7, 4 );
        V3 = _mm_add_epi16( T0, TR1l );

        TL1 = _mm_blendv_epi8( TL1, V2, _mm_cmpeq_epi16( FS, c_3 ) );
        TR1 = _mm_blendv_epi8( TR1, V3, _mm_cmpeq_epi16( FS, c_3 ) );

        // -1 0
        T2 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_9 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_6 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_17 );
        T5 = _mm_add_epi16( T2, T3 );
        T6 = _mm_add_epi16( T4, c_16 );
        T7 = _mm_add_epi16( T5, T6 );
        T0 = _mm_srai_epi16( T7, 5 );
        V4 = _mm_add_epi16( T0, TL0l );

        T2 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_9 );
        T3 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_6 );
        T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_17 );
        T5 = _mm_add_epi16( T2, T3 );
        T6 = _mm_add_epi16( T4, c_16 );
        T7 = _mm_add_epi16( T5, T6 );
        T0 = _mm_srai_epi16( T7, 5 );
        V5 = _mm_add_epi16( T0, TR0l );

        TL0 = _mm_blendv_epi8( TL0, V4, _mm_cmpeq_epi16( FS, c_3 ) );
        TR0 = _mm_blendv_epi8( TR0, V5, _mm_cmpeq_epi16( FS, c_3 ) );

        /* stroe result */
        _mm_storel_epi64( ( __m128i* )( SrcPtr - inc ), _mm_packus_epi16( TL0, c_0 ) );
        _mm_storel_epi64( ( __m128i* )( SrcPtr - 0 ), _mm_packus_epi16( TR0, c_0 ) );

        _mm_storel_epi64( ( __m128i* )( SrcPtr - inc2 ), _mm_packus_epi16( TL1, c_0 ) );
        _mm_storel_epi64( ( __m128i* )( SrcPtr + inc ), _mm_packus_epi16( TR1, c_0 ) );

        _mm_storel_epi64( ( __m128i* )( SrcPtr - inc3 ), _mm_packus_epi16( TL2, c_0 ) );
        _mm_storel_epi64( ( __m128i* )( SrcPtr + inc2 ), _mm_packus_epi16( TR2, c_0 ) );
    }
    else
    {
        /* stroe result */
        _mm_storel_epi64( ( __m128i* )( SrcPtr - inc ), _mm_packus_epi16( TL0, c_0 ) );
        _mm_storel_epi64( ( __m128i* )( SrcPtr - 0 ), _mm_packus_epi16( TR0, c_0 ) );

        _mm_storel_epi64( ( __m128i* )( SrcPtr - inc2 ), _mm_packus_epi16( TL1, c_0 ) );
        _mm_storel_epi64( ( __m128i* )( SrcPtr + inc ), _mm_packus_epi16( TR1, c_0 ) );
    }


    // -----------------  bChroma is flase  --------------------
    if ( !bChroma )
    {
        SrcPtr = SrcPtr + 8 ;
        TL3 = _mm_loadl_epi64( ( __m128i* )( SrcPtr - inc4 ) );
        TL2 = _mm_loadl_epi64( ( __m128i* )( SrcPtr - inc3 ) );
        TL1 = _mm_loadl_epi64( ( __m128i* )( SrcPtr - inc2 ) );
        TL0 = _mm_loadl_epi64( ( __m128i* )( SrcPtr - inc ) );
        TR0 = _mm_loadl_epi64( ( __m128i* )( SrcPtr + 0 ) );
        TR1 = _mm_loadl_epi64( ( __m128i* )( SrcPtr + inc ) );
        TR2 = _mm_loadl_epi64( ( __m128i* )( SrcPtr + inc2 ) );
        TR3 = _mm_loadl_epi64( ( __m128i* )( SrcPtr + inc3 ) );

        TL3 = _mm_unpacklo_epi8( TL3, c_0 );
        TL2 = _mm_unpacklo_epi8( TL2, c_0 );
        TL1 = _mm_unpacklo_epi8( TL1, c_0 );
        TL0 = _mm_unpacklo_epi8( TL0, c_0 );
        TR0 = _mm_unpacklo_epi8( TR0, c_0 );
        TR1 = _mm_unpacklo_epi8( TR1, c_0 );
        TR2 = _mm_unpacklo_epi8( TR2, c_0 );
        TR3 = _mm_unpacklo_epi8( TR3, c_0 );

        // ------------------- eight data set -------------------
#define _mm_subabs_epu16(a, b) _mm_abs_epi16(_mm_subs_epi16(a, b))

        mAxxxelta = _mm_subabs_epu16( TR0, TL0 ); // mAxxxelta = abs(R0 - L0);
        mLeftD = _mm_subabs_epu16( TL0, TL1 ); // LeftD = abs(L0 - L1);
        mRightD = _mm_subabs_epu16( TR0, TR1 ); // RightD = abs(R0 - R1);

        medge_th = _mm_min_epi16( c_3, mBeta );

        // flatedge = LeftD < edge_th && RightD < edge_th; [16 bit]
        T1 = _mm_cmplt_epi16( mLeftD, medge_th );
        T2 = _mm_cmplt_epi16( mRightD, medge_th );
        mflatedge = _mm_and_si128( T1, T2 );   // "&&" operatiLon

        // prepare for assigning value to fs
        T1 = _mm_cmplt_epi16( mLeftD, mBeta );
        T2 = _mm_cmplt_epi16( mRightD, mBeta );
        T3 = _mm_cmpgt_epi16( mAxxxelta, mLeftD );
        T4 = _mm_cmpgt_epi16( mAxxxelta, mRightD );
        T5 = _mm_cmplt_epi16( mAxxxelta, mAlpha );

        M0 = _mm_and_si128( T1, T2 );
        M1 = _mm_and_si128( M0, T3 );
        M2 = _mm_and_si128( M1, T4 );
        M3 = _mm_and_si128( M2, T5 );  // M3 is the 1st if's final result

        T1 = _mm_subabs_epu16( TL2, TL0 );
        T2 = _mm_subabs_epu16( TR2, TR0 );
        T3 = _mm_cmplt_epi16( T1, mBeta );
        T4 = _mm_cmplt_epi16( T2, mBeta );
        M0 = _mm_and_si128( T3, T4 ); // M0 is the 2nd if's final result

        // two nested if
        FS = _mm_blendv_epi8( _mm_blendv_epi8( c_2, c_1, _mm_cmpeq_epi16( M0, c_0 ) ), c_0, _mm_cmpeq_epi16( M3, c_0 ) );

        // if   !!!
        T1 = _mm_andnot_si128( mflatedge, mbChroma );
        FS_TMP = _mm_blendv_epi8( mTrue, mFalse, _mm_cmpeq_epi16( FS, c_0 ) );
        T2 = _mm_and_si128( T1, FS_TMP );
        FS = _mm_blendv_epi8( FS, c_1, T2 );

        // if
        T1 = _mm_cmpeq_epi16( FS, c_2 ); //
        T2 = _mm_subabs_epu16( TR3, TR0 );
        T3 = _mm_cmplt_epi16( T2, mBeta ); //
        T4 = _mm_subabs_epu16( TL3, TL0 );
        T5 = _mm_cmplt_epi16( T4, mBeta ); //

        T2 = _mm_andnot_si128( medge, mflatedge ); //
        T4 = _mm_andnot_si128( mbChroma, T1 );  //
        T6 = _mm_and_si128( T2, T4 );
        T7 = _mm_and_si128( T3, T5 );

        T1 = _mm_and_si128( T6, T7 ); // result
        FS = _mm_blendv_epi8( FS, c_3, T1 );

#undef _mm_subabs_epu16

        // 16bit
        TL0l = TL0;
        TL1l = TL1;
        TL2l = TL2;
        TR0l = TR0;
        TR1l = TR1;
        TR2l = TR2;

        /* fs == 1 */
        FS_TMP = _mm_cmpeq_epi16( FS, c_1 );

        if ( FS_TMP.m128i_u64[0] || FS_TMP.m128i_u64[1] )
        {
            T2 = _mm_add_epi16( _mm_sub_epi16( TL0l, TR0l ), c_2 ); // L0 - R0 + 2
            V0 = _mm_add_epi16( _mm_srai_epi16( T2, 2 ), TR0l );

            T2 = _mm_add_epi16( _mm_sub_epi16( TR0l, TL0l ), c_2 ); // R0 - L0 + 2
            V1 = _mm_add_epi16( _mm_srai_epi16( T2, 2 ), TL0l );

            TL0 = _mm_blendv_epi8( TL0, V1, _mm_cmpeq_epi16( FS, c_1 ) );
            TR0 = _mm_blendv_epi8( TR0, V0, _mm_cmpeq_epi16( FS, c_1 ) );
        }

        /* fs == 2 */
        FS_TMP = _mm_cmpeq_epi16( FS, c_2 );

        if ( FS_TMP.m128i_u64[0] || FS_TMP.m128i_u64[1] )
        {
            // -2 1
            T2 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_3 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL1l ), c_8 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_4 );
            T5 = _mm_add_epi16( T2, T3 );
            T6 = _mm_add_epi16( T4, c_8 );
            T7 = _mm_add_epi16( T5, T6 );
            T0 = _mm_srai_epi16( T7, 4 );

            V0 = _mm_add_epi16( T0, TL1l );

            T2 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_3 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR1l ), c_8 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_4 );
            T5 = _mm_add_epi16( T2, T3 );
            T6 = _mm_add_epi16( T4, c_8 );
            T7 = _mm_add_epi16( T5, T6 );
            T0 = _mm_srai_epi16( T7, 4 );

            V1 = _mm_add_epi16( T0, TR1l );

            TL1 = _mm_blendv_epi8( TL1, V0, _mm_cmpeq_epi16( FS, c_2 ) );
            TR1 = _mm_blendv_epi8( TR1, V1, _mm_cmpeq_epi16( FS, c_2 ) );

            // -1 0
            T1 = _mm_sub_epi16( TL2l, TR0l );
            T2 = _mm_sub_epi16( TR1l, TL0l );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TL1l, TR0l ), c_4 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_9 );
            T5 = _mm_add_epi16( T1, T2 );
            T6 = _mm_add_epi16( T3, T4 );
            T7 = _mm_add_epi16( T5, T6 );
            T7 = _mm_add_epi16( T7, c_8 );
            T0 = _mm_srai_epi16( T7, 4 );

            V2 = _mm_add_epi16( T0, TL0l );

            T1 = _mm_sub_epi16( TR2l, TL0l );
            T2 = _mm_sub_epi16( TL1l, TR0l );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TR1l, TL0l ), c_4 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_9 );
            T5 = _mm_add_epi16( T1, T2 );
            T6 = _mm_add_epi16( T3, T4 );
            T7 = _mm_add_epi16( T5, T6 );
            T7 = _mm_add_epi16( T7, c_8 );
            T0 = _mm_srai_epi16( T7, 4 );

            V3 = _mm_add_epi16( T0, TR0l );

            TL0 = _mm_blendv_epi8( TL0, V2, _mm_cmpeq_epi16( FS, c_2 ) ); // ѡV2
            TR0 = _mm_blendv_epi8( TR0, V3, _mm_cmpeq_epi16( FS, c_2 ) );
        }

        /* fs == 3 */
        FS_TMP = _mm_cmpeq_epi16( FS, c_3 );

        if ( FS_TMP.m128i_u64[0] || FS_TMP.m128i_u64[1] )
        {
            // -3 2
            T2 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_4 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL2l ), c_5 );
            T4 = _mm_add_epi16( T2, T3 );
            T4 = _mm_add_epi16( T4, c_4 );
            T5 = _mm_srai_epi16( T4, 3 );
            V0 = _mm_add_epi16( T5, TL2l );

            T2 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_4 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR2l ), c_5 );
            T4 = _mm_add_epi16( T2, T3 );
            T4 = _mm_add_epi16( T4, c_4 );
            T5 = _mm_srai_epi16( T4, 3 );
            V1 = _mm_add_epi16( T5, TR2l );

            TL2 = _mm_blendv_epi8( TL2, V0, _mm_cmpeq_epi16( FS, c_3 ) );
            TR2 = _mm_blendv_epi8( TR2, V1, _mm_cmpeq_epi16( FS, c_3 ) );

            // -2 1
            T2 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL1l ), c_16 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_6 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_7 );
            T5 = _mm_add_epi16( T2, T3 );
            T6 = _mm_add_epi16( T4, c_8 );
            T7 = _mm_add_epi16( T5, T6 );
            T0 = _mm_srai_epi16( T7, 4 );
            V2 = _mm_add_epi16( T0, TL1l );

            T2 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR1l ), c_16 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_6 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_7 );
            T5 = _mm_add_epi16( T2, T3 );
            T6 = _mm_add_epi16( T4, c_8 );
            T7 = _mm_add_epi16( T5, T6 );
            T0 = _mm_srai_epi16( T7, 4 );
            V3 = _mm_add_epi16( T0, TR1l );

            TL1 = _mm_blendv_epi8( TL1, V2, _mm_cmpeq_epi16( FS, c_3 ) );
            TR1 = _mm_blendv_epi8( TR1, V3, _mm_cmpeq_epi16( FS, c_3 ) );

            // -1 0
            T2 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_9 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_6 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TR0l, TL0l ), c_17 );
            T5 = _mm_add_epi16( T2, T3 );
            T6 = _mm_add_epi16( T4, c_16 );
            T7 = _mm_add_epi16( T5, T6 );
            T0 = _mm_srai_epi16( T7, 5 );
            V4 = _mm_add_epi16( T0, TL0l );

            T2 = _mm_mullo_epi16( _mm_sub_epi16( TR2l, TL0l ), c_9 );
            T3 = _mm_mullo_epi16( _mm_sub_epi16( TL2l, TR0l ), c_6 );
            T4 = _mm_mullo_epi16( _mm_sub_epi16( TL0l, TR0l ), c_17 );
            T5 = _mm_add_epi16( T2, T3 );
            T6 = _mm_add_epi16( T4, c_16 );
            T7 = _mm_add_epi16( T5, T6 );
            T0 = _mm_srai_epi16( T7, 5 );
            V5 = _mm_add_epi16( T0, TR0l );

            TL0 = _mm_blendv_epi8( TL0, V4, _mm_cmpeq_epi16( FS, c_3 ) );
            TR0 = _mm_blendv_epi8( TR0, V5, _mm_cmpeq_epi16( FS, c_3 ) );

            /* stroe result */
            _mm_storel_epi64( ( __m128i* )( SrcPtr - inc ), _mm_packus_epi16( TL0, c_0 ) );
            _mm_storel_epi64( ( __m128i* )( SrcPtr - 0 ), _mm_packus_epi16( TR0, c_0 ) );

            _mm_storel_epi64( ( __m128i* )( SrcPtr - inc2 ), _mm_packus_epi16( TL1, c_0 ) );
            _mm_storel_epi64( ( __m128i* )( SrcPtr + inc ), _mm_packus_epi16( TR1, c_0 ) );

            _mm_storel_epi64( ( __m128i* )( SrcPtr - inc3 ), _mm_packus_epi16( TL2, c_0 ) );
            _mm_storel_epi64( ( __m128i* )( SrcPtr + inc2 ), _mm_packus_epi16( TR2, c_0 ) );
        }
        else
        {
            /* stroe result */
            _mm_storel_epi64( ( __m128i* )( SrcPtr - inc ), _mm_packus_epi16( TL0, c_0 ) );
            _mm_storel_epi64( ( __m128i* )( SrcPtr - 0 ), _mm_packus_epi16( TR0, c_0 ) );

            _mm_storel_epi64( ( __m128i* )( SrcPtr - inc2 ), _mm_packus_epi16( TL1, c_0 ) );
            _mm_storel_epi64( ( __m128i* )( SrcPtr + inc ), _mm_packus_epi16( TR1, c_0 ) );
        }
    }
}

#endif