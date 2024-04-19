//
// Copyright (C) 2004-2006 Jasmine Langridge, ja-reiko@users.sourceforge.net
// Copyright (C) 2015 Andrei Bondor, ab396356@users.sourceforge.net
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include <cmath>
#include "main.h"
#include "shaders.h"

void MainApp::resize()
{
    glClearColor(1.0,1.0,1.0,1.0);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE,GL_ZERO);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glClearDepth(1.0);

    glEnable(GL_CULL_FACE);

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    const GLfloat ambcol[] = {0.1f, 0.1f, 0.1f, 0.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambcol);

    float white[] = { 1.0,1.0,1.0,1.0 };
    //float black[] = { 0.0,0.0,0.0,1.0 };
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,white);

    float spec[] = { 0.3f, 0.5f, 0.5f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 6.0f);

    float litcol[] = { 0.6,0.6,0.6,0.0 };
    glLightfv(GL_LIGHT0,GL_DIFFUSE,litcol);
    glLightfv(GL_LIGHT0,GL_SPECULAR,litcol);

    glEnable(GL_NORMALIZE);
}

/*
 * TODO: this function seems to be unused
 *
void drawBlades(float radius, float ang, float trace)
{
    float invtrace = 1.0 / trace;
    glPushMatrix();
    glScalef(radius, radius, 1.0);
    for (float ba=0; ba<PI*2.0-0.01; ba+=PI/2.0)
    {
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.1,0.1,0.1,0.24 * invtrace);
        glVertex2f(0.0,0.0);
        glColor4f(0.1,0.1,0.1,0.06 * invtrace);
        int num = (int)(trace / 0.1);
        if (num < 2) num = 2;
        float mult = trace / (float)(num-1);
        float angadd = ba + ang;
        for (int i=0; i<num; ++i)
        {
            float a = (float)i * mult + angadd;
            glVertex2f(cos(a),sin(a));
        }
        glEnd();
    }
    glPopMatrix();
}*/

void MainApp::renderWater()
{
    tex_water->bind();

    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    float alpha = 0.5f, maxalpha = 0.5f;

    if (game->water.useralpha)
        alpha = maxalpha = game->water.alpha;

    glPushMatrix();
    {
        int off_x = (int)(campos.x / 20.0);
        int off_y = (int)(campos.y / 20.0);

        glm::mat4 t(1.0f);

        glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(t));
        t = glm::scale(t, glm::vec3(20.0f, 20.0f, 1.0f));
        t = glm::translate(t, glm::vec3(off_x, off_y, 0.0f));
        glLoadMatrixf(glm::value_ptr(t));

        int minx = - 20,
            maxx = + 20,
            miny = - 20,
            maxy = + 20;

        int x_stride = maxx - minx;

        float* vbo = new float[(maxx-minx)*(maxy-miny+1)*12];
        memset(vbo, 0, (maxx-minx)*(maxy-miny+1)*12*sizeof(float));
        unsigned int* ibo = new unsigned int[(maxx-minx+1)*(maxy-miny)*2];

        for (int y=miny; y<maxy; ++y)
        {
            int y3 = y + 20;
            for (int x=minx; x<=maxx; ++x)
            {
                int x3 = x+20;
                if (!game->water.fixedalpha)
                {
                    float ht = game->terrain->getHeight((x+off_x)*20.0,(y+off_y)*20.0);
                    alpha = 1.0 - exp(ht - game->water.height);
                    CLAMP(alpha, 0.0f, maxalpha);
                }
                vbo[(y3 * x_stride + x3)*12 +  0] = x * 0.5;
                vbo[(y3 * x_stride + x3)*12 +  1] = (y+off_y-off_x) * 0.5; // TODO: WTF???
                vbo[(y3 * x_stride + x3)*12 +  2] = 1.0;
                vbo[(y3 * x_stride + x3)*12 +  3] = 1.0;
                vbo[(y3 * x_stride + x3)*12 +  4] = 1.0;
                vbo[(y3 * x_stride + x3)*12 +  5] = alpha;
                // Skip normal
                vbo[(y3 * x_stride + x3)*12 +  9] = x;
                vbo[(y3 * x_stride + x3)*12 + 10] = y;
                vbo[(y3 * x_stride + x3)*12 + 11] = game->water.height;
            }
        }

        for (int y2 = 0; y2 < (maxy-miny); y2++) {
            for (int x2 = 0; x2 < (maxx-minx); x2++) {
                ibo[(y2 * (x_stride+1) + x2)*2 + 0] = (y2 + 1) * (x_stride) + (x2 + 0);
                ibo[(y2 * (x_stride+1) + x2)*2 + 1] = (y2 + 0) * (x_stride) + (x2 + 0);
            }
            ibo[(y2 * (x_stride+1) + (maxx-minx))*2 + 0] = 0; // Restart strip
            ibo[(y2 * (x_stride+1) + (maxx-minx))*2 + 1] = 0;
        }

        glInterleavedArrays(GL_T2F_C4F_N3F_V3F, 12 * sizeof(GL_FLOAT), vbo);
        glDrawElements(GL_TRIANGLE_STRIP, (maxx-minx+1)*(maxy-miny)*2, GL_UNSIGNED_INT, ibo);

        delete[] ibo;
        delete[] vbo;
    }
    glPopMatrix();
    glBlendFunc(GL_ONE,GL_ZERO);
}

void MainApp::renderSky(const mat44f &cammat)
{
    glFogf(GL_FOG_DENSITY, game->weather.fog.density_sky);
    glDepthRange(0.999,1.0);
    glDisable(GL_CULL_FACE);

    glPushMatrix(); // 1
    {
      glLoadMatrixf(cammat);
#define CLRANGE     10
#define CLFACTOR    0.02//0.014
      {
        int x_stride = 2 * CLRANGE + 1;
        float* vbo = new float[(2 * CLRANGE + 1)*(2 * CLRANGE)*5];
        unsigned short* ibo = new unsigned short[(2 * CLRANGE + 2)*(2 * CLRANGE)*2];

        glm::mat4 t(1.0);
        t = glm::translate(t, glm::vec3(cloudscroll, 0.0f, 0.0f));
        t = glm::rotate(t, 30.0f / 360.0f * 2 * 3.141592653f, glm::vec3(0.0f, 0.0f, 1.0f));
        t = glm::scale(t, glm::vec3(0.4f, 0.4f, 1.0f));

        for (int y=-CLRANGE; y<CLRANGE; y++)
        {
            int y3 = y + CLRANGE;
            for (int x=-CLRANGE; x<CLRANGE+1; x++)
            {
                int x3 = x + CLRANGE;
                glm::vec4 k = t * glm::vec4(x, y, 0.0f, 1.0f);
                vbo[(y3 * x_stride + x3) * 5 + 0] = k.x;
                vbo[(y3 * x_stride + x3) * 5 + 1] = k.y;
                vbo[(y3 * x_stride + x3) * 5 + 2] = x;
                vbo[(y3 * x_stride + x3) * 5 + 3] = y;
                vbo[(y3 * x_stride + x3) * 5 + 4] = 0.3-(x*x+y*y)*CLFACTOR;
            }
        }

        for (int y2 = 0; y2 < 2 * CLRANGE; y2++) {
            for (int x2 = 0; x2 < 2 * CLRANGE + 1; x2++) {
                ibo[(y2 * (x_stride+1) + x2)*2 + 0] = (y2 + 1) * (x_stride) + (x2 + 0);
                ibo[(y2 * (x_stride+1) + x2)*2 + 1] = (y2 + 0) * (x_stride) + (x2 + 0);
            }
            ibo[(y2 * (x_stride+1) + 2*CLRANGE+1)*2 + 0] = 0; // Restart strip
            ibo[(y2 * (x_stride+1) + 2*CLRANGE+1)*2 + 1] = 0;
        }

        #if 0
        tex_sky[0]->bind();
        glInterleavedArrays(GL_T2F_V3F, 5 * sizeof(GL_FLOAT), vbo);
        glDrawElements(GL_TRIANGLE_STRIP, (2 * CLRANGE + 1)*(2 * CLRANGE)*2, GL_UNSIGNED_SHORT, ibo);
        #else
        glActiveTexture(GL_TEXTURE0);
        tex_sky[0]->bind();
        VAO vao(
                vbo, 5 * sizeof(float) * (2 * CLRANGE + 1)*(2 * CLRANGE),
                ibo, (2 * CLRANGE + 2)*(2 * CLRANGE)*2 * sizeof(unsigned short)
                );

        ShaderProgram sp("sky_vsh.glsl", "sky_fsh.glsl");
        sp.use();

        vao.bind();

        sp.attrib("tex_coord", 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
        sp.attrib("vert_coord", 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const void*)(2 * sizeof(float)));

        sp.uniform("t_transform", t);

        sp.uniform("tex", 0);

        glDrawElements(GL_TRIANGLE_STRIP, (2 * CLRANGE + 1)*(2 * CLRANGE)*2, GL_UNSIGNED_SHORT, 0);
        #endif

        delete[] ibo;
        delete[] vbo;
      }
    }
    glPopMatrix(); // 1
    glEnable(GL_CULL_FACE);
    glDepthRange(0.0,0.999);
    glFogf(GL_FOG_DENSITY, game->weather.fog.density);
}

