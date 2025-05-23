
// render.h [pengine]

// Copyright 2004-2006 Jasmine Langridge, jas@jareiko.net
// License: GPL version 2 (see included gpl.txt)

#ifndef RENDER_H_INCLUDED
#define RENDER_H_INCLUDED

#include <cmath>
#include <glm/mat4x4.hpp>
#include "rigidity.h"
#include "shaders.h"
#include "light.h"

struct PParticle_s {
  vec3f pos,linvel;
  float life;

  vec2f orix,oriy; // orientation vectors (2d)
};

class PParticleSystem {
protected:
  float colorstart[4],colorend[4];
  float startsize, endsize;
  float decay;
  const PTexture *tex;
  GLenum blendparam1, blendparam2;

  std::vector<PParticle_s> part;

public:
  PParticleSystem() {
    colorstart[0] = colorstart[1] = colorstart[2] = colorstart[3] = 1.0;
    colorend[0] = colorend[1] = colorend[2] = 1.0; colorend[3] = 0.0;
    startsize = 0.0;
    endsize = 1.0;
    decay = 1.0;
    tex = nullptr;
    blendparam1 = GL_SRC_ALPHA;
    blendparam2 = GL_ONE;
  }

public:
  void setColorStart(float r, float g, float b, float a) {
    colorstart[0] = r; colorstart[1] = g; colorstart[2] = b; colorstart[3] = a;
  }
  void setColorEnd(float r, float g, float b, float a) {
    colorend[0] = r; colorend[1] = g; colorend[2] = b; colorend[3] = a;
  }
  void setColor(float r, float g, float b) {
    colorstart[0] = colorend[0] = r;
    colorstart[1] = colorend[1] = g;
    colorstart[2] = colorend[2] = b;
  }
  void setSize(float start, float end) {
    startsize = start; endsize = end;
  }
  void setDecay(float _decay) {
    decay = _decay;
  }
  void setTexture(const PTexture *texptr) {
    tex = texptr;
  }
  void setBlend(GLenum b1, GLenum b2) {
    blendparam1 = b1;
    blendparam2 = b2;
  }

  void addParticle(const vec3f &pos, const vec3f &linvel);

  void tick(float delta);

  friend class PSSRender;
};

struct PVert_tv {
  vec2f st;
  vec3f xyz;
};

#define PTEXT_HZA_LEFT    0x00000000 // default
#define PTEXT_HZA_CENTER  0x00000001
#define PTEXT_HZA_RIGHT   0x00000002
#define PTEXT_VTA_BOTTOM  0x00000000 // default
#define PTEXT_VTA_CENTER  0x00000100
#define PTEXT_VTA_TOP     0x00000200
#define PTEXT_HIGHLIGHT   0x00010000

class PSSRender : public PSubsystem {
private:
  vec3f cam_pos;
  mat44f cam_orimat;
  ShaderProgram* sp_model;
  ShaderProgram* sp_text;
  ShaderProgram* sp_particle;

public:
  PSSRender(PApp &parentApp);
  ~PSSRender();

  void tick(float delta, const vec3f &eyepos, const mat44f &eyeori, const vec3f &eyevel);

  void render(PParticleSystem *psys, const glm::mat4& mv, const glm::mat4& p);

  void drawModel(PModel &model, PSSEffect &ssEffect, PSSTexture &ssTexture, const Light& light, const Material& material, bool is_ghost, const glm::mat4& mv, const glm::mat4& p);

  void drawText(const std::string &text, uint32 flags, const glm::mat4& mv, const glm::mat4& p);
  void drawText(const std::string &text, const glm::vec4& color, uint32 flags, const glm::mat4& mv, const glm::mat4& p);
  vec2f getTextDims(const std::string &text);
};



class PSSTexture : public PSubsystem {
private:
  PResourceList<PTexture> texlist;

public:
  PSSTexture(PApp &parentApp);
  ~PSSTexture();

  PTexture *loadTexture(const std::string &name, bool genMipmaps = true, bool clamp = false);
};


