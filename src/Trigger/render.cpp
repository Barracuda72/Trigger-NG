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
    //glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE,GL_ZERO);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glClearDepthf(1.0f);

    glEnable(GL_CULL_FACE);

    glViewport(0, 0, getWidth(), getHeight());

    default_light.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
    default_light.diffuse = glm::vec3(0.6f, 0.6f, 0.6f);
    default_light.specular = glm::vec3(0.6f, 0.6f, 0.6f);

    default_material.ambient = glm::vec3(1.0f, 1.0f, 1.0f);
    default_material.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    default_material.specular = glm::vec3(0.3f, 0.5f, 0.5f);
    default_material.shininess = 6.0f;
}

void MainApp::renderWater(const glm::mat4& mv, const glm::mat4& p)
{
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    float alpha = 0.5f, maxalpha = 0.5f;

    if (game->water.useralpha)
        alpha = maxalpha = game->water.alpha;

    {
        int off_x = (int)(campos.x / 20.0);
        int off_y = (int)(campos.y / 20.0);

        glm::mat4 t(1.0f);

        t = glm::scale(mv, glm::vec3(20.0f, 20.0f, 1.0f));
        t = glm::translate(t, glm::vec3(off_x, off_y, 0.0f));

        int minx = - 20,
            maxx = + 20,
            miny = - 20,
            maxy = + 20;

        int x_stride = maxx - minx;

        float* vbo = new float[(maxx-minx)*(maxy-miny+1)*12];
        memset(vbo, 0, (maxx-minx)*(maxy-miny+1)*12*sizeof(float));
        unsigned short* ibo = new unsigned short[(maxx-minx+1)*(maxy-miny)*2];

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

        VAO vao(
            vbo, (maxx-minx)*(maxy-miny+1)*12*sizeof(float),
            ibo, (maxx-minx+1)*(maxy-miny)*2*sizeof(unsigned short)
        );

        vao.bind();

        sp_water->use();
        sp_water->attrib("tex_coord", 2, GL_FLOAT, GL_FALSE, 12 * sizeof(GL_FLOAT), 0);
        sp_water->attrib("color", 4, GL_FLOAT, GL_FALSE, 12 * sizeof(GL_FLOAT), 2 * sizeof(GL_FLOAT));
        sp_water->attrib("normal", 3, GL_FLOAT, GL_FALSE, 12 * sizeof(GL_FLOAT), 6 * sizeof(GL_FLOAT));
        sp_water->attrib("position", 3, GL_FLOAT, GL_FALSE, 12 * sizeof(GL_FLOAT), 9 * sizeof(GL_FLOAT));

        sp_water->uniform("mv", t);
        sp_water->uniform("p", p);
        glActiveTexture(GL_TEXTURE0);
        tex_water->bind();
        sp_water->uniform("water", 0);

        glDrawElements(GL_TRIANGLE_STRIP, (maxx-minx+1)*(maxy-miny)*2, GL_UNSIGNED_SHORT, 0);

        sp_water->unuse();
        delete[] ibo;
        delete[] vbo;
    }

    glBlendFunc(GL_ONE,GL_ZERO);
}

