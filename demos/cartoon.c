#include <emscripten.h>
#include <emscripten/html5.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#define __gl2_h_
#include <GLES2/gl2ext.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cstdarg>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

#include "freetype-gl.h"
#include "vertex-buffer.h"
#include "markup.h"
#include "shader.h"
#include "mat4.h"
#include "screenshot-util.h"

EGLDisplay display;
EGLSurface surface;
EGLContext contextegl;
EGLint config_size,major,minor,attrib_position;
EGLConfig eglconfig=NULL;
EmscriptenWebGLContextAttributes attr;
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;

typedef struct {
float x, y, z;
float u, v;
vec4 color;
} vertex_t;

texture_atlas_t * atlas;
vertex_buffer_t * buffer;
GLuint shader;
mat4 model, view, projection;

void add_text(vertex_buffer_t * buffer,texture_font_t * font,char *text,vec2 pen,vec4 fg_color_1,vec4 fg_color_2){
size_t i;
for( i = 0; i < strlen(text); ++i ){
texture_glyph_t *glyph = texture_font_get_glyph( font, text + i );
float kerning = 0.0f;
if( i > 0)
{
kerning = texture_glyph_get_kerning( glyph, text + i - 1 );
}
pen.x += kerning;
float x0  = ( pen.x + glyph->offset_x );
float y0  = (int)( pen.y + glyph->offset_y );
float x1  = ( x0 + glyph->width );
float y1  = (int)( y0 - glyph->height );
float s0 = glyph->s0;
float t0 = glyph->t0;
float s1 = glyph->s1;
float t1 = glyph->t1;
GLuint index = buffer->vertices->size;
GLuint indices[] = {index, index+1, index+2,index, index+2, index+3};
vertex_t vertices[] = {
{ (int)x0,y0,0,  s0,t0,  fg_color_1 },
{ (int)x0,y1,0,  s0,t1,  fg_color_2 },
{ (int)x1,y1,0,  s1,t1,  fg_color_2 },
{ (int)x1,y0,0,  s1,t0,  fg_color_1 } };
vertex_buffer_push_back_indices( buffer, indices, 6 );
vertex_buffer_push_back_vertices( buffer, vertices, 4 );
pen.x += glyph->advance_x;
}}

void init( void ){
atlas = texture_atlas_new( 1024, 1024, 1 );
buffer = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );
texture_font_t *font =
texture_font_new_from_file( atlas, 128, "fonts/LuckiestGuy.ttf" );
vec2 pen    = {{50, 50}};
vec4 black  = {{0.0, 0.0, 0.0, 1.0}};
vec4 yellow = {{1.0, 1.0, 0.0, 1.0}};
vec4 orange1 = {{1.0, 0.9, 0.0, 1.0}};
vec4 orange2 = {{1.0, 0.6, 0.0, 1.0}};
font->rendermode = RENDER_OUTLINE_POSITIVE;
font->outline_thickness = 7;
add_text( buffer, font, "Freetype GL", pen, black, black );
font->rendermode = RENDER_OUTLINE_POSITIVE;
font->outline_thickness = 5;
add_text( buffer, font, "Freetype GL", pen, yellow, yellow );
font->rendermode = RENDER_OUTLINE_EDGE;
font->outline_thickness = 3;
add_text( buffer, font, "Freetype GL", pen, black, black );
font->rendermode = RENDER_NORMAL;
font->outline_thickness = 0;
add_text( buffer, font, "Freetype GL", pen, orange1, orange2 );
glGenTextures( 1, &atlas->id );
glBindTexture( GL_TEXTURE_2D, atlas->id );
glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, atlas->width, atlas->height,0, GL_RED, GL_UNSIGNED_BYTE, atlas->data );
shader = shader_load("shaders/v3f-t2f-c4f.vert","shaders/v3f-t2f-c4f.frag");
mat4_set_identity( &projection );
mat4_set_identity( &model );
mat4_set_identity( &view );
}

void display(){
glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
glUniform1i( glGetUniformLocation( shader, "texture" ),0 );
glUniformMatrix4fv( glGetUniformLocation( shader, "model" ),1, 0, model.data);
glUniformMatrix4fv( glGetUniformLocation( shader, "view" ),1, 0, view.data);
glUniformMatrix4fv( glGetUniformLocation( shader, "projection" ),1, 0, projection.data);
vertex_buffer_render( buffer, GL_TRIANGLES );
glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_BYTE,Indices);
eglSwapBuffers(display,surface);
}

int main( int argc, char **argv ){
char* screenshot_path = NULL;

init();
glClearColor( 1.0, 1.0, 1.0, 1.0 );
glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
S=EM_ASM_INT({return parseInt(document.getElementById('pmhig').innerHTML,10);});
Size=(float)S;
eglBindAPI(EGL_OPENGL_ES_API);
static const EGLint attribut_list[]={ 
EGL_NONE};
static const EGLint anEglCtxAttribs2[]={
EGL_CONTEXT_CLIENT_VERSION,v3,
EGL_CONTEXT_MINOR_VERSION_KHR,v0,
EGL_CONTEXT_PRIORITY_LEVEL_IMG,EGL_CONTEXT_PRIORITY_REALTIME_NV,
EGL_CONTEXT_FLAGS_KHR,EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR,
EGL_CONTEXT_FLAGS_KHR,EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR,
EGL_NONE};
static const EGLint attribute_list[]={
EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR,
EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT,EGL_TRUE,
EGL_DEPTH_ENCODING_NV,EGL_DEPTH_ENCODING_NONLINEAR_NV,
EGL_RENDER_BUFFER,EGL_QUADRUPLE_BUFFER_NV,
EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE,EGL_TRUE,
EGL_RED_SIZE,v8,
EGL_GREEN_SIZE,v8,
EGL_BLUE_SIZE,v8,
EGL_ALPHA_SIZE,v8,
EGL_DEPTH_SIZE,v24,
EGL_STENCIL_SIZE,v8,
EGL_BUFFER_SIZE,v32,
EGL_NONE
};
emscripten_webgl_init_context_attributes(&attr);
attr.alpha=EM_TRUE;
attr.stencil=EM_TRUE;
attr.depth=EM_TRUE;
attr.antialias=EM_FALSE;
attr.premultipliedAlpha=EM_FALSE;
attr.preserveDrawingBuffer=EM_TRUE;
attr.enableExtensionsByDefault=EM_FALSE;
attr.renderViaOffscreenBackBuffer=EM_FALSE;
attr.powerPreference=EM_WEBGL_POWER_PREFERENCE_DEFAULT;
attr.failIfMajorPerformanceCaveat=EM_FALSE;
attr.majorVersion=v2;
attr.minorVersion=v0;
ctx=emscripten_webgl_create_context("#scanvas",&attr);
display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
eglInitialize(display,&v3,&v0);
eglChooseConfig(display,attribute_list,&eglconfig,1,&config_size);
contextegl=eglCreateContext(display,eglconfig,EGL_NO_CONTEXT,anEglCtxAttribs2);
surface=eglCreateWindowSurface(display,eglconfig,0,attribut_list);
eglMakeCurrent(display,surface,surface,contextegl);
emscripten_webgl_make_context_current(ctx);
glEnable(GL_DEPTH_TEST);
glDepthFunc(GL_LESS)
glEnable( GL_BLEND );
glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
mat4_set_orthographic( &projection, 0, width, 0, height, -1, 1);
glUseProgram( shader );
glClearColor(F0,F0,F0,F);
glViewport(0,0,S,S);
glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
emscripten_set_main_loop((void(*)())display,0,0);
return EXIT_SUCCESS;
}
