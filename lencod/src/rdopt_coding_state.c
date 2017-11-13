/*
*************************************************************************************
* File name: rdopt_coding_state.c
* Function:
Storing/restoring coding state for
Rate-Distortion optimized mode decision
*
*************************************************************************************
*/

#include <math.h>
#include <memory.h>
#include "AEC.h"

void delete_coding_state ( CSptr cs )
{
    if ( cs != NULL )
    {
        if ( cs->bitstream != NULL )
        {
            free ( cs->bitstream );
        }

        delete_contexts_MotionInfo ( cs->mot_ctx );
        delete_contexts_TextureInfo ( cs->tex_ctx );

        //=== coding state structure ===
        free ( cs );
        cs=NULL;
    }
}
//#endif

/*
*************************************************************************
* Function:create structure for storing coding state
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

CSptr create_coding_state ()
{
    CSptr cs;

    //=== coding state structure ===
    if ( ( cs = ( CSptr )calloc( 1, sizeof( CSobj ) ) ) == NULL )
    {
        no_mem_exit( "init_coding_state: cs" );
    }

    // important variables of data partition array

    if ( ( cs->bitstream = ( Bitstream* )calloc( 1, sizeof( Bitstream ) ) ) == NULL )
    {
        no_mem_exit( "init_coding_state: cs->bitstream" );
    }

    cs->mot_ctx = create_contexts_MotionInfo();
    cs->tex_ctx = create_contexts_TextureInfo();

    return cs;
}


void copy_coding_state( CSobj *cs_dst, CSobj *cs_src )
{
    memcpy( &( cs_dst->ee_AEC ), &( cs_src->ee_AEC ), sizeof( Env_AEC ) );
    memcpy( cs_dst->bitstream, cs_src->bitstream, sizeof( Bitstream ) );

    memcpy( cs_dst->mot_ctx, cs_src->mot_ctx, sizeof( MotionInfoContexts ) );
    memcpy( cs_dst->tex_ctx, cs_src->tex_ctx, sizeof( TextureInfoContexts ) );

    //=== syntax element number and bitcounters ===
    memcpy( cs_dst->bitcounter, cs_src->bitcounter, MAX_BITCOUNTER_MB*sizeof( int ) );
}