/*****************************************************************************
* Authors: Ronggang Wang <rgwang@pkusz.edu.cn>
*          Zhenyu Wang <wangzhenyu@pkusz.edu.cn>
*          Kui Fan <kuifan@pku.edu.cn>
*          Shenghao Zhang <1219759986@qq.com>
* Affiliate: Peking University Shenzhen Graduate School
*****************************************************************************/

#include "intrinsic.h"

#if COMPILE_FOR_8BIT

void idct_4x4_sse128( int *curr_blk )
{
    int shift1, shift2;

    __m128i T0, T1, T2, T3;
    __m128i M0, M1, M2, M3;
    __m128i mTemp, mTemp0, mTemp1, mTemp2, mTemp3, mTemp5, mTemp6;
    __m128i mCoef1, mCoef2, mCoef3, mCoef4;

    shift1 = 7;
    shift2 = 3;

    mCoef1 = _mm_set1_epi32( 69 );
    mCoef2 = _mm_set1_epi32( 98 );
    mCoef3 = _mm_set1_epi32( 236 );
    mCoef4 = _mm_set1_epi32( 4 );

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

    mTemp0 = _mm_add_epi32( T0, T2 );
    mTemp2 = _mm_sub_epi32( T0, T2 );

    mTemp5 = _mm_add_epi32( T1, T3 );
    mTemp6 = _mm_mullo_epi32( mTemp5, mCoef1 );
    mTemp = _mm_srai_epi32( mTemp6, shift1 );

    mTemp1 = _mm_add_epi32( mTemp, _mm_srai_epi32( _mm_mullo_epi32( T1, mCoef2 ), shift1 ) );
    mTemp3 = _mm_sub_epi32( mTemp, _mm_srai_epi32( _mm_mullo_epi32( T3, mCoef3 ), shift1 ) );

    T0 = _mm_add_epi32( mTemp0, mTemp1 );
    T1 = _mm_add_epi32( mTemp2, mTemp3 );
    T2 = _mm_sub_epi32( mTemp2, mTemp3 );
    T3 = _mm_sub_epi32( mTemp0, mTemp1 );

    // ------------  store back  -------------
    M0 = _mm_unpacklo_epi32( T0, T1 );
    M1 = _mm_unpacklo_epi32( T2, T3 );
    M2 = _mm_unpackhi_epi32( T0, T1 );
    M3 = _mm_unpackhi_epi32( T2, T3 );

    T0 = _mm_unpacklo_epi64( M0, M1 );
    T1 = _mm_unpackhi_epi64( M0, M1 );
    T2 = _mm_unpacklo_epi64( M2, M3 );
    T3 = _mm_unpackhi_epi64( M2, M3 );

    _mm_storeu_si128( ( __m128i* )( curr_blk + 0 ), T0 );
    _mm_storeu_si128( ( __m128i* )( curr_blk + 4 ), T1 );
    _mm_storeu_si128( ( __m128i* )( curr_blk + 2 * 4 ), T2 );
    _mm_storeu_si128( ( __m128i* )( curr_blk + 3 * 4 ), T3 );

    // ------------------  2nd for circulation  -----------------
    T0 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 ) );
    T1 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 ) );
    T2 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2*4 ) );
    T3 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3*4 ) );

    mTemp0 = _mm_add_epi32( T0, T2 );
    mTemp2 = _mm_sub_epi32( T0, T2 );

    mTemp = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( T1, T3 ), mCoef1 ), shift1 );

    mTemp1 = _mm_add_epi32( mTemp, _mm_srai_epi32( _mm_mullo_epi32( T1, mCoef2 ), shift1 ) );
    mTemp3 = _mm_sub_epi32( mTemp, _mm_srai_epi32( _mm_mullo_epi32( T3, mCoef3 ), shift1 ) );

    T0 = _mm_add_epi32( mTemp0, mTemp1 );
    T1 = _mm_add_epi32( mTemp2, mTemp3 );
    T2 = _mm_sub_epi32( mTemp2, mTemp3 );
    T3 = _mm_sub_epi32( mTemp0, mTemp1 );

    //M0 = _mm_cmpgt_epi32(T0, c_0);
    T0 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T0 ), mCoef4 ), shift2 ), T0 );
    T1 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T1 ), mCoef4 ), shift2 ), T1 );
    T2 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T2 ), mCoef4 ), shift2 ), T2 );
    T3 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T3 ), mCoef4 ), shift2 ), T3 );

    // ------------  store back  -------------
    _mm_storeu_si128( ( __m128i* )( curr_blk + 0 ), T0 );
    _mm_storeu_si128( ( __m128i* )( curr_blk + 4 ), T1 );
    _mm_storeu_si128( ( __m128i* )( curr_blk + 2 * 4 ), T2 );
    _mm_storeu_si128( ( __m128i* )( curr_blk + 3 * 4 ), T3 );
}