void MainApp::buildSkyVao()
{
    const int CLRANGE = 10;
    const float CLFACTOR = 0.02f; //0.014

    int x_stride = 2 * CLRANGE + 1;
    float* vbo = new float[(2 * CLRANGE + 1)*(2 * CLRANGE)*5];
    unsigned short* ibo = new unsigned short[(2 * CLRANGE + 2)*(2 * CLRANGE)*2];

    for (int y=-CLRANGE; y<CLRANGE; y++)
    {
        int y3 = y + CLRANGE;
        for (int x=-CLRANGE; x<CLRANGE+1; x++)
        {
            int x3 = x + CLRANGE;
            vbo[(y3 * x_stride + x3) * 5 + 0] = x;
            vbo[(y3 * x_stride + x3) * 5 + 1] = y;
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

    sky_vao = new VAO(
        vbo, 5 * sizeof(float) * (2 * CLRANGE + 1)*(2 * CLRANGE),
        ibo, (2 * CLRANGE + 2)*(2 * CLRANGE)*2 * sizeof(unsigned short)
    );

    sky_size = (2 * CLRANGE + 1)*(2 * CLRANGE)*2;

    delete[] vbo;
    delete[] ibo;
}

void MainApp::renderSky(const glm::mat4 &cammat, const glm::mat4& p)
{
    glDepthRangef(0.999f, 1.0f);
    glDisable(GL_CULL_FACE);

    vec3f& fc = game->weather.fog.color;
    glm::vec3 fog_color(fc[0], fc[1], fc[2]);

    glm::mat4 t(1.0);
    t = glm::translate(t, glm::vec3(cloudscroll, 0.0f, 0.0f));
    t = glm::rotate(t, 30.0f / 360.0f * 2 * 3.141592653f, glm::vec3(0.0f, 0.0f, 1.0f));
    t = glm::scale(t, glm::vec3(0.4f, 0.4f, 1.0f));

    sp_sky->use();
    sky_vao->bind();

    sp_sky->attrib("tex_coord", 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
    sp_sky->attrib("vert_coord", 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 2 * sizeof(float));

    sp_sky->uniform("t_transform", t);
    sp_sky->uniform("fog_density", game->weather.fog.density_sky);
    sp_sky->uniform("fog_color", fog_color);

    glActiveTexture(GL_TEXTURE0);
    tex_sky[0]->bind();
    sp_sky->uniform("tex", 0);
    sp_sky->uniform("mv", cammat);
    sp_sky->uniform("p", p);

    glDrawElements(GL_TRIANGLE_STRIP, sky_size, GL_UNSIGNED_SHORT, 0);
    sp_sky->unuse();
    sky_vao->unbind();

    glEnable(GL_CULL_FACE);
    glDepthRangef(0.0f, 0.999f);
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

void MainApp::renderTexturedFullscreenQuad(const glm::mat4& p)
{
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3((float)getWidth()/(float)getHeight(), 1.0f, 1.0f));
    renderTexturedFullscreenQuad(scale, p);
}

void MainApp::renderTexturedFullscreenQuad(const glm::mat4& mv, const glm::mat4& p)
{
  bckgnd_vao->bind();

  sp_bckgnd->use();

  sp_bckgnd->attrib("tex_coord", 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 0);
  sp_bckgnd->attrib("position", 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 2 * sizeof(GL_FLOAT));

  sp_bckgnd->uniform("mv", mv);
  sp_bckgnd->uniform("p", p);
  glActiveTexture(GL_TEXTURE0);
  sp_bckgnd->uniform("background", 0);

  {
  // the background image is square and cut out a piece based on aspect ratio
  //glInterleavedArrays(GL_T2F_V3F, 5 * sizeof(GL_FLOAT), vbo);
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
  }
  sp_bckgnd->unuse();
  bckgnd_vao->unbind();
}

void MainApp::renderStateLoading(float eyetranslation)
{
    UNREFERENCED_PARAMETER(eyetranslation);

    glm::mat4 o = glm::ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    tex_splash_screen->bind();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderTexturedFullscreenQuad(o);

    tex_loading_screen->bind();

    glm::mat4 s = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, (float)getWidth()/(float)getHeight(), 1.0f));
    s = glm::scale(s, glm::vec3(1.0f/3.5f, 1.0f/3.5f, 1.0f));
    renderTexturedFullscreenQuad(s, o);

    glEnable(GL_DEPTH_TEST);
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

    glm::mat4 o = glm::ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    tex_end_screen->bind();

    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ZERO);

    renderTexturedFullscreenQuad(o);

    tex_fontSourceCodeOutlined->bind();

    o = glm::ortho(0 - hratio, hratio, 0 - vratio, vratio, 0 - 1.0, 1.0);

    //glOrtho(-1, 1, -1, 1, -1, 1);
    //glOrtho(800, 0, 600, 0, -1, 1);

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

            glm::vec4 cl = glm::vec4(1.0f, 1.0f, 1.0f, level);

            getSSRender().drawText(creditstext[i], cl, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, t * q, o);
        }
    }

    glEnable(GL_DEPTH_TEST);
}

// FIXME: following two functions are almost the same
// They shall be merged, but this requires tinkering with a game logic
void MainApp::renderVehicleType(PVehicleType* vtype, const glm::mat4& mv, const glm::mat4& p)
{
   for (unsigned int i=0; i<vtype->part.size(); ++i)
        {
            vec3f vpos = vtype->part[i].render_ref_local.pos;
            mat44f vorim = vtype->part[i].render_ref_local.ori_mat_inv;

            glm::mat4 t = glm::translate(mv, glm::vec3(vpos.x, vpos.y, vpos.y));
            glm::mat4 q = {
                vorim.row[0][0], vorim.row[1][0], vorim.row[2][0], vorim.row[3][0],
                vorim.row[0][1], vorim.row[1][1], vorim.row[2][1], vorim.row[3][1],
                vorim.row[0][2], vorim.row[1][2], vorim.row[2][2], vorim.row[3][2],
                vorim.row[0][3], vorim.row[1][3], vorim.row[2][3], vorim.row[3][3],
            };
            t = glm::transpose(q) * t;
            if (vtype->part[i].model)
            {
                float scale = vtype->part[i].scale;
                glm::mat4 s = glm::scale(t, glm::vec3(scale,scale,scale));
                drawModel(*vtype->part[i].model, default_light, default_material, s, p);
            }

            if (vtype->wheelmodel)
            {
                for (unsigned int j=0; j<vtype->part[i].wheel.size(); j++)
                {
                    vec3f &wpos = vtype->part[i].wheel[j].pt;
                    float scale = vtype->wheelscale * vtype->part[i].wheel[j].radius;
                    glm::mat4 k = glm::translate(t, glm::vec3(wpos.x, wpos.y, wpos.z));
                    k = glm::scale(k, glm::vec3(scale,scale,scale));

                    drawModel(*vtype->wheelmodel, default_light, default_material, k, p);
                }
            }
        }
}

