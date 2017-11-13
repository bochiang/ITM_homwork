#include <math.h>
#include "global.h"
#include "AEC.h"
#include "biariencode.h"
#include <assert.h>
#include "../../common/common.h"


//AC ENGINE PARAMETERS
i32u_t s1,t1,s2,t2;

/*************************************************************************
* Macro for writing bytes of code
************************************************************************/

void put_byte( Env_AEC *eep )
{
    //if(writeflag1)
    //  fprintf(ptest, "%d : %d\n",*eep->Ecodestrm_len,eep->Ebuffer);
    eep->Ecodestrm[( *eep->Ecodestrm_len )++] = eep->Ebuffer;
    eep->Ebits_to_go = 8;
    while ( eep->C > 7 )
    {
        eep->C-=8;
        eep->E++;
    }
}

void put_one_bit( Env_AEC *eep, int b )
{
    eep->Ebuffer <<= 1;
    eep->Ebuffer |= ( b );
    if ( --eep->Ebits_to_go == 0 )
    {
        put_byte( eep );
    }
}

void put_one_bit_plus_outstanding( Env_AEC *eep, int b )
{
    put_one_bit( eep, b );
    while ( eep->Ebits_to_follow > 0 )
    {
        eep->Ebits_to_follow--;
        put_one_bit( eep, !( b ) );
    }
}

/*************************************************************************
* \brief
*    Initializes the Env_AEC for the arithmetic coder
*************************************************************************/
void arienco_start_encoding( Env_AEC* eep, pel_t *code_buffer, int *code_len )
{
    eep->Ecodestrm = code_buffer;
    eep->Ecodestrm_len = code_len;
    eep->Elow = 0;
    eep->E_s1 = 0;
    eep->E_t1 = 0xFF;
    eep->Ebits_to_follow  =0 ;
    eep->Ebuffer = 0;
    eep->Ebits_to_go = 9;// to swallow first redundant bit
    s2=0;
    t2=0xff;
    eep->C = 0;
    eep->B = *code_len;
    eep->E = 0;
}

/*!
************************************************************************
* \brief
*    Returns the number of currently written bits
************************************************************************
*/
int arienco_bits_written( Env_AEC* eep )
{
    return ( 8*( *eep->Ecodestrm_len ) + eep->Ebits_to_follow + 8 - eep->Ebits_to_go + eep->E_s1 );
}


/*!
************************************************************************
* \brief
*    Terminates the arithmetic codeword, writes stop bit and stuffing bytes (if any)
************************************************************************
*/

void arienco_done_encoding( Env_AEC* eep )
{
    int i;
    put_one_bit_plus_outstanding( eep, ( eep->Elow >> ( B_BITS-1 ) ) & 1 ); //write eep->Ebits_to_follow+1 bits
    put_one_bit( eep, ( eep->Elow >> ( B_BITS-2 ) )&1 );
    put_one_bit( eep, 1 );
    for( i=0; i<8; i++ )
    {
        put_one_bit( eep, 0 );
    }

    stat->bit_use_stuffingBits[img->type]+=( 8-eep->Ebits_to_go );
    eep->E= eep->E*8 + eep->C; // no of processed bins
    eep->B= ( *eep->Ecodestrm_len - eep->B ); // no of written bytes
    eep->E -= ( img->current_mb_nr-img->currentSlice->start_mb_nr );
    eep->E = ( eep->E + 31 )>>5;
    // eep->E now contains the minimum number of bytes for the NAL unit
}