class PImage {
private:
  uint8 *data;
  int cx,cy,cc;

public:
  PImage () : data (nullptr) { }
  PImage (const std::string &filename) : data (nullptr) { load (filename); }
  PImage (int _cx, int _cy, int _cc) : data (nullptr) { load (_cx, _cy, _cc); }
  ~PImage ();

  void load (const std::string &filename);
  void load (int _cx, int _cy, int _cc);
  void unload ();

  void expandChannels();

  int getcx() const { return cx; }
  int getcy() const { return cy; }
  int getcc() const { return cc; }
  uint8 *getData() { return data; }

  const uint8 * getData() const
  {
    return data;
  }

  uint8 & getByte(int i)
  {
      return data[i];
  }

  uint8 getByte(int i) const
  {
      return data[i];
  }

  void swap (PImage &other) throw ()
  {
    { uint8 *tmp = data; data = other.data; other.data = tmp; }
    { int tmp = cx; cx = other.cx; other.cx = tmp; }
    { int tmp = cy; cy = other.cy; other.cy = tmp; }
    { int tmp = cc; cc = other.cc; other.cc = tmp; }
  }
};


class PTexture : public PResource {
private:
  GLuint texid;
  GLenum textarget;

  void scaleImage(GLuint format,
    GLsizei width_in, GLsizei height_in, GLenum type_in, const void* data_in,
    GLsizei width_out, GLsizei height_out, GLenum type_out, void* data_out
    );

public:
  PTexture () : texid (0) { }
  PTexture (const std::string &filename, GLfloat cfgAnisotropy, bool genMipmaps, bool clamp) : texid (0) { load (filename, cfgAnisotropy, genMipmaps, clamp); }
  PTexture (PImage &img, GLfloat cfgAnisotropy, bool genMipmaps, bool clamp) : texid (0) { load (img, cfgAnisotropy, genMipmaps, clamp); }
  ~PTexture() { unload (); }

  void load (const std::string &filename, GLfloat cfgAnisotropy, bool genMipmaps, bool clamp);
  void load(PImage &img, GLfloat cfgAnisotropy, bool genMipmaps = true, bool clamp = false);
  void loadPiece(PImage &img, int offx, int offy, int sizex, int sizey, bool genMipmaps = true, bool clamp = false);
  void loadAlpha(const std::string &filename, bool genMipmaps = true, bool clamp = false);
  void loadAlpha(PImage &img, bool genMipmaps = true, bool clamp = false);
  void loadCubeMap(const std::string &filenamePrefix, const std::string &filenameSuffix, bool genMipmaps = true);
  void unload();

  void bind() const;

  static void unbind();
};




class PSSEffect : public PSubsystem {
private:
  PResourceList<PEffect> fxlist;

public:
  PSSEffect(PApp &parentApp);
  ~PSSEffect();

  PEffect *loadEffect(const std::string &name);
};



#define CULLFACE_NONE       0
#define CULLFACE_CW         1
#define CULLFACE_CCW        2

#define BLEND_NONE          0
#define BLEND_ADD           1
#define BLEND_MULTIPLY      2
#define BLEND_ALPHA         3
#define BLEND_PREMULTALPHA  4


struct fx_renderstate_s {
  bool depthtest;

  bool lighting;
  bool lightmodeltwoside;

  struct {
    GLenum func;
    float ref;
  } alphatest;

  int cullface;

  int blendmode;

  struct {
    int texindex;
  } texunit[1];
};


struct fx_pass_s {
  fx_renderstate_s rs;
};

struct fx_technique_s {
  std::string name;

  std::vector<fx_pass_s> pass;

  bool validated;
  bool textures_ready;

  fx_technique_s() {
    validated = false;
    textures_ready = false;
  }
};

struct fx_texture_s {
  std::string name;

  std::string filename;

  GLenum type;

  // This is filled in just before rendering
  PTexture *texobject;

  fx_texture_s() {
    texobject = nullptr;
  }
};