void MainApp::renderVehicle(PVehicle* vehic, const glm::mat4& mv, const glm::mat4& p)
{
    for (unsigned int i=0; i<vehic->part.size(); ++i)
    {
        if (vehic->type->part[i].model)
        {
            vec3f vpos = vehic->part[i].ref_world.pos;
            mat44f vorim = vehic->part[i].ref_world.ori_mat_inv;
            float scale = vehic->type->part[i].scale;

            glm::mat4 t = glm::translate(mv, glm::vec3(vpos.x, vpos.y, vpos.z));
            glm::mat4 q = {
                vorim.row[0][0], vorim.row[1][0], vorim.row[2][0], vorim.row[3][0],
                vorim.row[0][1], vorim.row[1][1], vorim.row[2][1], vorim.row[3][1],
                vorim.row[0][2], vorim.row[1][2], vorim.row[2][2], vorim.row[3][2],
                vorim.row[0][3], vorim.row[1][3], vorim.row[2][3], vorim.row[3][3],
            };
            t = t * glm::transpose(q);
            t = glm::scale(t, glm::vec3(scale, scale, scale));

            drawModel(*vehic->type->part[i].model, default_light, default_material, t, p);
        }

        if (vehic->type->wheelmodel)
        {
            for (unsigned int j=0; j<vehic->type->part[i].wheel.size(); j++)
            {
                vec3f wpos = vehic->part[i].wheel[j].ref_world.getPosition();
                mat44f worim = vehic->part[i].wheel[j].ref_world.ori_mat_inv;
                float scale = vehic->type->wheelscale * vehic->type->part[i].wheel[j].radius;

                glm::mat4 t = glm::translate(mv, glm::vec3(wpos.x, wpos.y, wpos.z));
                glm::mat4 q = {
                    worim.row[0][0], worim.row[1][0], worim.row[2][0], worim.row[3][0],
                    worim.row[0][1], worim.row[1][1], worim.row[2][1], worim.row[3][1],
                    worim.row[0][2], worim.row[1][2], worim.row[2][2], worim.row[3][2],
                    worim.row[0][3], worim.row[1][3], worim.row[2][3], worim.row[3][3],
                };
                t = t * glm::transpose(q);
                t = glm::scale(t, glm::vec3(scale, scale, scale));

                drawModel(*vehic->type->wheelmodel, default_light, default_material, t, p);
            }
        }
    }
}

// render the car selection menu
void MainApp::renderStateChoose(float eyetranslation)
{
    PVehicleType *vtype = game->vehiclechoices[choose_type];

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glm::mat4 o = glm::ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

  // draw background image

  glBlendFunc(GL_ONE, GL_ZERO);
  glDisable(GL_DEPTH_TEST);

  tex_splash_screen->bind();

  renderTexturedFullscreenQuad(o);

    float fnear = 0.1f, fov = 0.6f;
    float aspect = (float)getWidth() / (float)getHeight();
    glm::mat4 p = stereoFrustum(-fnear*aspect*fov,fnear*aspect*fov,-fnear*fov,fnear*fov,fnear,100000.0f,
                  0.8f, eyetranslation);

    glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(-eyetranslation, 0.9f, -5.0f));
    t = glm::rotate(t, 28.0f * 3.141592653f / 180.0f, glm::vec3(1.0f, 0.0f, 0.0f));

    glEnable(GL_DEPTH_TEST);

    default_light.position = glm::vec3(0.0f, 1.0f, 0.0f);

    //float tmp = 1.0f;
    //float tmp = sinf(choose_spin * 2.0f) * 0.5f;
    float tmp = cosf(choose_spin * 2.0f) * 0.5f;
    tmp += choose_spin;

    t = glm::rotate(t, 3.141592653f / 2.0f, glm::vec3(-1.0f, 0.0f, 0.0f));
    t = glm::rotate(t, tmp, glm::vec3(0.0f, 0.0f, 1.0f));

    renderVehicleType(vtype, t, p);

    // use the same colors as the menu
    const GuiWidgetColors gwc = gui.getColors();

    tex_fontSourceCodeShadowed->bind();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    const GLdouble margin = (800.0 - 600.0 * cx / cy) / 2.0;

    glm::mat4 o2 = glm::ortho(margin, 600.0 * cx / cy + margin, 0.0, 600.0, -1.0, 1.0);

    glm::mat4 scale_small = glm::scale(glm::mat4(1.0f), glm::vec3(20.0f, 20.0f, 1.0f));
    glm::mat4 scale_big = glm::scale(glm::mat4(1.0f), glm::vec3(30.0f, 30.0f, 1.0f));

    glm::mat4 q(1.0f);

    glm::vec4 cl_weak = glm::vec4(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    glm::vec4 cl_strong = glm::vec4(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    glm::vec4 cl_header = glm::vec4(gwc.header.x, gwc.header.y, gwc.header.z, gwc.header.w);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 570.0f, 0.0f));
    getSSRender().drawText("Trigger Rally", cl_weak, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, q * scale_big, o2);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(790.0f, 570.0f, 0.0f));
    getSSRender().drawText(
        "car selection " + std::to_string(choose_type + 1) + '/' + std::to_string(game->vehiclechoices.size()),
        cl_weak, PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, q * scale_small, o2);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(100.0f, 230.0f, 0.0f));
    getSSRender().drawText(vtype->proper_name.substr(0, 9), cl_header, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, q * scale_big, o2);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(100.0f, 200.0f, 0.0f));
    getSSRender().drawText(vtype->proper_class.substr(0, 8), cl_strong, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, q * scale_small, o2);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(500.0f, 230.0f, 0.0f));
    getSSRender().drawText("Weight (Kg)", cl_weak, PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, q * scale_small, o2);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(500.0f, 190.0f, 0.0f));
    getSSRender().drawText("Engine (BHP)", cl_weak, PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, q * scale_small, o2);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(500.0f, 150.0f, 0.0f));
    getSSRender().drawText("Wheel drive", cl_weak, PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, q * scale_small, o2);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(500.0f, 110.0f, 0.0f));
    getSSRender().drawText("Roadholding", cl_weak, PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, q * scale_small, o2);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(520.0f, 230.0f, 0.0f));
    getSSRender().drawText(std::to_string(static_cast<int>(vtype->mass)), cl_strong, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, q * scale_big, o2);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(520.0f, 190.0f, 0.0f));
    getSSRender().drawText(vtype->pstat_enginepower, cl_strong, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, q * scale_big, o2);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(520.0f, 150.0f, 0.0f));
    getSSRender().drawText(vtype->pstat_wheeldrive, cl_strong, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, q * scale_big, o2);

    q = glm::translate(glm::mat4(1.0f), glm::vec3(520.0f, 110.0f, 0.0f));
    getSSRender().drawText(vtype->pstat_roadholding, cl_strong, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, q * scale_big, o2);

    std::string racename;

    if (lss.state == AM_TOP_EVT_PREP || lss.state == AM_TOP_PRAC_SEL_PREP)
        racename = events[lss.currentevent].name + ": " + events[lss.currentevent].levels[lss.currentlevel].name;
    else
    if (lss.state == AM_TOP_LVL_PREP)
        racename = levels[lss.currentlevel].name;

    q = glm::translate(glm::mat4(1.0f), glm::vec3(400.0f, 30.0f, 0.0f));
    getSSRender().drawText(racename, cl_weak, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, q * scale_small, o2);

    glBlendFunc(GL_ONE, GL_ZERO);
    glEnable(GL_DEPTH_TEST);
}