void MainApp::render(float eyetranslation)
{
    switch (appstate)
    {
        case AS_LOAD_1:
            renderStateLoading(eyetranslation);
            break;

        case AS_LOAD_2:
        case AS_LOAD_3:
            break;

        case AS_LEVEL_SCREEN:
            renderStateLevel(eyetranslation);
            break;

        case AS_CHOOSE_VEHICLE:
            renderStateChoose(eyetranslation);
            break;

        case AS_IN_GAME:
            renderStateGame(eyetranslation);
            break;

        case AS_END_SCREEN:
            renderStateEnd(eyetranslation);
            break;
    }

    glFinish();
}

void MainApp::renderTexturedFullscreenQuad()
{
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3((float)getWidth()/(float)getHeight(), 1.0f, 1.0f));
    renderTexturedFullscreenQuad(scale);
}

void MainApp::renderTexturedFullscreenQuad(glm::mat4& scale)
{
  float vbo[20] = {
    0.0f, 0.0f,  -1.0f,-1.0f, 0.0f,
    1.0f, 0.0f,   1.0f,-1.0f, 0.0f,
    1.0f, 1.0f,   1.0f, 1.0f, 0.0f,
    0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
  };

  unsigned int ibo[6] = {
    0, 1, 2,
    2, 3, 0,
  };

  glPushMatrix();
  {
  // the background image is square and cut out a piece based on aspect ratio
  glLoadMatrixf(glm::value_ptr(scale));

  glInterleavedArrays(GL_T2F_V3F, 5 * sizeof(GL_FLOAT), vbo);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, ibo);
  }
  glPopMatrix();
}

void MainApp::renderStateLoading(float eyetranslation)
{
    UNREFERENCED_PARAMETER(eyetranslation);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glm::mat4 o = glm::ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glLoadMatrixf(glm::value_ptr(o));
    glMatrixMode(GL_MODELVIEW);

    tex_splash_screen->bind();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    renderTexturedFullscreenQuad();

    tex_loading_screen->bind();

    glm::mat4 s = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, (float)getWidth()/(float)getHeight(), 1.0f));
    s = glm::scale(s, glm::vec3(1.0f/3.5f, 1.0f/3.5f, 1.0f));
    renderTexturedFullscreenQuad(s);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

const char *creditstext[] =
{
    "Trigger Rally " PACKAGE_VERSION,
    "",
    "Copyright (C) 2004-2006",
    "Jasmine Langridge and Richard Langridge",
    "Posit Interactive",
    "",
    "Copyright (C) 2006-2016",
    "Various Contributors",
    "(see DATA_AUTHORS.txt)",
    "",
    "",
    "",
    "Coding",
    "Jasmine Langridge",
    "",
    "Art & SFX",
    "Richard Langridge",
    "",
    "",
    "",
    "Contributors",
    "",
    "Build system",
    "Matze Braune",
    "",
    "Stereo support",
    "Chuck Sites",
    "",
    "Mac OS X porting",
    "Tim Douglas",
    "",
    "Fixes",
    "LavaPunk",
    "Bernhard Kaindl",
    "Liviu Andronic",
    "Ishmael Turner",
    "Iwan 'qubodup' Gabovitch",
    "Farrer",
    "Andrei Bondor",
    "Nikolay Orlyuk",
	"Emanuele Sorce",
    "",
    "New levels",
    "Tim Wintle",
    "David Pagnier",
    "Jared Buckner",
    "Andreas Rosdal",
    "Ivan",
    "Viktor Radnai",
    "Pierre-Alexis",
    "Bruno 'Fuddl' Kleinert",
    "Agnius Vasiliauskas",
    "Matthias Keysermann",
    "Marcio Bremm",
    "Onsemeliot",
    "",
    "Graphics",
    "Alex",
    "Roberto Diez Gonzalez",
    "",
    "",
    "",
    "",
    "",
    "Thanks to Jonathan C. Hatfull",
    "",
    "",
    "",
    "",
    "And thanks to Simon Brown too",
    "",
    "",
    "",
    "",
    "",
    "",
    "Thanks for playing Trigger"
};

#define NUMCREDITSTRINGS (sizeof(creditstext) / sizeof(char*))

void MainApp::renderStateEnd(float eyetranslation)
{
    eyetranslation = eyetranslation;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glm::mat4 o = glm::ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glLoadMatrixf(glm::value_ptr(o));
    glMatrixMode(GL_MODELVIEW);

    tex_end_screen->bind();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_LIGHTING);
    glBlendFunc(GL_ONE, GL_ZERO);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    renderTexturedFullscreenQuad();

    tex_fontSourceCodeOutlined->bind();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPushMatrix();
    o = glm::ortho(0 - hratio, hratio, 0 - vratio, vratio, 0 - 1.0, 1.0);
    glLoadMatrixf(glm::value_ptr(o));
    //glOrtho(-1, 1, -1, 1, -1, 1);
    //glOrtho(800, 0, 600, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float scroll = splashtimeout;
    const float maxscroll = (float)(NUMCREDITSTRINGS - 1) * 2.0f;
    RANGEADJUST(scroll, 0.0f, 0.9f, -10.0f, maxscroll);
    CLAMP_UPPER(scroll, maxscroll);

    glm::mat4 t = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 1.0f));
    t = glm::translate(t, glm::vec3(0.0f, scroll, 0.0f));

    for (int i = 0; i < (int)NUMCREDITSTRINGS; i++)
    {
        float level = fabsf(scroll + (float)i * -2.0f);
        RANGEADJUST(level, 0.0f, 9.0f, 3.0f, 0.0f);

        if (level > 0.0f)
        {
            CLAMP_UPPER(level, 1.0f);

            float enlarge = 1.0f;

#if 1
            if (splashtimeout > 0.9f)
            {
                float amt = (splashtimeout - 0.9f) * 10.0f;
                float amt2 = amt * amt;

                enlarge += amt2 / ((1.0001f - amt) * (1.0001f - amt));
                level -= amt2;
            }
#endif
            glm::mat4 q = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, (float)i * -2.0f, 0.0f));
            q = glm::scale(q, glm::vec3(enlarge, enlarge, 0.0f));

            glColor4f(1.0f, 1.0f, 1.0f, level);

            getSSRender().drawText(creditstext[i], PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, t * q);
        }
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// FIXME: following two functions are almost the same
// They shall be merged, but this requires tinkering with a game logic
void MainApp::renderVehicleType(PVehicleType* vtype)
{
   for (unsigned int i=0; i<vtype->part.size(); ++i)
        {
            glPushMatrix(); // 1

            vec3f vpos = vtype->part[i].ref_local.pos;
            glTranslatef(vpos.x, vpos.y, vpos.z);

            mat44f vorim = vtype->part[i].ref_local.ori_mat_inv;
            glMultMatrixf(vorim);
            if (vtype->part[i].model)
            {

                glPushMatrix(); // 2

                float scale = vtype->part[i].scale;
                glScalef(scale,scale,scale);
                drawModel(*vtype->part[i].model);

                glPopMatrix(); // 2
            }

            if (vtype->wheelmodel)
            {
                for (unsigned int j=0; j<vtype->part[i].wheel.size(); j++)
                {

                    glPushMatrix(); // 2

                    vec3f &wpos = vtype->part[i].wheel[j].pt;
                    glTranslatef(wpos.x, wpos.y, wpos.z);

                    float scale = vtype->wheelscale * vtype->part[i].wheel[j].radius;
                    glScalef(scale,scale,scale);

                    drawModel(*vtype->wheelmodel);

                    glPopMatrix(); // 2
                }
            }

            glPopMatrix(); // 1
        }
}

