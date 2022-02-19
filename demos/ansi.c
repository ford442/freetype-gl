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
#include <SDL2/SDL.h>
#include <unistd.h>

#include "freetype-gl.h"
#include "font-manager.h"
#include "vertex-buffer.h"
#include "text-buffer.h"
#include "markup.h"
#include "shader.h"
#include "mat4.h"
#include "screenshot-util.h"

EGLDisplay display;
EGLSurface surface;
EGLContext contextegl;
GLsizei nsources,i;
GLfloat fps;
EGLint config_size,major,minor,attrib_position;
EGLConfig eglconfig=NULL;
EmscriptenWebGLContextAttributes attr;
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;
font_manager_t * font_manager;
text_buffer_t * buffer;
mat4 model,view,projection;
GLuint text_shader;

void init_colors(vec4 *colors){
vec4 defaults[16]={
{{ 46/256.0f,  52/256.0f,  54/256.0f, 1.0f}},
{{204/256.0f,   0/256.0f,   0/256.0f, 1.0f}},
{{ 78/256.0f, 154/256.0f,   6/256.0f, 1.0f}},
{{196/256.0f, 160/256.0f,   0/256.0f, 1.0f}},
{{ 52/256.0f, 101/256.0f, 164/256.0f, 1.0f}},
{{117/256.0f,  80/256.0f, 123/256.0f, 1.0f}},
{{  6/256.0f, 152/256.0f, 154/256.0f, 1.0f}},
{{211/256.0f, 215/256.0f, 207/256.0f, 1.0f}},
{{ 85/256.0f,  87/256.0f,  83/256.0f, 1.0f}},
{{239/256.0f,  41/256.0f,  41/256.0f, 1.0f}},
{{138/256.0f, 226/256.0f,  52/256.0f, 1.0f}},
{{252/256.0f, 233/256.0f,  79/256.0f, 1.0f}},
{{114/256.0f, 159/256.0f, 207/256.0f, 1.0f}},
{{173/256.0f, 127/256.0f, 168/256.0f, 1.0f}},
{{ 52/256.0f, 226/256.0f, 226/256.0f, 1.0f}},
{{238/256.0f, 238/256.0f, 236/256.0f, 1.0f}}
};
size_t i=0;
for(i=0;i<16;++i){
colors[i]=defaults[i];
}
for(i=0;i<6*6*6;i++){
vec4 color={{ (i/6/6)/5.0f, ((i/6)%6)/5.0f, (i%6)/5.0f, 1.0f}};
colors[i+16]=color;
}
for(i=0;i<24;i++){
vec4 color={{i/24.0f,i/24.0f,i/24.0f,1.0f}};
colors[232+i]=color;
}}

void ansi_to_markup( char *sequence,size_t length,markup_t *markup){
size_t i;
int code=0;
int set_bg=-1;
int set_fg=-1;
vec4 none={{0,0,0,0}};
static vec4 * colors=0;
if(colors==0){
colors=(vec4 *)malloc(sizeof(vec4) * 256 );
init_colors(colors);
}
if(length<=1){
markup->foreground_color=colors[0];
markup->underline_color=markup->foreground_color;
markup->overline_color=markup->foreground_color;
markup->strikethrough_color=markup->foreground_color;
markup->outline_color=markup->foreground_color;
markup->background_color=none;
markup->underline=0;
markup->overline=0;
markup->bold=0;
markup->italic=0;
markup->strikethrough=0;
return;
}
for(i=0;i<length;++i){
char c=*(sequence+i);
if(c>='0'&&c<='9'){
code=code * 10+(c-'0');
}
else if((c==';')||(i==(length-1))){
if(set_fg==1){
markup->foreground_color=colors[code];
set_fg=-1;
}
else if(set_bg==1){
markup->background_color=colors[code];
set_bg=-1;
}else if((set_fg==0)&&(code==5)){
set_fg=1;
code=0;
}
else if((set_bg==0)&&(code==5)){
set_bg=1;
code=0;
}
else if((code>=30)&&(code<38)){
markup->foreground_color=colors[code-30];
}
else if((code>=40)&&(code<48)){
markup->background_color=colors[code-40];
}else{
switch(code){
case 0:
markup->foreground_color=colors[0];
markup->background_color=none;
markup->underline=0;
markup->overline=0;
markup->bold=0;
markup->italic=0;
markup->strikethrough=0;
break;
case 1: markup->bold=1;break;
case 21: markup->bold=0;break;
case 2: markup->foreground_color.alpha=0.5;break;
case 22: markup->foreground_color.alpha=1.0;break;
case 3:  markup->italic=1;break;
case 23: markup->italic=0;break;
case 4:  markup->underline=1;break;
case 24: markup->underline=0;break;
case 8: markup->foreground_color.alpha=0.0;break;
case 28: markup->foreground_color.alpha=1.0;break;
case 9:  markup->strikethrough=1;break;
case 29: markup->strikethrough=0;break;
case 53: markup->overline=1;break;
case 55: markup->overline=0;break;
case 39: markup->foreground_color=colors[0];break;
case 49: markup->background_color=none;break;
case 38: set_fg=0;break;
case 48: set_bg=0;break;
default: break;
}}
code=0;
}}
markup->underline_color=markup->foreground_color;
markup->overline_color=markup->foreground_color;
markup->strikethrough_color=markup->foreground_color;
markup->outline_color=markup->foreground_color;
if(markup->bold&&markup->italic){
markup->family="fonts/VeraMoBI.ttf";
}else if(markup->bold){
markup->family="fonts/VeraMoBd.ttf";
}else if(markup->italic){
markup->family="fonts/VeraMoIt.ttf";
}else{
markup->family = "fonts/VeraMono.ttf";
}}