void MainApp::renderRpmDial(float rpm, const glm::mat4& p)
{
      rpm_dial_vao->bind();
      sp_rpm_dial->use();
      sp_rpm_dial->attrib("tex_coord", 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 0);
      sp_rpm_dial->attrib("position", 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 2 * sizeof(GL_FLOAT));

      // position of rpm dial and needle
      //glTranslatef( hratio * (1.f - (5.75f/50.f)) - 0.3f, -vratio * (40.f/50.f) + 0.22f, 0.0f);

      glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(hratio * (1.f - (2.5f/50.f)) - 0.3f, -vratio * (43.5f/50.f) + 0.22f, 0.0f));
      t = glm::scale(t, glm::vec3(0.30f, 0.30f, 1.0f));

      glActiveTexture(GL_TEXTURE0);
      tex_hud_revs->bind();

      sp_rpm_dial->uniform("mv", t);
      sp_rpm_dial->uniform("p", p);
      sp_rpm_dial->uniform("dial", 0);
      glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
      sp_rpm_dial->unuse();

      sp_rpm_needle->use();
      sp_rpm_needle->attrib("tex_coord", 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 0);
      sp_rpm_needle->attrib("position", 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 2 * sizeof(GL_FLOAT));
      // draw the needle of the RPM dial
      const float my_pi = 3.67f; // TODO: I know that's stupid, but... whatever
      t = glm::rotate(t, (my_pi / 4 * 5) - (rpm / 1000.0f) * (my_pi / 12.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      t = glm::translate(t, glm::vec3(0.62f, 0.0f, 0.0f));
      t = glm::scale(t, glm::vec3(0.16f, 0.16f, 0.16f));

      glActiveTexture(GL_TEXTURE0);
      tex_hud_revneedle->bind();

      sp_rpm_needle->uniform("mv", t);
      sp_rpm_needle->uniform("p", p);
      sp_rpm_needle->uniform("needle", 0);

      glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);

      sp_rpm_needle->unuse();

      //glDisable(GL_TEXTURE_2D); // TODO: WTH it's here???

      rpm_dial_vao->unbind();
}