class PEffect : public PResource {
private:
  // resources
  std::vector<fx_texture_s> tex;

  // techniques
  std::vector<fx_technique_s> tech;

  int cur_tech;

public:
  PEffect(const std::string &filename);
  ~PEffect();

  void unload();

  void loadFX(const std::string &filename);
  void loadMTL(const std::string &filename);

  int getNumTechniques();
  bool validateTechnique(int technique);
  const std::string &getTechniqueName(int technique);
  bool findTechnique(const std::string &techname, int *technique);

  bool setCurrentTechnique(int technique);
  int getCurrentTechnique();

  bool setFirstValidTechnique();

  bool renderBegin(int *numPasses, PSSTexture &sstex);
  void renderPass(int pass);
  void renderEnd();

private:
  void migrateRenderState(fx_renderstate_s *rs_old, fx_renderstate_s *rs_new);
};




class PSSModel : public PSubsystem {
private:
  PResourceList<PModel> modlist;

public:
  PSSModel(PApp &parentApp);
  ~PSSModel();

  PModel *loadModel(const std::string &name);
};


class PFace {
public:
  vec3f facenormal;
  uint32 vt[3];
  uint32 tc[3];
  uint32 nr[3];

public:
//    PFace(vec
};


class PMesh {
public:
  std::vector<vec3f> vert;
  std::vector<vec2f> texco;
  std::vector<vec3f> norm;
  std::vector<PFace> face;

  std::string fxname;
  PEffect *effect;


  const int vertexSize = 2 + 3 + 3; // Tex, Normal, Vertex
  void buildGeometry();
  float* vbo;
  unsigned short* ibo;
  VAO* vao;

  ~PMesh();
};


class PModel : public PResource {
  void loadASE (const std::string &filename, float globalScale);
  void loadOBJ (const std::string &filename, float globalScale);

public:
  PModel (const std::string &filename, float globalScale = 1.0);
  void buildGeometry();
  std::vector<PMesh> mesh;
  std::pair<vec3f, vec3f> getExtents() const;
};

struct PTerrainFoliageBand {
  float middle, range;
  float density;
  int trycount;
  float scale;

  /*
  float scalemin;
  float scalemax;
  */

  PTexture *sprite_tex;
  int sprite_count;
};

struct PTerrainFoliage {
  vec3f pos;
  float ang;
  float scale;
  float rigidity;
};

struct PTerrainFoliageSet {
  std::vector<PTerrainFoliage> inst;

  VAO* vao;
  int numvert, numelem;
  ~PTerrainFoliageSet() { delete vao; }
};

struct PRoadSignSet {
    std::vector<PTerrainFoliage> inst;
    VAO* vao;
    int numvert;
    int numelem;

    ~PRoadSignSet() { delete vao; }
};

struct PTerrainTile {
  int posx, posy;
  int lru_counter;

  VAO* vao;
  int numverts;

  PTexture tex;

  vec3f mins,maxs; // AABB

  std::vector<PTerrainFoliageSet> foliage;
  std::vector<PRoadSignSet> roadsignset;
  // Straight vector for rapid search by collision detection
  std::vector<PTerrainFoliage> straight;

  ~PTerrainTile() { delete vao; }
};

///
/// @brief Loads road information.
///
class RoadMap
{
public:

    RoadMap() = default;

    bool load(const PImage &img)
    {
        if (img.getData() == nullptr)
            return false;

        bx = img.getcx();
        by = img.getcy();
        bitmap.clear();
        bitmap.reserve(img.getcx() * img.getcy());

        const std::size_t tb = img.getcx() * img.getcy() * img.getcc(); // Total Bytes

        for (auto pb = img.getData(); pb != img.getData() + tb; pb += img.getcc())
        {
            bool is_road = true;

            for (int i=0; i < img.getcc(); ++i)
                if (pb[i] != 0xFF) // the road is white
                {
                    is_road = false;
                    break;
                }

            bitmap.push_back(is_road);
        }

        return true;
    }