void idct_8x8_sse128( int *curr_blk )
{
    int shift1, shift2, shift3, shift4;

    __m128i T10, T11, T12, T13, T14, T15, T16, T17;
    __m128i T20, T21, T22, T23, T24, T25, T26, T27;
    __m128i M10, M11, M12, M13, M14, M15, M16, M17;
    __m128i M20, M21, M22, M23, M24, M25, M26, M27;

    __m128i mTemp110, mTemp111, mTemp112, mTemp113, mTemp114, mTemp115, mTemp116, mTemp117;
    __m128i mTemp120, mTemp121, mTemp122, mTemp123, mTemp124, mTemp125, mTemp126, mTemp127;

    __m128i mTemp220, mTemp230, mTemp221, mTemp231, mTemp222, mTemp232, mTemp223, mTemp233;
    __m128i mTemp224, mTemp234, mTemp225, mTemp235, mTemp226, mTemp236, mTemp227, mTemp237;

    __m128i mCoef1, mCoef2, mCoef3, mCoef4, mCoef5, mCoef6, mCoef7, mCoef8;

    mCoef1 = _mm_set1_epi32( 181 );
    mCoef2 = _mm_set1_epi32( 196 );
    mCoef3 = _mm_set1_epi32( 473 );
    mCoef4 = _mm_set1_epi32( 201 );
    mCoef5 = _mm_set1_epi32( 301 );
    mCoef6 = _mm_set1_epi32( 141 );
    mCoef7 = _mm_set1_epi32( 710 );
    mCoef8 = _mm_set1_epi32( 16 );

    shift1 = 7;
    shift2 = 8;
    shift3 = 9;
    shift4 = 5;

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
    mTemp110 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( T10, T14 ), mCoef1 ), shift1 );
    mTemp120 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( T20, T24 ), mCoef1 ), shift1 );

    mTemp111 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( T10, T14 ), mCoef1 ), shift1 );
    mTemp121 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( T20, T24 ), mCoef1 ), shift1 );

    mTemp112 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( T12, mCoef2 ), shift2 ), _mm_srai_epi32( _mm_mullo_epi32( T16, mCoef3 ), shift2 ) );
    mTemp122 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( T22, mCoef2 ), shift2 ), _mm_srai_epi32( _mm_mullo_epi32( T26, mCoef3 ), shift2 ) );

    mTemp113 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( T12, mCoef3 ), shift2 ), _mm_srai_epi32( _mm_mullo_epi32( T16, mCoef2 ), shift2 ) );
    mTemp123 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( T22, mCoef3 ), shift2 ), _mm_srai_epi32( _mm_mullo_epi32( T26, mCoef2 ), shift2 ) );

    mTemp224 = _mm_sub_epi32( T11, T17 );
    mTemp234 = _mm_sub_epi32( T21, T27 );

    mTemp227 = _mm_add_epi32( T11, T17 );
    mTemp237 = _mm_add_epi32( T21, T27 );

    mTemp225 = _mm_srai_epi32( _mm_mullo_epi32( T13, mCoef1 ), shift1 );
    mTemp235 = _mm_srai_epi32( _mm_mullo_epi32( T23, mCoef1 ), shift1 );

    mTemp226 = _mm_srai_epi32( _mm_mullo_epi32( T15, mCoef1 ), shift1 );
    mTemp236 = _mm_srai_epi32( _mm_mullo_epi32( T25, mCoef1 ), shift1 );

    mTemp114 = _mm_add_epi32( mTemp224, mTemp226 );
    mTemp124 = _mm_add_epi32( mTemp234, mTemp236 );

    mTemp115 = _mm_sub_epi32( mTemp227, mTemp225 );
    mTemp125 = _mm_sub_epi32( mTemp237, mTemp235 );

    mTemp116 = _mm_sub_epi32( mTemp224, mTemp226 );
    mTemp126 = _mm_sub_epi32( mTemp234, mTemp236 );

    mTemp117 = _mm_add_epi32( mTemp227, mTemp225 );
    mTemp127 = _mm_add_epi32( mTemp237, mTemp235 );

    // step2
    mTemp220 = _mm_add_epi32( mTemp110, mTemp113 );
    mTemp230 = _mm_add_epi32( mTemp120, mTemp123 );

    mTemp223 = _mm_sub_epi32( mTemp110, mTemp113 );
    mTemp233 = _mm_sub_epi32( mTemp120, mTemp123 );

    mTemp221 = _mm_add_epi32( mTemp111, mTemp112 );
    mTemp231 = _mm_add_epi32( mTemp121, mTemp122 );

    mTemp222 = _mm_sub_epi32( mTemp111, mTemp112 );
    mTemp232 = _mm_sub_epi32( mTemp121, mTemp122 );

    mTemp224 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp114, mCoef5 ), shift2 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp117, mCoef4 ), shift2 ) );
    mTemp234 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp124, mCoef5 ), shift2 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp127, mCoef4 ), shift2 ) );

    mTemp227 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp114, mCoef4 ), shift2 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp117, mCoef5 ), shift2 ) );
    mTemp237 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp124, mCoef4 ), shift2 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp127, mCoef5 ), shift2 ) );

    mTemp225 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp115, mCoef7 ), shift3 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp116, mCoef6 ), shift3 ) );
    mTemp235 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp125, mCoef7 ), shift3 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp126, mCoef6 ), shift3 ) );

    mTemp226 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp115, mCoef6 ), shift3 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp116, mCoef7 ), shift3 ) );
    mTemp236 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp125, mCoef6 ), shift3 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp126, mCoef7 ), shift3 ) );

    // step3 store back

    T10 = _mm_add_epi32( mTemp220, mTemp227 );
    T20 = _mm_add_epi32( mTemp230, mTemp237 );

    T11 = _mm_add_epi32( mTemp221, mTemp226 );
    T21 = _mm_add_epi32( mTemp231, mTemp236 );

    T12 = _mm_add_epi32( mTemp222, mTemp225 );
    T22 = _mm_add_epi32( mTemp232, mTemp235 );

    T13 = _mm_add_epi32( mTemp223, mTemp224 );
    T23 = _mm_add_epi32( mTemp233, mTemp234 );

    T14 = _mm_sub_epi32( mTemp223, mTemp224 );
    T24 = _mm_sub_epi32( mTemp233, mTemp234 );

    T15 = _mm_sub_epi32( mTemp222, mTemp225 );
    T25 = _mm_sub_epi32( mTemp232, mTemp235 );

    T16 = _mm_sub_epi32( mTemp221, mTemp226 );
    T26 = _mm_sub_epi32( mTemp231, mTemp236 );

    T17 = _mm_sub_epi32( mTemp220, mTemp227 );
    T27 = _mm_sub_epi32( mTemp230, mTemp237 );

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

    //////////////////////////////////////////
    mCoef1 = _mm_set1_epi32( 181 );
    mCoef2 = _mm_set1_epi32( 196 );
    mCoef3 = _mm_set1_epi32( 473 );
    mCoef4 = _mm_set1_epi32( 201 );
    mCoef5 = _mm_set1_epi32( 301 );
    mCoef6 = _mm_set1_epi32( 141 );
    mCoef7 = _mm_set1_epi32( 710 );
    mCoef8 = _mm_set1_epi32( 16 );

    shift1 = 7;
    shift2 = 8;
    shift3 = 9;
    //////////////////////////////////////////

    // step1
    mTemp110 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( T10, T14 ), mCoef1 ), shift1 );
    mTemp120 = _mm_srai_epi32( _mm_mullo_epi32( _mm_add_epi32( T20, T24 ), mCoef1 ), shift1 );

    mTemp111 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( T10, T14 ), mCoef1 ), shift1 );
    mTemp121 = _mm_srai_epi32( _mm_mullo_epi32( _mm_sub_epi32( T20, T24 ), mCoef1 ), shift1 );

    mTemp112 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( T12, mCoef2 ), shift2 ), _mm_srai_epi32( _mm_mullo_epi32( T16, mCoef3 ), shift2 ) );
    mTemp122 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( T22, mCoef2 ), shift2 ), _mm_srai_epi32( _mm_mullo_epi32( T26, mCoef3 ), shift2 ) );

    mTemp113 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( T12, mCoef3 ), shift2 ), _mm_srai_epi32( _mm_mullo_epi32( T16, mCoef2 ), shift2 ) );
    mTemp123 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( T22, mCoef3 ), shift2 ), _mm_srai_epi32( _mm_mullo_epi32( T26, mCoef2 ), shift2 ) );

    mTemp224 = _mm_sub_epi32( T11, T17 );
    mTemp234 = _mm_sub_epi32( T21, T27 );

    mTemp227 = _mm_add_epi32( T11, T17 );
    mTemp237 = _mm_add_epi32( T21, T27 );

    mTemp225 = _mm_srai_epi32( _mm_mullo_epi32( T13, mCoef1 ), shift1 );
    mTemp235 = _mm_srai_epi32( _mm_mullo_epi32( T23, mCoef1 ), shift1 );

    mTemp226 = _mm_srai_epi32( _mm_mullo_epi32( T15, mCoef1 ), shift1 );
    mTemp236 = _mm_srai_epi32( _mm_mullo_epi32( T25, mCoef1 ), shift1 );

    mTemp114 = _mm_add_epi32( mTemp224, mTemp226 );
    mTemp124 = _mm_add_epi32( mTemp234, mTemp236 );

    mTemp115 = _mm_sub_epi32( mTemp227, mTemp225 );
    mTemp125 = _mm_sub_epi32( mTemp237, mTemp235 );

    mTemp116 = _mm_sub_epi32( mTemp224, mTemp226 );
    mTemp126 = _mm_sub_epi32( mTemp234, mTemp236 );

    mTemp117 = _mm_add_epi32( mTemp227, mTemp225 );
    mTemp127 = _mm_add_epi32( mTemp237, mTemp235 );

    // step2
    mTemp220 = _mm_add_epi32( mTemp110, mTemp113 );
    mTemp230 = _mm_add_epi32( mTemp120, mTemp123 );

    mTemp223 = _mm_sub_epi32( mTemp110, mTemp113 );
    mTemp233 = _mm_sub_epi32( mTemp120, mTemp123 );

    mTemp221 = _mm_add_epi32( mTemp111, mTemp112 );
    mTemp231 = _mm_add_epi32( mTemp121, mTemp122 );

    mTemp222 = _mm_sub_epi32( mTemp111, mTemp112 );
    mTemp232 = _mm_sub_epi32( mTemp121, mTemp122 );

    mTemp224 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp114, mCoef5 ), shift2 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp117, mCoef4 ), shift2 ) );
    mTemp234 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp124, mCoef5 ), shift2 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp127, mCoef4 ), shift2 ) );

    mTemp227 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp114, mCoef4 ), shift2 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp117, mCoef5 ), shift2 ) );
    mTemp237 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp124, mCoef4 ), shift2 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp127, mCoef5 ), shift2 ) );

    mTemp225 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp115, mCoef7 ), shift3 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp116, mCoef6 ), shift3 ) );
    mTemp235 = _mm_sub_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp125, mCoef7 ), shift3 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp126, mCoef6 ), shift3 ) );

    mTemp226 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp115, mCoef6 ), shift3 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp116, mCoef7 ), shift3 ) );
    mTemp236 = _mm_add_epi32( _mm_srai_epi32( _mm_mullo_epi32( mTemp125, mCoef6 ), shift3 ), \
                              _mm_srai_epi32( _mm_mullo_epi32( mTemp126, mCoef7 ), shift3 ) );

    // step3 store back

    T10 = _mm_add_epi32( mTemp220, mTemp227 );
    T10 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T10 ), mCoef8 ), shift4 ), T10 );
    T20 = _mm_add_epi32( mTemp230, mTemp237 );
    T20 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T20 ), mCoef8 ), shift4 ), T20 );

    T11 = _mm_add_epi32( mTemp221, mTemp226 );
    T11 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T11 ), mCoef8 ), shift4 ), T11 );
    T21 = _mm_add_epi32( mTemp231, mTemp236 );
    T21 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T21 ), mCoef8 ), shift4 ), T21 );

    T12 = _mm_add_epi32( mTemp222, mTemp225 );
    T12 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T12 ), mCoef8 ), shift4 ), T12 );
    T22 = _mm_add_epi32( mTemp232, mTemp235 );
    T22 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T22 ), mCoef8 ), shift4 ), T22 );

    T13 = _mm_add_epi32( mTemp223, mTemp224 );
    T13 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T13 ), mCoef8 ), shift4 ), T13 );
    T23 = _mm_add_epi32( mTemp233, mTemp234 );
    T23 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T23 ), mCoef8 ), shift4 ), T23 );

    T14 = _mm_sub_epi32( mTemp223, mTemp224 );
    T14 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T14 ), mCoef8 ), shift4 ), T14 );
    T24 = _mm_sub_epi32( mTemp233, mTemp234 );
    T24 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T24 ), mCoef8 ), shift4 ), T24 );

    T15 = _mm_sub_epi32( mTemp222, mTemp225 );
    T15 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T15 ), mCoef8 ), shift4 ), T15 );
    T25 = _mm_sub_epi32( mTemp232, mTemp235 );
    T25 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T25 ), mCoef8 ), shift4 ), T25 );

    T16 = _mm_sub_epi32( mTemp221, mTemp226 );
    T16 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T16 ), mCoef8 ), shift4 ), T16 );
    T26 = _mm_sub_epi32( mTemp231, mTemp236 );
    T26 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T26 ), mCoef8 ), shift4 ), T26 );

    T17 = _mm_sub_epi32( mTemp220, mTemp227 );
    T17 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T17 ), mCoef8 ), shift4 ), T17 );
    T27 = _mm_sub_epi32( mTemp230, mTemp237 );
    T27 = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( T27 ), mCoef8 ), shift4 ), T27 );

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