void MainApp::renderMapMarker(const glm::vec2& vpos, float angle, const glm::vec4& col, float sc, const glm::mat4& mv, const glm::mat4& p)
{
    glm::mat4 t(1.0f);

    t = glm::translate(mv, glm::vec3(vpos.x, vpos.y, 0.0f));
    t = glm::rotate(t, angle, glm::vec3(0.0f, 0.0f, 1.0f));
    t = glm::scale(t, glm::vec3(30.0f, 30.0f, 1.0f));
    t = glm::scale(t, glm::vec3(sc, sc, 1.0f));

    map_marker_vao->bind();

    sp_map_marker->use();

    sp_map_marker->attrib("position", 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    sp_map_marker->attrib("alpha", 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 2 * sizeof(float));

    sp_map_marker->uniform("v_color", col);
    sp_map_marker->uniform("mv", t);
    sp_map_marker->uniform("p", p);

    glDrawElements(GL_TRIANGLE_FAN, 6, GL_UNSIGNED_SHORT, 0);

    sp_map_marker->unuse();
    map_marker_vao->unbind();
}

void MainApp::renderMap(int nextcp, const glm::mat4& p)
{
        // position and size of map
        //glViewport(getWidth() * (5.75f/100.f), getHeight() * (6.15f/100.f), getHeight()/3.5f, getHeight()/3.5f);
        glViewport(getWidth() * (2.5f/100.f), getHeight() * (2.5f/100.f), getHeight()/3.5f, getHeight()/3.5f);

        glm::mat4 s = glm::scale(glm::mat4(1.0f), glm::vec3(hratio, vratio, 1.0f));

        if (game->terrain->getHUDMapTexture())
        {
            //glEnable(GL_TEXTURE_2D);
            game->terrain->getHUDMapTexture()->bind();
        }

        float scalefac = 1.0f / game->terrain->getMapSize();
        glm::mat4 tex_t(1.0f);
        tex_t = glm::scale(tex_t, glm::vec3(scalefac, scalefac, 1.0f));
        tex_t = glm::translate(tex_t, glm::vec3(campos.x, campos.y, 0.0f));
        tex_t = glm::rotate(tex_t, camera_angle, glm::vec3(0.0f, 0.0f, 1.0f));
        tex_t = glm::scale(tex_t, glm::vec3(1.0f / 0.003f, 1.0f / 0.003f, 1.0f));

        map_vao->bind();
        sp_map->use();
        sp_map->attrib("position", 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GL_FLOAT), 0);

        glActiveTexture(GL_TEXTURE0);
        sp_map->uniform("map_texture", 0);
        sp_map->uniform("tex_transform", tex_t);
        sp_map->uniform("mv", s);
        sp_map->uniform("p", p);

        glDrawElements(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_SHORT, 0);
        sp_map->unuse();
        map_vao->unbind();

        //glDisable(GL_TEXTURE_2D);

        glm::mat4 t(1.0f);
        t = glm::scale(s, glm::vec3(0.003f, 0.003f, 1.0f));
        t = glm::rotate(t, -camera_angle, glm::vec3(0.0f, 0.0f, 1.0f));
        t = glm::translate(t, glm::vec3(-campos.x, -campos.y, 0.0f));

        for (unsigned int i=0; i<game->checkpt.size(); i++)
        {
            vec3f vpos = game->checkpt[i].pt;
            float sc = 1.0f;

            vec4f colr = checkpoint_col[2];

            if ((int)i == nextcp)
            {
                sc = 1.5f + sinf(cprotate * 10.0f) * 0.5f;
                colr = checkpoint_col[0];
            }
            else if ((int)i == (nextcp + 1) % (int)game->checkpt.size())
            {
                colr = checkpoint_col[1];
            }

            glm::vec2 v_pos = glm::vec2(vpos.x, vpos.y);
            glm::vec4 col = glm::vec4(colr[0], colr[1], colr[2], colr[3]);
            renderMapMarker(v_pos, camera_angle, col, sc, t, p);
        }
        for (unsigned int i=0; i<game->vehicle.size(); i++)
        {
            vec3f vpos = game->vehicle[i]->body->getPosition();
            glm::vec2 v_pos = glm::vec2(vpos.x, vpos.y);
            glm::vec4 col = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            renderMapMarker(v_pos, camera_angle, col, 1.0f, t, p);
        }

        glViewport(0, 0, getWidth(), getHeight());
}