    bool is_loaded() const
    {
        return !bitmap.empty();
    }

    ///
    /// @brief Decides if provided point is on the road.
    /// @param px           X coordinate of the point.
    /// @param py           Y coordinate of the point.
    /// @param ms           Map size.
    /// @see `getMapSize()`.
    /// @returns Whether or not the point is on the road.
    /// @retval true        If no roadmap was loaded.
    ///
    bool isOnRoad(float px, float py, float ms) const
    {
        if (bitmap.empty())
            return true;

        if (px >= ms)
        {
            do
                px -= ms;
            while (px > ms);
        }
        else
        if (px < 0)
        {
            do
                px += ms;
            while (px < 0);
        }

        if (py >= ms)
        {
            do
                py -= ms;
            while (py > ms);
        }
        else
        if (py < 0)
        {
            do
                py += ms;
            while (py < 0);
        }

        long int x = std::lround(px * bx / ms);
        long int y = std::lround(py * by / ms);

        CLAMP_UPPER(x, bx - 1);
        CLAMP_UPPER(y, by - 1);
        return bitmap.at(y * bx + x);
    }

private:

    std::vector<bool> bitmap;
    int bx;
    int by;
};

struct road_sign
{
public:

//
// being smart here makes the rest of the code more complicated;
// so let's be stupid for now...
//
#if 0
  struct road_sign_location
  {
    public:
      float x;
      float y;
      float deg;

      road_sign_location(float x=0, float y=0, float deg=0):
          x(x), y(y), deg(deg)
      {
      }
  };
  std::vector<road_sign_location> location;
#endif

  PTexture *sprite  = nullptr;
  float scale   = 1.0f;
  float x = 0.0f;
  float y = 0.0f;
  float deg = 0.0f;
  int sprite_count = 1;
};

class PTerrain // TODO: make this RAII conformant
{
protected:
  bool loaded;

  int tilesize, tilecount, totsize, totmask, totsizesq;

  float scale_hz, scale_vt, scale_hz_inv, scale_vt_inv, scale_tile_inv;

  int cmaptotsize, cmaptilesize, cmaptotmask;

  //std::vector<uint8> hmap;
  std::vector<float> hmap;

  // color map
  PImage cmap;
  // terrain map
  PImage tmap;
  // road map
  RoadMap rmap;

  std::vector<float> fmap;
  std::vector<PTerrainFoliageBand> foliageband;
  std::vector<road_sign> roadsigns;

  std::list<PTerrainTile> tile;

  // tiles share index buffers
  //PVBuffer ind;
  unsigned short* indices;
  int numinds;

  PTexture *tex_hud_map;

  ShaderProgram* sp_terrain;
  ShaderProgram* sp_tile;
  ShaderProgram* sp_shadow;

protected:

  PTerrainTile *getTile(int x, int y);

  float getInterp(float x, float y, float *data) {
    x *= scale_hz_inv;
    int xi = (int)x;
    if (x < 0.0) xi--;
    x -= (float)xi;
    int xiw = xi & totmask, xiw2 = (xiw+1) & totmask;

    y *= scale_hz_inv;
    int yi = (int)y;
    if (y < 0.0) yi--;
    y -= (float)yi;
    int yiw = yi & totmask, yiw2 = (yiw+1) & totmask;

    const int cx = totsize;

    float xv1,xv2;
    if (y > 0.0) {
      if (y < 1.0) {
        if (x < y) {
          xv1 = data[yiw*cx+xiw];
          xv2 = INTERP(data[yiw2*cx+xiw],data[yiw2*cx+xiw2],x/y);
        } else {
          xv1 = INTERP(data[yiw*cx+xiw],data[yiw*cx+xiw2],(x-y)/(1.0-y));
          xv2 = data[yiw2*cx+xiw2];
        }
        return INTERP(xv1,xv2,y);
      } else {
        return INTERP(data[yiw2*cx+xiw],data[yiw2*cx+xiw2],x);
      }
    } else {
      return INTERP(data[yiw*cx+xiw],data[yiw*cx+xiw2],x);
    }
  }

public:
  PTerrain(XMLElement *element, const std::string &filepath, PSSTexture &ssTexture, 
    const PRigidity &rigidity, bool cfgFoliage, bool cfgRoadsigns);
  ~PTerrain();