void MainApp::renderVehicle(PVehicle* vehic)
{
    for (unsigned int i=0; i<vehic->part.size(); ++i)
    {
        if (vehic->type->part[i].model)
        {
            glPushMatrix(); // 1

            vec3f vpos = vehic->part[i].ref_world.pos;
            glTranslatef(vpos.x, vpos.y, vpos.z);

            mat44f vorim = vehic->part[i].ref_world.ori_mat_inv;
            glMultMatrixf(vorim);

            float scale = vehic->type->part[i].scale;
            glScalef(scale,scale,scale);

            drawModel(*vehic->type->part[i].model);

            glPopMatrix(); // 1
        }

        if (vehic->type->wheelmodel)
        {
            for (unsigned int j=0; j<vehic->type->part[i].wheel.size(); j++)
            {

                glPushMatrix(); // 1

                vec3f wpos = vehic->part[i].wheel[j].ref_world.getPosition();
                glTranslatef(wpos.x,wpos.y,wpos.z);

                mat44f worim = vehic->part[i].wheel[j].ref_world.ori_mat_inv;
                glMultMatrixf(worim);

                float scale = vehic->type->wheelscale * vehic->type->part[i].wheel[j].radius;
                glScalef(scale,scale,scale);

                drawModel(*vehic->type->wheelmodel);

                glPopMatrix(); // 1
            }
        }
    }
}

void MainApp::renderStateChoose(float eyetranslation)
{
    PVehicleType *vtype = game->vehiclechoices[choose_type];

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

glMatrixMode(GL_PROJECTION);

  glPushMatrix();
  glm::mat4 o = glm::ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  glLoadMatrixf(glm::value_ptr(o));

  glMatrixMode(GL_MODELVIEW);

  // draw background image

  glBlendFunc(GL_ONE, GL_ZERO);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_FOG);
  glDisable(GL_LIGHTING);

  tex_splash_screen->bind();

  //glColor4f(0.0f, 0.0f, 0.2f, 1.0f); // make image dark blue
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // use image's normal colors
  //glColor4f(0.5f, 0.5f, 0.5f, 1.0f); // make image darker

  renderTexturedFullscreenQuad();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    float fnear = 0.1f, fov = 0.6f;
    float aspect = (float)getWidth() / (float)getHeight();
    stereoFrustum(-fnear*aspect*fov,fnear*aspect*fov,-fnear*fov,fnear*fov,fnear,100000.0f,
                  0.8f, eyetranslation);

    glMatrixMode(GL_MODELVIEW);


    glPushMatrix(); // 0

    glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(-eyetranslation, 0.9f, -5.0f));
    t = glm::rotate(t, 28.0f * 3.141592653f / 180.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    glLoadMatrixf(glm::value_ptr(t));

    glDisable(GL_FOG);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    vec4f lpos = vec4f(0.0f, 1.0f, 0.0f, 0.0f);
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);

    //float tmp = 1.0f;
    //float tmp = sinf(choose_spin * 2.0f) * 0.5f;
    float tmp = cosf(choose_spin * 2.0f) * 0.5f;
    tmp += choose_spin;

    t = glm::rotate(t, 3.141592653f / 2.0f, glm::vec3(-1.0f, 0.0f, 0.0f));
    t = glm::rotate(t, tmp, glm::vec3(0.0f, 0.0f, 1.0f));
    glLoadMatrixf(glm::value_ptr(t));

    renderVehicleType(vtype);

    glPopMatrix(); // 0

    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(glm::value_ptr(o)); // Already have it
    glMatrixMode(GL_MODELVIEW);

    // use the same colors as the menu
    const GuiWidgetColors gwc = gui.getColors();

    tex_fontSourceCodeShadowed->bind();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    const GLdouble margin = (800.0 - 600.0 * cx / cy) / 2.0;

    glm::mat4 o2 = glm::ortho(margin, 600.0 * cx / cy + margin, 0.0, 600.0, -1.0, 1.0);

    glm::mat4 scale_small = glm::scale(glm::mat4(1.0f), glm::vec3(20.0f, 20.0f, 1.0f));
    glm::mat4 scale_big = glm::scale(glm::mat4(1.0f), glm::vec3(30.0f, 30.0f, 1.0f));

    glm::mat4 q(1.0f);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 570.0f, 0.0f));
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Trigger Rally", PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, o2 * q * scale_big);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(790.0f, 570.0f, 0.0f));
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText(
        "car selection " + std::to_string(choose_type + 1) + '/' + std::to_string(game->vehiclechoices.size()),
        PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, o2 * q * scale_small);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(100.0f, 230.0f, 0.0f));
    glColor4f(gwc.header.x, gwc.header.y, gwc.header.z, gwc.header.w);
    getSSRender().drawText(vtype->proper_name.substr(0, 9), PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, o2 * q * scale_big);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(100.0f, 200.0f, 0.0f));
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->proper_class.substr(0, 8), PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, o2 * q * scale_small);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(500.0f, 230.0f, 0.0f));
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Weight (Kg)", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, o2 * q * scale_small);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(500.0f, 190.0f, 0.0f));
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Engine (BHP)", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, o2 * q * scale_small);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(500.0f, 150.0f, 0.0f));
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Wheel drive", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, o2 * q * scale_small);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(500.0f, 110.0f, 0.0f));
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Roadholding", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, o2 * q * scale_small);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(520.0f, 230.0f, 0.0f));
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(std::to_string(static_cast<int>(vtype->mass)), PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, o2 * q * scale_big);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(520.0f, 190.0f, 0.0f));
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->pstat_enginepower, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, o2 * q * scale_big);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(520.0f, 150.0f, 0.0f));
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->pstat_wheeldrive, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, o2 * q * scale_big);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(520.0f, 110.0f, 0.0f));
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->pstat_roadholding, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, o2 * q * scale_big);

    std::string racename;

    if (lss.state == AM_TOP_EVT_PREP || lss.state == AM_TOP_PRAC_SEL_PREP)
        racename = events[lss.currentevent].name + ": " + events[lss.currentevent].levels[lss.currentlevel].name;
    else
    if (lss.state == AM_TOP_LVL_PREP)
        racename = levels[lss.currentlevel].name;

    q = glm::translate(glm::mat4(1.0f), glm::vec3(400.0f, 30.0f, 0.0f));
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText(racename, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, o2 * q * scale_small);

    glBlendFunc(GL_ONE, GL_ZERO);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void MainApp::renderRpmDial(float rpm)
{
    // GL_T2F_V3F
    const float vbo[20] = {
      0.0f,1.0f, -1.0f, 1.0f, 0.0f,
      0.0f,0.0f, -1.0f,-1.0f, 0.0f,
      1.0f,1.0f,  1.0f, 1.0f, 0.0f,
      1.0f,0.0f,  1.0f,-1.0f, 0.0f,
    };

    const unsigned short ibo[4] = {
        0, 1, 2, 3,
    };

      glPushMatrix(); // 1
      // position of rpm dial and needle
      //glTranslatef( hratio * (1.f - (5.75f/50.f)) - 0.3f, -vratio * (40.f/50.f) + 0.22f, 0.0f);

      glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(hratio * (1.f - (2.5f/50.f)) - 0.3f, -vratio * (43.5f/50.f) + 0.22f, 0.0f));
      t = glm::scale(t, glm::vec3(0.30f, 0.30f, 1.0f));
      glLoadMatrixf(glm::value_ptr(t));

      tex_hud_revs->bind();

      glColor3f(1.0f, 1.0f, 1.0f);
      glInterleavedArrays(GL_T2F_V3F, 5 * sizeof(float), vbo);
      glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, ibo);

      // draw the needle of the RPM dial
      const float my_pi = 3.67f; // TODO: I know that's stupid, but... whatever
      t = glm::rotate(t, (my_pi / 4 * 5) - (rpm / 1000.0f) * (my_pi / 12.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      t = glm::translate(t, glm::vec3(0.62f, 0.0f, 0.0f));
      t = glm::scale(t, glm::vec3(0.16f, 0.16f, 0.16f));
      glLoadMatrixf(glm::value_ptr(t));

      tex_hud_revneedle->bind();

      glColor3f(1.0f, 1.0f, 1.0f);
      glInterleavedArrays(GL_T2F_V3F, 5 * sizeof(float), vbo);
      glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, ibo);

      glDisable(GL_TEXTURE_2D); // TODO: WTH it's here???
      glPopMatrix(); // 1
}

