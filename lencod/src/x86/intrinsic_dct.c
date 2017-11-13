/*****************************************************************************
* Authors: Ronggang Wang <rgwang@pkusz.edu.cn>
*          Zhenyu Wang <wangzhenyu@pkusz.edu.cn>
*          Kui Fan <kuifan@pku.edu.cn>
*          Shenghao Zhang <1219759986@qq.com>
* Affiliate: Peking University Shenzhen Graduate School
*****************************************************************************/

#include "intrinsic.h"

#if COMPILE_FOR_8BIT

void dct_4x4_sse128( int *curr_blk )
{
    int shift1, shift2;

    __m128i T0, T1, T2, T3;
    __m128i M0, M1, M2, M3;
    __m128i mTemp, mTemp0, mTemp1, mTemp2, mTemp3;
    __m128i mTemp2_0, mTemp2_1, mTemp2_2, mTemp2_3;
    __m128i mCoef1, mCoef2, mCoef3, mCoef4, mCoef5;

    shift1 = 7;
    shift2 = 2;

    mCoef1 = _mm_set1_epi32( 69 );
    mCoef2 = _mm_set1_epi32( 98 );
    mCoef3 = _mm_set1_epi32( 236 );
    mCoef4 = _mm_set1_epi32( 2 );
    mCoef5 = _mm_set1_epi32( -1 );

    // ------------------  1st for circulation  -----------------

    T0 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 ) );
    T1 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 ) );
    T2 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 4 ) );
    T3 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 4 ) );

    M0 = _mm_unpacklo_epi32( T0, T1 );
    M1 = _mm_unpacklo_epi32( T2, T3 );
    M2 = _mm_unpackhi_epi32( T0, T1 );
    M3 = _mm_unpackhi_epi32( T2, T3 );

    T0 = _mm_unpacklo_epi64( M0, M1 );
    T1 = _mm_unpackhi_epi64( M0, M1 );
    T2 = _mm_unpacklo_epi64( M2, M3 );
    T3 = _mm_unpackhi_epi64( M2, M3 );

    // step 1
    mTemp0 = _mm_add_epi32( T0, T3 );
    mTemp3 = _mm_sub_epi32( T0, T3 );
    mTemp1 = _mm_add_epi32( T1, T2 );
    mTemp2 = _mm_sub_epi32( T1, T2 );

    // step 2
    mTemp2_0 = _mm_add_epi32( mTemp0, mTemp1 );
    mTemp2_2 = _mm_sub_epi32( mTemp0, mTemp1 );
    mTemp = _mm_add_epi32( mTemp2, mTemp3 );
    mTemp = _mm_srai_epi32( _mm_mullo_epi32( mTemp, mCoef1 ), shift1 );
    mTemp2_1 = _mm_add_epi32( mTemp, _mm_srai_epi32( _mm_mullo_epi32( mTemp3, mCoef2 ), shift1 ) );
    mTemp2_3 = _mm_sub_epi32( mTemp, _mm_srai_epi32( _mm_mullo_epi32( mTemp2, mCoef3 ), shift1 ) );

    // ------------  store back  -------------
    M0 = _mm_unpacklo_epi32( mTemp2_0, mTemp2_1 );
    M1 = _mm_unpacklo_epi32( mTemp2_2, mTemp2_3 );
    M2 = _mm_unpackhi_epi32( mTemp2_0, mTemp2_1 );
    M3 = _mm_unpackhi_epi32( mTemp2_2, mTemp2_3 );

    mTemp2_0 = _mm_unpacklo_epi64( M0, M1 );
    mTemp2_1 = _mm_unpackhi_epi64( M0, M1 );
    mTemp2_2 = _mm_unpacklo_epi64( M2, M3 );
    mTemp2_3 = _mm_unpackhi_epi64( M2, M3 );


    _mm_storeu_si128( ( __m128i* )( curr_blk + 0 ), mTemp2_0 );
    _mm_storeu_si128( ( __m128i* )( curr_blk + 4 ), mTemp2_1 );
    _mm_storeu_si128( ( __m128i* )( curr_blk + 2 * 4 ), mTemp2_2 );
    _mm_storeu_si128( ( __m128i* )( curr_blk + 3 * 4 ), mTemp2_3 );

    // ------------------  2nd for circulation  -----------------
    T0 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 ) );
    T1 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 ) );
    T2 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 4 ) );
    T3 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 4 ) );

    // step 1
    mTemp0 = _mm_add_epi32( T0, T3 );
    mTemp3 = _mm_sub_epi32( T0, T3 );
    mTemp1 = _mm_add_epi32( T1, T2 );
    mTemp2 = _mm_sub_epi32( T1, T2 );

    // step 2
    mTemp2_0 = _mm_add_epi32( mTemp0, mTemp1 );
    mTemp2_2 = _mm_sub_epi32( mTemp0, mTemp1 );
    mTemp = _mm_add_epi32( mTemp2, mTemp3 );
    mTemp = _mm_srai_epi32( _mm_mullo_epi32( mTemp, mCoef1 ), shift1 );
    mTemp2_1 = _mm_add_epi32( mTemp, _mm_srai_epi32( _mm_mullo_epi32( mTemp3, mCoef2 ), shift1 ) );
    mTemp2_3 = _mm_sub_epi32( mTemp, _mm_srai_epi32( _mm_mullo_epi32( mTemp2, mCoef3 ), shift1 ) );

    T0 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp2_0 ), mCoef4 ), shift2 ), mTemp2_0 );
    T1 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp2_1 ), mCoef4 ), shift2 ), mTemp2_1 );
    T2 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp2_2 ), mCoef4 ), shift2 ), mTemp2_2 );
    T3 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp2_3 ), mCoef4 ), shift2 ), mTemp2_3 );

    // ------------  store back  -------------
    _mm_storeu_si128( ( __m128i* )( curr_blk + 0 ), T0 );
    _mm_storeu_si128( ( __m128i* )( curr_blk + 4 ), T1 );
    _mm_storeu_si128( ( __m128i* )( curr_blk + 2 * 4 ), T2 );
    _mm_storeu_si128( ( __m128i* )( curr_blk + 3 * 4 ), T3 );
}