void MainApp::renderStateGame(float eyetranslation)
{
    PVehicle *vehic = game->vehicle[0];

    glClear(GL_DEPTH_BUFFER_BIT);
    //glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    float fnear = 0.1f, fov = 0.6f;
    float aspect = (float)getWidth() / (float)getHeight();
    glm::mat4 p = stereoFrustum(-fnear*aspect*fov,fnear*aspect*fov,-fnear*fov,fnear*fov,fnear,100000.0f,
                  0.8f, eyetranslation);

    glDepthRangef(0.0f, 0.999f);

    glm::vec3 campos_gl = glm::vec3(campos.x, campos.y, campos.z);

    glm::mat4 cammat = camori.getGLMatrix();
    glm::mat4 cammat_inv = glm::transpose(cammat);

    //glTranslatef(0.0,0.0,-40.0);
    glm::mat4 mv = glm::translate(glm::mat4(1.0f), glm::vec3(-eyetranslation, 0.0f, 0.0f));

    mv = cammat * mv;

    mv = glm::translate(mv, -campos_gl);

    default_light.position = glm::vec3(0.2f, 0.5f, 1.0f);

    // draw terrain
    game->terrain->render(campos_gl, cammat_inv, tex_detail, game->weather.fog.color, game->weather.fog.density, mv, p);

    if (renderowncar)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        vec3f vpos = game->vehicle[0]->body->pos;
        vec3f forw = makevec3f(game->vehicle[0]->body->getOrientationMatrix().row[0]);
        float forwangle = atan2(forw.y, forw.x);
        game->terrain->drawShadow(vpos.x, vpos.y, 1.4f, forwangle + PI*0.5f, tex_shadow, mv, p);

        glBlendFunc(GL_ONE, GL_ZERO);
    }

    renderSky(cammat, p);

    for (unsigned int v=0; v<game->vehicle.size(); ++v)
    {

        if (!renderowncar && v == 0) continue;

        PVehicle *vehic = game->vehicle[v];

        renderVehicle(vehic, mv, p);
    }

    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    //glDisable(GL_TEXTURE_2D);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderRain(mv, p);

    renderSnow(mv, p);

    if (showcheckpoint)
    {
        renderCheckpoints(vehic->nextcp, mv, p);
    }

    //glEnable(GL_TEXTURE_2D);

    if (game->water.enabled)
        renderWater(mv, p);

    if (psys_dirt != nullptr) // cfg_dirteffect == false
        getSSRender().render(psys_dirt, mv, p);

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_ONE,GL_ZERO);
    glEnable(GL_CULL_FACE);

    glDisable(GL_DEPTH_TEST);

    glm::mat4 o = glm::ortho(0 - hratio, hratio, 0 - vratio, vratio, 0 - 1.0, 1.0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (showui)
    {
        game->renderCodriverSigns(glm::mat4(1.0f), o);

        renderRpmDial(vehic->getEngineRPM(), o);
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
        renderMap(vehic->nextcp, o);
    }

    //glEnable(GL_TEXTURE_2D);

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

      // time position (other time strings inherit this position)
      // -hratio is left border, 0 is center, +hratio is right
      // hratio * (1/50) gives 1% of the entire width
      // +vratio is top border, 0 is middle, -vratio is bottom

      {
      glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(-hratio + hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f), 0.0f));
      t = glm::scale(t, glm::vec3(0.125f, 0.125f, 1.0f));

      if (game->gamestate == Gamestate::finished)
      {
          glm::vec4 cl(1.0f, 1.0f, 1.0f, 1.0f);
          getSSRender().drawText(
              PUtil::formatTime(game->coursetime), cl,
              PTEXT_HZA_LEFT | PTEXT_VTA_TOP, t, o);
      }
      else if (game->coursetime < game->cptime + 1.50f)
      {
          glm::vec4 cl(1.0f, 1.0f, 1.0f, 1.0f);
          getSSRender().drawText(
              PUtil::formatTime(game->cptime), cl,
              PTEXT_HZA_LEFT | PTEXT_VTA_TOP, t, o);
      }
      else if (game->coursetime < game->cptime + 3.50f)
      {
          float a = (((game->cptime + 3.50f) - game->coursetime) / 2);
          glm::vec4 cl(1.0f, 1.0f, 1.0f, a);
          getSSRender().drawText(
              PUtil::formatTime(game->cptime), cl,
              PTEXT_HZA_LEFT | PTEXT_VTA_TOP, t, o);
      }
      else
      {
          glm::vec4 cl(1.0f, 1.0f, 1.0f, 1.0f);
          getSSRender().drawText(
              PUtil::formatTime(game->coursetime), cl,
              PTEXT_HZA_LEFT | PTEXT_VTA_TOP, t, o);
      }

      // show target time

      glm::vec4 cl_target_time(0.5f, 1.0f, 0.5f, 1.0f);

      glm::mat4 q = glm::translate(t, glm::vec3(0.0f, -0.8f, 0.0f));
      getSSRender().drawText(PUtil::formatTime(game->targettime), cl_target_time, PTEXT_HZA_LEFT | PTEXT_VTA_TOP, q, o);

    {
        // show the time penalty if there is any
        const float timepen = game->uservehicle->offroadtime_total * game->offroadtime_penalty_multiplier;

        if (timepen >= 0.1f)
        {
            glm::vec4 cl_time_pen(1.0f, 1.0f, 0.5f, 1.0f);
            glm::mat4 p = glm::translate(q, glm::vec3(0.0f, -1.60f, 0.0f));
            getSSRender().drawText(PUtil::formatTime(timepen) + '+', cl_time_pen, PTEXT_HZA_LEFT | PTEXT_VTA_TOP, p, o);
        }
    }

      // time label
    {
      glm::vec4 cl_time(1.0f, 1.0f, 1.0f, 1.0f);
      glm::mat4 k = glm::translate(t, glm::vec3(0.0f, .52f, 0.0f));
      k = glm::scale(k, glm::vec3(0.65f, 0.65f, 1.0f));
      getSSRender().drawText("TIME", cl_time, PTEXT_HZA_LEFT | PTEXT_VTA_TOP, k, o);
    }
    }

      // show Next/Total checkpoints
      {
          // checkpoint counter

          glm::vec4 cl_chkpt(1.0f, 1.0f, 1.0f, 1.0f);
          const std::string totalcp = std::to_string(game->checkpt.size());
          const std::string nextcp = std::to_string(vehic->nextcp);

          // checkpoint position
          glm::mat4 k = glm::translate(glm::mat4(1.0f), glm::vec3(hratio - hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f), 0.0f));
          k = glm::scale(k, glm::vec3(0.125f, 0.125f, 1.0f));

            if (game->getFinishState() != Gamefinish::not_finished)
                getSSRender().drawText(totalcp + '/' + totalcp, cl_chkpt, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, k, o);
            else
                getSSRender().drawText(nextcp + '/' + totalcp, cl_chkpt, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, k, o);

          // checkpoint label

          k = glm::translate(k, glm::vec3(0, 0.52f, 0.0f));
          k = glm::scale(k, glm::vec3(0.65f, 0.65f, 1.0f));
          getSSRender().drawText("CKPT", cl_chkpt, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, k, o);
      }

        // show Current/Total laps
        if (game->number_of_laps > 1)
        {
            const std::string currentlap = std::to_string(vehic->currentlap);
            const std::string number_of_laps = std::to_string(game->number_of_laps);

            glm::vec4 cl_laps(1.0f, 1.0f, 1.0f, 1.0f);
            glm::mat4 k = glm::translate(glm::mat4(1.0f), glm::vec3(hratio - hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f) - 0.20f, 0.0f));
            k = glm::scale(k, glm::vec3(0.125f, 0.125f, 1.0f));

            if (game->getFinishState() != Gamefinish::not_finished)
                getSSRender().drawText(number_of_laps + '/' + number_of_laps, cl_laps, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, k, o);
            else
                getSSRender().drawText(currentlap + '/' + number_of_laps, cl_laps, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, k, o);

            k = glm::translate(k, glm::vec3(0, 0.52f, 0.0f));
            k = glm::scale(k, glm::vec3(0.65f, 0.65f, 1.0f));
            getSSRender().drawText("LAP", cl_laps, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, k, o);
        }

#ifdef INDEVEL
        // show codriver checkpoint text (the pace notes)
        if (!game->codrivercheckpt.empty() && vehic->nextcdcp != 0)
        {
            glm::vec4 cl_codrv(1.0f, 1.0f, 0.0f, 1.0f);
            glm::mat4 k = glm::translate(t, glm::vec3(0.0f, 0.3f, 0.0f));
            k = glm::scale(k, glm::vec3(0.1f, 0.1f, 1.0f));
            getSSRender().drawText(game->codrivercheckpt[vehic->nextcdcp - 1].notes, cl_codrv, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, k);
        }
