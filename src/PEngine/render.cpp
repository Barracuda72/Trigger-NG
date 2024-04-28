
// render.cpp [pengine]

// Copyright 2004-2006 Jasmine Langridge, jas@jareiko.net
// License: GPL version 2 (see included gpl.txt)

#include "pengine.h"
#include "shaders.h"

PSSRender::PSSRender(PApp &parentApp) : PSubsystem(parentApp)
{
  PUtil::outLog() << "Initialising render subsystem" << std::endl;
  sp_model = new ShaderProgram("model");
  sp_text = new ShaderProgram("text");
}

PSSRender::~PSSRender()
{
  delete sp_model;
  delete sp_text;
  PUtil::outLog() << "Shutting down render subsystem" << std::endl;
}


void PSSRender::tick(float delta, const vec3f &eyepos, const mat44f &eyeori, const vec3f &eyevel)
{
  (void)delta;
  (void)eyevel;

  cam_pos = eyepos;
  cam_orimat = eyeori;
}


void PSSRender::render(PParticleSystem *psys)
{
  vec3f pushx = makevec3f(cam_orimat.row[0]);
  vec3f pushy = makevec3f(cam_orimat.row[1]);
  vec3f vert;

  glBlendFunc(psys->blendparam1, psys->blendparam2);

  if (psys->tex) psys->tex->bind();
  else glDisable(GL_TEXTURE_2D);

  float* vbo = new float[psys->part.size() * 4 * 5];
  unsigned int* ibo = new unsigned int[psys->part.size() * 6];

  for (unsigned int i=0; i<psys->part.size(); i++) {
    PParticle_s &part = psys->part[i];
    float sizenow = INTERP(psys->endsize, psys->startsize, part.life);
    vec3f pushxt = pushx * sizenow;
    vec3f pushyt = pushy * sizenow;
    vec3f pushx2 = pushxt * part.orix.x + pushyt * part.orix.y;
    vec3f pushy2 = pushxt * part.oriy.x + pushyt * part.oriy.y;

    glColor4f(INTERP(psys->colorend[0], psys->colorstart[0], part.life),
        INTERP(psys->colorend[1], psys->colorstart[1], part.life),
        INTERP(psys->colorend[2], psys->colorstart[2], part.life),
        INTERP(psys->colorend[3], psys->colorstart[3], part.life));

    for (int y = 0; y < 2; y++)
      for (int x = 0; x < 2; x++) {
        vert = part.pos + ((2*x - 1.0f) * pushx2) + ((2*y-1.0f) * pushy2);

        vbo[(i * 4 + (y * 2 + x)) * 5 + 0] = (float)x;
        vbo[(i * 4 + (y * 2 + x)) * 5 + 1] = (float)y;

        vbo[(i * 4 + (y * 2 + x)) * 5 + 2] = vert.x;
        vbo[(i * 4 + (y * 2 + x)) * 5 + 3] = vert.y;
        vbo[(i * 4 + (y * 2 + x)) * 5 + 4] = vert.z;
      }

    ibo[i * 6 + 0] = i * 4 + 0;
    ibo[i * 6 + 1] = i * 4 + 1;
    ibo[i * 6 + 2] = i * 4 + 2;

    ibo[i * 6 + 3] = i * 4 + 1;
    ibo[i * 6 + 4] = i * 4 + 3;
    ibo[i * 6 + 5] = i * 4 + 2;
  }

  glInterleavedArrays(GL_T2F_V3F, 5*sizeof(GL_FLOAT), vbo);
  glDrawElements(GL_TRIANGLES, psys->part.size() * 6, GL_UNSIGNED_INT, ibo);

  delete[] ibo;
  delete[] vbo;

  if (!psys->tex) glEnable(GL_TEXTURE_2D);
}