void MainApp::renderStateGame(float eyetranslation)
{
    PVehicle *vehic = game->vehicle[0];

    glClear(GL_DEPTH_BUFFER_BIT);
    //glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float fnear = 0.1f, fov = 0.6f;
    float aspect = (float)getWidth() / (float)getHeight();
    stereoFrustum(-fnear*aspect*fov,fnear*aspect*fov,-fnear*fov,fnear*fov,fnear,100000.0f,
                  0.8f, eyetranslation);
    glMatrixMode(GL_MODELVIEW);

    glColor3f(1.0,1.0,1.0);

    vec4f fogcolor(game->weather.fog.color, 1.0f);
    glFogfv(GL_FOG_COLOR, fogcolor);

    glDepthRange(0.0,0.999);

    glPushMatrix(); // 0

    mat44f cammat = camori.getMatrix();
    mat44f cammat_inv = cammat.transpose();

    //glTranslatef(0.0,0.0,-40.0);
    glTranslatef(-eyetranslation, 0.0f, 0.0f);

    glMultMatrixf(cammat);

    glTranslatef(-campos.x, -campos.y, -campos.z);

    float lpos[] = { 0.2, 0.5, 1.0, 0.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);

    glColor3ub(255,255,255);

    glDisable(GL_LIGHTING);

    glActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_ADD_SIGNED);
    glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_MODULATE);
    tex_detail->bind();
    glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    float tgens[] = { 0.05, 0.0, 0.0, 0.0 };
    float tgent[] = { 0.0, 0.05, 0.0, 0.0 };
    glTexGenfv(GL_S,GL_OBJECT_PLANE,tgens);
    glTexGenfv(GL_T,GL_OBJECT_PLANE,tgent);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glActiveTextureARB(GL_TEXTURE0_ARB);

    // draw terrain
    game->terrain->render(campos, cammat_inv);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);

    glActiveTextureARB(GL_TEXTURE1_ARB);
    glDisable(GL_TEXTURE_2D);
    glActiveTextureARB(GL_TEXTURE0_ARB);

    if (renderowncar)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        tex_shadow->bind();

        glColor4f(1.0f, 1.0f, 1.0f, 0.7f);

        vec3f vpos = game->vehicle[0]->body->pos;
        vec3f forw = makevec3f(game->vehicle[0]->body->getOrientationMatrix().row[0]);
        float forwangle = atan2(forw.y, forw.x);
        game->terrain->drawShadow(vpos.x, vpos.y, 1.4f, forwangle + PI*0.5f);

        glBlendFunc(GL_ONE, GL_ZERO);
    }

    renderSky(cammat);

    glEnable(GL_LIGHTING);

    for (unsigned int v=0; v<game->vehicle.size(); ++v)
    {

        if (!renderowncar && v == 0) continue;

        PVehicle *vehic = game->vehicle[v];

        renderVehicle(vehic);
    }

    glDisable(GL_LIGHTING);

    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glDisable(GL_TEXTURE_2D);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#define RAINDROP_WIDTH          0.015
    const glm::vec4 raindrop_col(0.5,0.5,0.5,0.4);

    vec3f offsetdrops = campos - campos_prev;

    float* vbo = new float[rain.size() * 6 * 10]; // 10 = 4 color, 3 normal (unused), 3 position
    memset(vbo, 0, rain.size() * 6 * 10 * sizeof(float));
    unsigned short* ibo = new unsigned short[rain.size() * 8]; // 2 extra for primitive restart

    // Set color for all vertices
    for (unsigned int i = 0; i < rain.size(); i++)
    {
        for (unsigned int j = 0; j < 6; j++)
        {
            vbo[(i * 6 + j)* 10 + 0] = raindrop_col.r;
            vbo[(i * 6 + j)* 10 + 1] = raindrop_col.g;
            vbo[(i * 6 + j)* 10 + 2] = raindrop_col.b;
            vbo[(i * 6 + j)* 10 + 3] = raindrop_col.a * (j == 2 || j == 3);
        }
    }

    for (unsigned int i = 0; i < rain.size(); i++)
    {
        vec3f tempv;
        const float prevlife = rain[i].prevlife;
        vec3f pt1 = rain[i].drop_pt + rain[i].drop_vect * prevlife + offsetdrops;
        vec3f pt2 = rain[i].drop_pt + rain[i].drop_vect * rain[i].life;
        vec3f zag = campos - rain[i].drop_pt;
        zag = zag.cross(rain[i].drop_vect);
        zag *= RAINDROP_WIDTH / zag.length();

        for (unsigned int j = 0; j < 6; j++)
        {
            float sign = j / 2 - 1.0f;
            vec3f pt = (j%2 == 0) ? pt1 : pt2;
            vec3f v = pt + sign * zag;

            vbo[(i * 6 + j)* 10 + 7] = v.x;
            vbo[(i * 6 + j)* 10 + 8] = v.y;
            vbo[(i * 6 + j)* 10 + 9] = v.z;

            ibo[i * 8 + j] = i * 6 + j;
        }
        ibo[i * 8 + 6] = 0; // Restart
        ibo[i * 8 + 7] = 0;
    }

    glInterleavedArrays(GL_C4F_N3F_V3F, 10 * sizeof(float), vbo);
    glDrawElements(GL_TRIANGLE_STRIP, rain.size() * 8, GL_UNSIGNED_SHORT, ibo);

    delete[] ibo;
    delete[] vbo;

#define SNOWFLAKE_POINT_SIZE        3.0f
#define SNOWFLAKE_BOX_SIZE          0.175f