void dct_8x8_sse128( int *curr_blk )
{
    int shift1, shift2, shift3, shift4;

    __m128i T10, T11, T12, T13, T14, T15, T16, T17;
    __m128i T20, T21, T22, T23, T24, T25, T26, T27;
    __m128i M10, M11, M12, M13, M14, M15, M16, M17;
    __m128i M20, M21, M22, M23, M24, M25, M26, M27;

    __m128i mTemp1, mTemp2;
    __m128i mTemp110, mTemp111, mTemp112, mTemp113, mTemp114, mTemp115, mTemp116, mTemp117;
    __m128i mTemp120, mTemp121, mTemp122, mTemp123, mTemp124, mTemp125, mTemp126, mTemp127;

    __m128i mTemp210, mTemp220, mTemp211, mTemp221, mTemp212, mTemp222, mTemp213, mTemp223;
    __m128i mTemp214, mTemp224, mTemp215, mTemp225, mTemp216, mTemp226, mTemp217, mTemp227;

    __m128i mCoef1, mCoef2, mCoef3, mCoef4, mCoef5, mCoef6, mCoef7, mCoef8, mCoef9, mCoef10, mCoef11;

    mCoef1 = _mm_set1_epi32( 201 );
    mCoef2 = _mm_set1_epi32( 100 );
    mCoef3 = _mm_set1_epi32( 502 );
    mCoef4 = _mm_set1_epi32( 141 );
    mCoef5 = _mm_set1_epi32( 569 );
    mCoef6 = _mm_set1_epi32( 851 );
    mCoef7 = _mm_set1_epi32( 181 );
    mCoef8 = _mm_set1_epi32( 196 );
    mCoef9 = _mm_set1_epi32( 277 );
    mCoef10 = _mm_set1_epi32( 669 );
    mCoef11 = _mm_set1_epi32( 8 );

    shift1 = 7;
    shift2 = 8;
    shift3 = 9;
    shift4 = 4;

    // prepare for the data
    T10 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 ) );
    T20 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 ) );
    T11 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 ) );
    T21 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 + 4 ) );
    T12 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 8 ) );
    T22 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 8 + 4 ) );
    T13 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 8 ) );
    T23 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 8 + 4 ) );
    T14 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 8 ) );
    T24 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 8 + 4 ) );
    T15 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 8 ) );
    T25 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 8 + 4 ) );
    T16 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 8 ) );
    T26 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 8 + 4 ) );
    T17 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 8 ) );
    T27 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 8 + 4 ) );

    M10 = _mm_unpacklo_epi32( T10, T11 );
    M11 = _mm_unpackhi_epi32( T10, T11 );
    M20 = _mm_unpacklo_epi32( T20, T21 );
    M21 = _mm_unpackhi_epi32( T20, T21 );
    M12 = _mm_unpacklo_epi32( T12, T13 );
    M13 = _mm_unpackhi_epi32( T12, T13 );
    M22 = _mm_unpacklo_epi32( T22, T23 );
    M23 = _mm_unpackhi_epi32( T22, T23 );
    M14 = _mm_unpacklo_epi32( T14, T15 );
    M15 = _mm_unpackhi_epi32( T14, T15 );
    M24 = _mm_unpacklo_epi32( T24, T25 );
    M25 = _mm_unpackhi_epi32( T24, T25 );
    M16 = _mm_unpacklo_epi32( T16, T17 );
    M17 = _mm_unpackhi_epi32( T16, T17 );
    M26 = _mm_unpacklo_epi32( T26, T27 );
    M27 = _mm_unpackhi_epi32( T26, T27 );

    T10 = _mm_unpacklo_epi64( M10, M12 );
    T11 = _mm_unpackhi_epi64( M10, M12 );
    T20 = _mm_unpacklo_epi64( M14, M16 );
    T21 = _mm_unpackhi_epi64( M14, M16 );
    T12 = _mm_unpacklo_epi64( M11, M13 );
    T13 = _mm_unpackhi_epi64( M11, M13 );
    T22 = _mm_unpacklo_epi64( M15, M17 );
    T23 = _mm_unpackhi_epi64( M15, M17 );
    T14 = _mm_unpacklo_epi64( M20, M22 );
    T15 = _mm_unpackhi_epi64( M20, M22 );
    T24 = _mm_unpacklo_epi64( M24, M26 );
    T25 = _mm_unpackhi_epi64( M24, M26 );
    T16 = _mm_unpacklo_epi64( M21, M23 );
    T17 = _mm_unpackhi_epi64( M21, M23 );
    T26 = _mm_unpacklo_epi64( M25, M27 );
    T27 = _mm_unpackhi_epi64( M25, M27 );

    // the 1st for-circulation
    // step1
    mTemp110 = _mm_add_epi32( T10, T17 );
    mTemp120 = _mm_add_epi32( T20, T27 );
    mTemp111 = _mm_add_epi32( T11, T16 );
    mTemp121 = _mm_add_epi32( T21, T26 );
    mTemp112 = _mm_add_epi32( T12, T15 );
    mTemp122 = _mm_add_epi32( T22, T25 );
    mTemp113 = _mm_add_epi32( T13, T14 );
    mTemp123 = _mm_add_epi32( T23, T24 );
    mTemp117 = _mm_sub_epi32( T10, T17 );
    mTemp127 = _mm_sub_epi32( T20, T27 );
    mTemp116 = _mm_sub_epi32( T11, T16 );
    mTemp126 = _mm_sub_epi32( T21, T26 );
    mTemp115 = _mm_sub_epi32( T12, T15 );
    mTemp125 = _mm_sub_epi32( T22, T25 );
    mTemp114 = _mm_sub_epi32( T13, T14 );
    mTemp124 = _mm_sub_epi32( T23, T24 );

    // step2
    mTemp210 = _mm_add_epi32( mTemp110, mTemp113 );
    mTemp220 = _mm_add_epi32( mTemp120, mTemp123 );
    mTemp211 = _mm_add_epi32( mTemp111, mTemp112 );
    mTemp221 = _mm_add_epi32( mTemp121, mTemp122 );
    mTemp213 = _mm_sub_epi32( mTemp110, mTemp113 );
    mTemp223 = _mm_sub_epi32( mTemp120, mTemp123 );
    mTemp212 = _mm_sub_epi32( mTemp111, mTemp112 );
    mTemp222 = _mm_sub_epi32( mTemp121, mTemp122 );

    mTemp1 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp114, mTemp117 ), mCoef1 ), shift2 );
    mTemp2 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp124, mTemp127 ), mCoef1 ), shift2 );

    mTemp214 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp114, mCoef2 ), shift2 ), mTemp1 );
    mTemp224 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp124, mCoef2 ), shift2 ), mTemp2 );
    mTemp217 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp117, mCoef3 ), shift2 ), mTemp1 );
    mTemp227 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp127, mCoef3 ), shift2 ), mTemp2 );

    mTemp1 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp115, mTemp116 ), mCoef4 ), shift3 );
    mTemp2 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp125, mTemp126 ), mCoef4 ), shift3 );

    mTemp215 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp115, mCoef5 ), shift3 ), mTemp1 );
    mTemp225 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp125, mCoef5 ), shift3 ), mTemp2 );
    mTemp216 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp116, mCoef6 ), shift3 ), mTemp1 );
    mTemp226 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp126, mCoef6 ), shift3 ), mTemp2 );

    // step3
    mTemp110 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp210, mTemp211 ), mCoef7 ), shift1 );
    mTemp120 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp220, mTemp221 ), mCoef7 ), shift1 );
    mTemp114 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( mTemp210, mTemp211 ), mCoef7 ), shift1 );
    mTemp124 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( mTemp220, mTemp221 ), mCoef7 ), shift1 );

    mTemp1 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp212, mTemp213 ), mCoef8 ), shift2 );
    mTemp2 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp222, mTemp223 ), mCoef8 ), shift2 );

    mTemp112 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp213, mCoef9 ), shift2 ), mTemp1 );
    mTemp122 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp223, mCoef9 ), shift2 ), mTemp2 );
    mTemp116 = _mm_sub_epi32( mTemp1, _mm_srai_epi32( _mm_mullo_epi32( mTemp212, mCoef10 ), shift2 ) );
    mTemp126 = _mm_sub_epi32( mTemp2, _mm_srai_epi32( _mm_mullo_epi32( mTemp222, mCoef10 ), shift2 ) );

    mTemp117 = _mm_add_epi32( mTemp214, mTemp216 );
    mTemp127 = _mm_add_epi32( mTemp224, mTemp226 );
    mTemp111 = _mm_add_epi32( mTemp217, mTemp215 );
    mTemp121 = _mm_add_epi32( mTemp227, mTemp225 );

    mTemp115 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( mTemp214, mTemp216 ), mCoef7 ), shift1 );
    mTemp125 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( mTemp224, mTemp226 ), mCoef7 ), shift1 );
    mTemp113 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( mTemp217, mTemp215 ), mCoef7 ), shift1 );
    mTemp123 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( mTemp227, mTemp225 ), mCoef7 ), shift1 );

    mTemp1 = _mm_add_epi32( mTemp117, mTemp111 );
    mTemp2 = _mm_add_epi32( mTemp127, mTemp121 );
    mTemp117 = _mm_sub_epi32( mTemp111, mTemp117 );
    mTemp127 = _mm_sub_epi32( mTemp121, mTemp127 );

    mTemp111 = mTemp1;
    mTemp121 = mTemp2;


    M10 = _mm_unpacklo_epi32( mTemp110, mTemp111 );
    M11 = _mm_unpackhi_epi32( mTemp110, mTemp111 );
    M20 = _mm_unpacklo_epi32( mTemp120, mTemp121 );
    M21 = _mm_unpackhi_epi32( mTemp120, mTemp121 );
    M12 = _mm_unpacklo_epi32( mTemp112, mTemp113 );
    M13 = _mm_unpackhi_epi32( mTemp112, mTemp113 );
    M22 = _mm_unpacklo_epi32( mTemp122, mTemp123 );
    M23 = _mm_unpackhi_epi32( mTemp122, mTemp123 );
    M14 = _mm_unpacklo_epi32( mTemp114, mTemp115 );
    M15 = _mm_unpackhi_epi32( mTemp114, mTemp115 );
    M24 = _mm_unpacklo_epi32( mTemp124, mTemp125 );
    M25 = _mm_unpackhi_epi32( mTemp124, mTemp125 );
    M16 = _mm_unpacklo_epi32( mTemp116, mTemp117 );
    M17 = _mm_unpackhi_epi32( mTemp116, mTemp117 );
    M26 = _mm_unpacklo_epi32( mTemp126, mTemp127 );
    M27 = _mm_unpackhi_epi32( mTemp126, mTemp127 );

    mTemp110 = _mm_unpacklo_epi64( M10, M12 );
    mTemp111 = _mm_unpackhi_epi64( M10, M12 );
    mTemp120 = _mm_unpacklo_epi64( M14, M16 );
    mTemp121 = _mm_unpackhi_epi64( M14, M16 );
    mTemp112 = _mm_unpacklo_epi64( M11, M13 );
    mTemp113 = _mm_unpackhi_epi64( M11, M13 );
    mTemp122 = _mm_unpacklo_epi64( M15, M17 );
    mTemp123 = _mm_unpackhi_epi64( M15, M17 );
    mTemp114 = _mm_unpacklo_epi64( M20, M22 );
    mTemp115 = _mm_unpackhi_epi64( M20, M22 );
    mTemp124 = _mm_unpacklo_epi64( M24, M26 );
    mTemp125 = _mm_unpackhi_epi64( M24, M26 );
    mTemp116 = _mm_unpacklo_epi64( M21, M23 );
    mTemp117 = _mm_unpackhi_epi64( M21, M23 );
    mTemp126 = _mm_unpacklo_epi64( M25, M27 );
    mTemp127 = _mm_unpackhi_epi64( M25, M27 );

    _mm_storeu_si128( ( __m128i * )curr_blk, mTemp110 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 4 ), mTemp120 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 8 ), mTemp111 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 8 + 4 ), mTemp121 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 8 ), mTemp112 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 8 + 4 ), mTemp122 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 8 ), mTemp113 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 8 + 4 ), mTemp123 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 8 ), mTemp114 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 8 + 4 ), mTemp124 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 8 ), mTemp115 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 8 + 4 ), mTemp125 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 8 ), mTemp116 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 8 + 4 ), mTemp126 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 8 ), mTemp117 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 8 + 4 ), mTemp127 );

    // the 2nd for-circulation
    // prepare for the data
    T10 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 ) );
    T20 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 ) );
    T11 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 ) );
    T21 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 + 4 ) );
    T12 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 8 ) );
    T22 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 8 + 4 ) );
    T13 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 8 ) );
    T23 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 8 + 4 ) );
    T14 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 8 ) );
    T24 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 8 + 4 ) );
    T15 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 8 ) );
    T25 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 8 + 4 ) );
    T16 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 8 ) );
    T26 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 8 + 4 ) );
    T17 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 8 ) );
    T27 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 8 + 4 ) );

    //step1
    mTemp110 = _mm_add_epi32( T10, T17 );
    mTemp120 = _mm_add_epi32( T20, T27 );
    mTemp111 = _mm_add_epi32( T11, T16 );
    mTemp121 = _mm_add_epi32( T21, T26 );
    mTemp112 = _mm_add_epi32( T12, T15 );
    mTemp122 = _mm_add_epi32( T22, T25 );
    mTemp113 = _mm_add_epi32( T13, T14 );
    mTemp123 = _mm_add_epi32( T23, T24 );
    mTemp117 = _mm_sub_epi32( T10, T17 );
    mTemp127 = _mm_sub_epi32( T20, T27 );
    mTemp116 = _mm_sub_epi32( T11, T16 );
    mTemp126 = _mm_sub_epi32( T21, T26 );
    mTemp115 = _mm_sub_epi32( T12, T15 );
    mTemp125 = _mm_sub_epi32( T22, T25 );
    mTemp114 = _mm_sub_epi32( T13, T14 );
    mTemp124 = _mm_sub_epi32( T23, T24 );

    //step2
    mTemp210 = _mm_add_epi32( mTemp110, mTemp113 );
    mTemp220 = _mm_add_epi32( mTemp120, mTemp123 );
    mTemp211 = _mm_add_epi32( mTemp111, mTemp112 );
    mTemp221 = _mm_add_epi32( mTemp121, mTemp122 );
    mTemp213 = _mm_sub_epi32( mTemp110, mTemp113 );
    mTemp223 = _mm_sub_epi32( mTemp120, mTemp123 );
    mTemp212 = _mm_sub_epi32( mTemp111, mTemp112 );
    mTemp222 = _mm_sub_epi32( mTemp121, mTemp122 );

    mTemp1 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp114, mTemp117 ), mCoef1 ), shift2 );
    mTemp2 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp124, mTemp127 ), mCoef1 ), shift2 );

    mTemp214 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp114, mCoef2 ), shift2 ), mTemp1 );
    mTemp224 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp124, mCoef2 ), shift2 ), mTemp2 );
    mTemp217 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp117, mCoef3 ), shift2 ), mTemp1 );
    mTemp227 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp127, mCoef3 ), shift2 ), mTemp2 );

    mTemp1 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp115, mTemp116 ), mCoef4 ), shift3 );
    mTemp2 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp125, mTemp126 ), mCoef4 ), shift3 );

    mTemp215 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp115, mCoef5 ), shift3 ), mTemp1 );
    mTemp225 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp125, mCoef5 ), shift3 ), mTemp2 );
    mTemp216 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp116, mCoef6 ), shift3 ), mTemp1 );
    mTemp226 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp126, mCoef6 ), shift3 ), mTemp2 );

    // step3
    mTemp110 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp210, mTemp211 ), mCoef7 ), shift1 );
    mTemp120 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp220, mTemp221 ), mCoef7 ), shift1 );
    mTemp114 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( mTemp210, mTemp211 ), mCoef7 ), shift1 );
    mTemp124 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( mTemp220, mTemp221 ), mCoef7 ), shift1 );

    mTemp1 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp212, mTemp213 ), mCoef8 ), shift2 );
    mTemp2 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( mTemp222, mTemp223 ), mCoef8 ), shift2 );

    mTemp112 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp213, mCoef9 ), shift2 ), mTemp1 );
    mTemp122 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp223, mCoef9 ), shift2 ), mTemp2 );
    mTemp116 = _mm_sub_epi32( mTemp1, _mm_srai_epi32( _mm_mullo_epi32( mTemp212, mCoef10 ), shift2 ) );
    mTemp126 = _mm_sub_epi32( mTemp2, _mm_srai_epi32( _mm_mullo_epi32( mTemp222, mCoef10 ), shift2 ) );

    mTemp117 = _mm_add_epi32( mTemp214, mTemp216 );
    mTemp127 = _mm_add_epi32( mTemp224, mTemp226 );
    mTemp111 = _mm_add_epi32( mTemp217, mTemp215 );
    mTemp121 = _mm_add_epi32( mTemp227, mTemp225 );

    mTemp115 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( mTemp214, mTemp216 ), mCoef7 ), shift1 );
    mTemp125 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( mTemp224, mTemp226 ), mCoef7 ), shift1 );
    mTemp113 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( mTemp217, mTemp215 ), mCoef7 ), shift1 );
    mTemp123 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( mTemp227, mTemp225 ), mCoef7 ), shift1 );

    mTemp1 = _mm_add_epi32( mTemp117, mTemp111 );
    mTemp2 = _mm_add_epi32( mTemp127, mTemp121 );
    mTemp117 = _mm_sub_epi32( mTemp111, mTemp117 );
    mTemp127 = _mm_sub_epi32( mTemp121, mTemp127 );

    mTemp111 = mTemp1;
    mTemp121 = mTemp2;

    //store
    T10 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp110 ), mCoef11 ), shift4 ), mTemp110 );
    T20 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp120 ), mCoef11 ), shift4 ), mTemp120 );
    T11 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp111 ), mCoef11 ), shift4 ), mTemp111 );
    T21 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp121 ), mCoef11 ), shift4 ), mTemp121 );
    T12 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp112 ), mCoef11 ), shift4 ), mTemp112 );
    T22 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp122 ), mCoef11 ), shift4 ), mTemp122 );
    T13 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp113 ), mCoef11 ), shift4 ), mTemp113 );
    T23 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp123 ), mCoef11 ), shift4 ), mTemp123 );
    T14 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp114 ), mCoef11 ), shift4 ), mTemp114 );
    T24 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp124 ), mCoef11 ), shift4 ), mTemp124 );
    T15 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp115 ), mCoef11 ), shift4 ), mTemp115 );
    T25 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp125 ), mCoef11 ), shift4 ), mTemp125 );
    T16 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp116 ), mCoef11 ), shift4 ), mTemp116 );
    T26 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp126 ), mCoef11 ), shift4 ), mTemp126 );
    T17 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp117 ), mCoef11 ), shift4 ), mTemp117 );
    T27 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( mTemp127 ), mCoef11 ), shift4 ), mTemp127 );

    _mm_storeu_si128( ( __m128i * )curr_blk, T10 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 4 ), T20 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 8 ), T11 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 8 + 4 ), T21 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 8 ), T12 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 8 + 4 ), T22 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 8 ), T13 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 8 + 4 ), T23 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 8 ), T14 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 8 + 4 ), T24 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 8 ), T15 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 8 + 4 ), T25 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 8 ), T16 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 8 + 4 ), T26 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 8 ), T17 );
    _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 8 + 4 ), T27 );
}