#endif

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
          getSSRender().drawText(buff, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, k, o);

          // speed number
          const int speed = std::fabs(vehic->getWheelSpeed()) * hud_speedo_mps_speed_mult;
          std::string speedstr = std::to_string(speed);

          k = glm::translate(k, glm::vec3(1.1f, -0.625f, 0.0f));
          k = glm::scale(k, glm::vec3(0.5f, 0.5f, 1.0f));
          getSSRender().drawText(speedstr, PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, k, o);

          // speed label
          k = glm::translate(k, glm::vec3(0.0f, -0.82f, 0.0f));
          k = glm::scale(k, glm::vec3(0.5f, 0.5f, 1.0f));

          if (cfg_speed_unit == MainApp::Speedunit::mph)
              getSSRender().drawText("MPH", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, k, o);
          else
              getSSRender().drawText("km/h", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, k, o);
      }

  #ifndef NDEBUG
      // draw revs for debugging
      {
      glm::mat4 k = glm::translate(t, glm::vec3(1.17f, 0.52f, 0.0f));
      k = glm::scale(k, glm::vec3(0.2f, 0.2f, 1.0f));
      getSSRender().drawText(std::to_string(vehic->getEngineRPM()), PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, k, o);
      }
  #endif

#ifndef NDEBUG
    // draw real time penalty for debugging
    {
    glm::mat4 k = glm::scale(t, glm::vec3(0.1f, 0.1f, 1.0f));
    k = glm::translate(k, glm::vec3(0.0f, -4.0f, 0.0f));
    tex_fontSourceCodeOutlined->bind();
    getSSRender().drawText(std::string("true time penalty: ") +
        std::to_string(game->getOffroadTime() * game->offroadtime_penalty_multiplier),
        PTEXT_HZA_CENTER | PTEXT_VTA_TOP, k, o);
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
            glm::mat4 t = glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 0.25f, 1.0f));

            offroad_vao->bind();

            sp_offroad->use();
            sp_offroad->attrib("tex_coord", 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GL_FLOAT), 0);
            sp_offroad->attrib("position", 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GL_FLOAT), 2 * sizeof(GL_FLOAT));
            sp_offroad->uniform("mv", t);
            sp_offroad->uniform("p", o);

            tex_hud_offroad->bind();
            glActiveTexture(GL_TEXTURE0);
            sp_offroad->uniform("offroad_sign", 0);

            glDrawElements(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_SHORT, 0);

            offroad_vao->unbind();
            sp_offroad->unuse();

            glm::mat4 k = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 1.0f));
            k = glm::translate(k, glm::vec3(0.0f, -2.5f, 0.0f));
            glm::vec4 cl_offroad(1.0f, 1.0f, 0.0f, 1.0f);
            tex_fontSourceCodeOutlined->bind();
            getSSRender().drawText(
                std::to_string(static_cast<int> (game->getOffroadTime() * game->offroadtime_penalty_multiplier)) +
                " seconds", cl_offroad,
                PTEXT_HZA_CENTER | PTEXT_VTA_TOP, k, o);
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

        glm::vec4 cl_terrinfo(1.0f, 1.0f, 1.0f);
        getSSRender().drawText(s, cl_terrinfo, PTEXT_HZA_CENTER | PTEXT_VTA_TOP, mv, o);
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

      glm::mat4 q = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.2f, 0.0f));
      q = glm::scale(q, glm::vec3(0.6f, 0.6f, 1.0f));

      if (game->gamestate == Gamestate::countdown)
      {
          float sizer = fmodf(game->othertime, 1.0f) + 0.5f;
          glm::mat4 k = glm::scale(q, glm::vec3(sizer, sizer, 1.0f));
          glm::vec4 cl_gamestate(1.0f, 0.0f, 0.0f, 1.0f);
          getSSRender().drawText(
              PUtil::formatInt(((int)game->othertime + 1), 1), cl_gamestate,
              PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, k, o);
      }
      else if (game->gamestate == Gamestate::finished)
      {
          if (game->getFinishState() == Gamefinish::pass)
          {
              glm::vec4 cl_gamestate(0.5f, 1.0f, 0.5f, 1.0f);
              getSSRender().drawText("WIN", cl_gamestate, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, q, o);
          }
          else
          {
              glm::mat4 k = glm::scale(q, glm::vec3(0.5f, 0.5f, 1.0f));
              glm::vec4 cl_gamestate(0.5f, 0.0f, 0.0f, 1.0f);
              getSSRender().drawText("TIME EXCEEDED", cl_gamestate, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, k, o);
          }
      }
      else if (game->coursetime < 1.0f)
      {
          glm::vec4 cl_gamestate(0.5f, 1.0f, 0.5f, 1.0f);
          getSSRender().drawText("GO!", cl_gamestate, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, q, o);
      }
      else if (game->coursetime < 2.0f)
      {
          float a = 1.0f - (game->coursetime - 1.0f);
          glm::vec4 cl_gamestate(0.5f, 1.0f, 0.5f, a);
          getSSRender().drawText("GO!", cl_gamestate, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, q, o);
      }

      if (game->gamestate == Gamestate::countdown)
      {
          glm::mat4 k = glm::translate(q, glm::vec3(0.0f, 0.6f, 0.0f));
          k = glm::scale(k, glm::vec3(0.08f, 0.08f, 1.0f));
          if (game->othertime < 1.0f)
          {
              glm::vec4 cl_gamestate(1.0f, 1.0f, 1.0f, game->othertime);
              getSSRender().drawText(game->comment, cl_gamestate, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, k, o);
          }
          else
          {
              glm::vec4 cl_gamestate(1.0f, 1.0f, 1.0f, 1.0f);
              getSSRender().drawText(game->comment, cl_gamestate, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, k, o);
          }
      }

        if (pauserace)
        {
            glm::mat4 k = glm::scale(q, glm::vec3(0.25f, 0.25f, 1.0f));
            glm::vec4 cl_pause(0.25f, 0.25f, 1.0f, 1.0f);
            getSSRender().drawText("PAUSED", cl_pause, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, k, o);
        }
    }

    glBlendFunc(GL_ONE, GL_ZERO);

    glEnable(GL_DEPTH_TEST);
}