// NOTE: must be greater than 1.0f
#define SNOWFLAKE_MAXLIFE           4.5f

    /*
     * TODO: while we could use points for snowflakes in modern OpenGL, it is kinda point-less,
     * because it won't lead to significant preformance improvements (if any at all).
     * Rendering snowflakes as circles instead of points doesn't look as a good idea either.
     * Thus I commented out the support for points but decided not to delete it entirely... just in case.
     */

    //GLfloat ops; // Original Point Size, for to be restored

    /*if (cfg_snowflaketype == SnowFlakeType::point)
    {
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        glGetFloatv(GL_POINT_SIZE, &ops);
        glPointSize(SNOWFLAKE_POINT_SIZE);
    }
    else*/
    if (cfg_snowflaketype == SnowFlakeType::textured)
    {
        glEnable(GL_TEXTURE_2D);
        glBlendFunc(GL_SRC_COLOR, GL_ONE);
        tex_snowflake->bind();
    }

    // GL_T2F_V3F
    // Texture coordinates will be ignored if texturing is disabled
    float snow_vbo[20] = {
        1.0f, 1.0f,  0.0f, 0.0f, 0.0f,
        0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
        1.0f, 0.0f,  1.0f, 1.0f, 0.0f,
        0.0f, 0.0f,  1.0f, 1.0f, 1.0f,
    };

    unsigned short snow_ibo[] = {
        0, 1, 2, 3,
    };

    for (const SnowFlake &sf: snowfall)
    {
        const vec3f pt = sf.drop_pt + sf.drop_vect * sf.life;
        GLfloat alpha;

        if (sf.life > SNOWFLAKE_MAXLIFE)
        {
            alpha = 0.0f;
        }
        else
        if (sf.life > 1.0f)
        {
#define ML      SNOWFLAKE_MAXLIFE
            // this equation ensures that snowflaks fade in
            alpha = (sf.life - ML) / (1 - ML);
#undef ML
        }
        else
            alpha = 1.0f;

        /*if (cfg_snowflaketype == SnowFlakeType::point)
        {
            glBegin(GL_POINTS);
            glColor4f(1.0f, 1.0f, 1.0f, alpha);
            glVertex3fv(pt);
            glEnd();
        }
        else*/
        {
            vec3f zag = campos - sf.drop_pt;

            zag = zag.cross(sf.drop_vect);
            zag.normalize();
            zag *= SNOWFLAKE_BOX_SIZE;

            glm::mat4 t(1.0f);
            glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(t));
            t = glm::translate(t, glm::vec3(pt.x, pt.y, pt.z));
            t = glm::scale(t, glm::vec3(zag.x, zag.y, zag.z + SNOWFLAKE_BOX_SIZE));

            glPushMatrix();
            glLoadMatrixf(glm::value_ptr(t));

            glColor4f(1.0f, 1.0f, 1.0f, alpha);
            glInterleavedArrays(GL_T2F_V3F, 5 * sizeof(float), snow_vbo);
            glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, snow_ibo);
            glPopMatrix();
        }
    }

    /*if (cfg_snowflaketype == SnowFlakeType::point)
        glPointSize(ops); // restore original point size*/

    // disable textures
    if (cfg_snowflaketype == SnowFlakeType::textured)
    {
        glDisable(GL_TEXTURE_2D);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    const vec4f checkpoint_col[3] =
    {
        vec4f(1.0f, 0.0f, 0.0f, 0.8f),  // 0 = next checkpoint
        vec4f(0.7f, 0.7f, 0.1f, 0.6f),  // 1 = checkpoint after next
        vec4f(0.2f, 0.8f, 0.2f, 0.4f)  // 2 = all other checkpoints
    };

    if (showcheckpoint)
    {
        ShaderProgram sp("chkpt_vsh.glsl", "chkpt_fsh.glsl");
        sp.use();

        // GL_C4F_N3F_V3F
        const int N_SPLITS = 20;

        float* check_vbo = new float[(N_SPLITS) * 3 * 10];
        memset(check_vbo, 0, (N_SPLITS) * 3 * 10 * sizeof(float));
        unsigned short* check_ibo = new unsigned short[(N_SPLITS+1)*4];
        vec4f col = checkpoint_col[2];

        for (int i = 0; i < N_SPLITS; i++)
        {
            float a = i * 2 * PI / N_SPLITS;

            for (int j = 0; j < 3; j++)
            {
                check_vbo[(i*3 + j) * 10 + 0] = 0.0f;
                check_vbo[(i*3 + j) * 10 + 1] = 0.0f;
                check_vbo[(i*3 + j) * 10 + 2] = 0.0f;
                check_vbo[(i*3 + j) * 10 + 3] = (j == 1);
                // Skip normal
                check_vbo[(i*3 + j) * 10 + 7] = cosf(a);
                check_vbo[(i*3 + j) * 10 + 8] = sinf(a);
                check_vbo[(i*3 + j) * 10 + 9] = j - 1;

            }
        }

        for (int i = 0; i < N_SPLITS+1; i++) {
            check_ibo[i * 2 + 0] = (i % N_SPLITS) * 3 + 0;
            check_ibo[i * 2 + 1] = (i % N_SPLITS) * 3 + 1;

            check_ibo[(i + N_SPLITS + 1) * 2 + 0] = (i % N_SPLITS) * 3 + 1;
            check_ibo[(i + N_SPLITS + 1) * 2 + 1] = (i % N_SPLITS) * 3 + 2;
        }

        VAO vao(
            check_vbo, (N_SPLITS) * 3 * 10 * sizeof(float),
            check_ibo, (N_SPLITS+1)*4 * sizeof(unsigned short)
        );

        vao.bind();

        sp.attrib("d_color", 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), 0);
        sp.attrib("normal", 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (const void*)(4 * sizeof(float)));
        sp.attrib("position", 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (const void*)(7 * sizeof(float)));

        for (unsigned int i=0; i<game->checkpt.size(); i++)
        {
            vec4f colr = checkpoint_col[2];

            if ((int)i == vehic->nextcp)
                colr = checkpoint_col[0];
            else if ((int)i == (vehic->nextcp + 1) % (int)game->checkpt.size())
                colr = checkpoint_col[1];

            //glPushMatrix(); // 1
            glm::mat4 t(1.0f);
            glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(t));
            t = glm::translate(t, glm::vec3(game->checkpt[i].pt.x, game->checkpt[i].pt.y, game->checkpt[i].pt.z));
            t = glm::scale(t, glm::vec3(25.0f, 25.0f, 1.0f));

#if 0 // Checkpoint style one (one strip)
            glBegin(GL_TRIANGLE_STRIP);

            for (float a = 0.0f; a < 0.99f; a += 0.05f)
            {
                glColor4f(colr[0], colr[1], colr[2], colr[3] * a);
                float ang = cprotate + a * 6.0f;
                float ht = sinf(ang * 1.7f) * 7.0f + 8.0f;
                glVertex3f(cosf(ang), sinf(ang), ht - 1.0f);
                glVertex3f(cosf(ang), sinf(ang), ht + 1.0f);
            }

            for (float a = 1.0f; a < 2.01f; a += 0.05f)
            {
                glColor4f(colr[0], colr[1], colr[2], colr[3] * (2.0f - a));
                float ang = cprotate + a * 6.0f;
                float ht = sinf(ang * 1.7f) * 7.0f + 8.0f;
                glVertex3f(cosf(ang), sinf(ang), ht - 1.0f);
                glVertex3f(cosf(ang), sinf(ang), ht + 1.0f);
            }

            glEnd();
#else // Regular checkpoint style (two strips)
            float ht = sinf(cprotate * 6.0f) * 7.0f + 8.0f;

            t = glm::translate(t, glm::vec3(0.0f, 0.0f, ht));

            //glLoadMatrixf(glm::value_ptr(t));
#if 0
            glBegin(GL_TRIANGLE_STRIP);
            for (int i = 0; i < N_SPLITS; i++)
            //for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                float a = i * 2 * PI / N_SPLITS;
                glColor4f(colr[0], colr[1], colr[2], 0.0f);
                glVertex3f(cosf(a), sinf(a), - 1.0f);
                glColor4f(colr[0], colr[1], colr[2], colr[3]);
                glVertex3f(cosf(a), sinf(a), + 0.0f);
            }
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, - 1.0f);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, + 0.0f);
            glEnd();

            glBegin(GL_TRIANGLE_STRIP);
            for (int i = 0; i < N_SPLITS; i++)
            //for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                float a = i * 2 * PI / N_SPLITS;
                glColor4f(colr[0], colr[1], colr[2], colr[3]);
                glVertex3f(cosf(a), sinf(a), - 0.0f);
                glColor4f(colr[0], colr[1], colr[2], 0.0f);
                glVertex3f(cosf(a), sinf(a), + 1.0f);
            }
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, - 0.0f);
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, + 1.0f);
            glEnd();
#else
            //glInterleavedArrays(GL_C4F_N3F_V3F, 10 * sizeof(float), check_vbo);
            //glDrawElements(GL_TRIANGLE_STRIP, (N_SPLITS+1)*4, GL_UNSIGNED_SHORT, check_ibo);
            sp.uniform("color", glm::vec4(colr[0], colr[1], colr[2], colr[3]));
            sp.uniform("mv", t);
            glDrawElements(GL_TRIANGLE_STRIP, (N_SPLITS+1)*4, GL_UNSIGNED_SHORT, 0);
