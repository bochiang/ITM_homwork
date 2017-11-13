#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_

typedef struct
{
    char *TokenName;
    void *Place;
    int Type;
} Mapping;

InpParams configinput;

#ifdef INCLUDED_BY_CONFIGFILE_C

Mapping Map[] =
{
    {"ProfileID",                &configinput.profile_id,              0},
    {"LevelID",                  &configinput.level_id,                0},
    {"IntraPeriod",              &configinput.intra_period,            0},
    {"VECPeriod",                &configinput.vec_period,              0},
    {"SeqHeaderPeriod",          &configinput.seqheader_period,        0},
    {"FramesToBeEncoded",        &configinput.FrmsToBeEncoded,         0},
    {"QPFirstFrame",             &configinput.qp_1st_frm,              0},
    {"QPRemainingFrame",         &configinput.qp_P_frm,                0},
    {"UseHadamard",              &configinput.hadamard,                0},
    {"FME",                      &configinput.usefme,                  0},
    {"SearchRange",              &configinput.search_range,            0},
    {"NumberReferenceFrames",    &configinput.inp_ref_num,             0},
    {"SourceWidth",              &configinput.width_org,               0},
    {"SourceHeight",             &configinput.height_org,              0},
    {"InputFile",                &configinput.infile,                  1},
    {"InputHeaderLength",        &configinput.infile_header,           0},
    {"OutputFile",               &configinput.outfile,                 1},
    {"ReconFile",                &configinput.ReconFile,               1},
    {"NumberBFrames",            &configinput.successive_Bframe,       0},
    {"QPBPicture",               &configinput.qp_B_frm,                0},
    {"InterSearch8x8",           &configinput.InterSearch8x8 ,         0},
    {"InterSearch16x8",          &configinput.InterSearch16x8 ,        0},
    {"InterSearch8x16",          &configinput.InterSearch8x16,         0},
    {"NumberOfRowsInSlice",      &configinput.slice_row_nr,            0},

    {"SliceParameter",           &configinput.slice_parameter,         0},
    {"FrameRate",                &configinput.frame_rate_code,         0},
    {"ChromaFormat",             &configinput.chroma_format,           0},

    // Rate Control on IVC Standard
    {"RateControlEnable",        &configinput.RCEnable,                0},
    {"Bitrate",                  &configinput.bit_rate,                0},
    {"InitialQP",                &configinput.SeinitialQP,             0},
    {"BasicUnit",                &configinput.basicunit,               0},
    {"ChannelType",              &configinput.channel_type,            0},

    {"BBS_size",                 &configinput.BBS_size,                0},
    {"BbvMode",                  &configinput.bbv_mode,                0},

#ifdef TDRDO
    {"TRDOLength",               &configinput.TRDOLength,              0},
#endif

    {"PSubType",                 &configinput.use_p_sub_type,          0},
    {"PSubQPDelta0",             &configinput.p_sub_type_delta0,       0},
    {"PSubQPDelta1",             &configinput.p_sub_type_delta1,       0},
    {"PSubType_NonAdaptive",     &configinput.p_sub_non_type_coding,   0},
    {"RDO_Q",                    &configinput.rdo_q_flag,              0},
    {"MultipleHP",               &configinput.multiple_hp_flag,        0},
    {"IF_TYPE",                  &configinput.if_type,                 0},
    {"LoopFilterDisable",        &configinput.deblk_disable,           0},
    {"Chroma_Enhancement",       &configinput.chroma_enhance,          0},
    {"OutPutEncPic",             &configinput.output_enc_pic,          0},
    {NULL,                       NULL,                                -1}
};

#endif

void Configure ( int ac, char *av[] );
void CheckToolsInProfile();
void DecideMvRange();

#endif