/*
void idct_16x16_sse128(int *curr_blk, int bsize)
{
    int i;
    int shift = 15;
    int pass, part;

    // load the data in the 16x16 coef matrix first
    const __m128i c32_p43_p45 = _mm_set_epi32(0x0000002B, 0x0000002D, 0x0000002B, 0x0000002D);      //row0
    const __m128i c32_p35_p40 = _mm_set_epi32(0x00000023, 0x00000028, 0x00000023, 0x00000028);
    const __m128i c32_p21_p29 = _mm_set_epi32(0x00000015, 0x0000001D, 0x00000015, 0x0000001D);
    const __m128i c32_p04_p13 = _mm_set_epi32(0x00000004, 0x0000000D, 0x00000004, 0x0000000D);
    const __m128i c32_p29_p43 = _mm_set_epi32(0x0000001D, 0x0000002B, 0x0000001D, 0x0000002B);      //row1
    const __m128i c32_n21_p04 = _mm_set_epi32(0xFFFFFFEB, 0x00000004, 0xFFFFFFEB, 0x00000004);
    const __m128i c32_n45_n40 = _mm_set_epi32(0xFFFFFFD3, 0xFFFFFFD8, 0xFFFFFFD3, 0xFFFFFFD8);
    const __m128i c32_n13_n35 = _mm_set_epi32(0xFFFFFFF3, 0xFFFFFFDD, 0xFFFFFFF3, 0xFFFFFFDD);
    const __m128i c32_p04_p40 = _mm_set_epi32(0x00000004, 0x00000028, 0x00000004, 0x00000028);      //row2
    const __m128i c32_n43_n35 = _mm_set_epi32(0xFFFFFFD5, 0xFFFFFFDD, 0xFFFFFFD5, 0xFFFFFFDD);
    const __m128i c32_p29_n13 = _mm_set_epi32(0x0000001D, 0xFFFFFFF3, 0x0000001D, 0xFFFFFFF3);
    const __m128i c32_p21_p45 = _mm_set_epi32(0x00000015, 0x0000002D, 0x00000015, 0x0000002D);
    const __m128i c32_n21_p35 = _mm_set_epi32(0xFFFFFFEB, 0x00000023, 0xFFFFFFEB, 0x00000023);      //row3
    const __m128i c32_p04_n43 = _mm_set_epi32(0x00000004, 0xFFFFFFD5, 0x00000004, 0xFFFFFFD5);
    const __m128i c32_p13_p45 = _mm_set_epi32(0x0000000D, 0x0000002D, 0x0000000D, 0x0000002D);
    const __m128i c32_n29_n40 = _mm_set_epi32(0xFFFFFFE3, 0xFFFFFFD8, 0xFFFFFFE3, 0xFFFFFFD8);
    const __m128i c32_n40_p29 = _mm_set_epi32(0xFFFFFFD8, 0x0000001D, 0xFFFFFFD8, 0x0000001D);      //row4
    const __m128i c32_p45_n13 = _mm_set_epi32(0x0000002D, 0xFFFFFFF3, 0x0000002D, 0xFFFFFFF3);
    const __m128i c32_n43_n04 = _mm_set_epi32(0xFFFFFFD5, 0xFFFFFFFC, 0xFFFFFFD5, 0xFFFFFFFC);
    const __m128i c32_p35_p21 = _mm_set_epi32(0x00000023, 0x00000015, 0x00000023, 0x00000015);
    const __m128i c32_n45_p21 = _mm_set_epi32(0xFFFFFFD3, 0x00000015, 0xFFFFFFD3, 0x00000015);      //row5
    const __m128i c32_p13_p29 = _mm_set_epi32(0x0000000D, 0x0000001D, 0x0000000D, 0x0000001D);
    const __m128i c32_p35_n43 = _mm_set_epi32(0x00000023, 0xFFFFFFD5, 0x00000023, 0xFFFFFFD5);
    const __m128i c32_n40_p04 = _mm_set_epi32(0xFFFFFFD8, 0x00000004, 0xFFFFFFD8, 0x00000004);
    const __m128i c32_n35_p13 = _mm_set_epi32(0xFFFFFFDD, 0x0000000D, 0xFFFFFFDD, 0x0000000D);      //row6
    const __m128i c32_n40_p45 = _mm_set_epi32(0xFFFFFFD8, 0x0000002D, 0xFFFFFFD8, 0x0000002D);
    const __m128i c32_p04_p21 = _mm_set_epi32(0x00000004, 0x00000015, 0x00000004, 0x00000015);
    const __m128i c32_p43_n29 = _mm_set_epi32(0x0000002B, 0xFFFFFFE3, 0x0000002B, 0xFFFFFFE3);
    const __m128i c32_n13_p04 = _mm_set_epi32(0xFFFFFFF3, 0x00000004, 0xFFFFFFF3, 0x00000004);      //row7
    const __m128i c32_n29_p21 = _mm_set_epi32(0xFFFFFFE3, 0x00000015, 0xFFFFFFE3, 0x00000015);
    const __m128i c32_n40_p35 = _mm_set_epi32(0xFFFFFFD8, 0x00000023, 0xFFFFFFD8, 0x00000023);
    const __m128i c32_n45_p43 = _mm_set_epi32(0xFFFFFFD3, 0x0000002B, 0xFFFFFFD3, 0x0000002B);

    const __m128i c32_p38_p44 = _mm_set_epi32(0x00000026, 0x0000002C, 0x00000026, 0x0000002C);      // R8
    const __m128i c32_p09_p25 = _mm_set_epi32(0x00000009, 0x00000019, 0x00000009, 0x00000019);
    const __m128i c32_n09_p38 = _mm_set_epi32(0xFFFFFFF7, 0x00000026, 0xFFFFFFF7, 0x00000026);
    const __m128i c32_n25_n44 = _mm_set_epi32(0xFFFFFFE7, 0xFFFFFFD4, 0xFFFFFFE7, 0xFFFFFFD4);
    const __m128i c32_n44_p25 = _mm_set_epi32(0xFFFFFFD4, 0x00000019, 0xFFFFFFD4, 0x00000019);
    const __m128i c32_p38_p09 = _mm_set_epi32(0x00000026, 0x00000009, 0x00000026, 0x00000009);
    const __m128i c32_n25_p09 = _mm_set_epi32(0xFFFFFFE7, 0x00000009, 0xFFFFFFE7, 0x00000009);
    const __m128i c32_n44_p38 = _mm_set_epi32(0xFFFFFFD4, 0x00000026, 0xFFFFFFD4, 0x00000026);

    const __m128i c32_p17_p42 = _mm_set_epi32(0x00000011, 0x0000002A, 0x00000011, 0x0000002A);      // R4
    const __m128i c32_n42_p17 = _mm_set_epi32(0xFFFFFFD6, 0x00000011, 0xFFFFFFD6, 0x00000011);

    const __m128i c32_n32_p32 = _mm_set_epi32(0xFFFFFFE0, 0x00000020, 0xFFFFFFE0, 0x00000020);
    const __m128i c32_p32_p32 = _mm_set_epi32(0x00000020, 0x00000020, 0x00000020, 0x00000020);

    const __m128i c32_rnd = _mm_set1_epi32(1 << (shift - 1));
    const __m128i max_val = _mm_set1_epi32(255);
    const __m128i min_val = _mm_set1_epi32(-255);

    // DCT1
    __m128i in00[4], in01[4], in02[4], in03[4], in04[4], in05[4], in06[4], in07[4];
    __m128i in08[4], in09[4], in10[4], in11[4], in12[4], in13[4], in14[4], in15[4];
    __m128i res00[4], res01[4], res02[4], res03[4], res04[4], res05[4], res06[4], res07[4];
    __m128i res08[4], res09[4], res10[4], res11[4], res12[4], res13[4], res14[4], res15[4];

    for (i = 0; i < 4; i++)
    {
        const int offset = i * 4;

        in00[i] = _mm_loadu_si128((const __m128i*)&curr_blk[0 * 16 + offset]);  // change _mm_loadu_si128 to _mm_load_si128
        in01[i] = _mm_loadu_si128((const __m128i*)&curr_blk[1 * 16 + offset]);
        in02[i] = _mm_loadu_si128((const __m128i*)&curr_blk[2 * 16 + offset]);
        in03[i] = _mm_loadu_si128((const __m128i*)&curr_blk[3 * 16 + offset]);
        in04[i] = _mm_loadu_si128((const __m128i*)&curr_blk[4 * 16 + offset]);
        in05[i] = _mm_loadu_si128((const __m128i*)&curr_blk[5 * 16 + offset]);
        in06[i] = _mm_loadu_si128((const __m128i*)&curr_blk[6 * 16 + offset]);
        in07[i] = _mm_loadu_si128((const __m128i*)&curr_blk[7 * 16 + offset]);
        in08[i] = _mm_loadu_si128((const __m128i*)&curr_blk[8 * 16 + offset]);
        in09[i] = _mm_loadu_si128((const __m128i*)&curr_blk[9 * 16 + offset]);
        in10[i] = _mm_loadu_si128((const __m128i*)&curr_blk[10 * 16 + offset]);
        in11[i] = _mm_loadu_si128((const __m128i*)&curr_blk[11 * 16 + offset]);
        in12[i] = _mm_loadu_si128((const __m128i*)&curr_blk[12 * 16 + offset]);
        in13[i] = _mm_loadu_si128((const __m128i*)&curr_blk[13 * 16 + offset]);
        in14[i] = _mm_loadu_si128((const __m128i*)&curr_blk[14 * 16 + offset]);
        in15[i] = _mm_loadu_si128((const __m128i*)&curr_blk[15 * 16 + offset]);
    }

    for (pass = 0; pass < 2; pass++)
    {
        for (part = 0; part < 4; part++)
        {
            const __m128i T_00_00A = _mm_unpacklo_epi32(in01[part], in03[part]);
            const __m128i T_00_00B = _mm_unpackhi_epi32(in01[part], in03[part]);
            const __m128i T_00_01A = _mm_unpacklo_epi32(in05[part], in07[part]);
            const __m128i T_00_01B = _mm_unpackhi_epi32(in05[part], in07[part]);
            const __m128i T_00_02A = _mm_unpacklo_epi32(in09[part], in11[part]);
            const __m128i T_00_02B = _mm_unpackhi_epi32(in09[part], in11[part]);
            const __m128i T_00_03A = _mm_unpacklo_epi32(in13[part], in15[part]);
            const __m128i T_00_03B = _mm_unpackhi_epi32(in13[part], in15[part]);

            const __m128i T_00_04A = _mm_unpacklo_epi32(in02[part], in06[part]);
            const __m128i T_00_04B = _mm_unpackhi_epi32(in02[part], in06[part]);
            const __m128i T_00_05A = _mm_unpacklo_epi32(in10[part], in14[part]);
            const __m128i T_00_05B = _mm_unpackhi_epi32(in10[part], in14[part]);

            const __m128i T_00_06A = _mm_unpacklo_epi32(in04[part], in12[part]);
            const __m128i T_00_06B = _mm_unpackhi_epi32(in04[part], in12[part]);

            const __m128i T_00_07A = _mm_unpacklo_epi32(in00[part], in08[part]);
            const __m128i T_00_07B = _mm_unpackhi_epi32(in00[part], in08[part]);

            __m128i O0A, O1A, O2A, O3A, O4A, O5A, O6A, O7A;
            __m128i O0B, O1B, O2B, O3B, O4B, O5B, O6B, O7B;
            __m128i EO0A, EO1A, EO2A, EO3A;
            __m128i EO0B, EO1B, EO2B, EO3B;
            __m128i EEO0A, EEO1A;
            __m128i EEO0B, EEO1B;
            __m128i EEE0A, EEE1A;
            __m128i EEE0B, EEE1B;
            __m128i T00, T01;
            __m128i mTemp1, mTemp2, mTemp3;
            __m128i mZero = _mm_set1_epi16(0);

// R8 multiplication
#define COMPUTE_ROW8(row0103, row0507, row0911, row1315, c0103, c0507, c0911, c1315, row) \
    T00 = _mm_hadd_epi32(_mm_mullo_epi32(row0103, c0103), _mm_mullo_epi32(row0507, c0507)); \
    T01 = _mm_hadd_epi32(_mm_mullo_epi32(row0911, c0911), _mm_mullo_epi32(row1315, c1315)); \
    mTemp1 = _mm_unpacklo_epi32(T00, T01); \
    mTemp2 = _mm_unpackhi_epi32(T00, T01); \
    mTemp3 = _mm_add_epi32(mTemp1, mTemp2); \
    row = _mm_hadd_epi32(mTemp3, mZero);        // only the low 64bit is valid!!!


            COMPUTE_ROW8(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c32_p43_p45, c32_p35_p40, c32_p21_p29, c32_p04_p13, O0A)
            COMPUTE_ROW8(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c32_p29_p43, c32_n21_p04, c32_n45_n40, c32_n13_n35, O1A)
            COMPUTE_ROW8(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c32_p04_p40, c32_n43_n35, c32_p29_n13, c32_p21_p45, O2A)
            COMPUTE_ROW8(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c32_n21_p35, c32_p04_n43, c32_p13_p45, c32_n29_n40, O3A)
            COMPUTE_ROW8(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c32_n40_p29, c32_p45_n13, c32_n43_n04, c32_p35_p21, O4A)
            COMPUTE_ROW8(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c32_n45_p21, c32_p13_p29, c32_p35_n43, c32_n40_p04, O5A)
            COMPUTE_ROW8(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c32_n35_p13, c32_n40_p45, c32_p04_p21, c32_p43_n29, O6A)
            COMPUTE_ROW8(T_00_00A, T_00_01A, T_00_02A, T_00_03A, c32_n13_p04, c32_n29_p21, c32_n40_p35, c32_n45_p43, O7A)

            COMPUTE_ROW8(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c32_p43_p45, c32_p35_p40, c32_p21_p29, c32_p04_p13, O0B)
            COMPUTE_ROW8(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c32_p29_p43, c32_n21_p04, c32_n45_n40, c32_n13_n35, O1B)
            COMPUTE_ROW8(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c32_p04_p40, c32_n43_n35, c32_p29_n13, c32_p21_p45, O2B)
            COMPUTE_ROW8(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c32_n21_p35, c32_p04_n43, c32_p13_p45, c32_n29_n40, O3B)
            COMPUTE_ROW8(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c32_n40_p29, c32_p45_n13, c32_n43_n04, c32_p35_p21, O4B)
            COMPUTE_ROW8(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c32_n45_p21, c32_p13_p29, c32_p35_n43, c32_n40_p04, O5B)
            COMPUTE_ROW8(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c32_n35_p13, c32_n40_p45, c32_p04_p21, c32_p43_n29, O6B)
            COMPUTE_ROW8(T_00_00B, T_00_01B, T_00_02B, T_00_03B, c32_n13_p04, c32_n29_p21, c32_n40_p35, c32_n45_p43, O7B)


#undef COMPUTE_ROW8

// R4 multiplication
#define COMPUTE_ROW4(row0206, row1014, c0206, c1014,row) \
    mTemp1 = _mm_mullo_epi32(row0206, c0206); \
    mTemp2 = _mm_mullo_epi32(row1014, c1014); \
    mTemp3 = _mm_add_epi32(mTemp1, mTemp2); \
    row = _mm_hadd_epi32(mTemp3, mZero);        // only the low 64bit is valid!!!

            COMPUTE_ROW4(T_00_04A, T_00_05A, c32_p38_p44, c32_p09_p25, EO0A) // EO0
            COMPUTE_ROW4(T_00_04B, T_00_05B, c32_p38_p44, c32_p09_p25, EO0B)
            COMPUTE_ROW4(T_00_04A, T_00_05A, c32_n09_p38, c32_n25_n44, EO1A) // EO1
            COMPUTE_ROW4(T_00_04B, T_00_05B, c32_n09_p38, c32_n25_n44, EO1B)
            COMPUTE_ROW4(T_00_04A, T_00_05A, c32_n44_p25, c32_p38_p09, EO2A) // EO2
            COMPUTE_ROW4(T_00_04B, T_00_05B, c32_n44_p25, c32_p38_p09, EO2B)
            COMPUTE_ROW4(T_00_04A, T_00_05A, c32_n25_p09, c32_n44_p38, EO3A) // EO3
            COMPUTE_ROW4(T_00_04B, T_00_05B, c32_n25_p09, c32_n44_p38, EO3B)

#undef COMPUTE_ROW4

// 4X4 FDCT
#define FDCT_MADD(row0412, c0412,row) \
    mTemp1 = _mm_mullo_epi32(row0412, c0412); \
    row = _mm_hadd_epi32(mTemp1, mZero);        // only the low 64bit is valid!!!

            FDCT_MADD(T_00_06A, c32_p17_p42, EEO0A);
            FDCT_MADD(T_00_06B, c32_p17_p42, EEO0B);
            FDCT_MADD(T_00_06A, c32_n42_p17, EEO1A);
            FDCT_MADD(T_00_06B, c32_n42_p17, EEO1B);

            FDCT_MADD(T_00_07A, c32_p32_p32, EEE0A);
            FDCT_MADD(T_00_07B, c32_p32_p32, EEE0B);
            FDCT_MADD(T_00_07A, c32_n32_p32, EEE1A);
            FDCT_MADD(T_00_07B, c32_n32_p32, EEE1B);

#undef FDCT_MADD

            {
                const __m128i EE0A = _mm_add_epi32(EEE0A, EEO0A);          // EE0 = EEE0 + EEO0
                const __m128i EE0B = _mm_add_epi32(EEE0B, EEO0B);
                const __m128i EE1A = _mm_add_epi32(EEE1A, EEO1A);          // EE1 = EEE1 + EEO1
                const __m128i EE1B = _mm_add_epi32(EEE1B, EEO1B);
                const __m128i EE3A = _mm_sub_epi32(EEE0A, EEO0A);          // EE2 = EEE0 - EEO0
                const __m128i EE3B = _mm_sub_epi32(EEE0B, EEO0B);
                const __m128i EE2A = _mm_sub_epi32(EEE1A, EEO1A);          // EE3 = EEE1 - EEO1
                const __m128i EE2B = _mm_sub_epi32(EEE1B, EEO1B);

                const __m128i E0A = _mm_add_epi32(EE0A, EO0A);          // E0 = EE0 + EO0
                const __m128i E0B = _mm_add_epi32(EE0B, EO0B);
                const __m128i E1A = _mm_add_epi32(EE1A, EO1A);          // E1 = EE1 + EO1
                const __m128i E1B = _mm_add_epi32(EE1B, EO1B);
                const __m128i E2A = _mm_add_epi32(EE2A, EO2A);          // E2 = EE2 + EO2
                const __m128i E2B = _mm_add_epi32(EE2B, EO2B);
                const __m128i E3A = _mm_add_epi32(EE3A, EO3A);          // E3 = EE3 + EO3
                const __m128i E3B = _mm_add_epi32(EE3B, EO3B);
                const __m128i E7A = _mm_sub_epi32(EE0A, EO0A);          // E0 = EE0 - EO0
                const __m128i E7B = _mm_sub_epi32(EE0B, EO0B);
                const __m128i E6A = _mm_sub_epi32(EE1A, EO1A);          // E1 = EE1 - EO1
                const __m128i E6B = _mm_sub_epi32(EE1B, EO1B);
                const __m128i E5A = _mm_sub_epi32(EE2A, EO2A);          // E2 = EE2 - EO2
                const __m128i E5B = _mm_sub_epi32(EE2B, EO2B);
                const __m128i E4A = _mm_sub_epi32(EE3A, EO3A);          // E3 = EE3 - EO3
                const __m128i E4B = _mm_sub_epi32(EE3B, EO3B);

                const __m128i T20A = _mm_add_epi32(E0A, O0A);          // E0 + O0 + rnd
                const __m128i T20B = _mm_add_epi32(E0B, O0B);
                const __m128i T21A = _mm_add_epi32(E1A, O1A);          // E1 + O1 + rnd
                const __m128i T21B = _mm_add_epi32(E1B, O1B);
                const __m128i T22A = _mm_add_epi32(E2A, O2A);          // E2 + O2 + rnd
                const __m128i T22B = _mm_add_epi32(E2B, O2B);
                const __m128i T23A = _mm_add_epi32(E3A, O3A);          // E3 + O3 + rnd
                const __m128i T23B = _mm_add_epi32(E3B, O3B);
                const __m128i T24A = _mm_add_epi32(E4A, O4A);          // E4
                const __m128i T24B = _mm_add_epi32(E4B, O4B);
                const __m128i T25A = _mm_add_epi32(E5A, O5A);          // E5
                const __m128i T25B = _mm_add_epi32(E5B, O5B);
                const __m128i T26A = _mm_add_epi32(E6A, O6A);          // E6
                const __m128i T26B = _mm_add_epi32(E6B, O6B);
                const __m128i T27A = _mm_add_epi32(E7A, O7A);          // E7
                const __m128i T27B = _mm_add_epi32(E7B, O7B);
                const __m128i T2FA = _mm_sub_epi32(E0A, O0A);          // E0 - O0 + rnd
                const __m128i T2FB = _mm_sub_epi32(E0B, O0B);
                const __m128i T2EA = _mm_sub_epi32(E1A, O1A);          // E1 - O1 + rnd
                const __m128i T2EB = _mm_sub_epi32(E1B, O1B);
                const __m128i T2DA = _mm_sub_epi32(E2A, O2A);          // E2 - O2 + rnd
                const __m128i T2DB = _mm_sub_epi32(E2B, O2B);
                const __m128i T2CA = _mm_sub_epi32(E3A, O3A);          // E3 - O3 + rnd
                const __m128i T2CB = _mm_sub_epi32(E3B, O3B);
                const __m128i T2BA = _mm_sub_epi32(E4A, O4A);          // E4
                const __m128i T2BB = _mm_sub_epi32(E4B, O4B);
                const __m128i T2AA = _mm_sub_epi32(E5A, O5A);          // E5
                const __m128i T2AB = _mm_sub_epi32(E5B, O5B);
                const __m128i T29A = _mm_sub_epi32(E6A, O6A);          // E6
                const __m128i T29B = _mm_sub_epi32(E6B, O6B);
                const __m128i T28A = _mm_sub_epi32(E7A, O7A);          // E7
                const __m128i T28B = _mm_sub_epi32(E7B, O7B);

                res00[part] = _mm_unpacklo_epi64 (T20A, T20B);        // [70 60 50 40 30 20 10 00]
                res01[part] = _mm_unpacklo_epi64 (T21A, T21B);        // [71 61 51 41 31 21 11 01]
                res02[part] = _mm_unpacklo_epi64 (T22A, T22B);        // [72 62 52 42 32 22 12 02]
                res03[part] = _mm_unpacklo_epi64 (T23A, T23B);        // [73 63 53 43 33 23 13 03]
                res04[part] = _mm_unpacklo_epi64 (T24A, T24B);        // [74 64 54 44 34 24 14 04]
                res05[part] = _mm_unpacklo_epi64 (T25A, T25B);        // [75 65 55 45 35 25 15 05]
                res06[part] = _mm_unpacklo_epi64 (T26A, T26B);        // [76 66 56 46 36 26 16 06]
                res07[part] = _mm_unpacklo_epi64 (T27A, T27B);        // [77 67 57 47 37 27 17 07]

                res08[part] = _mm_unpacklo_epi64 (T28A, T28B);        // [A0 ... 80]
                res09[part] = _mm_unpacklo_epi64 (T29A, T29B);        // [A1 ... 81]
                res10[part] = _mm_unpacklo_epi64 (T2AA, T2AB);        // [A2 ... 82]
                res11[part] = _mm_unpacklo_epi64 (T2BA, T2BB);        // [A3 ... 83]
                res12[part] = _mm_unpacklo_epi64 (T2CA, T2CB);        // [A4 ... 84]
                res13[part] = _mm_unpacklo_epi64 (T2DA, T2DB);        // [A5 ... 85]
                res14[part] = _mm_unpacklo_epi64 (T2EA, T2EB);        // [A6 ... 86]
                res15[part] = _mm_unpacklo_epi64 (T2FA, T2FB);        // [A7 ... 87]
            }
        }

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

            TRANSPOSE_8x8_16BIT(res00[0], res01[0], res02[0], res03[0], res04[0], res05[0], res06[0], res07[0], in00[0], in01[0], in02[0], in03[0], in00[1], in01[1], in02[1], in03[1])
            TRANSPOSE_8x8_16BIT(res08[0], res09[0], res10[0], res11[0], res12[0], res13[0], res14[0], res15[0], in00[2], in01[2], in02[2], in03[2], in00[3], in01[3], in02[3], in03[3])
            TRANSPOSE_8x8_16BIT(res00[1], res01[1], res02[1], res03[1], res04[1], res05[1], res06[1], res07[1], in04[0], in05[0], in06[0], in07[0], in04[1], in05[1], in06[1], in07[1])
            TRANSPOSE_8x8_16BIT(res08[1], res09[1], res10[1], res11[1], res12[1], res13[1], res14[1], res15[1], in04[2], in05[2], in06[2], in07[2], in04[3], in05[3], in06[3], in07[3])
            TRANSPOSE_8x8_16BIT(res00[2], res01[2], res02[2], res03[2], res04[2], res05[2], res06[2], res07[2], in08[0], in09[0], in10[0], in11[0], in08[1], in09[1], in10[1], in11[1])
            TRANSPOSE_8x8_16BIT(res08[2], res09[2], res10[2], res11[2], res12[2], res13[2], res14[2], res15[2], in08[2], in09[2], in10[2], in11[2], in08[3], in09[3], in10[3], in11[3])
            TRANSPOSE_8x8_16BIT(res00[3], res01[3], res02[3], res03[3], res04[3], res05[3], res06[3], res07[3], in12[0], in13[0], in14[0], in15[0], in12[1], in13[1], in14[1], in15[1])
            TRANSPOSE_8x8_16BIT(res08[3], res09[3], res10[3], res11[3], res12[3], res13[3], res14[3], res15[3], in12[2], in13[2], in14[2], in15[2], in12[3], in13[3], in14[3], in15[3])

#undef TRANSPOSE_8x8_16BI

        }
    }

    //  data compression
    in00[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in00[0]), c32_rnd), shift), in00[0]);
    in00[0] = _mm_min_epi32(in00[0], max_val);
    in00[0] = _mm_max_epi32(in00[0], min_val);

    in00[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in00[1]), c32_rnd), shift), in00[1]);
    in00[1] = _mm_min_epi32(in00[1], max_val);
    in00[1] = _mm_max_epi32(in00[1], min_val);

    in00[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in00[2]), c32_rnd), shift), in00[2]);
    in00[2] = _mm_min_epi32(in00[2], max_val);
    in00[2] = _mm_max_epi32(in00[2], min_val);

    in00[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in00[3]), c32_rnd), shift), in00[3]);
    in00[3] = _mm_min_epi32(in00[3], max_val);
    in00[3] = _mm_max_epi32(in00[3], min_val);

    in01[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in01[0]), c32_rnd), shift), in01[0]);
    in01[0] = _mm_min_epi32(in01[0], max_val);
    in01[0] = _mm_max_epi32(in01[0], min_val);

    in01[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in01[1]), c32_rnd), shift), in01[1]);
    in01[1] = _mm_min_epi32(in01[1], max_val);
    in01[1] = _mm_max_epi32(in01[1], min_val);

    in01[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in01[2]), c32_rnd), shift), in01[2]);
    in01[2] = _mm_min_epi32(in01[2], max_val);
    in01[2] = _mm_max_epi32(in01[2], min_val);

    in01[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in01[3]), c32_rnd), shift), in01[3]);
    in01[3] = _mm_min_epi32(in01[3], max_val);
    in01[3] = _mm_max_epi32(in01[3], min_val);

    in02[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in02[0]), c32_rnd), shift), in02[0]);
    in02[0] = _mm_min_epi32(in02[0], max_val);
    in02[0] = _mm_max_epi32(in02[0], min_val);

    in02[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in02[1]), c32_rnd), shift), in02[1]);
    in02[1] = _mm_min_epi32(in02[1], max_val);
    in02[1] = _mm_max_epi32(in02[1], min_val);

    in02[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in02[2]), c32_rnd), shift), in02[2]);
    in02[2] = _mm_min_epi32(in02[2], max_val);
    in02[2] = _mm_max_epi32(in02[2], min_val);

    in02[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in02[3]), c32_rnd), shift), in02[3]);
    in02[3] = _mm_min_epi32(in02[3], max_val);
    in02[3] = _mm_max_epi32(in02[3], min_val);

    in03[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in03[0]), c32_rnd), shift), in03[0]);
    in03[0] = _mm_min_epi32(in03[0], max_val);
    in03[0] = _mm_max_epi32(in03[0], min_val);

    in03[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in03[1]), c32_rnd), shift), in03[1]);
    in03[1] = _mm_min_epi32(in03[1], max_val);
    in03[1] = _mm_max_epi32(in03[1], min_val);

    in03[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in03[2]), c32_rnd), shift), in03[2]);
    in03[2] = _mm_min_epi32(in03[2], max_val);
    in03[2] = _mm_max_epi32(in03[2], min_val);

    in03[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in03[3]), c32_rnd), shift), in03[3]);
    in03[3] = _mm_min_epi32(in03[3], max_val);
    in03[3] = _mm_max_epi32(in03[3], min_val);

    in04[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in04[0]), c32_rnd), shift), in04[0]);
    in04[0] = _mm_min_epi32(in04[0], max_val);
    in04[0] = _mm_max_epi32(in04[0], min_val);

    in04[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in04[1]), c32_rnd), shift), in04[1]);
    in04[1] = _mm_min_epi32(in04[1], max_val);
    in04[1] = _mm_max_epi32(in04[1], min_val);

    in04[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in04[2]), c32_rnd), shift), in04[2]);
    in04[2] = _mm_min_epi32(in04[2], max_val);
    in04[2] = _mm_max_epi32(in04[2], min_val);

    in04[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in04[3]), c32_rnd), shift), in04[3]);
    in04[3] = _mm_min_epi32(in04[3], max_val);
    in04[3] = _mm_max_epi32(in04[3], min_val);

    in05[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in05[0]), c32_rnd), shift), in05[0]);
    in05[0] = _mm_min_epi32(in05[0], max_val);
    in05[0] = _mm_max_epi32(in05[0], min_val);

    in05[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in05[1]), c32_rnd), shift), in05[1]);
    in05[1] = _mm_min_epi32(in05[1], max_val);
    in05[1] = _mm_max_epi32(in05[1], min_val);

    in05[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in05[2]), c32_rnd), shift), in05[2]);
    in05[2] = _mm_min_epi32(in05[2], max_val);
    in05[2] = _mm_max_epi32(in05[2], min_val);

    in05[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in05[3]), c32_rnd), shift), in05[3]);
    in05[3] = _mm_min_epi32(in05[3], max_val);
    in05[3] = _mm_max_epi32(in05[3], min_val);

    in06[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in06[0]), c32_rnd), shift), in06[0]);
    in06[0] = _mm_min_epi32(in06[0], max_val);
    in06[0] = _mm_max_epi32(in06[0], min_val);

    in06[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in06[1]), c32_rnd), shift), in06[1]);
    in06[1] = _mm_min_epi32(in06[1], max_val);
    in06[1] = _mm_max_epi32(in06[1], min_val);

    in06[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in06[2]), c32_rnd), shift), in06[2]);
    in06[2] = _mm_min_epi32(in06[2], max_val);
    in06[2] = _mm_max_epi32(in06[2], min_val);

    in06[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in06[3]), c32_rnd), shift), in06[3]);
    in06[3] = _mm_min_epi32(in06[3], max_val);
    in06[3] = _mm_max_epi32(in06[3], min_val);

    in07[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in07[0]), c32_rnd), shift), in07[0]);
    in07[0] = _mm_min_epi32(in07[0], max_val);
    in07[0] = _mm_max_epi32(in07[0], min_val);

    in07[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in07[1]), c32_rnd), shift), in07[1]);
    in07[1] = _mm_min_epi32(in07[1], max_val);
    in07[1] = _mm_max_epi32(in07[1], min_val);

    in07[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in07[2]), c32_rnd), shift), in07[2]);
    in07[2] = _mm_min_epi32(in07[2], max_val);
    in07[2] = _mm_max_epi32(in07[2], min_val);

    in07[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in07[3]), c32_rnd), shift), in07[3]);
    in07[3] = _mm_min_epi32(in07[3], max_val);
    in07[3] = _mm_max_epi32(in07[3], min_val);

    in08[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in08[0]), c32_rnd), shift), in08[0]);
    in08[0] = _mm_min_epi32(in08[0], max_val);
    in08[0] = _mm_max_epi32(in08[0], min_val);

    in08[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in08[1]), c32_rnd), shift), in08[1]);
    in08[1] = _mm_min_epi32(in08[1], max_val);
    in08[1] = _mm_max_epi32(in08[1], min_val);

    in08[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in08[2]), c32_rnd), shift), in08[2]);
    in08[2] = _mm_min_epi32(in08[2], max_val);
    in08[2] = _mm_max_epi32(in08[2], min_val);

    in08[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in08[3]), c32_rnd), shift), in08[3]);
    in08[3] = _mm_min_epi32(in08[3], max_val);
    in08[3] = _mm_max_epi32(in08[3], min_val);

    in09[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in09[0]), c32_rnd), shift), in09[0]);
    in09[0] = _mm_min_epi32(in09[0], max_val);
    in09[0] = _mm_max_epi32(in09[0], min_val);

    in09[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in09[1]), c32_rnd), shift), in09[1]);
    in09[1] = _mm_min_epi32(in09[1], max_val);
    in09[1] = _mm_max_epi32(in09[1], min_val);

    in09[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in09[2]), c32_rnd), shift), in09[2]);
    in09[2] = _mm_min_epi32(in09[2], max_val);
    in09[2] = _mm_max_epi32(in09[2], min_val);

    in09[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in09[3]), c32_rnd), shift), in09[3]);
    in09[3] = _mm_min_epi32(in09[3], max_val);
    in09[3] = _mm_max_epi32(in09[3], min_val);

    in10[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in10[0]), c32_rnd), shift), in10[0]);
    in10[0] = _mm_min_epi32(in10[0], max_val);
    in10[0] = _mm_max_epi32(in10[0], min_val);

    in10[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in10[1]), c32_rnd), shift), in10[1]);
    in10[1] = _mm_min_epi32(in10[1], max_val);
    in10[1] = _mm_max_epi32(in10[1], min_val);

    in10[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in10[2]), c32_rnd), shift), in10[2]);
    in10[2] = _mm_min_epi32(in10[2], max_val);
    in10[2] = _mm_max_epi32(in10[2], min_val);

    in10[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in10[3]), c32_rnd), shift), in10[3]);
    in10[3] = _mm_min_epi32(in10[3], max_val);
    in10[3] = _mm_max_epi32(in10[3], min_val);

    in11[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in11[0]), c32_rnd), shift), in11[0]);
    in11[0] = _mm_min_epi32(in11[0], max_val);
    in11[0] = _mm_max_epi32(in11[0], min_val);

    in11[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in11[1]), c32_rnd), shift), in11[1]);
    in11[1] = _mm_min_epi32(in11[1], max_val);
    in11[1] = _mm_max_epi32(in11[1], min_val);

    in11[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in11[2]), c32_rnd), shift), in11[2]);
    in11[2] = _mm_min_epi32(in11[2], max_val);
    in11[2] = _mm_max_epi32(in11[2], min_val);

    in11[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in11[3]), c32_rnd), shift), in11[3]);
    in11[3] = _mm_min_epi32(in11[3], max_val);
    in11[3] = _mm_max_epi32(in11[3], min_val);

    in12[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in12[0]), c32_rnd), shift), in12[0]);
    in12[0] = _mm_min_epi32(in12[0], max_val);
    in12[0] = _mm_max_epi32(in12[0], min_val);

    in12[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in12[1]), c32_rnd), shift), in12[1]);
    in12[1] = _mm_min_epi32(in12[1], max_val);
    in12[1] = _mm_max_epi32(in12[1], min_val);

    in12[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in12[2]), c32_rnd), shift), in12[2]);
    in12[2] = _mm_min_epi32(in12[2], max_val);
    in12[2] = _mm_max_epi32(in12[2], min_val);

    in12[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in12[3]), c32_rnd), shift), in12[3]);
    in12[3] = _mm_min_epi32(in12[3], max_val);
    in12[3] = _mm_max_epi32(in12[3], min_val);

    in13[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in13[0]), c32_rnd), shift), in13[0]);
    in13[0] = _mm_min_epi32(in13[0], max_val);
    in13[0] = _mm_max_epi32(in13[0], min_val);

    in13[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in13[1]), c32_rnd), shift), in13[1]);
    in13[1] = _mm_min_epi32(in13[1], max_val);
    in13[1] = _mm_max_epi32(in13[1], min_val);

    in13[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in13[2]), c32_rnd), shift), in13[2]);
    in13[2] = _mm_min_epi32(in13[2], max_val);
    in13[2] = _mm_max_epi32(in13[2], min_val);

    in13[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in13[3]), c32_rnd), shift), in13[3]);
    in13[3] = _mm_min_epi32(in13[3], max_val);
    in13[3] = _mm_max_epi32(in13[3], min_val);

    in14[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in14[0]), c32_rnd), shift), in14[0]);
    in14[0] = _mm_min_epi32(in14[0], max_val);
    in14[0] = _mm_max_epi32(in14[0], min_val);

    in14[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in14[1]), c32_rnd), shift), in14[1]);
    in14[1] = _mm_min_epi32(in14[1], max_val);
    in14[1] = _mm_max_epi32(in14[1], min_val);

    in14[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in14[2]), c32_rnd), shift), in14[2]);
    in14[2] = _mm_min_epi32(in14[2], max_val);
    in14[2] = _mm_max_epi32(in14[2], min_val);

    in14[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in14[3]), c32_rnd), shift), in14[3]);
    in14[3] = _mm_min_epi32(in14[3], max_val);
    in14[3] = _mm_max_epi32(in14[3], min_val);

    in15[0] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in15[0]), c32_rnd), shift), in15[0]);
    in15[0] = _mm_min_epi32(in15[0], max_val);
    in15[0] = _mm_max_epi32(in15[0], min_val);

    in15[1] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in15[1]), c32_rnd), shift), in15[1]);
    in15[1] = _mm_min_epi32(in15[1], max_val);
    in15[1] = _mm_max_epi32(in15[1], min_val);

    in15[2] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in15[2]), c32_rnd), shift), in15[2]);
    in15[2] = _mm_min_epi32(in15[2], max_val);
    in15[2] = _mm_max_epi32(in15[2], min_val);

    in15[3] = _mm_sign_epi32(_mm_srai_epi32(_mm_add_epi32(_mm_abs_epi32(in15[3]), c32_rnd), shift), in15[3]);
    in15[3] = _mm_min_epi32(in15[3], max_val);
    in15[3] = _mm_max_epi32(in15[3], min_val);

    // store the final data
    _mm_storeu_si128((__m128i*)&curr_blk[0 * 16 + 0], in00[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[0 * 16 + 4], in00[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[0 * 16 + 8], in00[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[0 * 16 + 12], in00[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[1 * 16 + 0], in01[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[1 * 16 + 4], in01[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[1 * 16 + 8], in01[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[1 * 16 + 12], in01[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[2 * 16 + 0], in02[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[2 * 16 + 4], in02[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[2 * 16 + 8], in02[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[2 * 16 + 12], in02[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[3 * 16 + 0], in03[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[3 * 16 + 4], in03[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[3 * 16 + 8], in03[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[3 * 16 + 12], in03[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[4 * 16 + 0], in04[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[4 * 16 + 4], in04[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[4 * 16 + 8], in04[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[4 * 16 + 12], in04[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[5 * 16 + 0], in05[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[5 * 16 + 4], in05[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[5 * 16 + 8], in05[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[5 * 16 + 12], in05[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[6 * 16 + 0], in06[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[6 * 16 + 4], in06[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[6 * 16 + 8], in06[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[6 * 16 + 12], in06[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[7 * 16 + 0], in07[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[7 * 16 + 4], in07[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[7 * 16 + 8], in07[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[7 * 16 + 12], in07[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[8 * 16 + 0], in08[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[8 * 16 + 4], in08[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[8 * 16 + 8], in08[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[8 * 16 + 12], in08[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[9 * 16 + 0], in09[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[9 * 16 + 4], in09[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[9 * 16 + 8], in09[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[9 * 16 + 12], in09[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[10 * 16 + 0], in10[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[10 * 16 + 4], in10[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[10 * 16 + 8], in10[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[10 * 16 + 12], in10[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[11 * 16 + 0], in11[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[11 * 16 + 4], in11[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[11 * 16 + 8], in11[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[11 * 16 + 12], in11[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[12 * 16 + 0], in12[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[12 * 16 + 4], in12[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[12 * 16 + 8], in12[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[12 * 16 + 12], in12[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[13 * 16 + 0], in13[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[13 * 16 + 4], in13[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[13 * 16 + 8], in13[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[13 * 16 + 12], in13[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[14 * 16 + 0], in14[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[14 * 16 + 4], in14[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[14 * 16 + 8], in14[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[14 * 16 + 12], in14[3]);

    _mm_storeu_si128((__m128i*)&curr_blk[15 * 16 + 0], in15[0]);
    _mm_storeu_si128((__m128i*)&curr_blk[15 * 16 + 4], in15[1]);
    _mm_storeu_si128((__m128i*)&curr_blk[15 * 16 + 8], in15[2]);
    _mm_storeu_si128((__m128i*)&curr_blk[15 * 16 + 12], in15[3]);

}
*/