#endif
#endif
            //glPopMatrix(); // 1
        }

        delete[] check_vbo;
        delete[] check_ibo;

// codriver checkpoints rendering
#ifdef INDEVEL

    // codriver checkpoints for debugging purposes
    const vec4f cdcheckpoint_col[3] =
    {
        {0.0f, 0.0f, 1.0f, 0.8f},       // 0 = next checkpoint
        {0.3f, 0.3f, 1.0f, 0.6f},       // 1 = checkpoint after next
        {0.6f, 0.6f, 1.0f, 0.4f}        // 2 = all other checkpoints
    };

        for (unsigned int i=0; i<game->codrivercheckpt.size(); i++)
        {
            vec4f colr = cdcheckpoint_col[2];

            if (game->cdcheckpt_ordered)
            {
                if ((int)i == vehic->nextcdcp)
                    colr = cdcheckpoint_col[0];
                else if ((int)i == (vehic->nextcdcp + 1) % (int)game->codrivercheckpt.size())
                    colr = cdcheckpoint_col[1];
            }
            else
                colr = cdcheckpoint_col[1];

            glPushMatrix(); // 1
            glTranslatef(game->codrivercheckpt[i].pt.x, game->codrivercheckpt[i].pt.y, game->codrivercheckpt[i].pt.z);
            glScalef(15.0f, 15.0f, 1.0f);

            glBegin(GL_TRIANGLE_STRIP);
            float ht = sinf(cprotate * 6.0f) * 7.0f + 8.0f;
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht - 1.0f);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht + 0.0f);
            for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                glColor4f(colr[0], colr[1], colr[2], 0.0f);
                glVertex3f(cosf(a), sinf(a), ht - 1.0f);
                glColor4f(colr[0], colr[1], colr[2], colr[3]);
                glVertex3f(cosf(a), sinf(a), ht + 0.0f);
            }
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht - 1.0f);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht + 0.0f);
            glEnd();

            glBegin(GL_TRIANGLE_STRIP);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht - 0.0f);
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht + 1.0f);
            for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                glColor4f(colr[0], colr[1], colr[2], colr[3]);
                glVertex3f(cosf(a), sinf(a), ht - 0.0f);
                glColor4f(colr[0], colr[1], colr[2], 0.0f);
                glVertex3f(cosf(a), sinf(a), ht + 1.0f);
            }
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht - 0.0f);
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht + 1.0f);
            glEnd();
            glPopMatrix(); // 1
        }
#endif
    }

    glEnable(GL_TEXTURE_2D);

    if (game->water.enabled)
        renderWater();

    if (psys_dirt != nullptr) // cfg_dirteffect == false
        getSSRender().render(psys_dirt);

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_ONE,GL_ZERO);
    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
    glEnable(GL_FOG);

    glDisable(GL_LIGHTING);

    glPopMatrix(); // 0

    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glm::mat4 o = glm::ortho(0 - hratio, hratio, 0 - vratio, vratio, 0 - 1.0, 1.0);
    glLoadMatrixf(glm::value_ptr(o));

    glMatrixMode(GL_MODELVIEW);

    glPushMatrix(); // 0

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (showui)
    {
        game->renderCodriverSigns();

        renderRpmDial(vehic->getEngineRPM());
    }

    // checkpoint pointing arrow thing