void dct_16x16_sse128( int *curr_blk, int bsize )
{
    int i;
    int shift = 14;
    int pass, part;

    // load the data in the 16x16 coef matrix first
    const __m128i c32_p45 = _mm_set1_epi32( 0x0000002D );
    const __m128i c32_p43 = _mm_set1_epi32( 0x0000002B );
    const __m128i c32_p40 = _mm_set1_epi32( 0x00000028 );
    const __m128i c32_p35 = _mm_set1_epi32( 0x00000023 );
    const __m128i c32_p29 = _mm_set1_epi32( 0x0000001D );
    const __m128i c32_p21 = _mm_set1_epi32( 0x00000015 );
    const __m128i c32_p13 = _mm_set1_epi32( 0x0000000D );
    const __m128i c32_p04 = _mm_set1_epi32( 0x00000004 );
    const __m128i c32_n21 = _mm_set1_epi32( 0xFFFFFFEB );
    const __m128i c32_n40 = _mm_set1_epi32( 0xFFFFFFD8 );
    const __m128i c32_n45 = _mm_set1_epi32( 0xFFFFFFD3 );
    const __m128i c32_n35 = _mm_set1_epi32( 0xFFFFFFDD );
    const __m128i c32_n13 = _mm_set1_epi32( 0xFFFFFFF3 );
    const __m128i c32_n43 = _mm_set1_epi32( 0xFFFFFFD5 );
    const __m128i c32_n29 = _mm_set1_epi32( 0xFFFFFFE3 );
    const __m128i c32_n04 = _mm_set1_epi32( 0xFFFFFFFC );
    const __m128i c32_p44 = _mm_set1_epi32( 0x0000002C );
    const __m128i c32_p38 = _mm_set1_epi32( 0x00000026 );
    const __m128i c32_p25 = _mm_set1_epi32( 0x00000019 );
    const __m128i c32_p09 = _mm_set1_epi32( 0x00000009 );
    const __m128i c32_n09 = _mm_set1_epi32( 0xFFFFFFF7 );
    const __m128i c32_n44 = _mm_set1_epi32( 0xFFFFFFD4 );
    const __m128i c32_n25 = _mm_set1_epi32( 0xFFFFFFE7 );
    const __m128i c32_p42 = _mm_set1_epi32( 0x0000002A );
    const __m128i c32_p17 = _mm_set1_epi32( 0x00000011 );
    const __m128i c32_n42 = _mm_set1_epi32( 0xFFFFFFD6 );
    const __m128i c32_p32 = _mm_set1_epi32( 0x00000020 );
    const __m128i c32_n32 = _mm_set1_epi32( 0xFFFFFFE0 );


    const __m128i c32_rnd = _mm_set1_epi32( 1 << ( shift - 1 ) );

    // DCT1
    __m128i in00[4], in01[4], in02[4], in03[4], in04[4], in05[4], in06[4], in07[4];
    __m128i in08[4], in09[4], in10[4], in11[4], in12[4], in13[4], in14[4], in15[4];
    __m128i res00[4], res01[4], res02[4], res03[4], res04[4], res05[4], res06[4], res07[4];
    __m128i res08[4], res09[4], res10[4], res11[4], res12[4], res13[4], res14[4], res15[4];


    for ( i = 0; i < 4; i++ )
    {
        const int offset = i * 4;

        res00[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[0 * 16 + offset] ); // change _mm_loadu_si128 to _mm_load_si128
        res01[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[1 * 16 + offset] );
        res02[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[2 * 16 + offset] );
        res03[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[3 * 16 + offset] );
        res04[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[4 * 16 + offset] );
        res05[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[5 * 16 + offset] );
        res06[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[6 * 16 + offset] );
        res07[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[7 * 16 + offset] );
        res08[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[8 * 16 + offset] );
        res09[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[9 * 16 + offset] );
        res10[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[10 * 16 + offset] );
        res11[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[11 * 16 + offset] );
        res12[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[12 * 16 + offset] );
        res13[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[13 * 16 + offset] );
        res14[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[14 * 16 + offset] );
        res15[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[15 * 16 + offset] );
    }



    for ( pass = 0; pass < 2; pass++ )
    {
        //transpose matrix 8x8 16bit.
        {
            __m128i tr0_0, tr0_1, tr0_2, tr0_3, tr0_4, tr0_5, tr0_6, tr0_7;

#define TRANSPOSE_8x8_16BIT(I0, I1, I2, I3, I4, I5, I6, I7, O0, O1, O2, O3, O4, O5, O6, O7) \
    tr0_0 = _mm_unpacklo_epi32(I0, I1); \
    tr0_1 = _mm_unpacklo_epi32(I2, I3); \
    tr0_2 = _mm_unpackhi_epi32(I0, I1); \
    tr0_3 = _mm_unpackhi_epi32(I2, I3); \
    tr0_4 = _mm_unpacklo_epi32(I4, I5); \
    tr0_5 = _mm_unpacklo_epi32(I6, I7); \
    tr0_6 = _mm_unpackhi_epi32(I4, I5); \
    tr0_7 = _mm_unpackhi_epi32(I6, I7); \
    O0 = _mm_unpacklo_epi64(tr0_0, tr0_1); \
    O1 = _mm_unpackhi_epi64(tr0_0, tr0_1); \
    O2 = _mm_unpacklo_epi64(tr0_2, tr0_3); \
    O3 = _mm_unpackhi_epi64(tr0_2, tr0_3); \
    O4 = _mm_unpacklo_epi64(tr0_4, tr0_5); \
    O5 = _mm_unpackhi_epi64(tr0_4, tr0_5); \
    O6 = _mm_unpacklo_epi64(tr0_6, tr0_7); \
    O7 = _mm_unpackhi_epi64(tr0_6, tr0_7); \

            TRANSPOSE_8x8_16BIT( res00[0], res01[0], res02[0], res03[0], res04[0], res05[0], res06[0], res07[0], in00[0], in01[0], in02[0], in03[0], in00[1], in01[1], in02[1], in03[1] )
            TRANSPOSE_8x8_16BIT( res08[0], res09[0], res10[0], res11[0], res12[0], res13[0], res14[0], res15[0], in00[2], in01[2], in02[2], in03[2], in00[3], in01[3], in02[3], in03[3] )
            TRANSPOSE_8x8_16BIT( res00[1], res01[1], res02[1], res03[1], res04[1], res05[1], res06[1], res07[1], in04[0], in05[0], in06[0], in07[0], in04[1], in05[1], in06[1], in07[1] )
            TRANSPOSE_8x8_16BIT( res08[1], res09[1], res10[1], res11[1], res12[1], res13[1], res14[1], res15[1], in04[2], in05[2], in06[2], in07[2], in04[3], in05[3], in06[3], in07[3] )
            TRANSPOSE_8x8_16BIT( res00[2], res01[2], res02[2], res03[2], res04[2], res05[2], res06[2], res07[2], in08[0], in09[0], in10[0], in11[0], in08[1], in09[1], in10[1], in11[1] )
            TRANSPOSE_8x8_16BIT( res08[2], res09[2], res10[2], res11[2], res12[2], res13[2], res14[2], res15[2], in08[2], in09[2], in10[2], in11[2], in08[3], in09[3], in10[3], in11[3] )
            TRANSPOSE_8x8_16BIT( res00[3], res01[3], res02[3], res03[3], res04[3], res05[3], res06[3], res07[3], in12[0], in13[0], in14[0], in15[0], in12[1], in13[1], in14[1], in15[1] )
            TRANSPOSE_8x8_16BIT( res08[3], res09[3], res10[3], res11[3], res12[3], res13[3], res14[3], res15[3], in12[2], in13[2], in14[2], in15[2], in12[3], in13[3], in14[3], in15[3] )

#undef TRANSPOSE_8x8_16BI

        }

        for ( part = 0; part < 4; part++ )
        {
            __m128i O0A, O1A, O2A, O3A, O4A, O5A, O6A, O7A;
            __m128i EO0A, EO1A, EO2A, EO3A;
            __m128i E0A, E1A, E2A, E3A, E4A, E5A, E6A, E7A;
            __m128i EE0A, EE1A, EE2A, EE3A;
            __m128i EEO0A, EEO1A;
            __m128i EEE0A, EEE1A;
            __m128i T00, T01, T02, T03;
            __m128i mTemp1, mTemp2;

            /* E and O*/
            E0A = _mm_add_epi32( in00[part], in15[part] );
            E1A = _mm_add_epi32( in01[part], in14[part] );
            E2A = _mm_add_epi32( in02[part], in13[part] );
            E3A = _mm_add_epi32( in03[part], in12[part] );
            E4A = _mm_add_epi32( in04[part], in11[part] );
            E5A = _mm_add_epi32( in05[part], in10[part] );
            E6A = _mm_add_epi32( in06[part], in09[part] );
            E7A = _mm_add_epi32( in07[part], in08[part] );


            O0A = _mm_sub_epi32( in00[part], in15[part] );
            O1A = _mm_sub_epi32( in01[part], in14[part] );
            O2A = _mm_sub_epi32( in02[part], in13[part] );
            O3A = _mm_sub_epi32( in03[part], in12[part] );
            O4A = _mm_sub_epi32( in04[part], in11[part] );
            O5A = _mm_sub_epi32( in05[part], in10[part] );
            O6A = _mm_sub_epi32( in06[part], in09[part] );
            O7A = _mm_sub_epi32( in07[part], in08[part] );


            /* EE and EO */
            EE0A = _mm_add_epi32( E0A, E7A );
            EE1A = _mm_add_epi32( E1A, E6A );
            EE2A = _mm_add_epi32( E2A, E5A );
            EE3A = _mm_add_epi32( E3A, E4A );

            EO0A = _mm_sub_epi32( E0A, E7A );
            EO1A = _mm_sub_epi32( E1A, E6A );
            EO2A = _mm_sub_epi32( E2A, E5A );
            EO3A = _mm_sub_epi32( E3A, E4A );

            /* EEE and EEO */
            EEE0A = _mm_add_epi32( EE0A, EE3A );
            EEO0A = _mm_sub_epi32( EE0A, EE3A );
            EEE1A = _mm_add_epi32( EE1A, EE2A );
            EEO1A = _mm_sub_epi32( EE1A, EE2A );



            res00[part] = _mm_add_epi32( _mm_mullo_epi32( c32_p32, EEE0A ), _mm_mullo_epi32( c32_p32, EEE1A ) );
            res08[part] = _mm_add_epi32( _mm_mullo_epi32( c32_p32, EEE0A ), _mm_mullo_epi32( c32_n32, EEE1A ) );
            res04[part] = _mm_add_epi32( _mm_mullo_epi32( c32_p42, EEO0A ), _mm_mullo_epi32( c32_p17, EEO1A ) );
            res12[part] = _mm_add_epi32( _mm_mullo_epi32( c32_p17, EEO0A ), _mm_mullo_epi32( c32_n42, EEO1A ) );

            // R4 multiplication
#define COMPUTE_ROW4(row02, row06, row10, row14, c02, c06, c10, c14, row) \
    T00 = _mm_add_epi32(_mm_mullo_epi32(row02, c02), _mm_mullo_epi32(row06, c06)); \
    T01 = _mm_add_epi32(_mm_mullo_epi32(row10, c10), _mm_mullo_epi32(row14, c14)); \
    row = _mm_add_epi32(T00, T01);

            COMPUTE_ROW4( EO0A, EO1A, EO2A, EO3A, c32_p44, c32_p38, c32_p25, c32_p09, res02[part] ) // dst
            COMPUTE_ROW4( EO0A, EO1A, EO2A, EO3A, c32_p38, c32_n09, c32_n44, c32_n25, res06[part] ) // dst
            COMPUTE_ROW4( EO0A, EO1A, EO2A, EO3A, c32_p25, c32_n44, c32_p09, c32_p38, res10[part] ) // dst
            COMPUTE_ROW4( EO0A, EO1A, EO2A, EO3A, c32_p09, c32_n25, c32_p38, c32_n44, res14[part] ) // dst

#undef COMPUTE_ROW4

#define COMPUTE_ROW8(row01, row03, row05, row07, row09, row11, row13, row15, c01, c03, c05, c07, c09, c11, c13, c15, row) \
    T00 = _mm_add_epi32(_mm_mullo_epi32(row01, c01), _mm_mullo_epi32(row03, c03)); \
    T01 = _mm_add_epi32(_mm_mullo_epi32(row05, c05), _mm_mullo_epi32(row07, c07)); \
    T02 = _mm_add_epi32(_mm_mullo_epi32(row09, c09), _mm_mullo_epi32(row11, c11)); \
    T03 = _mm_add_epi32(_mm_mullo_epi32(row13, c13), _mm_mullo_epi32(row15, c15)); \
    mTemp1 = _mm_add_epi32(T00, T01); \
    mTemp2 = _mm_add_epi32(T02, T03); \
    row = _mm_add_epi32(mTemp1, mTemp2);

            COMPUTE_ROW8( O0A, O1A, O2A, O3A, O4A, O5A, O6A, O7A, \
                          c32_p45, c32_p43, c32_p40, c32_p35, c32_p29, c32_p21, c32_p13, c32_p04, res01[part] )
            COMPUTE_ROW8( O0A, O1A, O2A, O3A, O4A, O5A, O6A, O7A, \
                          c32_p43, c32_p29, c32_p04, c32_n21, c32_n40, c32_n45, c32_n35, c32_n13, res03[part] )
            COMPUTE_ROW8( O0A, O1A, O2A, O3A, O4A, O5A, O6A, O7A, \
                          c32_p40, c32_p04, c32_n35, c32_n43, c32_n13, c32_p29, c32_p45, c32_p21, res05[part] )
            COMPUTE_ROW8( O0A, O1A, O2A, O3A, O4A, O5A, O6A, O7A, \
                          c32_p35, c32_n21, c32_n43, c32_p04, c32_p45, c32_p13, c32_n40, c32_n29, res07[part] )
            COMPUTE_ROW8( O0A, O1A, O2A, O3A, O4A, O5A, O6A, O7A, \
                          c32_p29, c32_n40, c32_n13, c32_p45, c32_n04, c32_n43, c32_p21, c32_p35, res09[part] )
            COMPUTE_ROW8( O0A, O1A, O2A, O3A, O4A, O5A, O6A, O7A, \
                          c32_p21, c32_n45, c32_p29, c32_p13, c32_n43, c32_p35, c32_p04, c32_n40, res11[part] )
            COMPUTE_ROW8( O0A, O1A, O2A, O3A, O4A, O5A, O6A, O7A, \
                          c32_p13, c32_n35, c32_p45, c32_n40, c32_p21, c32_p04, c32_n29, c32_p43, res13[part] )
            COMPUTE_ROW8( O0A, O1A, O2A, O3A, O4A, O5A, O6A, O7A, \
                          c32_p04, c32_n13, c32_p21, c32_n29, c32_p35, c32_n40, c32_p43, c32_n45, res15[part] )

#undef COMPUTE_ROW8
        }

        for ( i = 0; i < 4; i++ )
        {

            in00[i] = res00[i];
            in01[i] = res01[i];
            in02[i] = res02[i];
            in03[i] = res03[i];
            in04[i] = res04[i];
            in05[i] = res05[i];
            in06[i] = res06[i];
            in07[i] = res07[i];
            in08[i] = res08[i];
            in09[i] = res09[i];
            in10[i] = res10[i];
            in11[i] = res11[i];
            in12[i] = res12[i];
            in13[i] = res13[i];
            in14[i] = res14[i];
            in15[i] = res15[i];
        }

    }

    //  data compression
    in00[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in00[0] ), c32_rnd ), shift ), in00[0] );

    in00[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in00[1] ), c32_rnd ), shift ), in00[1] );

    in00[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in00[2] ), c32_rnd ), shift ), in00[2] );

    in00[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in00[3] ), c32_rnd ), shift ), in00[3] );

    in01[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in01[0] ), c32_rnd ), shift ), in01[0] );

    in01[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in01[1] ), c32_rnd ), shift ), in01[1] );

    in01[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in01[2] ), c32_rnd ), shift ), in01[2] );

    in01[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in01[3] ), c32_rnd ), shift ), in01[3] );

    in02[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in02[0] ), c32_rnd ), shift ), in02[0] );

    in02[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in02[1] ), c32_rnd ), shift ), in02[1] );

    in02[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in02[2] ), c32_rnd ), shift ), in02[2] );

    in02[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in02[3] ), c32_rnd ), shift ), in02[3] );

    in03[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in03[0] ), c32_rnd ), shift ), in03[0] );

    in03[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in03[1] ), c32_rnd ), shift ), in03[1] );

    in03[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in03[2] ), c32_rnd ), shift ), in03[2] );

    in03[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in03[3] ), c32_rnd ), shift ), in03[3] );

    in04[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in04[0] ), c32_rnd ), shift ), in04[0] );

    in04[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in04[1] ), c32_rnd ), shift ), in04[1] );

    in04[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in04[2] ), c32_rnd ), shift ), in04[2] );

    in04[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in04[3] ), c32_rnd ), shift ), in04[3] );

    in05[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in05[0] ), c32_rnd ), shift ), in05[0] );

    in05[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in05[1] ), c32_rnd ), shift ), in05[1] );

    in05[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in05[2] ), c32_rnd ), shift ), in05[2] );

    in05[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in05[3] ), c32_rnd ), shift ), in05[3] );

    in06[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in06[0] ), c32_rnd ), shift ), in06[0] );

    in06[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in06[1] ), c32_rnd ), shift ), in06[1] );

    in06[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in06[2] ), c32_rnd ), shift ), in06[2] );

    in06[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in06[3] ), c32_rnd ), shift ), in06[3] );

    in07[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in07[0] ), c32_rnd ), shift ), in07[0] );

    in07[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in07[1] ), c32_rnd ), shift ), in07[1] );

    in07[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in07[2] ), c32_rnd ), shift ), in07[2] );

    in07[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in07[3] ), c32_rnd ), shift ), in07[3] );

    in08[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in08[0] ), c32_rnd ), shift ), in08[0] );

    in08[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in08[1] ), c32_rnd ), shift ), in08[1] );

    in08[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in08[2] ), c32_rnd ), shift ), in08[2] );

    in08[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in08[3] ), c32_rnd ), shift ), in08[3] );

    in09[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in09[0] ), c32_rnd ), shift ), in09[0] );

    in09[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in09[1] ), c32_rnd ), shift ), in09[1] );

    in09[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in09[2] ), c32_rnd ), shift ), in09[2] );

    in09[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in09[3] ), c32_rnd ), shift ), in09[3] );

    in10[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in10[0] ), c32_rnd ), shift ), in10[0] );

    in10[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in10[1] ), c32_rnd ), shift ), in10[1] );

    in10[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in10[2] ), c32_rnd ), shift ), in10[2] );

    in10[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in10[3] ), c32_rnd ), shift ), in10[3] );

    in11[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in11[0] ), c32_rnd ), shift ), in11[0] );

    in11[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in11[1] ), c32_rnd ), shift ), in11[1] );

    in11[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in11[2] ), c32_rnd ), shift ), in11[2] );

    in11[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in11[3] ), c32_rnd ), shift ), in11[3] );

    in12[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in12[0] ), c32_rnd ), shift ), in12[0] );

    in12[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in12[1] ), c32_rnd ), shift ), in12[1] );

    in12[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in12[2] ), c32_rnd ), shift ), in12[2] );

    in12[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in12[3] ), c32_rnd ), shift ), in12[3] );

    in13[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in13[0] ), c32_rnd ), shift ), in13[0] );

    in13[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in13[1] ), c32_rnd ), shift ), in13[1] );

    in13[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in13[2] ), c32_rnd ), shift ), in13[2] );

    in13[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in13[3] ), c32_rnd ), shift ), in13[3] );

    in14[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in14[0] ), c32_rnd ), shift ), in14[0] );

    in14[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in14[1] ), c32_rnd ), shift ), in14[1] );

    in14[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in14[2] ), c32_rnd ), shift ), in14[2] );

    in14[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in14[3] ), c32_rnd ), shift ), in14[3] );

    in15[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in15[0] ), c32_rnd ), shift ), in15[0] );

    in15[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in15[1] ), c32_rnd ), shift ), in15[1] );

    in15[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in15[2] ), c32_rnd ), shift ), in15[2] );

    in15[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in15[3] ), c32_rnd ), shift ), in15[3] );

    // store the final data
    _mm_storeu_si128( ( __m128i* )&curr_blk[0 * 16 + 0], in00[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[0 * 16 + 4], in00[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[0 * 16 + 8], in00[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[0 * 16 + 12], in00[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[1 * 16 + 0], in01[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[1 * 16 + 4], in01[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[1 * 16 + 8], in01[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[1 * 16 + 12], in01[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[2 * 16 + 0], in02[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[2 * 16 + 4], in02[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[2 * 16 + 8], in02[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[2 * 16 + 12], in02[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[3 * 16 + 0], in03[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[3 * 16 + 4], in03[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[3 * 16 + 8], in03[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[3 * 16 + 12], in03[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[4 * 16 + 0], in04[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[4 * 16 + 4], in04[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[4 * 16 + 8], in04[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[4 * 16 + 12], in04[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[5 * 16 + 0], in05[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[5 * 16 + 4], in05[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[5 * 16 + 8], in05[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[5 * 16 + 12], in05[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[6 * 16 + 0], in06[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[6 * 16 + 4], in06[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[6 * 16 + 8], in06[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[6 * 16 + 12], in06[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[7 * 16 + 0], in07[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[7 * 16 + 4], in07[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[7 * 16 + 8], in07[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[7 * 16 + 12], in07[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[8 * 16 + 0], in08[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[8 * 16 + 4], in08[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[8 * 16 + 8], in08[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[8 * 16 + 12], in08[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[9 * 16 + 0], in09[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[9 * 16 + 4], in09[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[9 * 16 + 8], in09[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[9 * 16 + 12], in09[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[10 * 16 + 0], in10[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[10 * 16 + 4], in10[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[10 * 16 + 8], in10[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[10 * 16 + 12], in10[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[11 * 16 + 0], in11[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[11 * 16 + 4], in11[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[11 * 16 + 8], in11[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[11 * 16 + 12], in11[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[12 * 16 + 0], in12[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[12 * 16 + 4], in12[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[12 * 16 + 8], in12[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[12 * 16 + 12], in12[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[13 * 16 + 0], in13[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[13 * 16 + 4], in13[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[13 * 16 + 8], in13[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[13 * 16 + 12], in13[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[14 * 16 + 0], in14[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[14 * 16 + 4], in14[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[14 * 16 + 8], in14[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[14 * 16 + 12], in14[3] );

    _mm_storeu_si128( ( __m128i* )&curr_blk[15 * 16 + 0], in15[0] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[15 * 16 + 4], in15[1] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[15 * 16 + 8], in15[2] );
    _mm_storeu_si128( ( __m128i* )&curr_blk[15 * 16 + 12], in15[3] );

}

#endif
