/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

extern "C" {
#include <m_pd.h>
#include <z_hooks.h>
#include <s_net.h>
}

#include <clocale>
#include <string>
#include <cstring>
#include "Setup.h"

static t_class* plugdata_receiver_class;

typedef struct _plugdata_receiver {
    t_object x_obj;
    t_symbol* x_sym;
    void* x_ptr;

    t_plugdata_banghook x_hook_bang;
    t_plugdata_floathook x_hook_float;
    t_plugdata_symbolhook x_hook_symbol;
    t_plugdata_listhook x_hook_list;
    t_plugdata_messagehook x_hook_message;
} t_plugdata_receiver;

static void plugdata_receiver_bang(t_plugdata_receiver* x)
{
    if (x->x_hook_bang)
        x->x_hook_bang(x->x_ptr, x->x_sym->s_name);
}

static void plugdata_receiver_float(t_plugdata_receiver* x, t_float const f)
{
    if (x->x_hook_float)
        x->x_hook_float(x->x_ptr, x->x_sym->s_name, f);
}

static void plugdata_receiver_symbol(t_plugdata_receiver* x, t_symbol* s)
{
    if (x->x_hook_symbol)
        x->x_hook_symbol(x->x_ptr, x->x_sym->s_name, s->s_name);
}

static void plugdata_receiver_list(t_plugdata_receiver* x, t_symbol*, int const argc, t_atom* argv)
{
    if (x->x_hook_list)
        x->x_hook_list(x->x_ptr, x->x_sym->s_name, argc, argv);
}

static void plugdata_receiver_anything(t_plugdata_receiver* x, t_symbol* s, int const argc, t_atom* argv)
{
    if (x->x_hook_message)
        x->x_hook_message(x->x_ptr, x->x_sym->s_name, s->s_name, argc, argv);
}

static void plugdata_receiver_free(t_plugdata_receiver* x)
{
    pd_unbind(&x->x_obj.ob_pd, x->x_sym);
}

static t_class* plugdata_midi_class;

typedef struct _plugdata_midi {
    t_object x_obj;
    void* x_ptr;

    t_plugdata_noteonhook x_hook_noteon;
    t_plugdata_controlchangehook x_hook_controlchange;
    t_plugdata_programchangehook x_hook_programchange;
    t_plugdata_pitchbendhook x_hook_pitchbend;
    t_plugdata_aftertouchhook x_hook_aftertouch;
    t_plugdata_polyaftertouchhook x_hook_polyaftertouch;
    t_plugdata_midibytehook x_hook_midibyte;
} t_plugdata_midi;

static void plugdata_noteon(int const channel, int const pitch, int const velocity)
{
    auto const* x = reinterpret_cast<t_plugdata_midi*>(gensym("#plugdata_midi")->s_thing);
    if (x && x->x_hook_noteon) {
        x->x_hook_noteon(x->x_ptr, channel, pitch, velocity);
    }
}

static void plugdata_controlchange(int const channel, int const controller, int const value)
{
    auto const* x = reinterpret_cast<t_plugdata_midi*>(gensym("#plugdata_midi")->s_thing);
    if (x && x->x_hook_controlchange) {
        x->x_hook_controlchange(x->x_ptr, channel, controller, value);
    }
}

static void plugdata_programchange(int const channel, int const value)
{
    auto const* x = reinterpret_cast<t_plugdata_midi*>(gensym("#plugdata_midi")->s_thing);
    if (x && x->x_hook_programchange) {
        x->x_hook_programchange(x->x_ptr, channel, value);
    }
}

static void plugdata_pitchbend(int const channel, int const value)
{
    auto const* x = reinterpret_cast<t_plugdata_midi*>(gensym("#plugdata_midi")->s_thing);
    if (x && x->x_hook_pitchbend) {
        x->x_hook_pitchbend(x->x_ptr, channel, value);
    }
}

static void plugdata_aftertouch(int const channel, int const value)
{
    auto const* x = reinterpret_cast<t_plugdata_midi*>(gensym("#plugdata_midi")->s_thing);
    if (x && x->x_hook_aftertouch) {
        x->x_hook_aftertouch(x->x_ptr, channel, value);
    }
}

static void plugdata_polyaftertouch(int const channel, int const pitch, int const value)
{
    auto const* x = reinterpret_cast<t_plugdata_midi*>(gensym("#plugdata_midi")->s_thing);
    if (x && x->x_hook_polyaftertouch) {
        x->x_hook_polyaftertouch(x->x_ptr, channel, pitch, value);
    }
}

static void plugdata_midibyte(int const port, int const byte)
{
    auto const* x = reinterpret_cast<t_plugdata_midi*>(gensym("#plugdata_midi")->s_thing);
    if (x && x->x_hook_midibyte) {
        x->x_hook_midibyte(x->x_ptr, port, byte);
    }
}

static void plugdata_midi_free(t_plugdata_midi* x)
{
    pd_unbind(&x->x_obj.ob_pd, gensym("#plugdata_midi"));
}

static t_class* plugdata_print_class;

typedef struct _plugdata_print {
    t_object x_obj;
    void* x_ptr;
    t_plugdata_printhook x_hook;
} t_plugdata_print;

static void plugdata_print(void* object, char const* message)
{
    auto const* x = reinterpret_cast<t_plugdata_print*>(gensym("#plugdata_print")->s_thing);
    if (x && x->x_hook) {
        x->x_hook(x->x_ptr, object, message);
    }
}