  void unload();

  void render(const glm::vec3 &campos, const glm::mat4 &camorim, PTexture* tex_detail, const vec3f& fog_color, float fog_density, const glm::mat4& mv, const glm::mat4& p);

  void drawShadow(float x, float y, float scale, float angle, PTexture* tex_shadow, const glm::mat4& mv, const glm::mat4& p);

  const std::vector<PTerrainFoliage> *getFoliageAtPos(const vec3f &pos) const;

  struct ContactInfo {
    vec3f pos;
    vec3f normal;
  };

    ///
    /// @brief Returns whether or not the given position is on road.
    /// @param [in] pos         Position to be checked.
    /// @returns Whether or not `pos` is on the road.
    /// @retval true            If no roadmap was loaded.
    /// @see `RoadMap`.
    ///
    bool getRmapOnRoad(const vec3f &pos) const
    {
        return rmap.isOnRoad(pos.x, pos.y, getMapSize());
    }

    ///
    /// @brief Returns the color of the pixel in the colormap that corresponds
    ///  to the given position in the terrain.
    /// @note The height component Z is ignored.
    /// @todo Should check if cmap.getcc() returns at least 3?
    /// @todo Should check if cmap.getcx() == cmap.getcy()?
    /// @todo Should remove paranoid clampings?
    /// @todo Should actually measure performance of float vs int.
    /// @param [in] pos   Position in the terrain.
    /// @returns Color in OpenGL-style RGB.
    ///
    vec3f getCmapColor(const vec3f &pos) const
    {
        vec3f r;
#if 0
        const float ms = getMapSize();
        float px = pos.x;
        float py = pos.y;
#else
        const int ms = static_cast<int> (getMapSize());
        int px = static_cast<int> (pos.x);
        int py = static_cast<int> (pos.y);
#endif
        if (px >= ms)
        {
            do
                px -= ms;
            while (px > ms);
        }
        else
        if (px < 0)
        {
            do
                px += ms;
            while (px < 0);
        }

        if (py >= ms)
        {
            do
                py -= ms;
            while (py > ms);
        }
        else
        if (py < 0)
        {
            do
                py += ms;
            while (py < 0);
        }

        long int x = std::lround(px * cmap.getcx() / getMapSize());
        long int y = std::lround(py * cmap.getcy() / getMapSize());

        CLAMP_UPPER(x, cmap.getcx() - 1);
        CLAMP_UPPER(y, cmap.getcy() - 1);
        r.x = cmap.getByte((y * cmap.getcx() + x) * cmap.getcc() + 0) / 255.0f;
        r.y = cmap.getByte((y * cmap.getcx() + x) * cmap.getcc() + 1) / 255.0f;
        r.z = cmap.getByte((y * cmap.getcx() + x) * cmap.getcc() + 2) / 255.0f;
        return r;
    }