void idct_16x16_sse128( int *curr_blk, int bsize )
{
    int i;
    int shift = 15;
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
    const __m128i max_val = _mm_set1_epi32( 255 );
    const __m128i min_val = _mm_set1_epi32( -255 );

    // DCT1
    __m128i in00[4], in01[4], in02[4], in03[4], in04[4], in05[4], in06[4], in07[4];
    __m128i in08[4], in09[4], in10[4], in11[4], in12[4], in13[4], in14[4], in15[4];
    __m128i res00[4], res01[4], res02[4], res03[4], res04[4], res05[4], res06[4], res07[4];
    __m128i res08[4], res09[4], res10[4], res11[4], res12[4], res13[4], res14[4], res15[4];

    for ( i = 0; i < 4; i++ )
    {
        const int offset = i * 4;

        in00[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[0 * 16 + offset] ); // change _mm_loadu_si128 to _mm_load_si128
        in01[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[1 * 16 + offset] );
        in02[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[2 * 16 + offset] );
        in03[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[3 * 16 + offset] );
        in04[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[4 * 16 + offset] );
        in05[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[5 * 16 + offset] );
        in06[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[6 * 16 + offset] );
        in07[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[7 * 16 + offset] );
        in08[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[8 * 16 + offset] );
        in09[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[9 * 16 + offset] );
        in10[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[10 * 16 + offset] );
        in11[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[11 * 16 + offset] );
        in12[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[12 * 16 + offset] );
        in13[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[13 * 16 + offset] );
        in14[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[14 * 16 + offset] );
        in15[i] = _mm_loadu_si128( ( const __m128i* )&curr_blk[15 * 16 + offset] );
    }

    for ( pass = 0; pass < 2; pass++ )
    {
        for ( part = 0; part < 4; part++ )
        {
            __m128i O0A, O1A, O2A, O3A, O4A, O5A, O6A, O7A;
            __m128i EO0A, EO1A, EO2A, EO3A;
            __m128i EEO0A, EEO1A;
            __m128i EEE0A, EEE1A;
            __m128i T00, T01, T02, T03;
            __m128i mTemp1, mTemp2;


            // R8 multiplication
#define COMPUTE_ROW8(row01, row03, row05, row07, row09, row11, row13, row15, c01, c03, c05, c07, c09, c11, c13, c15, row) \
    T00 = _mm_add_epi32(_mm_mullo_epi32(row01, c01), _mm_mullo_epi32(row03, c03)); \
    T01 = _mm_add_epi32(_mm_mullo_epi32(row05, c05), _mm_mullo_epi32(row07, c07)); \
    T02 = _mm_add_epi32(_mm_mullo_epi32(row09, c09), _mm_mullo_epi32(row11, c11)); \
    T03 = _mm_add_epi32(_mm_mullo_epi32(row13, c13), _mm_mullo_epi32(row15, c15)); \
    mTemp1 = _mm_add_epi32(T00, T01); \
    mTemp2 = _mm_add_epi32(T02, T03); \
    row = _mm_add_epi32(mTemp1, mTemp2);


            COMPUTE_ROW8( in01[part], in03[part], in05[part], in07[part], in09[part], in11[part], in13[part], in15[part], \
                          c32_p45, c32_p43, c32_p40, c32_p35, c32_p29, c32_p21, c32_p13, c32_p04, O0A )
            COMPUTE_ROW8( in01[part], in03[part], in05[part], in07[part], in09[part], in11[part], in13[part], in15[part], \
                          c32_p43, c32_p29, c32_p04, c32_n21, c32_n40, c32_n45, c32_n35, c32_n13, O1A )
            COMPUTE_ROW8( in01[part], in03[part], in05[part], in07[part], in09[part], in11[part], in13[part], in15[part], \
                          c32_p40, c32_p04, c32_n35, c32_n43, c32_n13, c32_p29, c32_p45, c32_p21, O2A )
            COMPUTE_ROW8( in01[part], in03[part], in05[part], in07[part], in09[part], in11[part], in13[part], in15[part], \
                          c32_p35, c32_n21, c32_n43, c32_p04, c32_p45, c32_p13, c32_n40, c32_n29, O3A )
            COMPUTE_ROW8( in01[part], in03[part], in05[part], in07[part], in09[part], in11[part], in13[part], in15[part], \
                          c32_p29, c32_n40, c32_n13, c32_p45, c32_n04, c32_n43, c32_p21, c32_p35, O4A )
            COMPUTE_ROW8( in01[part], in03[part], in05[part], in07[part], in09[part], in11[part], in13[part], in15[part], \
                          c32_p21, c32_n45, c32_p29, c32_p13, c32_n43, c32_p35, c32_p04, c32_n40, O5A )
            COMPUTE_ROW8( in01[part], in03[part], in05[part], in07[part], in09[part], in11[part], in13[part], in15[part], \
                          c32_p13, c32_n35, c32_p45, c32_n40, c32_p21, c32_p04, c32_n29, c32_p43, O6A )
            COMPUTE_ROW8( in01[part], in03[part], in05[part], in07[part], in09[part], in11[part], in13[part], in15[part], \
                          c32_p04, c32_n13, c32_p21, c32_n29, c32_p35, c32_n40, c32_p43, c32_n45, O7A )


#undef COMPUTE_ROW8

            // R4 multiplication
#define COMPUTE_ROW4(row02, row06, row10, row14, c02, c06, c10, c14, row) \
    T00 = _mm_add_epi32(_mm_mullo_epi32(row02, c02), _mm_mullo_epi32(row06, c06)); \
    T01 = _mm_add_epi32(_mm_mullo_epi32(row10, c10), _mm_mullo_epi32(row14, c14)); \
    row = _mm_add_epi32(T00, T01);

            COMPUTE_ROW4( in02[part], in06[part], in10[part], in14[part], c32_p44, c32_p38, c32_p25, c32_p09, EO0A ) // EO0
            COMPUTE_ROW4( in02[part], in06[part], in10[part], in14[part], c32_p38, c32_n09, c32_n44, c32_n25, EO1A )
            COMPUTE_ROW4( in02[part], in06[part], in10[part], in14[part], c32_p25, c32_n44, c32_p09, c32_p38, EO2A )
            COMPUTE_ROW4( in02[part], in06[part], in10[part], in14[part], c32_p09, c32_n25, c32_p38, c32_n44, EO3A )

#undef COMPUTE_ROW4

            // 4X4 FDCT
#define FDCT_MADD(row04, row12, c04, c12,row) \
    row = _mm_add_epi32(_mm_mullo_epi32(row04, c04), _mm_mullo_epi32(row12, c12));

            FDCT_MADD( in04[part], in12[part], c32_p42, c32_p17, EEO0A );
            FDCT_MADD( in04[part], in12[part], c32_p17, c32_n42, EEO1A );


            FDCT_MADD( in00[part], in08[part], c32_p32, c32_p32, EEE0A );
            FDCT_MADD( in00[part], in08[part], c32_p32, c32_n32, EEE1A );

#undef FDCT_MADD

            {
                const __m128i EE0A = _mm_add_epi32( EEE0A, EEO0A );        // EE0 = EEE0 + EEO0
                const __m128i EE1A = _mm_add_epi32( EEE1A, EEO1A );        // EE1 = EEE1 + EEO1
                const __m128i EE3A = _mm_sub_epi32( EEE0A, EEO0A );        // EE2 = EEE0 - EEO0
                const __m128i EE2A = _mm_sub_epi32( EEE1A, EEO1A );        // EE3 = EEE1 - EEO1

                const __m128i E0A = _mm_add_epi32( EE0A, EO0A );        // E0 = EE0 + EO0
                const __m128i E1A = _mm_add_epi32( EE1A, EO1A );        // E1 = EE1 + EO1
                const __m128i E2A = _mm_add_epi32( EE2A, EO2A );        // E2 = EE2 + EO2
                const __m128i E3A = _mm_add_epi32( EE3A, EO3A );        // E3 = EE3 + EO3
                const __m128i E7A = _mm_sub_epi32( EE0A, EO0A );        // E0 = EE0 - EO0
                const __m128i E6A = _mm_sub_epi32( EE1A, EO1A );        // E1 = EE1 - EO1
                const __m128i E5A = _mm_sub_epi32( EE2A, EO2A );        // E2 = EE2 - EO2
                const __m128i E4A = _mm_sub_epi32( EE3A, EO3A );        // E3 = EE3 - EO3

                res00[part] = _mm_add_epi32( E0A, O0A );        // E0 + O0 + rnd
                res01[part] = _mm_add_epi32( E1A, O1A );        // E1 + O1 + rnd
                res02[part] = _mm_add_epi32( E2A, O2A );        // E2 + O2 + rnd
                res03[part] = _mm_add_epi32( E3A, O3A );        // E3 + O3 + rnd
                res04[part] = _mm_add_epi32( E4A, O4A );        // E4
                res05[part] = _mm_add_epi32( E5A, O5A );        // E5
                res06[part] = _mm_add_epi32( E6A, O6A );        // E6
                res07[part] = _mm_add_epi32( E7A, O7A );        // E7

                res15[part] = _mm_sub_epi32( E0A, O0A );        // E0 - O0 + rnd
                res14[part] = _mm_sub_epi32( E1A, O1A );        // E1 - O1 + rnd
                res13[part] = _mm_sub_epi32( E2A, O2A );        // E2 - O2 + rnd
                res12[part] = _mm_sub_epi32( E3A, O3A );        // E3 - O3 + rnd
                res11[part] = _mm_sub_epi32( E4A, O4A );        // E4
                res10[part] = _mm_sub_epi32( E5A, O5A );        // E5
                res09[part] = _mm_sub_epi32( E6A, O6A );        // E6
                res08[part] = _mm_sub_epi32( E7A, O7A );        // E7

            }
        }

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
    }

    //  data compression
    in00[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in00[0] ), c32_rnd ), shift ), in00[0] );
    in00[0] = _mm_min_epi32( in00[0], max_val );
    in00[0] = _mm_max_epi32( in00[0], min_val );

    in00[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in00[1] ), c32_rnd ), shift ), in00[1] );
    in00[1] = _mm_min_epi32( in00[1], max_val );
    in00[1] = _mm_max_epi32( in00[1], min_val );

    in00[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in00[2] ), c32_rnd ), shift ), in00[2] );
    in00[2] = _mm_min_epi32( in00[2], max_val );
    in00[2] = _mm_max_epi32( in00[2], min_val );

    in00[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in00[3] ), c32_rnd ), shift ), in00[3] );
    in00[3] = _mm_min_epi32( in00[3], max_val );
    in00[3] = _mm_max_epi32( in00[3], min_val );

    in01[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in01[0] ), c32_rnd ), shift ), in01[0] );
    in01[0] = _mm_min_epi32( in01[0], max_val );
    in01[0] = _mm_max_epi32( in01[0], min_val );

    in01[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in01[1] ), c32_rnd ), shift ), in01[1] );
    in01[1] = _mm_min_epi32( in01[1], max_val );
    in01[1] = _mm_max_epi32( in01[1], min_val );

    in01[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in01[2] ), c32_rnd ), shift ), in01[2] );
    in01[2] = _mm_min_epi32( in01[2], max_val );
    in01[2] = _mm_max_epi32( in01[2], min_val );

    in01[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in01[3] ), c32_rnd ), shift ), in01[3] );
    in01[3] = _mm_min_epi32( in01[3], max_val );
    in01[3] = _mm_max_epi32( in01[3], min_val );

    in02[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in02[0] ), c32_rnd ), shift ), in02[0] );
    in02[0] = _mm_min_epi32( in02[0], max_val );
    in02[0] = _mm_max_epi32( in02[0], min_val );

    in02[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in02[1] ), c32_rnd ), shift ), in02[1] );
    in02[1] = _mm_min_epi32( in02[1], max_val );
    in02[1] = _mm_max_epi32( in02[1], min_val );

    in02[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in02[2] ), c32_rnd ), shift ), in02[2] );
    in02[2] = _mm_min_epi32( in02[2], max_val );
    in02[2] = _mm_max_epi32( in02[2], min_val );

    in02[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in02[3] ), c32_rnd ), shift ), in02[3] );
    in02[3] = _mm_min_epi32( in02[3], max_val );
    in02[3] = _mm_max_epi32( in02[3], min_val );

    in03[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in03[0] ), c32_rnd ), shift ), in03[0] );
    in03[0] = _mm_min_epi32( in03[0], max_val );
    in03[0] = _mm_max_epi32( in03[0], min_val );

    in03[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in03[1] ), c32_rnd ), shift ), in03[1] );
    in03[1] = _mm_min_epi32( in03[1], max_val );
    in03[1] = _mm_max_epi32( in03[1], min_val );

    in03[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in03[2] ), c32_rnd ), shift ), in03[2] );
    in03[2] = _mm_min_epi32( in03[2], max_val );
    in03[2] = _mm_max_epi32( in03[2], min_val );

    in03[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in03[3] ), c32_rnd ), shift ), in03[3] );
    in03[3] = _mm_min_epi32( in03[3], max_val );
    in03[3] = _mm_max_epi32( in03[3], min_val );

    in04[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in04[0] ), c32_rnd ), shift ), in04[0] );
    in04[0] = _mm_min_epi32( in04[0], max_val );
    in04[0] = _mm_max_epi32( in04[0], min_val );

    in04[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in04[1] ), c32_rnd ), shift ), in04[1] );
    in04[1] = _mm_min_epi32( in04[1], max_val );
    in04[1] = _mm_max_epi32( in04[1], min_val );

    in04[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in04[2] ), c32_rnd ), shift ), in04[2] );
    in04[2] = _mm_min_epi32( in04[2], max_val );
    in04[2] = _mm_max_epi32( in04[2], min_val );

    in04[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in04[3] ), c32_rnd ), shift ), in04[3] );
    in04[3] = _mm_min_epi32( in04[3], max_val );
    in04[3] = _mm_max_epi32( in04[3], min_val );

    in05[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in05[0] ), c32_rnd ), shift ), in05[0] );
    in05[0] = _mm_min_epi32( in05[0], max_val );
    in05[0] = _mm_max_epi32( in05[0], min_val );

    in05[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in05[1] ), c32_rnd ), shift ), in05[1] );
    in05[1] = _mm_min_epi32( in05[1], max_val );
    in05[1] = _mm_max_epi32( in05[1], min_val );

    in05[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in05[2] ), c32_rnd ), shift ), in05[2] );
    in05[2] = _mm_min_epi32( in05[2], max_val );
    in05[2] = _mm_max_epi32( in05[2], min_val );

    in05[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in05[3] ), c32_rnd ), shift ), in05[3] );
    in05[3] = _mm_min_epi32( in05[3], max_val );
    in05[3] = _mm_max_epi32( in05[3], min_val );

    in06[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in06[0] ), c32_rnd ), shift ), in06[0] );
    in06[0] = _mm_min_epi32( in06[0], max_val );
    in06[0] = _mm_max_epi32( in06[0], min_val );

    in06[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in06[1] ), c32_rnd ), shift ), in06[1] );
    in06[1] = _mm_min_epi32( in06[1], max_val );
    in06[1] = _mm_max_epi32( in06[1], min_val );

    in06[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in06[2] ), c32_rnd ), shift ), in06[2] );
    in06[2] = _mm_min_epi32( in06[2], max_val );
    in06[2] = _mm_max_epi32( in06[2], min_val );

    in06[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in06[3] ), c32_rnd ), shift ), in06[3] );
    in06[3] = _mm_min_epi32( in06[3], max_val );
    in06[3] = _mm_max_epi32( in06[3], min_val );

    in07[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in07[0] ), c32_rnd ), shift ), in07[0] );
    in07[0] = _mm_min_epi32( in07[0], max_val );
    in07[0] = _mm_max_epi32( in07[0], min_val );

    in07[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in07[1] ), c32_rnd ), shift ), in07[1] );
    in07[1] = _mm_min_epi32( in07[1], max_val );
    in07[1] = _mm_max_epi32( in07[1], min_val );

    in07[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in07[2] ), c32_rnd ), shift ), in07[2] );
    in07[2] = _mm_min_epi32( in07[2], max_val );
    in07[2] = _mm_max_epi32( in07[2], min_val );

    in07[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in07[3] ), c32_rnd ), shift ), in07[3] );
    in07[3] = _mm_min_epi32( in07[3], max_val );
    in07[3] = _mm_max_epi32( in07[3], min_val );

    in08[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in08[0] ), c32_rnd ), shift ), in08[0] );
    in08[0] = _mm_min_epi32( in08[0], max_val );
    in08[0] = _mm_max_epi32( in08[0], min_val );

    in08[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in08[1] ), c32_rnd ), shift ), in08[1] );
    in08[1] = _mm_min_epi32( in08[1], max_val );
    in08[1] = _mm_max_epi32( in08[1], min_val );

    in08[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in08[2] ), c32_rnd ), shift ), in08[2] );
    in08[2] = _mm_min_epi32( in08[2], max_val );
    in08[2] = _mm_max_epi32( in08[2], min_val );

    in08[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in08[3] ), c32_rnd ), shift ), in08[3] );
    in08[3] = _mm_min_epi32( in08[3], max_val );
    in08[3] = _mm_max_epi32( in08[3], min_val );

    in09[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in09[0] ), c32_rnd ), shift ), in09[0] );
    in09[0] = _mm_min_epi32( in09[0], max_val );
    in09[0] = _mm_max_epi32( in09[0], min_val );

    in09[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in09[1] ), c32_rnd ), shift ), in09[1] );
    in09[1] = _mm_min_epi32( in09[1], max_val );
    in09[1] = _mm_max_epi32( in09[1], min_val );

    in09[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in09[2] ), c32_rnd ), shift ), in09[2] );
    in09[2] = _mm_min_epi32( in09[2], max_val );
    in09[2] = _mm_max_epi32( in09[2], min_val );

    in09[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in09[3] ), c32_rnd ), shift ), in09[3] );
    in09[3] = _mm_min_epi32( in09[3], max_val );
    in09[3] = _mm_max_epi32( in09[3], min_val );

    in10[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in10[0] ), c32_rnd ), shift ), in10[0] );
    in10[0] = _mm_min_epi32( in10[0], max_val );
    in10[0] = _mm_max_epi32( in10[0], min_val );

    in10[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in10[1] ), c32_rnd ), shift ), in10[1] );
    in10[1] = _mm_min_epi32( in10[1], max_val );
    in10[1] = _mm_max_epi32( in10[1], min_val );

    in10[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in10[2] ), c32_rnd ), shift ), in10[2] );
    in10[2] = _mm_min_epi32( in10[2], max_val );
    in10[2] = _mm_max_epi32( in10[2], min_val );

    in10[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in10[3] ), c32_rnd ), shift ), in10[3] );
    in10[3] = _mm_min_epi32( in10[3], max_val );
    in10[3] = _mm_max_epi32( in10[3], min_val );

    in11[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in11[0] ), c32_rnd ), shift ), in11[0] );
    in11[0] = _mm_min_epi32( in11[0], max_val );
    in11[0] = _mm_max_epi32( in11[0], min_val );

    in11[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in11[1] ), c32_rnd ), shift ), in11[1] );
    in11[1] = _mm_min_epi32( in11[1], max_val );
    in11[1] = _mm_max_epi32( in11[1], min_val );

    in11[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in11[2] ), c32_rnd ), shift ), in11[2] );
    in11[2] = _mm_min_epi32( in11[2], max_val );
    in11[2] = _mm_max_epi32( in11[2], min_val );

    in11[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in11[3] ), c32_rnd ), shift ), in11[3] );
    in11[3] = _mm_min_epi32( in11[3], max_val );
    in11[3] = _mm_max_epi32( in11[3], min_val );

    in12[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in12[0] ), c32_rnd ), shift ), in12[0] );
    in12[0] = _mm_min_epi32( in12[0], max_val );
    in12[0] = _mm_max_epi32( in12[0], min_val );

    in12[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in12[1] ), c32_rnd ), shift ), in12[1] );
    in12[1] = _mm_min_epi32( in12[1], max_val );
    in12[1] = _mm_max_epi32( in12[1], min_val );

    in12[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in12[2] ), c32_rnd ), shift ), in12[2] );
    in12[2] = _mm_min_epi32( in12[2], max_val );
    in12[2] = _mm_max_epi32( in12[2], min_val );

    in12[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in12[3] ), c32_rnd ), shift ), in12[3] );
    in12[3] = _mm_min_epi32( in12[3], max_val );
    in12[3] = _mm_max_epi32( in12[3], min_val );

    in13[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in13[0] ), c32_rnd ), shift ), in13[0] );
    in13[0] = _mm_min_epi32( in13[0], max_val );
    in13[0] = _mm_max_epi32( in13[0], min_val );

    in13[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in13[1] ), c32_rnd ), shift ), in13[1] );
    in13[1] = _mm_min_epi32( in13[1], max_val );
    in13[1] = _mm_max_epi32( in13[1], min_val );

    in13[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in13[2] ), c32_rnd ), shift ), in13[2] );
    in13[2] = _mm_min_epi32( in13[2], max_val );
    in13[2] = _mm_max_epi32( in13[2], min_val );

    in13[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in13[3] ), c32_rnd ), shift ), in13[3] );
    in13[3] = _mm_min_epi32( in13[3], max_val );
    in13[3] = _mm_max_epi32( in13[3], min_val );

    in14[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in14[0] ), c32_rnd ), shift ), in14[0] );
    in14[0] = _mm_min_epi32( in14[0], max_val );
    in14[0] = _mm_max_epi32( in14[0], min_val );

    in14[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in14[1] ), c32_rnd ), shift ), in14[1] );
    in14[1] = _mm_min_epi32( in14[1], max_val );
    in14[1] = _mm_max_epi32( in14[1], min_val );

    in14[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in14[2] ), c32_rnd ), shift ), in14[2] );
    in14[2] = _mm_min_epi32( in14[2], max_val );
    in14[2] = _mm_max_epi32( in14[2], min_val );

    in14[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in14[3] ), c32_rnd ), shift ), in14[3] );
    in14[3] = _mm_min_epi32( in14[3], max_val );
    in14[3] = _mm_max_epi32( in14[3], min_val );

    in15[0] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in15[0] ), c32_rnd ), shift ), in15[0] );
    in15[0] = _mm_min_epi32( in15[0], max_val );
    in15[0] = _mm_max_epi32( in15[0], min_val );

    in15[1] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in15[1] ), c32_rnd ), shift ), in15[1] );
    in15[1] = _mm_min_epi32( in15[1], max_val );
    in15[1] = _mm_max_epi32( in15[1], min_val );

    in15[2] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in15[2] ), c32_rnd ), shift ), in15[2] );
    in15[2] = _mm_min_epi32( in15[2], max_val );
    in15[2] = _mm_max_epi32( in15[2], min_val );

    in15[3] = _mm_sign_epi32( _mm_srai_epi32( _mm_add_epi32( _mm_abs_epi32( in15[3] ), c32_rnd ), shift ), in15[3] );
    in15[3] = _mm_min_epi32( in15[3], max_val );
    in15[3] = _mm_max_epi32( in15[3], min_val );

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