extern "C" {

void pd_init();

#if ENABLE_GEM
void Gem_setup(t_symbol* plugin_path);
void gemcubeframebuffer_setup();
void gemframebuffer_setup();
void gemhead_setup();
void gemkeyboard_setup();
void gemkeyname_setup();
void gemlist_setup();
void gemlist_info_setup();
void gemlist_matrix_setup();
void gemmanager_setup();
void gemmouse_setup();
void gemreceive_setup();
void gemwin_setup();
void modelfiler_setup();
void render_trigger_setup();
void GemSplash_setup();
void circle_setup();
void colorSquare_setup();
void cone_setup();
void cube_setup();
void cuboid_setup();
void curve_setup();
void curve3d_setup();
void cylinder_setup();
void disk_setup();
void gemvertexbuffer_setup();
void imageVert_setup();
void mesh_line_setup();
void mesh_square_setup();
void model_setup();
void multimodel_setup();
void newWave_setup();
void polygon_setup();
void pqtorusknots_setup();
void primTri_setup();
void rectangle_setup();
void ripple_setup();
void rubber_setup();
void scopeXYZ_setup();
void slideSquares_setup();
void sphere_setup();
void sphere3d_setup();
void square_setup();
void surface3d_setup();
void teapot_setup();
void text2d_setup();
void text3d_setup();
void textextruded_setup();
void textoutline_setup();
void torus_setup();
void trapezoid_setup();
void triangle_setup();
void tube_setup();
void accumrotate_setup();
void alpha_setup();
void ambient_setup();
void ambientRGB_setup();
void camera_setup();
void color_setup();
void colorRGB_setup();
void depth_setup();
void diffuse_setup();
void diffuseRGB_setup();
void emission_setup();
void emissionRGB_setup();
void fragment_program_setup();
void glsl_fragment_setup();
void glsl_geometry_setup();
void glsl_program_setup();
void glsl_tesscontrol_setup();
void glsl_tesseval_setup();
void glsl_vertex_setup();
void linear_path_setup();
void ortho_setup();
void polygon_smooth_setup();
void rotate_setup();
void rotateXYZ_setup();
void scale_setup();
void gemrepeat_setup();
void scaleXYZ_setup();
void separator_setup();
void shearXY_setup();
void shearXZ_setup();
void shearYX_setup();
void shearYZ_setup();
void shearZX_setup();
void shearZY_setup();
void shininess_setup();
void specular_setup();
void specularRGB_setup();
void spline_path_setup();
void translate_setup();
void translateXYZ_setup();
void vertex_program_setup();
void light_setup();
void spot_light_setup();
void world_light_setup();
void part_color_setup();
void part_damp_setup();
void part_draw_setup();
void part_follow_setup();
void part_gravity_setup();
void part_head_setup();
void part_killold_setup();
void part_killslow_setup();
void part_orbitpoint_setup();
void part_render_setup();
void part_sink_setup();
void part_size_setup();
void part_source_setup();
void part_targetcolor_setup();
void part_targetsize_setup();
void part_velcone_setup();
void part_velocity_setup();
void part_velsphere_setup();
void part_vertex_setup();
void pix_2grey_setup();
void pix_a_2grey_setup();
void pix_add_setup();
void pix_aging_setup();
void pix_alpha_setup();
void pix_background_setup();
void pix_backlight_setup();
void pix_biquad_setup();
void pix_bitmask_setup();
void pix_blob_setup();
void pix_blur_setup();
void pix_buf_setup();
void pix_buffer_setup();
void pix_buffer_read_setup();
void pix_buffer_write_setup();
void pix_chroma_key_setup();
void pix_clearblock_setup();
void pix_color_setup();
void pix_coloralpha_setup();
void pix_colorclassify_setup();
void pix_colormatrix_setup();
void pix_colorreduce_setup();
void pix_compare_setup();
void pix_composite_setup();
void pix_contrast_setup();
void pix_convert_setup();
void pix_convolve_setup();
void pix_coordinate_setup();
void pix_crop_setup();
void pix_cubemap_setup();
void pix_curve_setup();
void pix_data_setup();
void pix_deinterlace_setup();
void pix_delay_setup();
void pix_diff_setup();
void pix_dot_setup();
void pix_draw_setup();
void pix_dump_setup();
void pix_duotone_setup();
void pix_emboss_setup();
void pix_equal_setup();
void pix_film_setup();
void pix_flip_setup();
void pix_freeframe_setup();
void pix_frei0r_setup();
void pix_gain_setup();
void pix_grey_setup();
void pix_halftone_setup();
void pix_histo_setup();
void pix_hsv2rgb_setup();
void pix_image_setup();
void pix_imageInPlace_setup();
void pix_info_setup();
void pix_invert_setup();
void pix_kaleidoscope_setup();
void pix_levels_setup();
void pix_lumaoffset_setup();
void pix_mask_setup();
void pix_mean_color_setup();
void pix_metaimage_setup();
void pix_mix_setup();
void pix_motionblur_setup();
void pix_movement_setup();
void pix_movement2_setup();
void pix_movie_setup();
void pix_multiblob_setup();
void pix_multiimage_setup();
void pix_multiply_setup();
void pix_multitexture_setup();
void pix_noise_setup();
void pix_normalize_setup();
void pix_offset_setup();
void pix_posterize_setup();
void pix_puzzle_setup();
void pix_rds_setup();
void pix_record_setup();
void pix_rectangle_setup();
void pix_refraction_setup();
void pix_resize_setup();
void pix_rgb2hsv_setup();
void pix_rgba_setup();
void pix_roi_setup();
void pix_roll_setup();
void pix_rtx_setup();
void pix_scanline_setup();
void pix_set_setup();
void pix_share_read_setup();
void pix_share_write_setup();
void pix_snap_setup();
void pix_snap2tex_setup();
void pix_subtract_setup();
void pix_sig2pix_setup();
void pix_pix2sig_setup();
void pix_tIIR_setup();
void pix_tIIRf_setup();
void pix_takealpha_setup();
void pix_test_setup();
void pix_texture_setup();
void pix_threshold_setup();
void pix_threshold_bernsen_setup();
void pix_video_setup();
void pix_vpaint_setup();
void pix_write_setup();
void pix_writer_setup();
void pix_yuv_setup();
void pix_zoom_setup();
void vertex_add_setup();
void vertex_combine_setup();
void vertex_draw_setup();
void vertex_grid_setup();
void vertex_info_setup();
// void vertex_model_setup();
void vertex_mul_setup();
void vertex_offset_setup();
void vertex_quad_setup();
void vertex_scale_setup();
void vertex_set_setup();
void vertex_tabread_setup();
void GEMglAccum_setup();
void GEMglActiveTexture_setup();
void GEMglActiveTextureARB_setup();
void GEMglAlphaFunc_setup();
void GEMglAreTexturesResident_setup();
void GEMglArrayElement_setup();
void GEMglBegin_setup();
void GEMglBindProgramARB_setup();
void GEMglBindTexture_setup();
void GEMglBitmap_setup();
void GEMglBlendEquation_setup();
void GEMglBlendFunc_setup();
void GEMglCallList_setup();
void GEMglClear_setup();
void GEMglClearAccum_setup();
void GEMglClearColor_setup();
void GEMglClearDepth_setup();
void GEMglClearIndex_setup();
void GEMglClearStencil_setup();
void GEMglClipPlane_setup();
void GEMglColor3b_setup();
void GEMglColor3bv_setup();
void GEMglColor3d_setup();
void GEMglColor3dv_setup();
void GEMglColor3f_setup();
void GEMglColor3fv_setup();
void GEMglColor3i_setup();
void GEMglColor3iv_setup();
void GEMglColor3s_setup();
void GEMglColor3sv_setup();
void GEMglColor3ub_setup();
void GEMglColor3ubv_setup();
void GEMglColor3ui_setup();
void GEMglColor3uiv_setup();
void GEMglColor3us_setup();
void GEMglColor3usv_setup();
void GEMglColor4b_setup();
void GEMglColor4bv_setup();
void GEMglColor4d_setup();
void GEMglColor4dv_setup();
void GEMglColor4f_setup();
void GEMglColor4fv_setup();
void GEMglColor4i_setup();
void GEMglColor4iv_setup();
void GEMglColor4s_setup();
void GEMglColor4sv_setup();
void GEMglColor4ub_setup();
void GEMglColor4ubv_setup();
void GEMglColor4ui_setup();
void GEMglColor4uiv_setup();
void GEMglColor4us_setup();
void GEMglColor4usv_setup();
void GEMglColorMask_setup();
void GEMglColorMaterial_setup();
void GEMglCopyPixels_setup();
void GEMglCopyTexImage1D_setup();
void GEMglCopyTexImage2D_setup();
void GEMglCopyTexSubImage1D_setup();
void GEMglCopyTexSubImage2D_setup();
void GEMglCullFace_setup();
void GEMglDeleteTextures_setup();
void GEMglDepthFunc_setup();
void GEMglDepthMask_setup();
void GEMglDepthRange_setup();
void GEMglDisable_setup();
void GEMglDisableClientState_setup();
void GEMglDrawArrays_setup();
void GEMglDrawBuffer_setup();
void GEMglDrawElements_setup();
void GEMglEdgeFlag_setup();
void GEMglEnable_setup();
void GEMglEnableClientState_setup();
void GEMglEnd_setup();
void GEMglEndList_setup();
void GEMglEvalCoord1d_setup();
void GEMglEvalCoord1dv_setup();
void GEMglEvalCoord1f_setup();
void GEMglEvalCoord1fv_setup();
void GEMglEvalCoord2d_setup();
void GEMglEvalCoord2dv_setup();
void GEMglEvalCoord2f_setup();
void GEMglEvalCoord2fv_setup();
void GEMglEvalMesh1_setup();
void GEMglEvalMesh2_setup();
void GEMglEvalPoint1_setup();
void GEMglEvalPoint2_setup();
void GEMglFeedbackBuffer_setup();
void GEMglFinish_setup();
void GEMglFlush_setup();
void GEMglFogf_setup();
void GEMglFogfv_setup();
void GEMglFogi_setup();
void GEMglFogiv_setup();
void GEMglFrontFace_setup();
void GEMglFrustum_setup();
void GEMglGenLists_setup();
void GEMglGenProgramsARB_setup();
void GEMglGenTextures_setup();
void GEMglGenerateMipmap_setup();
void GEMglGetError_setup();
void GEMglGetFloatv_setup();
void GEMglGetIntegerv_setup();
void GEMglGetMapdv_setup();
void GEMglGetMapfv_setup();
void GEMglGetMapiv_setup();
void GEMglGetPointerv_setup();
void GEMglGetString_setup();
void GEMglHint_setup();
void GEMglIndexMask_setup();
void GEMglIndexd_setup();
void GEMglIndexdv_setup();
void GEMglIndexf_setup();
void GEMglIndexfv_setup();
void GEMglIndexi_setup();
void GEMglIndexiv_setup();
void GEMglIndexs_setup();
void GEMglIndexsv_setup();
void GEMglIndexub_setup();
void GEMglIndexubv_setup();
void GEMglInitNames_setup();
void GEMglIsEnabled_setup();
void GEMglIsList_setup();
void GEMglIsTexture_setup();
void GEMglLightModelf_setup();
void GEMglLightModeli_setup();
void GEMglLightf_setup();
void GEMglLighti_setup();
void GEMglLineStipple_setup();
void GEMglLineWidth_setup();
void GEMglLoadIdentity_setup();
void GEMglLoadMatrixd_setup();
void GEMglLoadMatrixf_setup();
void GEMglLoadName_setup();
void GEMglLoadTransposeMatrixd_setup();
void GEMglLoadTransposeMatrixf_setup();
void GEMglLogicOp_setup();
void GEMglMap1d_setup();
void GEMglMap1f_setup();
void GEMglMap2d_setup();
void GEMglMap2f_setup();
void GEMglMapGrid1d_setup();
void GEMglMapGrid1f_setup();
void GEMglMapGrid2d_setup();
void GEMglMapGrid2f_setup();
void GEMglMaterialf_setup();
void GEMglMaterialfv_setup();
void GEMglMateriali_setup();
void GEMglMatrixMode_setup();
void GEMglMultMatrixd_setup();
void GEMglMultMatrixf_setup();
void GEMglMultTransposeMatrixd_setup();
void GEMglMultTransposeMatrixf_setup();
void GEMglMultiTexCoord2f_setup();
void GEMglMultiTexCoord2fARB_setup();
void GEMglNewList_setup();
void GEMglNormal3b_setup();
void GEMglNormal3bv_setup();
void GEMglNormal3d_setup();
void GEMglNormal3dv_setup();
void GEMglNormal3f_setup();
void GEMglNormal3fv_setup();
void GEMglNormal3i_setup();
void GEMglNormal3iv_setup();
void GEMglNormal3s_setup();
void GEMglNormal3sv_setup();
void GEMglOrtho_setup();
void GEMglPassThrough_setup();
void GEMglPixelStoref_setup();
void GEMglPixelStorei_setup();
void GEMglPixelTransferf_setup();
void GEMglPixelTransferi_setup();
void GEMglPixelZoom_setup();
void GEMglPointSize_setup();
void GEMglPolygonMode_setup();
void GEMglPolygonOffset_setup();
void GEMglPopAttrib_setup();
void GEMglPopClientAttrib_setup();
void GEMglPopMatrix_setup();
void GEMglPopName_setup();
void GEMglPrioritizeTextures_setup();
void GEMglProgramEnvParameter4dARB_setup();
void GEMglProgramEnvParameter4fvARB_setup();
void GEMglProgramLocalParameter4fvARB_setup();
void GEMglProgramStringARB_setup();
void GEMglPushAttrib_setup();
void GEMglPushClientAttrib_setup();
void GEMglPushMatrix_setup();
void GEMglPushName_setup();
void GEMglRasterPos2d_setup();
void GEMglRasterPos2dv_setup();
void GEMglRasterPos2f_setup();
void GEMglRasterPos2fv_setup();
void GEMglRasterPos2i_setup();
void GEMglRasterPos2iv_setup();
void GEMglRasterPos2s_setup();
void GEMglRasterPos2sv_setup();
void GEMglRasterPos3d_setup();
void GEMglRasterPos3dv_setup();
void GEMglRasterPos3f_setup();
void GEMglRasterPos3fv_setup();
void GEMglRasterPos3i_setup();
void GEMglRasterPos3iv_setup();
void GEMglRasterPos3s_setup();
void GEMglRasterPos3sv_setup();
void GEMglRasterPos4d_setup();
void GEMglRasterPos4dv_setup();
void GEMglRasterPos4f_setup();
void GEMglRasterPos4fv_setup();
void GEMglRasterPos4i_setup();
void GEMglRasterPos4iv_setup();
void GEMglRasterPos4s_setup();
void GEMglRasterPos4sv_setup();
void GEMglRectd_setup();
void GEMglRectf_setup();
void GEMglRecti_setup();
void GEMglRects_setup();
void GEMglRenderMode_setup();
void GEMglReportError_setup();
void GEMglRotated_setup();
void GEMglRotatef_setup();
void GEMglScaled_setup();
void GEMglScalef_setup();
void GEMglScissor_setup();
void GEMglSelectBuffer_setup();
void GEMglShadeModel_setup();
void GEMglStencilFunc_setup();
void GEMglStencilMask_setup();
void GEMglStencilOp_setup();
void GEMglTexCoord1d_setup();
void GEMglTexCoord1dv_setup();
void GEMglTexCoord1f_setup();
void GEMglTexCoord1fv_setup();
void GEMglTexCoord1i_setup();
void GEMglTexCoord1iv_setup();
void GEMglTexCoord1s_setup();
void GEMglTexCoord1sv_setup();
void GEMglTexCoord2d_setup();
void GEMglTexCoord2dv_setup();
void GEMglTexCoord2f_setup();
void GEMglTexCoord2fv_setup();
void GEMglTexCoord2i_setup();
void GEMglTexCoord2iv_setup();
void GEMglTexCoord2s_setup();
void GEMglTexCoord2sv_setup();
void GEMglTexCoord3d_setup();
void GEMglTexCoord3dv_setup();
void GEMglTexCoord3f_setup();
void GEMglTexCoord3fv_setup();
void GEMglTexCoord3i_setup();
void GEMglTexCoord3iv_setup();
void GEMglTexCoord3s_setup();
void GEMglTexCoord3sv_setup();
void GEMglTexCoord4d_setup();
void GEMglTexCoord4dv_setup();
void GEMglTexCoord4f_setup();
void GEMglTexCoord4fv_setup();
void GEMglTexCoord4i_setup();
void GEMglTexCoord4iv_setup();
void GEMglTexCoord4s_setup();
void GEMglTexCoord4sv_setup();
void GEMglTexEnvf_setup();
void GEMglTexEnvi_setup();
void GEMglTexGend_setup();
void GEMglTexGenf_setup();
void GEMglTexGenfv_setup();
void GEMglTexGeni_setup();
void GEMglTexImage2D_setup();
void GEMglTexParameterf_setup();
void GEMglTexParameteri_setup();
void GEMglTexSubImage1D_setup();
void GEMglTexSubImage2D_setup();
void GEMglTranslated_setup();
void GEMglTranslatef_setup();
void GEMglUniform1f_setup();
void GEMglUniform1fARB_setup();
void GEMglUseProgramObjectARB_setup();
void GEMglVertex2d_setup();
void GEMglVertex2dv_setup();
void GEMglVertex2f_setup();
void GEMglVertex2fv_setup();
void GEMglVertex2i_setup();
void GEMglVertex2iv_setup();
void GEMglVertex2s_setup();
void GEMglVertex2sv_setup();
void GEMglVertex3d_setup();
void GEMglVertex3dv_setup();
void GEMglVertex3f_setup();
void GEMglVertex3fv_setup();
void GEMglVertex3i_setup();
void GEMglVertex3iv_setup();
void GEMglVertex3s_setup();
void GEMglVertex3sv_setup();
void GEMglVertex4d_setup();
void GEMglVertex4dv_setup();
void GEMglVertex4f_setup();
void GEMglVertex4fv_setup();
void GEMglVertex4i_setup();
void GEMglVertex4iv_setup();
void GEMglVertex4s_setup();
void GEMglVertex4sv_setup();
void GEMglViewport_setup();
void GEMgluLookAt_setup();
void GEMgluPerspective_setup();
void GLdefine_setup();

void setup_modelOBJ();
void setup_modelASSIMP3();
void setup_imageSTBLoader();
void setup_imageSTBSaver();
void setup_recordPNM();

#    if ENABLE_FFMPEG
void setup_filmFFMPEG();
#    endif

#    if __APPLE__
void setup_videoAVF();
void setup_filmAVF();
#    elif _MSC_VER
void setup_videoVFW();
void setup_filmDS();
#    else
// void setup_videoV4L2();
// void setup_recordV4L2();
#    endif
#endif

// pd-extra objects functions declaration
void bob_tilde_setup();
void bonk_tilde_setup();
void choice_setup();
void fiddle_tilde_setup();
void loop_tilde_setup();
void lrshift_tilde_setup();
void pd_tilde_setup();
void pique_setup();
void sigmund_tilde_setup();
void stdout_setup();

// cyclone objects functions declaration
void cyclone_setup();
void accum_setup();
void acos_setup();
void acosh_setup();
void active_setup();
void anal_setup();
void append_setup();
void asin_setup();
void asinh_setup();
void atanh_setup();
void atodb_setup();
void bangbang_setup();
void bondo_setup();
void borax_setup();
void bucket_setup();
void buddy_setup();
void capture_setup();
void cartopol_setup();
void clip_setup();
void coll_setup();
void cosh_setup();
void counter_setup();
void cycle_setup();
void dbtoa_setup();
void decide_setup();
void decode_setup();
void drunk_setup();
void flush_setup();
void forward_setup();
void fromsymbol_setup();
void funnel_setup();
void funbuff_setup();
void gate_setup();
void grab_setup();
void histo_setup();
void iter_setup();
void join_setup();
void linedrive_setup();
void listfunnel_setup();
void loadmess_setup();
void match_setup();
void maximum_setup();
void mean_setup();
void midiflush_setup();
void midiformat_setup();
void midiparse_setup();
void minimum_setup();
void mousefilter_setup();
void mousestate_setup();
void mtr_setup();
void next_setup();
void offer_setup();
void onebang_setup();
void pak_setup();
void past_setup();
void peak_setup();
void poltocar_setup();
void pong_setup();
void prepend_setup();
void prob_setup();
void cyclone_pink_tilde_setup();
void pv_setup();
void rdiv_setup();
void rminus_setup();
void round_setup();
void cyclone_scale_setup();
void seq_setup();
void sinh_setup();
void speedlim_setup();
void spell_setup();
void split_setup();
void spray_setup();
void sprintf_setup();
void substitute_setup();
void sustain_setup();
void switch_setup();
void table_setup();
void tanh_setup();
void thresh_setup();
void togedge_setup();
void tosymbol_setup();
void trough_setup();
void cyclone_trunc_tilde_setup();
void universal_setup();
void unjoin_setup();
void urn_setup();
void uzi_setup();
void xbendin_setup();
void xbendin2_setup();
void xbendout_setup();
void xbendout2_setup();
void xnotein_setup();
void xnoteout_setup();
void zl_setup();
void setup_zl0x2eecils();
void setup_zl0x2egroup();
void setup_zl0x2eiter();
void setup_zl0x2ejoin();
void setup_zl0x2elen();
void setup_zl0x2emth();
void setup_zl0x2enth();
void setup_zl0x2ereg();
void setup_zl0x2erev();
void setup_zl0x2erot();
void setup_zl0x2esect();
void setup_zl0x2eslice();
void setup_zl0x2esort();
void setup_zl0x2esub();
void setup_zl0x2eunion();
void setup_zl0x2echange();
void setup_zl0x2ecompare();
void setup_zl0x2edelace();
void setup_zl0x2efilter();
void setup_zl0x2elace();
void setup_zl0x2elookup();
void setup_zl0x2emedian();
void setup_zl0x2equeue();
void setup_zl0x2escramble();
void setup_zl0x2estack();
void setup_zl0x2estream();
void setup_zl0x2esum();
void setup_zl0x2ethin();
void setup_zl0x2eunique();
void setup_zl0x2eindexmap();
void setup_zl0x2eswap();

void acos_tilde_setup();
void acosh_tilde_setup();
void allpass_tilde_setup();
void asin_tilde_setup();
void asinh_tilde_setup();
void atan_tilde_setup();
void atan2_tilde_setup();
void atanh_tilde_setup();
void atodb_tilde_setup();
void average_tilde_setup();
void avg_tilde_setup();
void bitand_tilde_setup();
void bitnot_tilde_setup();
void bitor_tilde_setup();
void bitsafe_tilde_setup();
void bitshift_tilde_setup();
void bitxor_tilde_setup();
void buffir_tilde_setup();
void capture_tilde_setup();
void cartopol_tilde_setup();
void change_tilde_setup();
void click_tilde_setup();
void clip_tilde_setup();
void comb_tilde_setup();
void cosh_tilde_setup();
void cosx_tilde_setup();
void count_tilde_setup();
void cross_tilde_setup();
void curve_tilde_setup();
void cycle_tilde_setup();
void dbtoa_tilde_setup();
void degrade_tilde_setup();
void delay_tilde_setup();
void delta_tilde_setup();
void deltaclip_tilde_setup();
void downsamp_tilde_setup();
void edge_tilde_setup();
void equals_tilde_setup();
void funbuff_setup();
void frameaccum_tilde_setup();
void framedelta_tilde_setup();
void gate_tilde_setup();
void greaterthan_tilde_setup();
void greaterthaneq_tilde_setup();
void index_tilde_setup();
void kink_tilde_setup();
void lessthan_tilde_setup();
void lessthaneq_tilde_setup();
void line_tilde_setup();
void lookup_tilde_setup();
void lores_tilde_setup();
void matrix_tilde_setup();
void maximum_tilde_setup();
void minimum_tilde_setup();
void minmax_tilde_setup();
void modulo_tilde_setup();
void mstosamps_tilde_setup();
void notequals_tilde_setup();
void numbox_tilde_setup();
void onepole_tilde_setup();
void overdrive_tilde_setup();
void peakamp_tilde_setup();
void peek_tilde_setup();
void phaseshift_tilde_setup();
void phasewrap_tilde_setup();
void play_tilde_setup();
void plusequals_tilde_setup();
void poke_tilde_setup();
void poltocar_tilde_setup();
void pong_tilde_setup();
void pow_tilde_setup();
void Pow_tilde_setup();
void rampsmooth_tilde_setup();
void rand_tilde_setup();
void rdiv_tilde_setup();
void record_tilde_setup();
void reson_tilde_setup();
void rminus_tilde_setup();
void round_tilde_setup();
void sah_tilde_setup();
void sampstoms_tilde_setup();
void scale_tilde_setup();
void selector_tilde_setup();
void sinh_tilde_setup();
void sinx_tilde_setup();
void slide_tilde_setup();
void snapshot_tilde_setup();
void spike_tilde_setup();
void svf_tilde_setup();
void tanh_tilde_setup();
void tanx_tilde_setup();
void teeth_tilde_setup();
void thresh_tilde_setup();
void train_tilde_setup();
void trapezoid_tilde_setup();
void triangle_tilde_setup();
void trunc_tilde_setup();
void vectral_tilde_setup();
void wave_tilde_setup();
void zerox_tilde_setup();

void above_tilde_setup();
void add_tilde_setup();
void adsr_tilde_setup();
void setup_allpass0x2e2nd_tilde();
void setup_allpass0x2erev_tilde();
void args_setup();
void asr_tilde_setup();
void autofade_tilde_setup();
void autofade2_tilde_setup();
void balance_tilde_setup();
void bandpass_tilde_setup();
void bandstop_tilde_setup();
void setup_bend0x2ein();
void setup_bend0x2eout();
void setup_bl0x2esaw_tilde();
void setup_bl0x2esaw2_tilde();
void setup_bl0x2eimp_tilde();
void setup_bl0x2eimp2_tilde();
void setup_bl0x2esquare_tilde();
void setup_bl0x2etri_tilde();
void setup_bl0x2evsaw_tilde();
void setup_osc0x2eformat();
void setup_osc0x2eparse();
void setup_osc0x2eroute();
void beat_tilde_setup();
void bicoeff_setup();
void bicoeff2_setup();
void bitnormal_tilde_setup();
void biquads_tilde_setup();
void blocksize_tilde_setup();
void break_setup();
void brown_tilde_setup();
void buffer_setup();
void button_setup();
void setup_canvas0x2eactive();
void setup_canvas0x2ebounds();
void setup_canvas0x2eedit();
void setup_canvas0x2egop();
void setup_canvas0x2emouse();
void setup_canvas0x2ename();
void setup_canvas0x2epos();
void setup_canvas0x2esetname();
void setup_canvas0x2evis();
void setup_canvas0x2ezoom();
void ceil_setup();
void ceil_tilde_setup();
void cents2ratio_setup();
void cents2ratio_tilde_setup();
void chance_setup();
void chance_tilde_setup();
void changed_setup();
void changed_tilde_setup();
void changed2_tilde_setup();
void click_setup();
void colors_setup();
void setup_comb0x2efilt_tilde();
void setup_comb0x2erev_tilde();
void cosine_tilde_setup();
void crackle_tilde_setup();
void crossover_tilde_setup();
void setup_ctl0x2ein();
void setup_ctl0x2eout();
void cusp_tilde_setup();
void datetime_setup();
void db2lin_tilde_setup();
void decay_tilde_setup();
void decay2_tilde_setup();
void default_setup();
void del_tilde_setup();
void detect_tilde_setup();
void dollsym_setup();
void downsample_tilde_setup();
void drive_tilde_setup();
void dust_tilde_setup();
void dust2_tilde_setup();
void else_setup();
void envgen_tilde_setup();
void eq_tilde_setup();
void factor_setup();
void fader_tilde_setup();
void fbdelay_tilde_setup();
void fbsine_tilde_setup();
void fbsine2_tilde_setup();
void setup_fdn0x2erev_tilde();
void ffdelay_tilde_setup();
void float2bits_setup();
void floor_setup();
void floor_tilde_setup();
void fold_setup();
void fold_tilde_setup();
void fontsize_setup();
void format_setup();
void filterdelay_tilde_setup();
void setup_freq0x2eshift_tilde();
void function_setup();
void function_tilde_setup();
void gate2imp_tilde_setup();
void gaussian_tilde_setup();
void gbman_tilde_setup();
void gcd_setup();
void gendyn_tilde_setup();
void setup_giga0x2erev_tilde();
void glide_tilde_setup();
void glide2_tilde_setup();
void gray_tilde_setup();
void henon_tilde_setup();
void highpass_tilde_setup();
void highshelf_tilde_setup();
void hot_setup();
void hz2rad_setup();
void ikeda_tilde_setup();
void imp_tilde_setup();
void imp2_tilde_setup();
void impseq_tilde_setup();
void impulse_tilde_setup();
void impulse2_tilde_setup();
void initmess_setup();
void keyboard_setup();
void keycode_setup();
void lag_tilde_setup();
void lag2_tilde_setup();
void lastvalue_tilde_setup();
void latoocarfian_tilde_setup();
void lb_setup();
void lfnoise_tilde_setup();
void limit_setup();
void lincong_tilde_setup();
void loadbanger_setup();
void logistic_tilde_setup();
void loop_setup();
void lop2_tilde_setup();
void lorenz_tilde_setup();
void lowpass_tilde_setup();
void lowshelf_tilde_setup();
void match_tilde_setup();
void median_tilde_setup();
void merge_setup();
void message_setup();
void messbox_setup();
void metronome_setup();
void midi_setup();
void mouse_setup();
void setup_mov0x2eavg_tilde();
void setup_mov0x2erms_tilde();
void mtx_tilde_setup();
void note_setup();
void setup_note0x2ein();
void setup_note0x2eout();
void noteinfo_setup();
void nyquist_tilde_setup();
void op_tilde_setup();
void openfile_setup();
void scope_tilde_setup();
void pack2_setup();
void pad_setup();
void pan2_tilde_setup();
void pan4_tilde_setup();
void panic_setup();
void parabolic_tilde_setup();
void peak_tilde_setup();
void setup_pgm0x2ein();
void setup_pgm0x2eout();
void pic_setup();
void pimp_tilde_setup();
void pimpmul_tilde_setup();
void pink_tilde_setup();
void pluck_tilde_setup();
void plaits_tilde_setup();
void power_tilde_setup();
void properties_setup();
void pulse_tilde_setup();
void pulsecount_tilde_setup();
void pulsediv_tilde_setup();
void quad_tilde_setup();
void quantizer_setup();
void quantizer_tilde_setup();
void rad2hz_setup();
void ramp_tilde_setup();
void rampnoise_tilde_setup();
void setup_rand0x2ef();
void setup_rand0x2ehist();
void setup_rand0x2ef_tilde();
void setup_rand0x2ei();
void setup_rand0x2ei_tilde();
void setup_rand0x2eu();
void sfont_tilde_setup();
void route2_setup();
void randpulse_tilde_setup();
void randpulse2_tilde_setup();
void range_tilde_setup();
void ratio2cents_setup();
void ratio2cents_tilde_setup();
void rec_setup();
void receiver_setup();
void rescale_setup();
void rescale_tilde_setup();
void resonant_tilde_setup();
void resonant2_tilde_setup();
void retrieve_setup();
void rint_setup();
void rint_tilde_setup();
void rms_tilde_setup();
void rotate_tilde_setup();
void routeall_setup();
void router_setup();
void routetype_setup();
void s2f_tilde_setup();
void saw_tilde_setup();
void saw2_tilde_setup();
void schmitt_tilde_setup();
void selector_setup();
void separate_setup();
void sequencer_tilde_setup();
void sh_tilde_setup();
void shaper_tilde_setup();
void sig2float_tilde_setup();
void sin_tilde_setup();
void sine_tilde_setup();
void slew_tilde_setup();
void slew2_tilde_setup();
void slice_setup();
void sort_setup();
void spread_setup();
void spread_tilde_setup();
void square_tilde_setup();
void sr_tilde_setup();
void standard_tilde_setup();
void status_tilde_setup();
void stepnoise_tilde_setup();
void susloop_tilde_setup();
void suspedal_setup();
void svfilter_tilde_setup();
void symbol2any_setup();
// void table_tilde_setup();
void tabplayer_tilde_setup();
void tabreader_setup();
void tabreader_tilde_setup();
void tabwriter_tilde_setup();
void tempo_tilde_setup();
void setup_timed0x2egate_tilde();
void toggleff_tilde_setup();
void setup_touch0x2ein();
void setup_touch0x2eout();
void tri_tilde_setup();
void setup_trig0x2edelay_tilde();
void setup_trig0x2edelay2_tilde();
void trighold_tilde_setup();
void trunc_setup();
void unmerge_setup();
void voices_setup();
void vsaw_tilde_setup();
void vu_tilde_setup();
void wt_tilde_setup();
void wavetable_tilde_setup();
void wrap2_setup();
void wrap2_tilde_setup();
void white_tilde_setup();
void xfade_tilde_setup();
void xgate_tilde_setup();
void xgate2_tilde_setup();
void xmod_tilde_setup();
void xmod2_tilde_setup();
void xselect_tilde_setup();
void xselect2_tilde_setup();
void zerocross_tilde_setup();

void nchs_tilde_setup();
void get_tilde_setup();
void pick_tilde_setup();
void sigs_tilde_setup();
void select_tilde_setup();
void setup_xselect0x2emc_tilde();
void merge_tilde_setup();
void unmerge_tilde_setup();
void phaseseq_tilde_setup();
void pol2car_tilde_setup();
void car2pol_tilde_setup();
void lin2db_tilde_setup();
void sum_tilde_setup();
void slice_tilde_setup();
void order_setup();
void repeat_tilde_setup();
void setup_xgate0x2emc_tilde();
void setup_xfade0x2emc_tilde();
void sender_setup();
void setup_ptouch0x2ein();
void setup_ptouch0x2eout();
void setup_spread0x2emc_tilde();
void setup_rotate0x2emc_tilde();
void pipe2_setup();
void circuit_tilde_setup();

void setup_autofade20x2emc_tilde();
void setup_mtx0x2emc_tilde();
void pan_tilde_setup();
void setup_pan0x2emc_tilde();
void setup_xgate20x2emc_tilde();
void setup_xselect20x2emc_tilde();
void setup_autofade0x2emc_tilde();
void wt2d_tilde_setup();

void pm_tilde_setup();
void pm2_tilde_setup();
void pm4_tilde_setup();
void pm6_tilde_setup();
void var_setup();
void conv_tilde_setup();
void fm_tilde_setup();
void vcf2_tilde_setup();
void setup_mpe0x2ein();
void velvet_tilde_setup();
void popmenu_setup();
void dropzone_setup();

#ifdef ENABLE_FFMPEG
void setup_play0x2efile_tilde();
void sfload_setup();
void sfinfo_setup();
#endif

#ifdef ENABLE_SFIZZ
void sfz_tilde_setup();
#endif
void knob_setup();
void pdlink_setup();
void pdlink_tilde_setup();

void pdlua_setup(char const* datadir, char* vers, int vers_len, void (*register_class_callback)(char const*));
void pdlua_instance_setup();
}