    ///
    /// @brief Returns the road surface type corresponding to the given position
    ///  in the terrain.
    /// @note The height component Z is ignored.
    /// @todo Should remove paranoid clampings?
    /// @todo Should actually measure performance of float vs int. (int should be intrinsecally faster)
    /// @param [in] pos   Position in the terrain.
    /// @returns Terrain type.
    ///
    TerrainType getRoadSurface(const vec3f &pos) const
    {
        if (tmap.getData() == nullptr)
            return TerrainType::Unknown;
#if 0
        const float ms = getMapSize();
        float px = pos.x;
        float py = pos.y;
#else
        const int ms = static_cast<int> (getMapSize());
        int px = static_cast<int> (pos.x);
        int py = static_cast<int> (pos.y);
#endif
        if (px >= ms)
        {
            do
                px -= ms;
            while (px > ms);
        }
        else
        if (px < 0)
        {
            do
                px += ms;
            while (px < 0);
        }

        if (py >= ms)
        {
            do
                py -= ms;
            while (py > ms);
        }
        else
        if (py < 0)
        {
            do
                py += ms;
            while (py < 0);
        }

        long int x = std::lround(px * tmap.getcx() / getMapSize());
        long int y = std::lround(py * tmap.getcy() / getMapSize());
        rgbcolor temp;

        CLAMP_UPPER(x, tmap.getcx() - 1);
        CLAMP_UPPER(y, tmap.getcy() - 1);
        temp.r = tmap.getByte((y * tmap.getcx() + x) * tmap.getcc() + 0);
        temp.g = tmap.getByte((y * tmap.getcx() + x) * tmap.getcc() + 1);
        temp.b = tmap.getByte((y * tmap.getcx() + x) * tmap.getcc() + 2);
        return PUtil::decideRoadSurface(temp);
    }

  ///
  /// @brief get the information about a contact point (its coordinates and normal) with the ground
  ///
  void getContactInfo(ContactInfo &tci) {

    float x = tci.pos.x * scale_hz_inv;
    // int part of x
    int xi = (int)x;
    if (x < 0.0) xi--;
    // x is the decimal part
    x -= (float)xi;
    int xiw = xi & totmask, xiw2 = (xi+1) & totmask;

    float y = tci.pos.y * scale_hz_inv;
    int yi = (int)y;
    if (y < 0.0) yi--;
    y -= (float)yi;
    int yiw = yi & totmask, yiw2 = (yi+1) & totmask;

    float *data = &hmap[0];
    const int cx = totsize;

    float xv1,xv2;
    if (y > 0.0) {
      if (y < 1.0) {
        if (x < y) {
          tci.normal.x = data[yiw2*cx+xiw] - data[yiw2*cx+xiw2];
          tci.normal.y = data[yiw*cx+xiw] - data[yiw2*cx+xiw];
          xv1 = data[yiw*cx+xiw];
          xv2 = INTERP(data[yiw2*cx+xiw],data[yiw2*cx+xiw2],x/y);
        } else {
          tci.normal.x = data[yiw*cx+xiw] - data[yiw*cx+xiw2];
          tci.normal.y = data[yiw*cx+xiw2] - data[yiw2*cx+xiw2];
          xv1 = INTERP(data[yiw*cx+xiw],data[yiw*cx+xiw2],(x-y)/(1.0-y));
          xv2 = data[yiw2*cx+xiw2];
        }
        tci.pos.z = INTERP(xv1,xv2,y);
      } else {
        tci.normal.x = data[yiw2*cx+xiw] - data[yiw2*cx+xiw2];
        tci.normal.y = data[yiw*cx+xiw] - data[yiw2*cx+xiw];
        tci.pos.z = INTERP(data[yiw2*cx+xiw],data[yiw2*cx+xiw2],x);
      }
    } else {
      tci.normal.x = data[yiw*cx+xiw] - data[yiw*cx+xiw2];
      tci.normal.y = data[yiw*cx+xiw2] - data[yiw2*cx+xiw2];
      tci.pos.z = INTERP(data[yiw*cx+xiw],data[yiw*cx+xiw2],x);
    }
    tci.normal.z = scale_hz;
    tci.normal.normalize();
  }

  float getHeight(float x, float y) {
    return getInterp(x, y, &hmap[0]);
  }

  float getFoliageLevel(float x, float y) {
    return getInterp(x, y, &fmap[0]);
  }

  PTexture *getHUDMapTexture() { return tex_hud_map; }

  float getMapSize() const { return totsize * scale_hz; }

private:
  const PTerrainTile *getTileAtPos(const vec3f &pos) const;

  // Rigidity map for foliage and road signs
  const PRigidity &rigidity;
};

#endif // RENDER_H_INCLUDED