void PSSRender::drawModel(PModel &model, PSSEffect &ssEffect, PSSTexture &ssTexture, const glm::mat4& mv, const glm::mat4& p)
{
  sp_model->use();
  sp_model->uniform("mv", mv);
  sp_model->uniform("p", p);
  glActiveTexture(GL_TEXTURE0);
  sp_model->uniform("tex", 0);

  for (std::vector<PMesh>::iterator mesh = model.mesh.begin();
    mesh != model.mesh.end();
    mesh++) {
    if (!mesh->effect)
      mesh->effect = ssEffect.loadEffect(mesh->fxname);

    mesh->vao->bind();
    sp_model->attrib("tex_coord", 2, GL_FLOAT, GL_FALSE, mesh->vertexSize * sizeof(GL_FLOAT), 0);
    sp_model->attrib("normal", 3, GL_FLOAT, GL_FALSE, mesh->vertexSize * sizeof(GL_FLOAT), 2 * sizeof(GL_FLOAT));
    sp_model->attrib("position", 3, GL_FLOAT, GL_FALSE, mesh->vertexSize * sizeof(GL_FLOAT), 5 * sizeof(GL_FLOAT));

    int numPasses = 0;
    if (mesh->effect->renderBegin(&numPasses, ssTexture)) {
      for (int i=0; i<numPasses; i++) {
        mesh->effect->renderPass(i);
        glDrawArrays(GL_TRIANGLES, 0, mesh->face.size() * 3);
      }
      mesh->effect->renderEnd();
    }

    mesh->vao->unbind();
  }
  sp_model->unuse();
}

/// The Char at (0, 11) is used to display "unprintable" characters.
#define PTEXT_HARDCODED_DEFAROW     0
#define PTEXT_HARDCODED_DEFACOL     11

///
/// X-macro defining hardcoded positions for the characters in the font texture.
/// The information contained is: (Char, Row, Column).
///     Char    represents the current character
///     Row     represents the row index
///     Column  represents the column index
/// The bottom row is the first row (0), and the top row is the last row (7).
/// The columns are numbered from left to right.
///
#define PTEXT_HARDCODED_POSITIONS   \
    /* Row 0 */                     \
    X('&',      0,      0)          \
    X('\'',     0,      1)          \
    X('(',      0,      2)          \
    X(')',      0,      3)          \
    X('*',      0,      4)          \
    X('+',      0,      5)          \
    X(',',      0,      6)          \
    X('-',      0,      7)          \
    X('.',      0,      8)          \
    X('/',      0,      9)          \
    X(' ',      0,     10)          \
    /* Row 1 */                     \
    X('\\',     1,      0)          \
    X(']',      1,      1)          \
    X('^',      1,      2)          \
    X('_',      1,      3)          \
    X(':',      1,      4)          \
    X(';',      1,      5)          \
    X('?',      1,      6)          \
    X('!',      1,      7)          \
    X('"',      1,      8)          \
    X('#',      1,      9)          \
    X('$',      1,     10)          \
    X('%',      1,     11)          \
    /* Row 2 */                     \
    X('8',      2,      0)          \
    X('9',      2,      1)          \
    X('<',      2,      2)          \
    X('=',      2,      3)          \
    X('>',      2,      4)          \
    X('`',      2,      5)          \
    X('@',      2,      6)          \
    X('{',      2,      7)          \
    X('|',      2,      8)          \
    X('}',      2,      9)          \
    X('~',      2,     10)          \
    X('[',      2,     11)          \
    /* Row 3 */                     \
    X('W',      3,      0)          \
    X('X',      3,      1)          \
    X('Y',      3,      2)          \
    X('Z',      3,      3)          \
    X('0',      3,      4)          \
    X('1',      3,      5)          \
    X('2',      3,      6)          \
    X('3',      3,      7)          \
    X('4',      3,      8)          \
    X('5',      3,      9)          \
    X('6',      3,     10)          \
    X('7',      3,     11)          \
    /* Row 4 */                     \
    X('K',      4,      0)          \
    X('L',      4,      1)          \
    X('M',      4,      2)          \
    X('N',      4,      3)          \
    X('O',      4,      4)          \
    X('P',      4,      5)          \
    X('Q',      4,      6)          \
    X('R',      4,      7)          \
    X('S',      4,      8)          \
    X('T',      4,      9)          \
    X('U',      4,     10)          \
    X('V',      4,     11)          \
    /* Row 5 */                     \
    X('y',      5,      0)          \
    X('z',      5,      1)          \
    X('A',      5,      2)          \
    X('B',      5,      3)          \
    X('C',      5,      4)          \
    X('D',      5,      5)          \
    X('E',      5,      6)          \
    X('F',      5,      7)          \
    X('G',      5,      8)          \
    X('H',      5,      9)          \
    X('I',      5,     10)          \
    X('J',      5,     11)          \
    /* Row 6 */                     \
    X('m',      6,      0)          \
    X('n',      6,      1)          \
    X('o',      6,      2)          \
    X('p',      6,      3)          \
    X('q',      6,      4)          \
    X('r',      6,      5)          \
    X('s',      6,      6)          \
    X('t',      6,      7)          \
    X('u',      6,      8)          \
    X('v',      6,      9)          \
    X('w',      6,     10)          \
    X('x',      6,     11)          \
    /* Row 7 */                     \
    X('a',      7,      0)          \
    X('b',      7,      1)          \
    X('c',      7,      2)          \
    X('d',      7,      3)          \
    X('e',      7,      4)          \
    X('f',      7,      5)          \
    X('g',      7,      6)          \
    X('h',      7,      7)          \
    X('i',      7,      8)          \
    X('j',      7,      9)          \
    X('k',      7,     10)          \
    X('l',      7,     11)