/*************************************************************************
* \brief
*    Actually arithmetic encoding of one binary symbol by using
*    the probability estimate of its associated context model
*************************************************************************/
void biari_encode_symbol( Env_AEC* eep, uchar_t symbol, BiContextTypePtr bi_ct ) //type 0 normal,1 equal,2 final
{
    uchar_t cycno = bi_ct->cycno;
    i32u_t low = eep->Elow;
    uchar_t cwr=0;

    i32u_t lg_pmps = bi_ct->LG_PMPS;
    i32u_t t_rLPS=0;
    uchar_t s_flag=0,is_LPS=0;

    i32u_t  curr_byte=0;
    short int bitstogo=0;
    uchar_t bit_o=0,bit_oa=0,byte_no=0;
    i32u_t low_byte[3]= {0};
    static int bits_counter=0;

    bits_counter++;
    s1 = eep->E_s1;
    t1 = eep->E_t1;

    low_byte[0]=0;
    low_byte[1]=0;
    low_byte[2]=0;

    assert( eep!=NULL );

    cwr = ( cycno<=1 )?3:( cycno==2 )?4:5;

    if ( symbol != 0 )
    {
        symbol = 1;
    }

    if ( t1>=( lg_pmps>>LG_PMPS_SHIFTNO ) )
    {
        s2 = s1;
        t2 = t1 - ( lg_pmps>>LG_PMPS_SHIFTNO );
        s_flag = 0;
    }
    else
    {
        s2 = s1+1;
        t2 = 256 + t1 - ( lg_pmps>>LG_PMPS_SHIFTNO );
        s_flag = 1;
    }

    if ( symbol==bi_ct->MPS ) //MPS happens
    {

        if ( cycno==0 )
        {
            cycno =1;
        }
        s1 = s2;
        t1 = t2;

        //no updating of interval range and low here or
        //renorm to guarantee s1<8, after renorm, s1 --;
        if ( s1 == 8 ) //renorm
        {
            //left shift 1 bit

            bit_o = ( low>>9 ) &1;

            {
                bit_oa = ( low>>8 ) &1;

                if ( bit_o )
                {
                    put_one_bit_plus_outstanding( eep, 1 );

                }
                else
                {
                    if ( !bit_oa ) //00
                    {
                        put_one_bit_plus_outstanding( eep, 0 );
                    }
                    else  //01
                    {
                        eep->Ebits_to_follow++;
                        bit_oa = 0;
                    }
                }
                s1--;
            }

            //restore low
            low = ( ( bit_oa<<8 ) | ( low & 0xff ) )<<1;
        }
    }
    else //--LPS
    {
        is_LPS = 1;
        cycno=( cycno<=2 )?( cycno+1 ):3;

        if ( s_flag==0 )
        {
            t_rLPS = lg_pmps>>LG_PMPS_SHIFTNO;    //t_rLPS -- 9bits
        }
        else //s2=s1 + 1
        {
            t_rLPS = t1 + ( lg_pmps>>LG_PMPS_SHIFTNO );    //t_rLPS<HALF
        }

        low_byte[0] = low + ( ( t2+256 )>>s2 ); //first low_byte: 10bits
        low_byte[1] = ( t2<<( 8-s2 ) ) & 0xff;

        //restore range
        while ( t_rLPS<QUARTER )
        {
            t_rLPS = t_rLPS<<1;
            s2 ++;
        }

        //left shift s2 bits
        {
            curr_byte = low_byte[0];
            bitstogo = 9;
            bit_oa = ( curr_byte>>bitstogo ) &1;
            byte_no = 0;

            while ( s2>0 )
            {
                bit_o = bit_oa;
                bitstogo--;
                if ( bitstogo<0 )
                {
                    curr_byte = low_byte[++byte_no];
                    bitstogo = 7;
                }
                bit_oa = ( curr_byte>>bitstogo ) &1;

                if ( bit_o )
                {
                    put_one_bit_plus_outstanding( eep, 1 );
                }
                else
                {
                    if ( !bit_oa ) //00
                    {
                        put_one_bit_plus_outstanding( eep, 0 );
                    }
                    else  //01
                    {
                        eep->Ebits_to_follow++;
                        bit_oa = 0;
                    }
                }
                s2--;
            }

            //restore low
            low = bit_oa;
            s2 = 9;
            while ( s2>0 )
            {
                bitstogo--;
                if ( bitstogo<0 )
                {
                    curr_byte = low_byte[++byte_no];
                    bitstogo = 7;
                }
                bit_oa = ( curr_byte>>bitstogo ) &1;
                low = ( low<<1 ) | bit_oa;
                s2--;
            }
        }

        s1 = 0;
        t1 = t_rLPS & 0xff;
    }

    //updating other parameters
    bi_ct->cycno = cycno;
    eep->Elow = low;
    eep->E_s1 = s1;
    eep->E_t1 = t1;
    eep->C++;

    //update probability estimation
    if ( is_LPS )
    {
        switch( cwr )
        {
            case 3:
                lg_pmps = lg_pmps + 197;
                break;
            case 4:
                lg_pmps = lg_pmps + 95;
                break;
            default:
                lg_pmps = lg_pmps + 46;
        }

        if ( lg_pmps>=( 256<<LG_PMPS_SHIFTNO ) )
        {
            lg_pmps = ( 512<<LG_PMPS_SHIFTNO ) - 1 - lg_pmps;
            bi_ct->MPS = !( bi_ct->MPS );
        }
    }
    else
    {
        lg_pmps = lg_pmps - ( i32u_t )( lg_pmps>>cwr ) - ( i32u_t )( lg_pmps>>( cwr+2 ) );
    }
    bi_ct->LG_PMPS = lg_pmps;


}