void print( text_buffer_t * buffer,vec2 * pen,
char *text,markup_t *markup){
char *seq_start=text,*seq_end=text;
char *p;
size_t i;
for(p=text;p<(text+strlen(text));++p){
char *start=strstr(p,"\033[");
char *end=NULL;
if(start){
end=strstr(start+1,"m");
}
if((start==p)&&(end>start)){
seq_start=start+2;
seq_end=end;
p=end;
}else{
int seq_size=(seq_end-seq_start)+1;
char * text_start=p;
int text_size=0;
if(start){
text_size=start-p;
p=start-1;
}else{
text_size=text+strlen(text)-p;
p=text+strlen(text);
}
ansi_to_markup(seq_start,seq_size,markup);
markup->font=font_manager_get_from_markup(font_manager,markup);
text_buffer_add_text(buffer,pen,markup,text_start,text_size);
}}}

void init(void){
text_shader=shader_load("shaders/text.vert","shaders/text.frag");
font_manager=font_manager_new(512,512,LCD_FILTERING_OFF);
buffer=text_buffer_new();
vec4 black={{0.0,0.0,0.0,1.0}};
vec4 none={{1.0,1.0,1.0,0.0}};
markup_t markup;
markup.family="fonts/VeraMono.ttf";
markup.size=15.0;
markup.bold=0;
markup.italic=0;
markup.spacing=0.0;
markup.gamma=1.0;
markup.foreground_color=black;
markup.background_color=none;
markup.underline=0;
markup.underline_color=black;
markup.overline=0;
markup.overline_color=black;
markup.strikethrough=0;
markup.strikethrough_color=black;
markup.font=0;
vec2 pen={{10.0,480.0}};
FILE *file=fopen("data/256colors.txt","r");
if(file!=NULL){
char line[1024];
while(fgets(line,sizeof(line),file)!=NULL){
print(buffer,&pen,line,&markup);
}
fclose(file);
}
glGenTextures(1,&font_manager->atlas->id);
glBindTexture(GL_TEXTURE_2D,font_manager->atlas->id);
glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
glTexImage2D(GL_TEXTURE_2D,0,GL_RED,font_manager->atlas->width,
font_manager->atlas->height,0,GL_RED,GL_UNSIGNED_BYTE,font_manager->atlas->data);
mat4_set_identity(&projection);
mat4_set_identity(&model);
mat4_set_identity(&view);
}

void display(){
eglSwapBuffers(display,surface);
glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_BYTE,Indices);
//     glColor4f(1.00,1.00,1.00,1.00);
vertex_buffer_render(buffer->buffer,GL_TRIANGLES);    
}

int main(){ 
char* screenshot_path=NULL;
#ifndef __APPLE__
glewExperimental=GL_TRUE;
GLenum err=glewInit();
if(GLEW_OK!=err){
fprintf( stderr,"Error: %s\n",glewGetErrorString(err));
exit(EXIT_FAILURE);
}
fprintf(stderr,"Using GLEW %s\n",glewGetString(GLEW_VERSION));
#endif

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
glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT,GL_NICEST);
glGenBuffers(v1,&EBO);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(Indices),Indices,GL_STATIC_DRAW);
glGenVertexArrays(v1,&VCO);
glBindVertexArray(VCO);
glGenBuffers(v1,&VBO);
glBindBuffer(GL_ARRAY_BUFFER,VBO);
glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
init();
glUseProgram(text_shader);
glUniformMatrix4fv( glGetUniformLocation( text_shader, "model" ),1, 0, model.data);
glUniformMatrix4fv( glGetUniformLocation( text_shader, "view" ),1, 0, view.data);
glUniformMatrix4fv( glGetUniformLocation( text_shader, "projection" ),1, 0, projection.data);
glUniform1i( glGetUniformLocation( text_shader, "tex" ), 0 );
glUniform3f( glGetUniformLocation( text_shader, "pixel" ),1.0f/font_manager->atlas->width,1.0f/font_manager->atlas->height,(float)font_manager->atlas->depth );
glActiveTexture( GL_TEXTURE0 );
glBindTexture( GL_TEXTURE_2D, font_manager->atlas->id );
glEnable(GL_BLEND);
glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
glEnable(GL_DEPTH_TEST);
glDepthFunc(GL_LESS);
glClearColor(F0,F0,F0,F);
glViewport(0,0,S,S);
mat4_set_orthographic(&projection,0,width,0,height,-1,1);
glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
t1=steady_clock::now();
emscripten_set_main_loop((void(*)())display,0,0);
return EXIT_SUCCESS;
}