///
/// @brief Draws a text to the screen.
/// @param [in] text    Text to be displayed.
/// @param flags        Flags used to align the text.
/// @details Flags can be:
///  (PTEXT_VTA_CENTER xor PTEXT_VTA_TOP) or
///  (PTEXT_HZA_CENTER xor PTEXT_HZA_RIGHT)
///
void PSSRender::drawText(const std::string &text, uint32 flags, const glm::mat4& mv, const glm::mat4& p)
{
    glm::vec4 default_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    drawText(text, default_color, flags, mv, p);
}

void PSSRender::drawText(const std::string &text, const glm::vec4& color, uint32 flags, const glm::mat4& mv, const glm::mat4& p)
{
    // FIXME: what the aspect should be...
    const GLfloat font_aspect = 8.0f / 12.0f;
    // FIXME: what the aspect must be, because of the menu...
    //const GLfloat font_aspect = 0.6f;
    const GLfloat addx = 1.0f / 12.0f;
    const GLfloat addy = 1.0f / 8.0f;

    float x_offset = 0.0f;
    float y_offset = 0.0f;

    if (flags & PTEXT_VTA_CENTER)
        y_offset = -0.5f;
    else
    if (flags & PTEXT_VTA_TOP)
        y_offset = -1.0f;

    if (flags & PTEXT_HZA_CENTER)
        x_offset = -0.5f;
    else
    if (flags & PTEXT_HZA_RIGHT)
        x_offset = -1.0f;

    glm::mat4 t = glm::translate(mv, glm::vec3(x_offset * text.length() * font_aspect, y_offset, 0.0f));

    float* vbo = new float[(text.length()) * 4 * 5]; // N letters, each letter has 4 vertices, each vertex has 5 attributes
    unsigned short* ibo = new unsigned short[text.length() * 6]; // N letters, each has 6 indices

    int i = 0;

    for (char c: text)
    {
        GLfloat tx;
        GLfloat ty;

        switch (c)
        {
#define X(Char, Row, Column)    case Char: tx = Column * addx; ty = Row * addy; break;
            PTEXT_HARDCODED_POSITIONS
#undef X
            default:
                tx = PTEXT_HARDCODED_DEFACOL * addx;
                ty = PTEXT_HARDCODED_DEFAROW * addy;
                break;
        }

        for (int y = 0; y < 2; y++)
            for (int x = 0; x < 2; x++) {
              vbo[(i * 4 + (y * 2 + x)) * 5 + 0] = tx + addx * x;
              vbo[(i * 4 + (y * 2 + x)) * 5 + 1] = ty + addy * y;

              vbo[(i * 4 + (y * 2 + x)) * 5 + 2] = font_aspect * (i + x);
              vbo[(i * 4 + (y * 2 + x)) * 5 + 3] = (float)y;
              vbo[(i * 4 + (y * 2 + x)) * 5 + 4] = 0.0f;
            }

        ibo[i * 6 + 0] = i * 4 + 0;
        ibo[i * 6 + 1] = i * 4 + 1;
        ibo[i * 6 + 2] = i * 4 + 2;

        ibo[i * 6 + 3] = i * 4 + 1;
        ibo[i * 6 + 4] = i * 4 + 3;
        ibo[i * 6 + 5] = i * 4 + 2;

        i++;
    }

    VAO vao(
        vbo, (text.length()) * 4 * 5 * sizeof(float),
        ibo, text.length() * 6 * sizeof(unsigned short)
    );

    vao.bind();

    sp_text->use();
    sp_text->attrib("tex_coord", 2, GL_FLOAT, GL_FALSE, 5*sizeof(GL_FLOAT), 0);
    sp_text->attrib("position", 3, GL_FLOAT, GL_FALSE, 5*sizeof(GL_FLOAT), 2 * sizeof(GL_FLOAT));

    //glActiveTexture(GL_TEXTURE0);
    sp_text->uniform("font", 0);
    sp_text->uniform("text_color", color);
    sp_text->uniform("mv", t);
    sp_text->uniform("p", p);

    glDrawElements(GL_TRIANGLES, text.length() * 6, GL_UNSIGNED_SHORT, 0);

    sp_text->unuse();

    delete[] ibo;
    delete[] vbo;
}