void biari_encode_symbolW( Env_AEC* eep, uchar_t symbol, BiContextTypePtr bi_ct1, BiContextTypePtr bi_ct2 )
{

    i32u_t low = eep->Elow;
    i32u_t cwr1=0,cycno1=bi_ct1->cycno;
    i32u_t cwr2=0,cycno2=bi_ct2->cycno;
    i32u_t lg_pmps1= bi_ct1->LG_PMPS,lg_pmps2= bi_ct2->LG_PMPS;
    uchar_t bit1=bi_ct1->MPS,bit2=bi_ct2->MPS;

    i32u_t t_rLPS=0;
    uchar_t s_flag=0,is_LPS=0;

    uchar_t pred_MPS=0;
    i32u_t  lg_pmps=0;

    i32u_t low_byte[3]= {0};

    i32u_t  curr_byte=0;
    short int bitstogo=0;
    uchar_t bit_o=0,bit_oa=0,byte_no=0;

    low_byte[0]=0;
    low_byte[1]=0;
    low_byte[2]=0;
    s1 = eep->E_s1;
    t1 = eep->E_t1;

    assert( eep!=NULL );
    cwr1 = ( cycno1<=1 )?3:( cycno1==2 )?4:5;
    cwr2 = ( cycno2<=1 )?3:( cycno2==2 )?4:5;

    if ( symbol != 0 )
    {
        symbol = 1;
    }

    if ( bit1 == bit2 )
    {
        pred_MPS = bit1;
        lg_pmps = ( lg_pmps1 + lg_pmps2 )/2;
    }
    else
    {
        if ( lg_pmps1<lg_pmps2 )
        {
            pred_MPS = bit1;
            lg_pmps = ( 256<<LG_PMPS_SHIFTNO ) - 1 - ( ( lg_pmps2 - lg_pmps1 )>>1 );
        }
        else
        {
            pred_MPS = bit2;
            lg_pmps = ( 256<<LG_PMPS_SHIFTNO ) - 1 - ( ( lg_pmps1 - lg_pmps2 )>>1 );
        }
    }

    if ( t1>=( lg_pmps>>LG_PMPS_SHIFTNO ) )
    {
        s2 = s1;
        t2 = t1 - ( lg_pmps>>LG_PMPS_SHIFTNO );
        s_flag = 0;
    }
    else
    {
        s2 = s1+1;
        t2 = 256 + t1 - ( lg_pmps>>LG_PMPS_SHIFTNO );
        s_flag = 1;
    }

    if ( symbol==pred_MPS ) //MPS happens
    {
        s1 = s2;
        t1 = t2;

        //no updating of interval range and low here or
        //renorm to guarantee s1<8, after renorm, s1 --;
        if ( s1 == 8 ) //renorm
        {
            //left shift 1 bit
            bit_o = ( low>>9 ) &1;


            {
                bit_oa = ( low>>8 ) &1;

                if ( bit_o )
                {
                    put_one_bit_plus_outstanding( eep, 1 );
                }
                else
                {
                    if ( !bit_oa ) //00
                    {
                        put_one_bit_plus_outstanding( eep, 0 );
                    }
                    else  //01
                    {
                        eep->Ebits_to_follow++;
                        bit_oa = 0;
                    }
                }
                s1--;
            }

            //restore low
            low = ( ( bit_oa<<8 ) | ( low & 0xff ) )<<1;
        }
    }
    else
    {
        is_LPS = 1;

        if ( s_flag==0 )
        {
            t_rLPS    = ( lg_pmps>>LG_PMPS_SHIFTNO );
        }
        else //s2=s1 + 1
        {
            t_rLPS = t1 + ( lg_pmps>>LG_PMPS_SHIFTNO );
        }

        low_byte[0] = low + ( ( t2+256 )>>s2 ); //first low_byte: 10bits
        low_byte[1] = ( t2<<( 8-s2 ) ) & 0xff;

        //restore range
        while ( t_rLPS<QUARTER )
        {
            t_rLPS = t_rLPS<<1;
            s2 ++;
        }
        //left shift s2 bits
        {
            curr_byte = low_byte[0];
            bitstogo = 9;
            bit_oa = ( curr_byte>>bitstogo ) &0x01;
            byte_no = 0;

            while ( s2>0 )
            {
                bit_o = bit_oa;
                bitstogo--;
                if ( bitstogo<0 )
                {
                    curr_byte = low_byte[++byte_no];
                    bitstogo = 7;
                }
                bit_oa = ( curr_byte>>bitstogo ) &0x01;

                if ( bit_o )
                {
                    put_one_bit_plus_outstanding( eep, 1 );
                }
                else
                {
                    if ( 1==bit_oa ) //01
                    {
                        eep->Ebits_to_follow++;
                        bit_oa = 0;
                    }
                    else //00
                    {
                        put_one_bit_plus_outstanding( eep, 0 );
                    }

                }
                s2--;
            }

            //restore low
            low = bit_oa;
            s2 = 9;
            while ( s2>0 )
            {
                bitstogo--;
                if ( bitstogo<0 )
                {
                    curr_byte = low_byte[++byte_no];
                    bitstogo = 7;
                }
                bit_oa = ( curr_byte>>bitstogo ) &1;
                low = ( low<<1 ) | bit_oa;
                s2--;
            }
        }


        s1 = 0;
        t1 = t_rLPS & 0xff;
    }


    if ( symbol !=bit1 )
    {
        cycno1 = ( cycno1<=2 )?( cycno1+1 ):3; //LPS occurs
    }
    else
    {
        if ( cycno1==0 )
        {
            cycno1 =1;
        }
    }

    if ( symbol !=bit2 )
    {
        cycno2 = ( cycno2<=2 )?( cycno2+1 ):3; //LPS occurs
    }
    else
    {
        if ( cycno2==0 )
        {
            cycno2 =1;
        }
    }
    bi_ct1->cycno = ( uchar_t )cycno1;
    bi_ct2->cycno = ( uchar_t )cycno2;
    eep->Elow = low;
    eep->E_s1 = s1;
    eep->E_t1 = t1;
    eep->C++;

    //update probability estimation
    {
        //bi_ct1
        if ( symbol==bit1 )
        {
            lg_pmps1 = lg_pmps1 - ( i32u_t )( lg_pmps1>>cwr1 ) - ( i32u_t )( lg_pmps1>>( cwr1+2 ) );
        }
        else
        {
            switch( cwr1 )
            {
                case 3:
                    lg_pmps1 = lg_pmps1 + 197;
                    break;
                case 4:
                    lg_pmps1 = lg_pmps1 + 95;
                    break;
                default:
                    lg_pmps1 = lg_pmps1 + 46;
            }

            if ( lg_pmps1>=( 256<<LG_PMPS_SHIFTNO ) )
            {
                lg_pmps1 = ( 512<<LG_PMPS_SHIFTNO ) - 1 - lg_pmps1;
                bi_ct1->MPS = !( bi_ct1->MPS );
            }
        }
        bi_ct1->LG_PMPS = lg_pmps1;

        //bi_ct2
        if ( symbol==bit2 )
        {
            lg_pmps2 = lg_pmps2 - ( i32u_t )( lg_pmps2>>cwr2 ) - ( i32u_t )( lg_pmps2>>( cwr2+2 ) );
        }
        else
        {
            switch( cwr2 )
            {
                case 3:
                    lg_pmps2 = lg_pmps2 + 197;
                    break;
                case 4:
                    lg_pmps2 = lg_pmps2 + 95;
                    break;
                default:
                    lg_pmps2 = lg_pmps2 + 46;
            }

            if ( lg_pmps2>=( 256<<LG_PMPS_SHIFTNO ) )
            {
                lg_pmps2 = ( 512<<LG_PMPS_SHIFTNO ) - 1 - lg_pmps2;
                bi_ct2->MPS = !( bi_ct2->MPS );
            }
        }
        bi_ct2->LG_PMPS = lg_pmps2;
    }


}



void biari_encode_symbol_eq_prob( Env_AEC* eep, uchar_t  symbol )
{
    BiContextType octx;
    BiContextTypePtr ctx=&octx;
    ctx->LG_PMPS = ( QUARTER<<LG_PMPS_SHIFTNO )-1;
    ctx->MPS = 0;
    ctx->cycno =0;
    biari_encode_symbol( eep,symbol,ctx );
}


void biari_encode_symbol_final( Env_AEC* eep, uchar_t symbol )
{
    BiContextType octx;
    BiContextTypePtr ctx=&octx;
    ctx->LG_PMPS = 1<<LG_PMPS_SHIFTNO;
    ctx->MPS = 0;
    ctx->cycno =0;
    biari_encode_symbol( eep,symbol,ctx );
}