void MainApp::renderRain(const glm::mat4& mv, const glm::mat4& p)
{
    glm::vec4 raindrop_col(0.5f, 0.5f, 0.5f, 0.4f);
    const float raindrop_width = 0.015f;

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
        zag *= raindrop_width / zag.length();

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

    VAO vao(
        vbo, rain.size() * 6 * 10 * sizeof(float),
        ibo, rain.size() * 8 * sizeof(unsigned short)
    );
    vao.bind();

    sp_rain->use();
    sp_rain->attrib("color", 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), 0);
    sp_rain->attrib("normal", 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), 4 * sizeof(float));
    sp_rain->attrib("position", 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), 7 * sizeof(float));
    sp_rain->uniform("mv", mv);
    sp_rain->uniform("p", p);
    //glInterleavedArrays(GL_C4F_N3F_V3F, 10 * sizeof(float), vbo);
    glDrawElements(GL_TRIANGLE_STRIP, rain.size() * 8, GL_UNSIGNED_SHORT, 0);
    sp_rain->unuse();

    delete[] ibo;
    delete[] vbo;
}

void MainApp::renderSnow(const glm::mat4& mv, const glm::mat4& p)
{
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
    snow_vao->bind();
    if (cfg_snowflaketype == SnowFlakeType::textured)
    {
        //glEnable(GL_TEXTURE_2D);
        glBlendFunc(GL_SRC_COLOR, GL_ONE);
        tex_snowflake->bind();
    }

    sp_snow->use();
    sp_snow->attrib("tex_coord", 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 0);
    sp_snow->attrib("position", 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 2 * sizeof(GL_FLOAT));
    sp_snow->uniform("p", p);
    sp_snow->uniform("snow", 0);

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
            t = glm::translate(mv, glm::vec3(pt.x, pt.y, pt.z));
            t = glm::scale(t, glm::vec3(zag.x, zag.y, zag.z + SNOWFLAKE_BOX_SIZE));

            sp_snow->uniform("mv", t);
            sp_snow->uniform("alpha", alpha);

            glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
        }
    }

    /*if (cfg_snowflaketype == SnowFlakeType::point)
        glPointSize(ops); // restore original point size*/

    // disable textures
    if (cfg_snowflaketype == SnowFlakeType::textured)
    {
        //glDisable(GL_TEXTURE_2D);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    sp_snow->unuse();
    snow_vao->unbind();
}

void MainApp::buildChkptVao()
{
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

    chkpt_vao = new VAO(
        check_vbo, (N_SPLITS) * 3 * 10 * sizeof(float),
        check_ibo, (N_SPLITS+1)*4 * sizeof(unsigned short)
    );

    delete[] check_vbo;
    delete[] check_ibo;

    chkpt_size = (N_SPLITS+1)*4;
}

void MainApp::renderCheckpoints(int nextcp, const glm::mat4& mv, const glm::mat4& p)
{
    chkpt_vao->bind();
    sp_chkpt->use();
    sp_chkpt->uniform("p", p);

    sp_chkpt->attrib("d_color", 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), 0);
    sp_chkpt->attrib("normal", 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), 4 * sizeof(float));
    sp_chkpt->attrib("position", 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), 7 * sizeof(float));

    for (unsigned int i=0; i<game->checkpt.size(); i++)
    {
        vec4f colr = checkpoint_col[2];

        if ((int)i == nextcp)
            colr = checkpoint_col[0];
        else if ((int)i == (nextcp + 1) % (int)game->checkpt.size())
            colr = checkpoint_col[1];

        glm::mat4 t(1.0f);
        t = glm::translate(mv, glm::vec3(game->checkpt[i].pt.x, game->checkpt[i].pt.y, game->checkpt[i].pt.z));
        t = glm::scale(t, glm::vec3(25.0f, 25.0f, 1.0f));

#if 0 // Checkpoint style one (one strip)
            glPushMatrix(); // 1
            glLoadMatrixf(glm::value_ptr(t);
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
            glPopMatrix(); // 1
#else // Regular checkpoint style (two strips)
        float ht = sinf(cprotate * 6.0f) * 7.0f + 8.0f;

        t = glm::translate(t, glm::vec3(0.0f, 0.0f, ht));

        sp_chkpt->uniform("color", glm::vec4(colr[0], colr[1], colr[2], colr[3]));
        sp_chkpt->uniform("mv", t);

        glDrawElements(GL_TRIANGLE_STRIP, chkpt_size, GL_UNSIGNED_SHORT, 0);
#endif
    }

    sp_chkpt->unuse();
    chkpt_vao->unbind();

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