#if 0
    glPushMatrix(); // 1

    glTranslatef(0.0f, 0.8f, 0.0f);

    glScalef(0.2f, 0.2f, 0.2f);

    glRotatef(-30.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(DEGREES(nextcpangle), 0.0f, -1.0f, 0.0f);

    glBegin(GL_TRIANGLES);
    glColor4f(0.8f, 0.4f, 0.4f, 0.6f);
    glVertex3f(0.0f, 0.0f, -2.0f);
    glColor4f(0.8f, 0.8f, 0.8f, 0.6f);
    glVertex3f(1.0f, 0.0f, 1.0f);
    glVertex3f(-1.0f, 0.0f, 1.0f);
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
    glColor4f(0.8f, 0.4f, 0.4f, 0.6f);
    glVertex3f(0.0f, 0.0f, -2.0f);
    glColor4f(1.0f, 0.5f, 0.5f, 0.6f);
    glVertex3f(0.0f, 0.2f, -2.0f);
    glColor4f(0.8f, 0.8f, 0.8f, 0.6f);
    glVertex3f(1.0f, 0.0f, 1.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
    glVertex3f(1.0f, 0.2f, 1.0f);
    glColor4f(0.8f, 0.8f, 0.8f, 0.6f);
    glVertex3f(-1.0f, 0.0f, 1.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
    glVertex3f(-1.0f, 0.2f, 1.0f);
    glColor4f(0.8f, 0.4f, 0.4f, 0.6f);
    glVertex3f(0.0f, 0.0f, -2.0f);
    glColor4f(1.0f, 0.5f, 0.5f, 0.6f);
    glVertex3f(0.0f, 0.2f, -2.0f);
    glEnd();

    glPopMatrix(); // 1
#endif

    if (showmap)
    {
        // position and size of map
        //glViewport(getWidth() * (5.75f/100.f), getHeight() * (6.15f/100.f), getHeight()/3.5f, getHeight()/3.5f);
        glViewport(getWidth() * (2.5f/100.f), getHeight() * (2.5f/100.f), getHeight()/3.5f, getHeight()/3.5f);

        glPushMatrix(); // 1
        glScalef(hratio, vratio, 1.0f);

        if (game->terrain->getHUDMapTexture())
        {
            glEnable(GL_TEXTURE_2D);
            game->terrain->getHUDMapTexture()->bind();
        }

        glMatrixMode(GL_TEXTURE);
        glPushMatrix();
        float scalefac = 1.0f / game->terrain->getMapSize();
        glScalef(scalefac, scalefac, 1.0f);
        glTranslatef(campos.x, campos.y, 0.0f);
        glRotatef(DEGREES(camera_angle), 0.0f, 0.0f, 1.0f);
        glScalef(1.0f / 0.003f, 1.0f / 0.003f, 1.0f);

        glBegin(GL_QUADS);
        glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(-1.0f, 1.0f);
        glVertex2f(-1.0f, 1.0f);
        glTexCoord2f(-1.0f, -1.0f);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, -1.0f);
        glVertex2f(1.0f, -1.0f);
        glEnd();

        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

        glDisable(GL_TEXTURE_2D);

        glPushMatrix(); // 2
        glScalef(0.003f, 0.003f, 1.0f);
        glRotatef(DEGREES(-camera_angle), 0.0f, 0.0f, 1.0f);
        glTranslatef(-campos.x, -campos.y, 0.0f);
        for (unsigned int i=0; i<game->checkpt.size(); i++)
        {
            glPushMatrix();
            vec3f vpos = game->checkpt[i].pt;
            glTranslatef(vpos.x, vpos.y, 0.0f);
            glRotatef(DEGREES(camera_angle), 0.0f, 0.0f, 1.0f);
            glScalef(30.0f, 30.0f, 1.0f);
            vec4f colr = checkpoint_col[2];
            if ((int)i == vehic->nextcp)
            {
                float sc = 1.5f + sinf(cprotate * 10.0f) * 0.5f;
                glScalef(sc, sc, 1.0f);
                colr = checkpoint_col[0];
            }
            else if ((int)i == (vehic->nextcp + 1) % (int)game->checkpt.size())
            {
                colr = checkpoint_col[1];
            }
            glBegin(GL_TRIANGLE_FAN);
            glColor4fv(colr);
            glVertex2f(0.0f, 0.0f);
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            //glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glVertex2f(1.0f, 0.0f);
            glVertex2f(0.0f, 1.0f);
            glVertex2f(-1.0f, 0.0f);
            glVertex2f(0.0f, -1.0f);
            glVertex2f(1.0f, 0.0f);
            glEnd();
            glPopMatrix();
        }
        for (unsigned int i=0; i<game->vehicle.size(); i++)
        {
            glPushMatrix();
            vec3f vpos = game->vehicle[i]->body->getPosition();
            glTranslatef(vpos.x, vpos.y, 0.0f);
            glRotatef(DEGREES(camera_angle), 0.0f, 0.0f, 1.0f);
            glScalef(30.0f, 30.0f, 1.0f);
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glVertex2f(0.0f, 0.0f);
            glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glVertex2f(1.0f, 0.0f);
            glVertex2f(0.0f, 1.0f);
            glVertex2f(-1.0f, 0.0f);
            glVertex2f(0.0f, -1.0f);
            glVertex2f(1.0f, 0.0f);
            glEnd();
            glPopMatrix();
        }
        glPopMatrix(); // 2

        glPopMatrix(); // 1

        glViewport(0, 0, getWidth(), getHeight());
    }

    glEnable(GL_TEXTURE_2D);

    if (showui)
    {
      /*
      tex_hud_gear->bind();
      glPushMatrix(); // 2

      glTranslatef(1.0f, 0.35f, 0.0f);
      glScalef(0.2f, 0.2f, 1.0f);
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
      glBegin(GL_QUADS);
      glTexCoord2f(1.0f,1.0f); glVertex2f(1.0f,1.0f);
      glTexCoord2f(0.0f,1.0f); glVertex2f(-1.0f,1.0f);
      glTexCoord2f(0.0f,0.0f); glVertex2f(-1.0f,-1.0f);
      glTexCoord2f(1.0f,0.0f); glVertex2f(1.0f,-1.0f);
      glEnd();

      glPopMatrix(); // 2
      */

      tex_fontSourceCodeOutlined->bind();

      // time counter

      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

      // time position (other time strings inherit this position)
      // -hratio is left border, 0 is center, +hratio is right
      // hratio * (1/50) gives 1% of the entire width
      // +vratio is top border, 0 is middle, -vratio is bottom

      {
      glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(-hratio + hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f), 0.0f));
      t = glm::scale(t, glm::vec3(0.125f, 0.125f, 1.0f));

      if (game->gamestate == Gamestate::finished)
      {
          getSSRender().drawText(
              PUtil::formatTime(game->coursetime),
              PTEXT_HZA_LEFT | PTEXT_VTA_TOP, t);
      }
      else if (game->coursetime < game->cptime + 1.50f)
      {
          getSSRender().drawText(
              PUtil::formatTime(game->cptime),
              PTEXT_HZA_LEFT | PTEXT_VTA_TOP, t);
      }
      else if (game->coursetime < game->cptime + 3.50f)
      {
          float a = (((game->cptime + 3.50f) - game->coursetime) / 2);
          glColor4f(1.0f, 1.0f, 1.0f, a);
          getSSRender().drawText(
              PUtil::formatTime(game->cptime),
              PTEXT_HZA_LEFT | PTEXT_VTA_TOP, t);
      }
      else
      {
          getSSRender().drawText(
              PUtil::formatTime(game->coursetime),
              PTEXT_HZA_LEFT | PTEXT_VTA_TOP, t);
      }

      // show target time

      glColor4f(0.5f, 1.0f, 0.5f, 1.0f);

      glm::mat4 q = glm::translate(t, glm::vec3(0.0f, -0.8f, 0.0f));
      getSSRender().drawText(PUtil::formatTime(game->targettime), PTEXT_HZA_LEFT | PTEXT_VTA_TOP, q);

    {
        // show the time penalty if there is any
        const float timepen = game->uservehicle->offroadtime_total * game->offroadtime_penalty_multiplier;

        // FIXME: why do glPushMatrix() and glPopMatrix() not work for me?
        if (timepen >= 0.1f)
        {
            glColor4f(1.0f, 1.0f, 0.5f, 1.0f);
            glm::mat4 p = glm::translate(q, glm::vec3(0.0f, -1.60f, 0.0f));
            getSSRender().drawText(PUtil::formatTime(timepen) + '+', PTEXT_HZA_LEFT | PTEXT_VTA_TOP, p);
        }
    }

      // time label
    {
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
      glm::mat4 k = glm::translate(t, glm::vec3(0.0f, .52f, 0.0f));
      k = glm::scale(k, glm::vec3(0.65f, 0.65f, 1.0f));
      getSSRender().drawText("TIME", PTEXT_HZA_LEFT | PTEXT_VTA_TOP, k);
    }
    }

      // show Next/Total checkpoints
      {
          // checkpoint counter

          glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
          const std::string totalcp = std::to_string(game->checkpt.size());
          const std::string nextcp = std::to_string(vehic->nextcp);

          // checkpoint position
          glm::mat4 k = glm::translate(glm::mat4(1.0f), glm::vec3(hratio - hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f), 0.0f));
          k = glm::scale(k, glm::vec3(0.125f, 0.125f, 1.0f));

            if (game->getFinishState() != Gamefinish::not_finished)
                getSSRender().drawText(totalcp + '/' + totalcp, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, k);
            else
                getSSRender().drawText(nextcp + '/' + totalcp, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, k);

          // checkpoint label

          //glPushMatrix(); // 3
          k = glm::translate(k, glm::vec3(0, 0.52f, 0.0f));
          k = glm::scale(k, glm::vec3(0.65f, 0.65f, 1.0f));
          getSSRender().drawText("CKPT", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, k);
      }

        // show Current/Total laps
        if (game->number_of_laps > 1)
        {
            const std::string currentlap = std::to_string(vehic->currentlap);
            const std::string number_of_laps = std::to_string(game->number_of_laps);

            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glm::mat4 k = glm::translate(glm::mat4(1.0f), glm::vec3(hratio - hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f) - 0.20f, 0.0f));
            k = glm::scale(k, glm::vec3(0.125f, 0.125f, 1.0f));

            if (game->getFinishState() != Gamefinish::not_finished)
                getSSRender().drawText(number_of_laps + '/' + number_of_laps, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, k);
            else
                getSSRender().drawText(currentlap + '/' + number_of_laps, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, k);

            k = glm::translate(k, glm::vec3(0, 0.52f, 0.0f));
            k = glm::scale(k, glm::vec3(0.65f, 0.65f, 1.0f));
            getSSRender().drawText("LAP", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, k);
        }

#ifdef INDEVEL
        // show codriver checkpoint text (the pace notes)
        if (!game->codrivercheckpt.empty() && vehic->nextcdcp != 0)
        {
            glColor3f(1.0f, 1.0f, 0.0f);
            glm::mat4 k = glm::translate(t, glm::vec3(0.0f, 0.3f, 0.0f));
            k = glm::scale(k, glm::vec3(0.1f, 0.1f, 1.0f));
            getSSRender().drawText(game->codrivercheckpt[vehic->nextcdcp - 1].notes, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, k);
        }
#endif

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

      tex_fontSourceCodeBold->bind();

      // show current gear and speed
      {
          // gear number
          const int gear = vehic->getCurrentGear();
          const std::string buff = (gear >= 0) ? PUtil::formatInt(gear + 1, 1) : "R";

          // position of gear & speed number & label
          //glTranslatef( hratio * (1.f - (5.75f/50.f)) - 0.3f, -vratio * (40.f/50.f) + 0.21f, 0.0f);
          glm::mat4 k = glm::translate(glm::mat4(1.0f), glm::vec3(hratio * (1.f - (2.5f/50.f)) - 0.3f, -vratio * (43.5f/50.f) + 0.21f, 0.0f));
          k = glm::scale(k, glm::vec3(0.20f, 0.20f, 1.0f));
          getSSRender().drawText(buff, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, k);

          // speed number
          const int speed = std::fabs(vehic->getWheelSpeed()) * hud_speedo_mps_speed_mult;
          std::string speedstr = std::to_string(speed);

          //glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
          k = glm::translate(k, glm::vec3(1.1f, -0.625f, 0.0f));
          k = glm::scale(k, glm::vec3(0.5f, 0.5f, 1.0f));
          getSSRender().drawText(speedstr, PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, k);

          // speed label
          k = glm::translate(k, glm::vec3(0.0f, -0.82f, 0.0f));
          k = glm::scale(k, glm::vec3(0.5f, 0.5f, 1.0f));

          if (cfg_speed_unit == MainApp::Speedunit::mph)
              getSSRender().drawText("MPH", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, k);
          else
              getSSRender().drawText("km/h", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, k);
      }

  #ifndef NDEBUG
      // draw revs for debugging
      {
      glm::mat4 k = glm::translate(t, glm::vec3(1.17f, 0.52f, 0.0f));
      k = glm::scale(k, glm::vec3(0.2f, 0.2f, 1.0f));
      getSSRender().drawText(std::to_string(vehic->getEngineRPM()), PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, k);
      }
  #endif

