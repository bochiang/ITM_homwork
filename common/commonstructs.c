#include "global.h"
#include "memalloc.h"

Bitstream *AllocateBitstream()
{
    Bitstream *bitstream;

    bitstream = ( Bitstream * ) calloc( 1, sizeof( Bitstream ) );
    if ( bitstream == NULL )
    {
        fprintf( stderr, "AllocBitstream: Memory allocation for Bitstream failed" );
        exit(1);
    }
    bitstream->streamBuffer = ( uchar_t * ) calloc( MAX_CODED_FRAME_SIZE, sizeof( uchar_t ) );
    if ( bitstream->streamBuffer == NULL )
    {
        fprintf(stderr, "AllocBitstream: Memory allocation for streamBuffer failed");
        exit(1);
    }
    bitstream->bits_to_go = 8;
    return bitstream;
}


void FreeBitstream( Bitstream *bitstr )
{
    if ( bitstr->streamBuffer )
    {
        free( bitstr->streamBuffer );
    }
    if ( bitstr )
    {
        free( bitstr );
    }
}

MotionInfoContexts* create_contexts_MotionInfo( void )
{
    MotionInfoContexts* enco_ctx;

    enco_ctx = ( MotionInfoContexts* ) calloc( 1, sizeof( MotionInfoContexts ) );
    if( enco_ctx == NULL )
    {
        no_mem_exit( "create_contexts_MotionInfo: enco_ctx" );
    }

    return enco_ctx;
}

TextureInfoContexts* create_contexts_TextureInfo( void )
{
    TextureInfoContexts*  enco_ctx;

    enco_ctx = ( TextureInfoContexts* ) calloc( 1, sizeof( TextureInfoContexts ) );
    if( enco_ctx == NULL )
    {
        no_mem_exit( "create_contexts_TextureInfo: enco_ctx" );
    }

    return enco_ctx;
}

void delete_contexts_MotionInfo( MotionInfoContexts *enco_ctx )
{
    if( enco_ctx == NULL )
    {
        return;
    }
    free( enco_ctx );
    return;
}

void delete_contexts_TextureInfo( TextureInfoContexts *enco_ctx )
{
    if( enco_ctx == NULL )
    {
        return;
    }
    free( enco_ctx );
    return;
}