namespace pd {

static int defaultfontshit[] = {
    8, 5, 11, 10, 6, 13, 12, 7, 16, 16, 10, 19, 24, 14, 29, 36, 22, 44,
    16, 10, 22, 20, 12, 26, 24, 14, 32, 32, 20, 38, 48, 28, 58, 72, 44, 88
}; // normal & zoomed (2x)
constexpr int ndefaultfont = std::size(defaultfontshit);

int Setup::initialisePd()
{
    static int initialized = 0;
    if (!initialized) {
        libpd_set_printhook(plugdata_print);

        // Initialise pd
        libpd_init();

        sys_lock();

        plugdata_receiver_class = class_new(gensym("plugdata_receiver"), static_cast<t_newmethod>(nullptr), reinterpret_cast<t_method>(plugdata_receiver_free),
            sizeof(t_plugdata_receiver), CLASS_DEFAULT, A_NULL, 0);
        class_addbang(plugdata_receiver_class, plugdata_receiver_bang);
        class_addfloat(plugdata_receiver_class, plugdata_receiver_float);
        class_addsymbol(plugdata_receiver_class, plugdata_receiver_symbol);
        class_addlist(plugdata_receiver_class, plugdata_receiver_list);
        class_addanything(plugdata_receiver_class, plugdata_receiver_anything);

        plugdata_midi_class = class_new(gensym("plugdata_midi"), static_cast<t_newmethod>(nullptr), reinterpret_cast<t_method>(plugdata_midi_free),
            sizeof(t_plugdata_midi), CLASS_DEFAULT, A_NULL, 0);

        plugdata_print_class = class_new(gensym("plugdata_print"), static_cast<t_newmethod>(nullptr), static_cast<t_method>(nullptr),
            sizeof(t_plugdata_print), CLASS_DEFAULT, A_NULL, 0);

        t_atom zz[ndefaultfont + 2];
        SETSYMBOL(zz, gensym("."));
        SETFLOAT(zz + 1, 0);

        for (int i = 0; i < ndefaultfont; i++) {
            SETFLOAT(zz + i + 2, defaultfontshit[i]);
        }
        pd_typedmess(gensym("pd")->s_thing, gensym("init"), 2 + ndefaultfont, zz);

        socket_init();

        sys_getrealtime(); // Init realtime
        sys_unlock();

        initialized = 1;
    }

    return 0;
}

void* Setup::createReceiver(void* ptr, char const* s,
    t_plugdata_banghook const hook_bang,
    t_plugdata_floathook const hook_float,
    t_plugdata_symbolhook const hook_symbol,
    t_plugdata_listhook const hook_list,
    t_plugdata_messagehook const hook_message)
{
    auto* x = reinterpret_cast<t_plugdata_receiver*>(pd_new(plugdata_receiver_class));
    if (x) {
        sys_lock();
        x->x_sym = gensym(s);
        sys_unlock();
        pd_bind(&x->x_obj.ob_pd, x->x_sym);
        x->x_ptr = ptr;
        x->x_hook_bang = hook_bang;
        x->x_hook_float = hook_float;
        x->x_hook_symbol = hook_symbol;
        x->x_hook_list = hook_list;
        x->x_hook_message = hook_message;
    }
    return x;
}

void Setup::initialisePdLua(char const* datadir, char* vers, int const vers_len, void (*register_class_callback)(char const*))
{
    pdlua_setup(datadir, vers, vers_len, register_class_callback);
}

void Setup::initialisePdLuaInstance()
{
    pdlua_instance_setup();
}

void* Setup::createPrintHook(void* ptr, t_plugdata_printhook const hook_print)
{
    auto* x = reinterpret_cast<t_plugdata_print*>(pd_new(plugdata_print_class));
    if (x) {
        sys_lock();
        t_symbol* s = gensym("#plugdata_print");
        sys_unlock();
        pd_bind(&x->x_obj.ob_pd, s);
        x->x_ptr = ptr;
        x->x_hook = hook_print;
    }
    return x;
}

void* Setup::createMIDIHook(void* ptr,
    t_plugdata_noteonhook const hook_noteon,
    t_plugdata_controlchangehook const hook_controlchange,
    t_plugdata_programchangehook const hook_programchange,
    t_plugdata_pitchbendhook const hook_pitchbend,
    t_plugdata_aftertouchhook const hook_aftertouch,
    t_plugdata_polyaftertouchhook const hook_polyaftertouch,
    t_plugdata_midibytehook const hook_midibyte)
{

    auto* x = reinterpret_cast<t_plugdata_midi*>(pd_new(plugdata_midi_class));
    if (x) {
        sys_lock();
        t_symbol* s = gensym("#plugdata_midi");
        sys_unlock();
        pd_bind(&x->x_obj.ob_pd, s);
        x->x_ptr = ptr;
        x->x_hook_noteon = hook_noteon;
        x->x_hook_controlchange = hook_controlchange;
        x->x_hook_programchange = hook_programchange;
        x->x_hook_pitchbend = hook_pitchbend;
        x->x_hook_aftertouch = hook_aftertouch;
        x->x_hook_polyaftertouch = hook_polyaftertouch;
        x->x_hook_midibyte = hook_midibyte;
    }

    libpd_set_noteonhook(plugdata_noteon);
    libpd_set_controlchangehook(plugdata_controlchange);
    libpd_set_programchangehook(plugdata_programchange);
    libpd_set_pitchbendhook(plugdata_pitchbend);
    libpd_set_aftertouchhook(plugdata_aftertouch);
    libpd_set_polyaftertouchhook(plugdata_polyaftertouch);
    libpd_set_midibytehook(plugdata_midibyte);

    return x;
}

extern "C" {
EXTERN int sys_argparse(int argc, char const** argv);
}

void Setup::parseArguments(char const** argv, size_t const argc, t_namelist** sys_openlist, t_namelist** sys_messagelist)
{
    sys_lock();
    sys_argparse(argc, argv);
    // TODO: some args need to be parsed manually still!
    sys_unlock();
}

void Setup::initialiseELSE()
{
    pdlink_setup();
    pdlink_tilde_setup();

    knob_setup();
    above_tilde_setup();
    add_tilde_setup();
    adsr_tilde_setup();
    setup_allpass0x2e2nd_tilde();
    setup_allpass0x2erev_tilde();
    args_setup();
    asr_tilde_setup();
    autofade_tilde_setup();
    autofade2_tilde_setup();
    balance_tilde_setup();
    bandpass_tilde_setup();
    bandstop_tilde_setup();
    setup_bend0x2ein();
    setup_bend0x2eout();
    setup_bl0x2esaw_tilde();
    setup_bl0x2esaw2_tilde();
    setup_bl0x2eimp_tilde();
    setup_bl0x2eimp2_tilde();
    setup_bl0x2esquare_tilde();
    setup_bl0x2etri_tilde();
    setup_bl0x2evsaw_tilde();
    setup_osc0x2eformat();
    setup_osc0x2eparse();
    setup_osc0x2eroute();
    beat_tilde_setup();
    bicoeff_setup();
    bicoeff2_setup();
    bitnormal_tilde_setup();
    biquads_tilde_setup();
    blocksize_tilde_setup();
    break_setup();
    brown_tilde_setup();
    buffer_setup();
    button_setup();
    setup_canvas0x2eactive();
    setup_canvas0x2ebounds();
    setup_canvas0x2eedit();
    setup_canvas0x2egop();
    setup_canvas0x2emouse();
    setup_canvas0x2ename();
    setup_canvas0x2epos();
    setup_canvas0x2esetname();
    setup_canvas0x2evis();
    setup_canvas0x2ezoom();
    ceil_setup();
    ceil_tilde_setup();
    cents2ratio_setup();
    cents2ratio_tilde_setup();
    chance_setup();
    chance_tilde_setup();
    changed_setup();
    changed_tilde_setup();
    changed2_tilde_setup();
    click_setup();
    white_tilde_setup();
    colors_setup();
    setup_comb0x2efilt_tilde();
    setup_comb0x2erev_tilde();
    cosine_tilde_setup();
    crackle_tilde_setup();
    crossover_tilde_setup();
    setup_ctl0x2ein();
    setup_ctl0x2eout();
    cusp_tilde_setup();
    datetime_setup();
    db2lin_tilde_setup();
    decay_tilde_setup();
    decay2_tilde_setup();
    default_setup();
    del_tilde_setup();
    detect_tilde_setup();
    dollsym_setup();
    downsample_tilde_setup();
    drive_tilde_setup();
    dust_tilde_setup();
    dust2_tilde_setup();
    else_setup();
    envgen_tilde_setup();
    eq_tilde_setup();
    factor_setup();
    fader_tilde_setup();
    fbdelay_tilde_setup();
    fbsine_tilde_setup();
    fbsine2_tilde_setup();
    setup_fdn0x2erev_tilde();
    ffdelay_tilde_setup();
    float2bits_setup();
    floor_setup();
    floor_tilde_setup();
    fold_setup();
    fold_tilde_setup();
    fontsize_setup();
    format_setup();
    filterdelay_tilde_setup();
    setup_freq0x2eshift_tilde();
    function_setup();
    function_tilde_setup();
    gate2imp_tilde_setup();
    gaussian_tilde_setup();
    gbman_tilde_setup();
    gcd_setup();
    gendyn_tilde_setup();
    setup_giga0x2erev_tilde();
    glide_tilde_setup();
    glide2_tilde_setup();
    gray_tilde_setup();
    henon_tilde_setup();
    highpass_tilde_setup();
    highshelf_tilde_setup();
    hot_setup();
    hz2rad_setup();
    ikeda_tilde_setup();
    imp_tilde_setup();
    imp2_tilde_setup();
    impseq_tilde_setup();
    impulse_tilde_setup();
    impulse2_tilde_setup();
    initmess_setup();
    keyboard_setup();
    keycode_setup();
    lag_tilde_setup();
    lag2_tilde_setup();
    lastvalue_tilde_setup();
    latoocarfian_tilde_setup();
    lb_setup();
    lfnoise_tilde_setup();
    limit_setup();
    lincong_tilde_setup();
    loadbanger_setup();
    logistic_tilde_setup();
    loop_setup();
    lop2_tilde_setup();
    lorenz_tilde_setup();
    lowpass_tilde_setup();
    lowshelf_tilde_setup();
    match_tilde_setup();
    median_tilde_setup();
    merge_setup();
    message_setup();
    messbox_setup();
    metronome_setup();
    midi_setup();
    mouse_setup();
    setup_mov0x2eavg_tilde();
    setup_mov0x2erms_tilde();
    mtx_tilde_setup();
    note_setup();
    setup_note0x2ein();
    setup_note0x2eout();
    noteinfo_setup();
    nyquist_tilde_setup();
    op_tilde_setup();
    openfile_setup();
    scope_tilde_setup();
    pack2_setup();
    pad_setup();
    pan2_tilde_setup();
    pan4_tilde_setup();
    panic_setup();
    parabolic_tilde_setup();
    peak_tilde_setup();
    setup_pgm0x2ein();
    setup_pgm0x2eout();
    pic_setup();
    pimp_tilde_setup();
    pink_tilde_setup();
    pimpmul_tilde_setup();
    plaits_tilde_setup();
    pluck_tilde_setup();
    power_tilde_setup();
    properties_setup();
    pulse_tilde_setup();
    pulsecount_tilde_setup();
    pulsediv_tilde_setup();
    quad_tilde_setup();
    quantizer_setup();
    quantizer_tilde_setup();
    rad2hz_setup();
    ramp_tilde_setup();
    rampnoise_tilde_setup();
    setup_rand0x2ef();
    setup_rand0x2eu();
    setup_rand0x2ef_tilde();
    setup_rand0x2ehist();
    s2f_tilde_setup();
    sfont_tilde_setup();
    setup_rand0x2ei();
    setup_rand0x2ei_tilde();
    numbox_tilde_setup();
    route2_setup();
    randpulse_tilde_setup();
    randpulse2_tilde_setup();
    range_tilde_setup();
    ratio2cents_setup();
    ratio2cents_tilde_setup();
    rec_setup();
    receiver_setup();
    rescale_setup();
    rescale_tilde_setup();
    resonant_tilde_setup();
    resonant2_tilde_setup();
    retrieve_setup();
    rint_setup();
    rint_tilde_setup();
    rms_tilde_setup();
    rotate_tilde_setup();
    routeall_setup();
    router_setup();
    routetype_setup();
    saw_tilde_setup();
    saw2_tilde_setup();
    schmitt_tilde_setup();
    selector_setup();
    separate_setup();
    sequencer_tilde_setup();
    sh_tilde_setup();
    shaper_tilde_setup();
    sig2float_tilde_setup();
    sin_tilde_setup();
    sine_tilde_setup();
    slew_tilde_setup();
    slew2_tilde_setup();
    slice_setup();
    sort_setup();
    spread_setup();
    spread_tilde_setup();
    square_tilde_setup();
    sr_tilde_setup();
    standard_tilde_setup();
    status_tilde_setup();
    stepnoise_tilde_setup();
    susloop_tilde_setup();
    suspedal_setup();
    svfilter_tilde_setup();
    symbol2any_setup();
    // table_tilde_setup();
    tabplayer_tilde_setup();
    tabreader_setup();
    tabreader_tilde_setup();
    tabwriter_tilde_setup();
    tempo_tilde_setup();
    setup_timed0x2egate_tilde();
    toggleff_tilde_setup();
    setup_touch0x2ein();
    setup_touch0x2eout();
    tri_tilde_setup();
    setup_trig0x2edelay_tilde();
    setup_trig0x2edelay2_tilde();
    trighold_tilde_setup();
    trunc_setup();
    trunc_tilde_setup();
    unmerge_setup();
    voices_setup();
    vsaw_tilde_setup();
    vu_tilde_setup();
    wt_tilde_setup();
    wavetable_tilde_setup();
    wrap2_setup();
    wrap2_tilde_setup();
    xfade_tilde_setup();
    xgate_tilde_setup();
    xgate2_tilde_setup();
    xmod_tilde_setup();
    xmod2_tilde_setup();
    xselect_tilde_setup();
    xselect2_tilde_setup();
    zerocross_tilde_setup();
    nchs_tilde_setup();
    get_tilde_setup();
    pick_tilde_setup();
    sigs_tilde_setup();
    select_tilde_setup();
    setup_xselect0x2emc_tilde();
    merge_tilde_setup();
    unmerge_tilde_setup();
    phaseseq_tilde_setup();
    pol2car_tilde_setup();
    car2pol_tilde_setup();
    lin2db_tilde_setup();
    sum_tilde_setup();
    slice_tilde_setup();
    order_setup();
    repeat_tilde_setup();
    setup_xgate0x2emc_tilde();
    setup_xfade0x2emc_tilde();
#ifdef ENABLE_SFIZZ
    sfz_tilde_setup();
#endif
    sender_setup();
    setup_ptouch0x2ein();
    setup_ptouch0x2eout();
    setup_spread0x2emc_tilde();
    setup_rotate0x2emc_tilde();
    pipe2_setup();
    circuit_tilde_setup();

    setup_autofade0x2emc_tilde();
    setup_autofade20x2emc_tilde();
    setup_mtx0x2emc_tilde();
    pan_tilde_setup();
    setup_pan0x2emc_tilde();
    setup_xgate20x2emc_tilde();
    setup_xselect20x2emc_tilde();
    wt2d_tilde_setup();

    pm_tilde_setup();
    pm2_tilde_setup();
    pm4_tilde_setup();
    pm6_tilde_setup();
    velvet_tilde_setup();
    popmenu_setup();
    //dropzone_setup();

    var_setup();
    conv_tilde_setup();
    fm_tilde_setup();
    vcf2_tilde_setup();
    setup_mpe0x2ein();
#if ENABLE_FFMPEG
    setup_play0x2efile_tilde();
    sfload_setup();
    sfinfo_setup();
#endif
}

void Setup::initialiseGem(std::string const& gemPluginPath)
{
#if ENABLE_GEM
    Gem_setup(gensym(gemPluginPath.c_str()));
    gemcubeframebuffer_setup();
    gemframebuffer_setup();
    gemhead_setup();
    gemkeyboard_setup();
    gemkeyname_setup();
    gemlist_setup();
    gemlist_info_setup();
    gemlist_matrix_setup();
    gemmanager_setup();
    gemmouse_setup();
    gemreceive_setup();
    gemwin_setup();
    modelfiler_setup();
    render_trigger_setup();
    GemSplash_setup();
    circle_setup();
    colorSquare_setup();
    cone_setup();
    cube_setup();
    cuboid_setup();
    curve_setup();
    curve3d_setup();
    cylinder_setup();
    disk_setup();
    gemvertexbuffer_setup();
    imageVert_setup();
    mesh_line_setup();
    mesh_square_setup();
    model_setup();
    multimodel_setup();
    newWave_setup();
    polygon_setup();
    pqtorusknots_setup();
    primTri_setup();
    rectangle_setup();
    ripple_setup();
    rubber_setup();
    scopeXYZ_setup();
    slideSquares_setup();
    sphere_setup();
    sphere3d_setup();
    square_setup();
    surface3d_setup();
    teapot_setup();
    text2d_setup();
    text3d_setup();
    textextruded_setup();
    textoutline_setup();
    torus_setup();
    trapezoid_setup();
    triangle_setup();
    tube_setup();
    accumrotate_setup();
    alpha_setup();
    ambient_setup();
    ambientRGB_setup();
    camera_setup();
    color_setup();
    colorRGB_setup();
    depth_setup();
    diffuse_setup();
    diffuseRGB_setup();
    emission_setup();
    emissionRGB_setup();
    fragment_program_setup();
    glsl_fragment_setup();
    glsl_geometry_setup();
    glsl_program_setup();
    glsl_tesscontrol_setup();
    glsl_tesseval_setup();
    glsl_vertex_setup();
    linear_path_setup();
    ortho_setup();
    polygon_smooth_setup();
    rotate_setup();
    rotateXYZ_setup();
    scale_setup();
    gemrepeat_setup();
    scaleXYZ_setup();
    separator_setup();
    shearXY_setup();
    shearXZ_setup();
    shearYX_setup();
    shearYZ_setup();
    shearZX_setup();
    shearZY_setup();
    shininess_setup();
    specular_setup();
    specularRGB_setup();
    spline_path_setup();
    translate_setup();
    translateXYZ_setup();
    vertex_program_setup();
    light_setup();
    spot_light_setup();
    world_light_setup();

    /*
    gemmacwindow_setup();
    gemglfw2window_setup();
    gemglfw3window_setup();
    gemglutwindow_setup();
    gemglxwindow_setup();
    gemsdl2window_setup();
    gemsdlwindow_setup();
    gemw32window_setup(); */

    part_color_setup();
    part_damp_setup();
    part_draw_setup();
    part_follow_setup();
    part_gravity_setup();
    part_head_setup();
    part_killold_setup();
    part_killslow_setup();
    part_orbitpoint_setup();
    part_render_setup();
    part_sink_setup();
    part_size_setup();
    part_source_setup();
    part_targetcolor_setup();
    part_targetsize_setup();
    part_velcone_setup();
    part_velocity_setup();
    part_velsphere_setup();
    part_vertex_setup();

    pix_2grey_setup();
    pix_a_2grey_setup();
    pix_add_setup();
    pix_aging_setup();
    pix_alpha_setup();
    pix_background_setup();
    pix_backlight_setup();
    pix_biquad_setup();
    pix_bitmask_setup();
    pix_blob_setup();
    pix_blur_setup();
    pix_buf_setup();
    pix_buffer_setup();
    pix_buffer_read_setup();
    pix_buffer_write_setup();
    pix_chroma_key_setup();
    pix_clearblock_setup();
    pix_color_setup();
    pix_coloralpha_setup();
    pix_colorclassify_setup();
    pix_colormatrix_setup();
    pix_colorreduce_setup();
    pix_compare_setup();
    pix_composite_setup();
    pix_contrast_setup();
    pix_convert_setup();
    pix_convolve_setup();
    pix_coordinate_setup();
    pix_crop_setup();
    pix_cubemap_setup();
    pix_curve_setup();
    pix_data_setup();
    pix_deinterlace_setup();
    pix_delay_setup();
    pix_diff_setup();
    pix_dot_setup();
    pix_draw_setup();
    pix_dump_setup();
    pix_duotone_setup();
    pix_emboss_setup();
    pix_equal_setup();
    pix_film_setup();
    pix_flip_setup();
    pix_freeframe_setup();
    pix_frei0r_setup();
    pix_gain_setup();
    pix_grey_setup();
    pix_halftone_setup();
    pix_histo_setup();
    pix_hsv2rgb_setup();
    pix_image_setup();
    pix_imageInPlace_setup();
    pix_info_setup();
    pix_invert_setup();
    pix_kaleidoscope_setup();
    pix_levels_setup();
    pix_lumaoffset_setup();
    pix_mask_setup();
    pix_mean_color_setup();
    pix_metaimage_setup();
    pix_mix_setup();
    pix_motionblur_setup();
    pix_movement_setup();
    pix_movement2_setup();
    pix_movie_setup();
    pix_multiblob_setup();
    pix_multiimage_setup();
    pix_multiply_setup();
    pix_multitexture_setup();
    pix_noise_setup();
    pix_normalize_setup();
    pix_offset_setup();
    pix_posterize_setup();
    pix_puzzle_setup();
    pix_rds_setup();
    pix_record_setup();
    pix_rectangle_setup();
    pix_refraction_setup();
    pix_resize_setup();
    pix_rgb2hsv_setup();
    pix_rgba_setup();
    pix_roi_setup();
    pix_roll_setup();
    pix_rtx_setup();
    pix_scanline_setup();
    pix_set_setup();
    pix_share_read_setup();
    pix_share_write_setup();
    pix_snap_setup();
    pix_snap2tex_setup();
    pix_subtract_setup();
    pix_sig2pix_setup();
    pix_pix2sig_setup();
    pix_tIIR_setup();
    pix_tIIRf_setup();
    pix_takealpha_setup();
    pix_test_setup();
    pix_texture_setup();
    pix_threshold_setup();
    pix_threshold_bernsen_setup();
    pix_video_setup();
    pix_vpaint_setup();
    pix_write_setup();
    pix_writer_setup();
    pix_yuv_setup();
    pix_zoom_setup();
    vertex_add_setup();
    vertex_combine_setup();
    vertex_draw_setup();
    vertex_grid_setup();
    vertex_info_setup();
    // vertex_model_setup();
    vertex_mul_setup();
    vertex_offset_setup();
    vertex_quad_setup();
    vertex_scale_setup();
    vertex_set_setup();
    vertex_tabread_setup();
    GEMglAccum_setup();
    GEMglActiveTexture_setup();
    GEMglActiveTextureARB_setup();
    GEMglAlphaFunc_setup();
    GEMglAreTexturesResident_setup();
    GEMglArrayElement_setup();
    GEMglBegin_setup();
    GEMglBindProgramARB_setup();
    GEMglBindTexture_setup();
    GEMglBitmap_setup();
    GEMglBlendEquation_setup();
    GEMglBlendFunc_setup();
    GEMglCallList_setup();
    GEMglClear_setup();
    GEMglClearAccum_setup();
    GEMglClearColor_setup();
    GEMglClearDepth_setup();
    GEMglClearIndex_setup();
    GEMglClearStencil_setup();
    GEMglClipPlane_setup();
    GEMglColor3b_setup();
    GEMglColor3bv_setup();
    GEMglColor3d_setup();
    GEMglColor3dv_setup();
    GEMglColor3f_setup();
    GEMglColor3fv_setup();
    GEMglColor3i_setup();
    GEMglColor3iv_setup();
    GEMglColor3s_setup();
    GEMglColor3sv_setup();
    GEMglColor3ub_setup();
    GEMglColor3ubv_setup();
    GEMglColor3ui_setup();
    GEMglColor3uiv_setup();
    GEMglColor3us_setup();
    GEMglColor3usv_setup();
    GEMglColor4b_setup();
    GEMglColor4bv_setup();
    GEMglColor4d_setup();
    GEMglColor4dv_setup();
    GEMglColor4f_setup();
    GEMglColor4fv_setup();
    GEMglColor4i_setup();
    GEMglColor4iv_setup();
    GEMglColor4s_setup();
    GEMglColor4sv_setup();
    GEMglColor4ub_setup();
    GEMglColor4ubv_setup();
    GEMglColor4ui_setup();
    GEMglColor4uiv_setup();
    GEMglColor4us_setup();
    GEMglColor4usv_setup();
    GEMglColorMask_setup();
    GEMglColorMaterial_setup();
    GEMglCopyPixels_setup();
    GEMglCopyTexImage1D_setup();
    GEMglCopyTexImage2D_setup();
    GEMglCopyTexSubImage1D_setup();
    GEMglCopyTexSubImage2D_setup();
    GEMglCullFace_setup();
    GEMglDeleteTextures_setup();
    GEMglDepthFunc_setup();
    GEMglDepthMask_setup();
    GEMglDepthRange_setup();
    GEMglDisable_setup();
    GEMglDisableClientState_setup();
    GEMglDrawArrays_setup();
    GEMglDrawBuffer_setup();
    GEMglDrawElements_setup();
    GEMglEdgeFlag_setup();
    GEMglEnable_setup();
    GEMglEnableClientState_setup();
    GEMglEnd_setup();
    GEMglEndList_setup();
    GEMglEvalCoord1d_setup();
    GEMglEvalCoord1dv_setup();
    GEMglEvalCoord1f_setup();
    GEMglEvalCoord1fv_setup();
    GEMglEvalCoord2d_setup();
    GEMglEvalCoord2dv_setup();
    GEMglEvalCoord2f_setup();
    GEMglEvalCoord2fv_setup();
    GEMglEvalMesh1_setup();
    GEMglEvalMesh2_setup();
    GEMglEvalPoint1_setup();
    GEMglEvalPoint2_setup();
    GEMglFeedbackBuffer_setup();
    GEMglFinish_setup();
    GEMglFlush_setup();
    GEMglFogf_setup();
    GEMglFogfv_setup();
    GEMglFogi_setup();
    GEMglFogiv_setup();
    GEMglFrontFace_setup();
    GEMglFrustum_setup();
    GEMglGenLists_setup();
    GEMglGenProgramsARB_setup();
    GEMglGenTextures_setup();
    GEMglGenerateMipmap_setup();
    GEMglGetError_setup();
    GEMglGetFloatv_setup();
    GEMglGetIntegerv_setup();
    GEMglGetMapdv_setup();
    GEMglGetMapfv_setup();
    GEMglGetMapiv_setup();
    GEMglGetPointerv_setup();
    GEMglGetString_setup();
    GEMglHint_setup();
    GEMglIndexMask_setup();
    GEMglIndexd_setup();
    GEMglIndexdv_setup();
    GEMglIndexf_setup();
    GEMglIndexfv_setup();
    GEMglIndexi_setup();
    GEMglIndexiv_setup();
    GEMglIndexs_setup();
    GEMglIndexsv_setup();
    GEMglIndexub_setup();
    GEMglIndexubv_setup();
    GEMglInitNames_setup();
    GEMglIsEnabled_setup();
    GEMglIsList_setup();
    GEMglIsTexture_setup();
    GEMglLightModelf_setup();
    GEMglLightModeli_setup();
    GEMglLightf_setup();
    GEMglLighti_setup();
    GEMglLineStipple_setup();
    GEMglLineWidth_setup();
    GEMglLoadIdentity_setup();
    GEMglLoadMatrixd_setup();
    GEMglLoadMatrixf_setup();
    GEMglLoadName_setup();
    GEMglLoadTransposeMatrixd_setup();
    GEMglLoadTransposeMatrixf_setup();
    GEMglLogicOp_setup();
    GEMglMap1d_setup();
    GEMglMap1f_setup();
    GEMglMap2d_setup();
    GEMglMap2f_setup();
    GEMglMapGrid1d_setup();
    GEMglMapGrid1f_setup();
    GEMglMapGrid2d_setup();
    GEMglMapGrid2f_setup();
    GEMglMaterialf_setup();
    GEMglMaterialfv_setup();
    GEMglMateriali_setup();
    GEMglMatrixMode_setup();
    GEMglMultMatrixd_setup();
    GEMglMultMatrixf_setup();
    GEMglMultTransposeMatrixd_setup();
    GEMglMultTransposeMatrixf_setup();
    GEMglMultiTexCoord2f_setup();
    GEMglMultiTexCoord2fARB_setup();
    GEMglNewList_setup();
    GEMglNormal3b_setup();
    GEMglNormal3bv_setup();
    GEMglNormal3d_setup();
    GEMglNormal3dv_setup();
    GEMglNormal3f_setup();
    GEMglNormal3fv_setup();
    GEMglNormal3i_setup();
    GEMglNormal3iv_setup();
    GEMglNormal3s_setup();
    GEMglNormal3sv_setup();
    GEMglOrtho_setup();
    GEMglPassThrough_setup();
    GEMglPixelStoref_setup();
    GEMglPixelStorei_setup();
    GEMglPixelTransferf_setup();
    GEMglPixelTransferi_setup();
    GEMglPixelZoom_setup();
    GEMglPointSize_setup();
    GEMglPolygonMode_setup();
    GEMglPolygonOffset_setup();
    GEMglPopAttrib_setup();
    GEMglPopClientAttrib_setup();
    GEMglPopMatrix_setup();
    GEMglPopName_setup();
    GEMglPrioritizeTextures_setup();
    GEMglProgramEnvParameter4dARB_setup();
    GEMglProgramEnvParameter4fvARB_setup();
    GEMglProgramLocalParameter4fvARB_setup();
    GEMglProgramStringARB_setup();
    GEMglPushAttrib_setup();
    GEMglPushClientAttrib_setup();
    GEMglPushMatrix_setup();
    GEMglPushName_setup();
    GEMglRasterPos2d_setup();
    GEMglRasterPos2dv_setup();
    GEMglRasterPos2f_setup();
    GEMglRasterPos2fv_setup();
    GEMglRasterPos2i_setup();
    GEMglRasterPos2iv_setup();
    GEMglRasterPos2s_setup();
    GEMglRasterPos2sv_setup();
    GEMglRasterPos3d_setup();
    GEMglRasterPos3dv_setup();
    GEMglRasterPos3f_setup();
    GEMglRasterPos3fv_setup();
    GEMglRasterPos3i_setup();
    GEMglRasterPos3iv_setup();
    GEMglRasterPos3s_setup();
    GEMglRasterPos3sv_setup();
    GEMglRasterPos4d_setup();
    GEMglRasterPos4dv_setup();
    GEMglRasterPos4f_setup();
    GEMglRasterPos4fv_setup();
    GEMglRasterPos4i_setup();
    GEMglRasterPos4iv_setup();
    GEMglRasterPos4s_setup();
    GEMglRasterPos4sv_setup();
    GEMglRectd_setup();
    GEMglRectf_setup();
    GEMglRecti_setup();
    GEMglRects_setup();
    GEMglRenderMode_setup();
    GEMglReportError_setup();
    GEMglRotated_setup();
    GEMglRotatef_setup();
    GEMglScaled_setup();
    GEMglScalef_setup();
    GEMglScissor_setup();
    GEMglSelectBuffer_setup();
    GEMglShadeModel_setup();
    GEMglStencilFunc_setup();
    GEMglStencilMask_setup();
    GEMglStencilOp_setup();
    GEMglTexCoord1d_setup();
    GEMglTexCoord1dv_setup();
    GEMglTexCoord1f_setup();
    GEMglTexCoord1fv_setup();
    GEMglTexCoord1i_setup();
    GEMglTexCoord1iv_setup();
    GEMglTexCoord1s_setup();
    GEMglTexCoord1sv_setup();
    GEMglTexCoord2d_setup();
    GEMglTexCoord2dv_setup();
    GEMglTexCoord2f_setup();
    GEMglTexCoord2fv_setup();
    GEMglTexCoord2i_setup();
    GEMglTexCoord2iv_setup();
    GEMglTexCoord2s_setup();
    GEMglTexCoord2sv_setup();
    GEMglTexCoord3d_setup();
    GEMglTexCoord3dv_setup();
    GEMglTexCoord3f_setup();
    GEMglTexCoord3fv_setup();
    GEMglTexCoord3i_setup();
    GEMglTexCoord3iv_setup();
    GEMglTexCoord3s_setup();
    GEMglTexCoord3sv_setup();
    GEMglTexCoord4d_setup();
    GEMglTexCoord4dv_setup();
    GEMglTexCoord4f_setup();
    GEMglTexCoord4fv_setup();
    GEMglTexCoord4i_setup();
    GEMglTexCoord4iv_setup();
    GEMglTexCoord4s_setup();
    GEMglTexCoord4sv_setup();
    GEMglTexEnvf_setup();
    GEMglTexEnvi_setup();
    GEMglTexGend_setup();
    GEMglTexGenf_setup();
    GEMglTexGenfv_setup();
    GEMglTexGeni_setup();
    GEMglTexImage2D_setup();
    GEMglTexParameterf_setup();
    GEMglTexParameteri_setup();
    GEMglTexSubImage1D_setup();
    GEMglTexSubImage2D_setup();
    GEMglTranslated_setup();
    GEMglTranslatef_setup();
    GEMglUniform1f_setup();
    GEMglUniform1fARB_setup();
    GEMglUseProgramObjectARB_setup();
    GEMglVertex2d_setup();
    GEMglVertex2dv_setup();
    GEMglVertex2f_setup();
    GEMglVertex2fv_setup();
    GEMglVertex2i_setup();
    GEMglVertex2iv_setup();
    GEMglVertex2s_setup();
    GEMglVertex2sv_setup();
    GEMglVertex3d_setup();
    GEMglVertex3dv_setup();
    GEMglVertex3f_setup();
    GEMglVertex3fv_setup();
    GEMglVertex3i_setup();
    GEMglVertex3iv_setup();
    GEMglVertex3s_setup();
    GEMglVertex3sv_setup();
    GEMglVertex4d_setup();
    GEMglVertex4dv_setup();
    GEMglVertex4f_setup();
    GEMglVertex4fv_setup();
    GEMglVertex4i_setup();
    GEMglVertex4iv_setup();
    GEMglVertex4s_setup();
    GEMglVertex4sv_setup();
    GEMglViewport_setup();
    GEMgluLookAt_setup();
    GEMgluPerspective_setup();
    GLdefine_setup();

    setup_modelOBJ();
    setup_modelASSIMP3();
    setup_imageSTBLoader();
    setup_imageSTBSaver();
    setup_recordPNM();
#    if __APPLE__
    setup_videoAVF();
    setup_filmAVF();
#    elif _MSC_VER
    setup_videoVFW();
    setup_filmDS();
#    else
    // Unfortunately, these plugins have big problems in plugdata
    // they render the whole app unusable
    // setup_videoV4L2();
    // setup_recordV4L2();
#    endif
#    if ENABLE_FFMPEG
    setup_filmFFMPEG();
#    endif

#endif
}

void Setup::initialiseCyclone()
{
    cyclone_setup();
    accum_setup();
    acos_setup();
    acosh_setup();
    active_setup();
    anal_setup();
    append_setup();
    asin_setup();
    asinh_setup();
    atanh_setup();
    atodb_setup();
    bangbang_setup();
    bondo_setup();
    borax_setup();
    bucket_setup();
    buddy_setup();
    capture_setup();
    cartopol_setup();
    clip_setup();
    coll_setup();
    cosh_setup();
    counter_setup();
    cycle_setup();
    dbtoa_setup();
    decide_setup();
    decode_setup();
    drunk_setup();
    flush_setup();
    forward_setup();
    fromsymbol_setup();
    funnel_setup();
    funbuff_setup();
    gate_setup();
    grab_setup();
    histo_setup();
    iter_setup();
    join_setup();
    linedrive_setup();
    listfunnel_setup();
    loadmess_setup();
    match_setup();
    maximum_setup();
    mean_setup();
    midiflush_setup();
    midiformat_setup();
    midiparse_setup();
    minimum_setup();
    mousefilter_setup();
    mousestate_setup();
    mtr_setup();
    next_setup();
    offer_setup();
    onebang_setup();
    pak_setup();
    past_setup();
    peak_setup();
    poltocar_setup();
    pong_setup();
    prepend_setup();
    prob_setup();
    cyclone_pink_tilde_setup();
    pv_setup();
    rdiv_setup();
    rminus_setup();
    round_setup();
    cyclone_scale_setup();
    seq_setup();
    sinh_setup();
    speedlim_setup();
    spell_setup();
    split_setup();
    spray_setup();
    sprintf_setup();
    substitute_setup();
    sustain_setup();
    switch_setup();
    table_setup();
    tanh_setup();
    thresh_setup();
    togedge_setup();
    tosymbol_setup();
    trough_setup();
    cyclone_trunc_tilde_setup();
    universal_setup();
    unjoin_setup();
    urn_setup();
    uzi_setup();
    xbendin_setup();
    xbendin2_setup();
    xbendout_setup();
    xbendout2_setup();
    xnotein_setup();
    xnoteout_setup();
    zl_setup();
    setup_zl0x2eecils();
    setup_zl0x2egroup();
    setup_zl0x2eiter();
    setup_zl0x2ejoin();
    setup_zl0x2elen();
    setup_zl0x2emth();
    setup_zl0x2enth();
    setup_zl0x2ereg();
    setup_zl0x2erev();
    setup_zl0x2erot();
    setup_zl0x2esect();
    setup_zl0x2eslice();
    setup_zl0x2esort();
    setup_zl0x2esub();
    setup_zl0x2eunion();
    setup_zl0x2echange();
    setup_zl0x2ecompare();
    setup_zl0x2edelace();
    setup_zl0x2efilter();
    setup_zl0x2elace();
    setup_zl0x2elookup();
    setup_zl0x2emedian();
    setup_zl0x2equeue();
    setup_zl0x2escramble();
    setup_zl0x2estack();
    setup_zl0x2estream();
    setup_zl0x2esum();
    setup_zl0x2ethin();
    setup_zl0x2eunique();
    setup_zl0x2eindexmap();
    setup_zl0x2eswap();
    acos_tilde_setup();
    acosh_tilde_setup();
    allpass_tilde_setup();
    asin_tilde_setup();
    asinh_tilde_setup();
    atan_tilde_setup();
    atan2_tilde_setup();
    atanh_tilde_setup();
    atodb_tilde_setup();
    average_tilde_setup();
    avg_tilde_setup();
    bitand_tilde_setup();
    bitnot_tilde_setup();
    bitor_tilde_setup();
    bitsafe_tilde_setup();
    bitshift_tilde_setup();
    bitxor_tilde_setup();
    buffir_tilde_setup();
    capture_tilde_setup();
    cartopol_tilde_setup();
    change_tilde_setup();
    click_tilde_setup();
    clip_tilde_setup();
    comb_tilde_setup();
    cosh_tilde_setup();
    cosx_tilde_setup();
    count_tilde_setup();
    cross_tilde_setup();
    curve_tilde_setup();
    cycle_tilde_setup();
    dbtoa_tilde_setup();
    degrade_tilde_setup();
    delay_tilde_setup();
    delta_tilde_setup();
    deltaclip_tilde_setup();
    downsamp_tilde_setup();
    edge_tilde_setup();
    equals_tilde_setup();
    frameaccum_tilde_setup();
    framedelta_tilde_setup();
    gate_tilde_setup();
    greaterthan_tilde_setup();
    greaterthaneq_tilde_setup();
    index_tilde_setup();
    kink_tilde_setup();
    lessthan_tilde_setup();
    lessthaneq_tilde_setup();
    line_tilde_setup();
    lookup_tilde_setup();
    lores_tilde_setup();
    matrix_tilde_setup();
    maximum_tilde_setup();
    minimum_tilde_setup();
    minmax_tilde_setup();
    modulo_tilde_setup();
    mstosamps_tilde_setup();
    notequals_tilde_setup();
    onepole_tilde_setup();
    overdrive_tilde_setup();
    peakamp_tilde_setup();
    peek_tilde_setup();
    phaseshift_tilde_setup();
    phasewrap_tilde_setup();
    play_tilde_setup();
    plusequals_tilde_setup();
    poke_tilde_setup();
    poltocar_tilde_setup();
    pong_tilde_setup();
    pow_tilde_setup();
    Pow_tilde_setup();
    rampsmooth_tilde_setup();
    rand_tilde_setup();
    rdiv_tilde_setup();
    record_tilde_setup();
    reson_tilde_setup();
    rminus_tilde_setup();
    round_tilde_setup();
    sah_tilde_setup();
    sampstoms_tilde_setup();
    scale_tilde_setup();
    selector_tilde_setup();
    sinh_tilde_setup();
    sinx_tilde_setup();
    slide_tilde_setup();
    snapshot_tilde_setup();
    spike_tilde_setup();
    svf_tilde_setup();
    tanh_tilde_setup();
    tanx_tilde_setup();
    teeth_tilde_setup();
    thresh_tilde_setup();
    train_tilde_setup();
    trapezoid_tilde_setup();
    triangle_tilde_setup();
    vectral_tilde_setup();
    wave_tilde_setup();
    zerox_tilde_setup();
}

}