#ifndef NDEBUG
    // draw real time penalty for debugging
    {
    glm::mat4 k = glm::scale(t, glm::vec3(0.1f, 0.1f, 1.0f));
    k = glm::translate(k, glm::vec3(0.0f, -4.0f, 0.0f));
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    tex_fontSourceCodeOutlined->bind();
    getSSRender().drawText(std::string("true time penalty: ") +
        std::to_string(game->offroadtime_total * game->offroadtime_penalty_multiplier),
        PTEXT_HZA_CENTER | PTEXT_VTA_TOP, k);
    }
#endif

    tex_fontSourceCodeShadowed->bind();

    // draw "off road" warning sign and text
    if (game->isRacing())
    {
        //const vec3f bodypos = vehic->part[0].ref_world.getPosition();
        const vec3f bodypos = vehic->body->getPosition();

        if (!game->terrain->getRmapOnRoad(bodypos))
        {
            glPushMatrix();
            glLoadIdentity();
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glScalef(0.25f, 0.25f, 1.0f);
            tex_hud_offroad->bind();
            glBegin(GL_QUADS);
                glTexCoord2f(   1.0f,   1.0f);
                glVertex2f(     1.0f,   1.0f);
                glTexCoord2f(   0.0f,   1.0f);
                glVertex2f(    -1.0f,   1.0f);
                glTexCoord2f(   0.0f,   0.0f);
                glVertex2f(    -1.0f,  -1.0f);
                glTexCoord2f(   1.0f,   0.0f);
                glVertex2f(     1.0f,  -1.0f);
            glEnd();
            glPopMatrix();

            glm::mat4 k = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 1.0f));
            k = glm::translate(k, glm::vec3(0.0f, -2.5f, 0.0f));
            glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
            tex_fontSourceCodeOutlined->bind();
            getSSRender().drawText(
                std::to_string(static_cast<int> (game->getOffroadTime() * game->offroadtime_penalty_multiplier)) +
                " seconds",
                PTEXT_HZA_CENTER | PTEXT_VTA_TOP, k);
        }
    }

    // draw terrain info for debugging
    #ifdef INDEVEL
    {
        const vec3f wheelpos = vehic->part[0].wheel[0].ref_world.getPosition(); // wheel 0
        const TerrainType tt = game->terrain->getRoadSurface(wheelpos);
        const rgbcolor c = PUtil::getTerrainColor(tt);
        const std::string s = PUtil::getTerrainInfo(tt);

        glPushMatrix(); // 2
        glTranslatef(0.0f, 0.5f, 0.0f);
        glScalef(0.1f, 0.1f, 1.0f);

        if (tt != TerrainType::Unknown)
        {
            const GLfloat endx = s.length() * 8.0f / 12.0f + 0.1f;

            glPushMatrix();
            glDisable(GL_TEXTURE_2D);
            glTranslatef(-0.5f * s.length() * 8.0f / 12.0f, 0.0f, 0.0f);
            glTranslatef(0.0f, -1.0f, 0.0f);
            glBegin(GL_TRIANGLE_STRIP);
                glColor3f(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
                glVertex2f(-0.2f,   0.0f);
                glVertex2f(endx,    0.0f);
                glVertex2f(-0.2f,   1.1f);
                glVertex2f(endx,    1.1f);
            glEnd();
            glEnable(GL_TEXTURE_2D);
            glPopMatrix();
        }

        glColor3f(1.0f, 1.0f, 1.0f);
        getSSRender().drawText(s, PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
        glPopMatrix(); // 2
    }
    #endif

    // draw if we're on road for debugging
    //#ifdef INDEVEL
    #if 0
    {
        const vec3f wheelpos = vehic->part[0].wheel[0].ref_world.getPosition(); // wheel 0
        std::string s;
        rgbcolor c;

        if (game->terrain->getRmapOnRoad(wheelpos))
        {
            c = rgbcolor(0xFF, 0xFF, 0xFF);
            s = "on the road";
        }
        else
        {
            c = rgbcolor(0x00, 0x00, 0x00);
            s = "off-road";
        }

        const GLfloat endx = s.length() * 8.0f / 12.0f + 0.1f;

        glPushMatrix(); // 2
        glTranslatef(0.0f, 0.25f, 0.0f);
        glScalef(0.1f, 0.1f, 1.0f);
        glPushMatrix();
        glDisable(GL_TEXTURE_2D);
        glTranslatef(-0.5f * s.length() * 8.0f / 12.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, -1.0f, 0.0f);
        glBegin(GL_TRIANGLE_STRIP);
            glColor3f(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
            glVertex2f(-0.2f,   0.0f);
            glVertex2f(endx,    0.0f);
            glVertex2f(-0.2f,   1.1f);
            glVertex2f(endx,    1.1f);
        glEnd();
        glEnable(GL_TEXTURE_2D);
        glPopMatrix();
        glColor3f(1.0f, 1.0f, 1.0f);
        getSSRender().drawText(s, PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
        glPopMatrix(); // 2
    }
    #endif

      tex_fontSourceCodeOutlined->bind();

      glColor4f(1.0f, 0.0f, 0.0f, 1.0f);

      glm::mat4 q = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.2f, 0.0f));
      q = glm::scale(q, glm::vec3(0.6f, 0.6f, 1.0f));

      if (game->gamestate == Gamestate::countdown)
      {
          float sizer = fmodf(game->othertime, 1.0f) + 0.5f;
          glm::mat4 k = glm::scale(q, glm::vec3(sizer, sizer, 1.0f));
          getSSRender().drawText(
              PUtil::formatInt(((int)game->othertime + 1), 1),
              PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, k);
      }
      else if (game->gamestate == Gamestate::finished)
      {
          if (game->getFinishState() == Gamefinish::pass)
          {
              glColor4f(0.5f, 1.0f, 0.5f, 1.0f);
              getSSRender().drawText("WIN", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, q);
          }
          else
          {
              glm::mat4 k = glm::scale(q, glm::vec3(0.5f, 0.5f, 1.0f));
              glColor4f(0.5f, 0.0f, 0.0f, 1.0f);
              getSSRender().drawText("TIME EXCEEDED", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, k);
          }
      }
      else if (game->coursetime < 1.0f)
      {
          glColor4f(0.5f, 1.0f, 0.5f, 1.0f);
          getSSRender().drawText("GO!", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, q);
      }
      else if (game->coursetime < 2.0f)
      {
          float a = 1.0f - (game->coursetime - 1.0f);
          glColor4f(0.5f, 1.0f, 0.5f, a);
          getSSRender().drawText("GO!", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, q);
      }

      if (game->gamestate == Gamestate::countdown)
      {
          glm::mat4 k = glm::translate(q, glm::vec3(0.0f, 0.6f, 0.0f));
          k = glm::scale(k, glm::vec3(0.08f, 0.08f, 1.0f));
          if (game->othertime < 1.0f)
          {
              glColor4f(1.0f, 1.0f, 1.0f, game->othertime);
              getSSRender().drawText(game->comment, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, k);
          }
          else
          {
              glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
              getSSRender().drawText(game->comment, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, k);
          }
      }

        if (pauserace)
        {
            glm::mat4 k = glm::scale(q, glm::vec3(0.25f, 0.25f, 1.0f));
            glColor4f(0.25f, 0.25f, 1.0f, 1.0f);
            getSSRender().drawText("PAUSED", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, k);
        }
    }

    glPopMatrix(); // 0

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glBlendFunc(GL_ONE, GL_ZERO);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}