vec2f PSSRender::getTextDims(const std::string &text)
{
//  const float font_aspect = 0.6f;
    const float font_aspect = 8.0f / 12.0f;

  return vec2f((float)text.length() * font_aspect, 1.0f);
}


void PParticleSystem::addParticle(const vec3f &pos, const vec3f &linvel)
{
  part.push_back(PParticle_s());
  part.back().pos = pos;
  part.back().linvel = linvel;
  part.back().life = 1.0;

  float ang = randm11 * PI;
  part.back().orix = vec2f(cos(ang),sin(ang));
  part.back().oriy = vec2f(-sin(ang),cos(ang));
}


void PParticleSystem::tick(float delta)
{
  float decr = delta * decay;

  // update life and delete dead particles
  unsigned int j=0;
  for (unsigned int i=0; i<part.size(); i++) {
    part[j] = part[i];
    part[j].life -= decr;
    if (part[j].life > 0.0) j++;
  }
  part.resize(j);

  for (unsigned int i=0; i<part.size(); i++) {
    part[i].pos += part[i].linvel * delta;
  }
}

void PMesh::buildGeometry()
{
  this->vbo = new float[this->face.size() * 3 * vertexSize];
  this->ibo = new unsigned short[this->face.size() * 3];

  for (unsigned int f=0; f<this->face.size(); f++) {
    //glNormal3fv(mesh->face[f].facenormal);
    for (unsigned int v = 0; v < 3; v++) {
      vec3f norm = this->norm[this->face[f].nr[v]];
      vec2f tex = this->texco[this->face[f].tc[v]];
      vec3f vert = this->vert[this->face[f].vt[v]];

      this->vbo[(f * 3 + v) * vertexSize + 0] = tex.x;
      this->vbo[(f * 3 + v) * vertexSize + 1] = tex.y;

      this->vbo[(f * 3 + v) * vertexSize + 2] = norm.x;
      this->vbo[(f * 3 + v) * vertexSize + 3] = norm.y;
      this->vbo[(f * 3 + v) * vertexSize + 4] = norm.z;

      this->vbo[(f * 3 + v) * vertexSize + 5] = vert.x;
      this->vbo[(f * 3 + v) * vertexSize + 6] = vert.y;
      this->vbo[(f * 3 + v) * vertexSize + 7] = vert.z;

      this->ibo[f * 3 + v] = f * 3 + v;
    }
  }

  this->vao = new VAO(
    this->vbo, this->face.size() * 3 * vertexSize * sizeof(float),
    this->ibo, this->face.size() * 3 * sizeof(unsigned short)
  );
}

PMesh::~PMesh()
{
  delete[] this->vbo;
  delete[] this->ibo;
  delete this->vao;
}
