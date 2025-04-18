
// terrain.cpp [pengine]

// Copyright 2004-2006 Jasmine Langridge, jas@jareiko.net
// License: GPL version 2 (see included gpl.txt)


#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr
#include <glm/mat4x4.hpp> // For glm::mat4
#include <glm/ext/matrix_transform.hpp> // For glm::translate
#include <glm/ext/matrix_clip_space.hpp> // For glm::frustrum

#include "pengine.h"

#include "main.h"

PTerrain::~PTerrain ()
{
  unload();
  delete[] indices;
  delete sp_terrain;
  delete sp_tile;
  delete sp_shadow;
}


void PTerrain::unload()
{
  loaded = false;

  tile.clear();

  hmap.clear();
}


PTerrain::PTerrain (XMLElement *element, const std::string &filepath, PSSTexture &ssTexture, 
  const PRigidity &rigidity, bool cfgFoliage, bool cfgRoadsigns) :
    loaded (false), rigidity(rigidity)
{
  unload();

  std::string heightmap, colormap, terrainmap, roadmap, foliagemap, hudmap;

  scale_hz = 1.0;
  scale_vt = 1.0;

  const char *val;

  val = element->Attribute("tilesize");
  if (val) tilesize = atoi(val);

  val = element->Attribute("horizontalscale");
  if (val) scale_hz = atof(val);

  val = element->Attribute("verticalscale");
  if (val) scale_vt = atof(val);

  val = element->Attribute("heightmap");
  if (val) heightmap = val;

  val = element->Attribute("colormap");
  if (val) colormap = val;

  val = element->Attribute("terrainmap");
  if (val != nullptr) terrainmap = val;

  val = element->Attribute("roadmap");
  if (val != nullptr) roadmap = val;

  val = element->Attribute("foliagemap");
  if (val && cfgFoliage) foliagemap = val;

  val = element->Attribute("hudmap");
  if (val) hudmap = val;

  XMLElement *node = element->FirstChildElement("blurfilter");
  std::vector<std::vector<float> > blurfilter;

  if (node != nullptr) {
    for (XMLElement *walk = node->FirstChildElement("row");
        walk != nullptr;
        walk = walk->NextSiblingElement("row")) {
      const char *srow = walk->Attribute("data");

      if (srow == nullptr)
          continue;

      std::stringstream bfrow(srow);
      float coef;
      std::vector<float> row;

      while (bfrow >> coef)
          row.push_back(coef);

      blurfilter.push_back(row);
    }
  }
  else {
    blurfilter = {
      {0.03f, 0.12f, 0.03f},
      {0.12f, 0.40f, 0.12f},
      {0.03f, 0.12f, 0.03f}
    };
  }

  for (XMLElement *walk = element->FirstChildElement();
    walk; walk = walk->NextSiblingElement()) {

    if (strcmp(walk->Value(), "roadsign") == 0 && cfgRoadsigns) {
      road_sign temprs;

      val = walk->Attribute("sprite");
      if (val != nullptr)
        temprs.sprite = ssTexture.loadTexture(PUtil::assemblePath(val, filepath));

      val = walk->Attribute("scale");
      if (val != nullptr)
        temprs.scale = atof(val);

      val = walk->Attribute("spritecount");
      if (val != nullptr)
        temprs.sprite_count = atof(val);

      for (XMLElement *walk2 = walk->FirstChildElement();
          walk2 != nullptr;
          walk2 = walk2->NextSiblingElement())
      {
        if (strcmp(walk2->Value(), "location") == 0) {
          float deg = 0;

          val = walk2->Attribute("oridegrees");
          if (val != nullptr)
            deg = RADIANS(atof(val));

          val = walk2->Attribute("coords");
          if (val != nullptr) {
            float x, y;

            if (sscanf(val, "%f, %f", &x, &y) == 2) {
              temprs.x = x;
              temprs.y = y;
              temprs.deg = deg;

              if (temprs.sprite != nullptr)
                roadsigns.push_back(temprs);
            }
          }
        }
      }
    }
    else if (!strcmp(walk->Value(), "foliageband") && cfgFoliage) {

      PTerrainFoliageBand tfb;
      tfb.middle = 0.5f;
      tfb.range = 0.5f;
      tfb.density = 1.0f;
      tfb.scale = 1.0f;
      //tfb.scalemin = 1.0f;
      //tfb.scalemax = 1.4f;
      //tfb.model = nullptr;
      //tfb.modelscale = 1.0f;
      tfb.sprite_tex = nullptr;
      tfb.sprite_count = 1;

      val = walk->Attribute("middle");
      if (val) tfb.middle = atof(val);

      val = walk->Attribute("range");
      if (val) tfb.range = atof(val);

      val = walk->Attribute("density");
      if (val) tfb.density = atof(val);

      val = walk->Attribute("scale");
      if (val) tfb.scale = atof(val);

      /*
      val = walk->Attribute("scalemin");
      if (val) tfb.scalemin = atof(val);

      val = walk->Attribute("scalemax");
      if (val) tfb.scalemax = atof(val);
        */
      /*
      val = walk->Attribute("model");
      if (val) tfb.model = ssModel.loadModel(PUtil::assemblePath(val, filepath));

      val = walk->Attribute("modelscale");
      if (val) tfb.modelscale = atof(val);
      */

      val = walk->Attribute("sprite");
      if (val) tfb.sprite_tex = ssTexture.loadTexture(PUtil::assemblePath(val, filepath));

      val = walk->Attribute("spritecount");
      if (val) tfb.sprite_count = atoi(val);

      foliageband.push_back(tfb);
    }
  }


  if (!heightmap.length()) {
    throw MakePException ("Load failed: terrain has no heightmap");
  }

  if (!colormap.length()) {
    throw MakePException ("Load failed: terrain has no colormap");
  }

  if (tilesize != (tilesize & (-tilesize)) ||
    tilesize < 4) {
    throw MakePException ("Load failed: tile size not power of two dimension, or too small");
  }

  if (scale_hz <= 0.0 || scale_vt == 0.0) {
    throw MakePException ("Load failed: invalid scale value");
  }

  scale_hz_inv = 1.0 / scale_hz;
  scale_vt_inv = 1.0 / scale_vt;
  scale_tile_inv = scale_hz_inv / (float)tilesize;

  PImage img;
  try
  {
    img.load (PUtil::assemblePath (heightmap, filepath));
  }
  catch (...)
  {
    PUtil::outLog() << "Load failed: couldn't open heightmap \"" << heightmap << "\"\n";
    throw;
  }

  totsize = img.getcx();
  if (totsize != img.getcy() ||
    totsize != (totsize & (-totsize)) ||
    totsize < 16) {
    throw MakePException ("Load failed: heightmap not square, or not power of two dimension, or too small");
  }

  totsizesq = totsize * totsize;

  if (tilesize > totsize) tilesize = totsize;

  tilecount = totsize / tilesize;
  totmask = totsize - 1;

  //PUtil::outLog() << "img: " << totsize << " squared, " << img.getcc() << " cc\n";

  hmap.resize(totsizesq);

#if 0
  if (img.getcc() != 1) {
    if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
      PUtil::outLog() << "Warning: heightmap is not single channel\n";
    int cc = img.getcc();
    uint8 *dat = img.getData();
    for (int s=0, d=0; d<totsizesq; s+=cc, d+=1) hmap[d] = dat[s];
  } else {
    std::copy(img.getData(), img.getData() + totsize * totsize, &hmap[0]);
  }
#else
  if (img.getcc() != 1) {
    if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
      PUtil::outLog() << "Warning: heightmap is not single channel\n";
  }

  int cc = img.getcc();
  uint8 *dat = img.getData();

  for (int y=0; y<totsize; ++y) {
    for (int x=0; x<totsize; ++x) {
      float accum = 0.0;
      for (int yi=0; yi < static_cast<int> (blurfilter.size()); ++yi) {
        for (int xi=0; xi < static_cast<int> (blurfilter[yi].size()); ++xi) {
          accum += (float)dat[
            (((y + yi - (blurfilter.size()-1)/2) & totmask) * totsize +
            ((x + xi - (blurfilter[yi].size()-1)/2) & totmask)) * cc] * blurfilter[yi][xi];
        }
      }
      hmap[y*totsize + x] = accum * scale_vt;
    }
  }
#endif

  img.unload();

  try
  {
    cmap.load(PUtil::assemblePath(colormap, filepath));
  }
  catch (...)
  {
    PUtil::outLog() << "Load failed: couldn't open colormap \"" << colormap << "\"\n";
    throw;
  }

  cmaptotsize = cmap.getcx();
  if (cmaptotsize != cmap.getcy() ||
    cmaptotsize != (cmaptotsize & (-cmaptotsize)) ||
    cmaptotsize < tilecount) {
    throw MakePException ("Load failed: colormap not square, or not power of two dimension, or too small");
  }

  cmaptilesize = cmaptotsize / tilecount;
  cmaptotmask = cmaptotsize - 1;

  // load terrain map image
  try
  {
      if (!terrainmap.empty())
        tmap.load(PUtil::assemblePath(terrainmap, filepath));
  }
  catch (...)
  {
    PUtil::outLog() << "Load failed: couldn't open terrainmap \"" << terrainmap << "\"\n";
    throw;
  }

    if (tmap.getData() != nullptr && tmap.getcx() != tmap.getcy())
        throw MakePException("Load failed: terrainmap not square");

    PImage rmap_img;

    // load road map image
    try
    {
        if (!roadmap.empty())
            rmap_img.load(PUtil::assemblePath(roadmap, filepath));
    }
    catch (...)
    {
        PUtil::outLog() << "Load failed: couldn't open roadmap \"" << roadmap << "\"\n";
        throw;
    }

    if (rmap_img.getData() != nullptr)
    {
        if (rmap_img.getcx() != rmap_img.getcy())
            throw MakePException("Load failed: roadmap not square");
        else
        if (!rmap.load(rmap_img))
            throw MakePException("Load failed: bad roadmap image");
    }

  // calculate foliage try counts for tile size

  for (unsigned int b = 0; b < foliageband.size(); b++) {
    foliageband[b].trycount =
      (int) (foliageband[b].density * (float)totsizesq * scale_hz * scale_hz);
  }

  // load foliage map

  fmap.resize(totsizesq, 0.0f);

  if (foliagemap.length()) {
    try
    {
      img.load(PUtil::assemblePath(foliagemap, filepath));
    }
    catch (...)
    {
      PUtil::outLog() << "Load failed: couldn't open foliage map \"" << foliagemap << "\"\n";
      throw;
    }

    if (totsize != img.getcy() ||
      totsize != img.getcx()) {
      throw MakePException ("Load failed: foliage map size doesn't match heightmap");
    }

    int cc = img.getcc();
    uint8 *dat = img.getData();

    if (cc != 1) {
      if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
        PUtil::outLog() << "Warning: foliage map is not single channel\n";
    }

    for (int i = 0; i < totsizesq; i++) {
      fmap[i] = (float) dat[i * cc] / 255.0f;
    }
  }

  // load hud map

  tex_hud_map = nullptr;

  if (hudmap.length()) {
    tex_hud_map = ssTexture.loadTexture(PUtil::assemblePath(hudmap, filepath));
  }

  // prepare shared index buffers

  int tilesizep1 = tilesize + 1;

  numinds = 0;

  /*PRamFile ramfile;
  uint16 index;
  for (int y=0; y<tilesize; ++y) {
    int add1 = (y+1) * tilesizep1;
    int add2 = (y+0) * tilesizep1;
    if (y > 0) {
      index = 0 + add1;
      ramfile.write(&index, sizeof(uint16));
      numinds += 1;
    }
    for (int x=0; x<tilesizep1; ++x) {
      index = x + add1;
      ramfile.write(&index, sizeof(uint16));
      index = x + add2;
      ramfile.write(&index, sizeof(uint16));
      numinds += 2;
    }
    if (y+1 < tilesize) {
      ramfile.write(&index, sizeof(uint16));
      numinds += 1;
    }
  }

  ind.create(ramfile.getSize(), PVBuffer::IndexContent, PVBuffer::StaticUsage, ramfile.getData());
  ramfile.clear();*/

  // TODO: this code should be rewritten but I'm too lazy for that
  indices = new unsigned short[(tilesizep1*2 + 2) * tilesize];

  uint16 index = 0;
  for (int y=0; y<tilesize; ++y) {
    int add1 = (y+1) * tilesizep1;
    int add2 = (y+0) * tilesizep1;
    if (y > 0) {
      index = 0 + add1;
      indices[numinds] = index;
      numinds++;
    }
    for (int x=0; x<tilesizep1; ++x) {
      index = x + add1;
      indices[numinds] = index;
      numinds++;
      index = x + add2;
      indices[numinds] = index;
      numinds++;
    }
    if (y+1 < tilesize) {
      indices[numinds] = index;
      numinds++;
    }
  }

  sp_terrain = new ShaderProgram("terrain");

  sp_tile = new ShaderProgram("tile");

  sp_shadow = new ShaderProgram("shadow");

  loaded = true;
}

///
/// @brief Gets terrain tile for each world position
/// @param pos = world position
/// @retval Pointer to terrain tile
///
const PTerrainTile *PTerrain::getTileAtPos(const vec3f &pos) const
{
  int tilex = pos.x * scale_tile_inv;
  int tiley = pos.y * scale_tile_inv;

  if (pos.x < 0.0)
    --tilex;
  if (pos.y < 0.0)
    --tiley;

  for (std::list<PTerrainTile>::const_iterator iter = tile.begin(); iter != tile.end(); ++iter) {
    if (iter->posx == tilex && iter->posy == tiley) {
      return &*iter;
    }
  }
  return nullptr;
}

///
/// @brief Gets vector of objects on tile at world position
/// @param pos = world position
/// @retval Pointer to world objects on terrain tile
///
const std::vector<PTerrainFoliage> *PTerrain::getFoliageAtPos(const vec3f &pos) const
{
  const PTerrainTile *tile = getTileAtPos(pos);

  if (tile) {
    return &tile->straight;
  }
  return nullptr;
}

PTerrainTile *PTerrain::getTile(int tilex, int tiley)
{
  // find the least recently used tile while searching for x,y
  int best_lru = 0, unused = 0;
  PTerrainTile *tileptr = nullptr;
  for (std::list<PTerrainTile>::iterator iter = tile.begin();
    iter != tile.end(); ++iter) {
    if (iter->posx == tilex && iter->posy == tiley) {
      iter->lru_counter = 0;
      //PUtil::outLog() << "1: " << tilex << " " << tiley << std::endl;
      return &*iter;
    }

    if (best_lru < iter->lru_counter) {
      best_lru = iter->lru_counter;
      tileptr = &*iter;
    }

    if (iter->lru_counter > 1) ++unused;
  }

  // if there aren't enough unused tiles, create a new one

  if (unused < 10 || best_lru <= 1) {
    tile.push_back(PTerrainTile());
    tileptr = &tile.back();
  }

  tileptr->posx = tilex;
  tileptr->posy = tiley;
  tileptr->lru_counter = 0;

  tileptr->mins = vec3f((float)tilex * scale_hz, (float)tiley * scale_hz, 1000000000.0);
  tileptr->maxs = vec3f((float)(tilex+1) * scale_hz, (float)(tiley+1) * scale_hz, -1000000000.0);

  // TODO: quadtree based thing

  //std::vector<bool>

  //static PRamFile ramfile1, ramfile2;

  //ramfile1.clear();

  int tileoffsety = tiley * tilesize;
  int tileoffsetx = tilex * tilesize;
  int tilesizep1 = tilesize + 1;

  tileptr->numverts = tilesizep1 * tilesizep1;
  float* vbo = new float[tileptr->numverts * 3];

  int index = 0;

  for (int y=0; y<tilesizep1; ++y) {
    int posy = tileoffsety + y;
    int sampley = posy & totmask;
    for (int x=0; x<tilesizep1; ++x) {
      int posx = tileoffsetx + x;
      int samplex = posx & totmask;
      vec3f vert = vec3f(
        (float)posx * scale_hz,
        (float)posy * scale_hz,
        (float)hmap[(sampley * totsize) + samplex]);
      //ramfile1.write(vert, sizeof(vec3f));
      vbo[index*3 + 0] = vert.x;
      vbo[index*3 + 1] = vert.y;
      vbo[index*3 + 2] = vert.z;
      index++;
      if (tileptr->mins.z > vert.z)
        tileptr->mins.z = vert.z;
      if (tileptr->maxs.z < vert.z)
        tileptr->maxs.z = vert.z;
    }
  }
  //tileptr->vert.create(ramfile1.getSize(), PVBuffer::VertexContent, PVBuffer::StaticUsage, ramfile1.getData());
  tileptr->vao = new VAO(
    vbo, tileptr->numverts * 3 * sizeof(float),
    indices, numinds * sizeof(unsigned short)
  );

  delete[] vbo;

  //tileptr->maxs.z += 10.0;

  //tileptr->mins = vec3f((float)tilex * scale_hz, (float)tiley * scale_hz, 0.0);
  //tileptr->maxs = vec3f((float)(tilex+1) * scale_hz, (float)(tiley+1) * scale_hz, 100.0);

  tileptr->tex.loadPiece(cmap,
    (tilex * cmaptilesize) & cmaptotmask, (tiley * cmaptilesize) & cmaptotmask,
    cmaptilesize, cmaptilesize, true, true);

  // Create foliage

  srand(1);

  tileptr->foliage.resize(foliageband.size());

  for (unsigned int b = 0; b < foliageband.size(); b++) {

    tileptr->foliage[b].inst.clear();

    // Create foliage instances

    for (int i = 0; i < foliageband[b].trycount; i++) {
      float rigidityvalue = 0.0f;
      vec2f ftry = vec2f(
        (float)((tileptr->posx * tilesize) + rand01 * tilesize) * scale_hz,
        (float)((tileptr->posy * tilesize) + rand01 * tilesize) * scale_hz);

      float fol = getFoliageLevel(ftry.x, ftry.y);

      if ((1.0 - fabs((fol - foliageband[b].middle) / foliageband[b].range)) < rand01) continue;

      tileptr->foliage[b].inst.push_back(PTerrainFoliage());
      tileptr->foliage[b].inst.back().pos.x = ftry.x;
      tileptr->foliage[b].inst.back().pos.y = ftry.y;
      tileptr->foliage[b].inst.back().pos.z = getHeight(ftry.x, ftry.y);
      tileptr->foliage[b].inst.back().ang = rand01 * PI*2.0f;
      //tileptr->foliage[b].inst.back().scale = (1.0f + fol * 0.5f) * (rand01 * rand01 + 0.5) * 1.4;
      //tileptr->foliage[b].inst.back().scale = (foliageband[b].scalemin + fol * 0.5f) * (rand01 * rand01 + 0.5) * foliageband[b].scalemax;
      tileptr->foliage[b].inst.back().scale = (foliageband[b].scale + fol * 0.5f) * (rand01 * rand01 + 0.5) * 1.4;

      rigidityvalue = rigidity.getRigidity(foliageband[b].sprite_tex->getName());
      if (rigidityvalue != 0.0f) {
        tileptr->foliage[b].inst.back().rigidity = rigidityvalue;
        tileptr->straight.push_back(tileptr->foliage[b].inst.back());
      }
    }

    // Create vertex buffers for rendering

#define HMULT   1.0
#define VMULT   2.0
    // TODO: this also requires inspection and rewrite
    //ramfile1.clear();
    //ramfile2.clear();

    tileptr->foliage[b].numvert = 0;
    tileptr->foliage[b].numelem = 0;

    vbo = new float[tileptr->foliage[b].inst.size() * foliageband[b].sprite_count * 4 * 5];
    unsigned short* ibo = new unsigned short[tileptr->foliage[b].inst.size() * foliageband[b].sprite_count * 6];

    PVert_tv* pvt = (PVert_tv*)vbo;

    float angincr = PI / (float)foliageband[b].sprite_count;
    for (unsigned int j=0; j<tileptr->foliage[b].inst.size(); j++) {
      for (float anga = 0.0f; anga < PI - 0.01f; anga += angincr) {
        float interang = tileptr->foliage[b].inst[j].ang + anga;
        int stv = tileptr->foliage[b].numvert;

        PVert_tv tmpv;

        tmpv.xyz = tileptr->foliage[b].inst[j].pos +
          vec3f(cos(interang)*HMULT,sin(interang)*HMULT,0.0f) * tileptr->foliage[b].inst[j].scale;
        tmpv.st = vec2f(1.0f,0.0f);
        //ramfile1.write(&tmpv,sizeof(PVert_tv));
        pvt[tileptr->foliage[b].numvert] = tmpv;
        tileptr->foliage[b].numvert++;

        tmpv.xyz = tileptr->foliage[b].inst[j].pos +
          vec3f(-cos(interang)*HMULT,-sin(interang)*HMULT,0.0f) * tileptr->foliage[b].inst[j].scale;
        tmpv.st = vec2f(0.0f,0.0f);
        //ramfile1.write(&tmpv,sizeof(PVert_tv));
        pvt[tileptr->foliage[b].numvert] = tmpv;
        tileptr->foliage[b].numvert++;

        tmpv.xyz = tileptr->foliage[b].inst[j].pos +
          vec3f(-cos(interang)*HMULT,-sin(interang)*HMULT,VMULT) * tileptr->foliage[b].inst[j].scale;
        tmpv.st = vec2f(0.0f,1.0f/*-1.0f/32.0f*/);
        //ramfile1.write(&tmpv,sizeof(PVert_tv));
        pvt[tileptr->foliage[b].numvert] = tmpv;
        tileptr->foliage[b].numvert++;

        tmpv.xyz = tileptr->foliage[b].inst[j].pos +
          vec3f(cos(interang)*HMULT,sin(interang)*HMULT,VMULT) * tileptr->foliage[b].inst[j].scale;
        tmpv.st = vec2f(1.0f,1.0f/*-1.0f/32.0f*/);
        //ramfile1.write(&tmpv,sizeof(PVert_tv));
        pvt[tileptr->foliage[b].numvert] = tmpv;
        tileptr->foliage[b].numvert++;

        int ind;
        ind = stv + 0;
        //ramfile2.write(&ind,sizeof(uint32));
        ibo[tileptr->foliage[b].numelem] = ind;
        tileptr->foliage[b].numelem++;
        ind = stv + 1;
        //ramfile2.write(&ind,sizeof(uint32));
        ibo[tileptr->foliage[b].numelem] = ind;
        tileptr->foliage[b].numelem++;
        ind = stv + 2;
        //ramfile2.write(&ind,sizeof(uint32));
        ibo[tileptr->foliage[b].numelem] = ind;
        tileptr->foliage[b].numelem++;
        ind = stv + 0;
        //ramfile2.write(&ind,sizeof(uint32));
        ibo[tileptr->foliage[b].numelem] = ind;
        tileptr->foliage[b].numelem++;
        ind = stv + 2;
        //ramfile2.write(&ind,sizeof(uint32));
        ibo[tileptr->foliage[b].numelem] = ind;
        tileptr->foliage[b].numelem++;
        ind = stv + 3;
        //ramfile2.write(&ind,sizeof(uint32));
        ibo[tileptr->foliage[b].numelem] = ind;
        tileptr->foliage[b].numelem++;
      }
    }

    if (tileptr->foliage[b].numelem) {
      /*tileptr->foliage[b].buff[0].create(ramfile1.getSize(),
        PVBuffer::VertexContent, PVBuffer::StaticUsage, ramfile1.getData());
      tileptr->foliage[b].buff[1].create(ramfile2.getSize(),
        PVBuffer::IndexContent, PVBuffer::StaticUsage, ramfile2.getData());*/
      tileptr->foliage[b].vao = new VAO(
        vbo, tileptr->foliage[b].numvert * 5 * sizeof(float),
        ibo, tileptr->foliage[b].numelem * sizeof(unsigned short)
      );
    }

    delete[] vbo;
    delete[] ibo;
  }

  tileptr->roadsignset.resize(roadsigns.size());

  for (unsigned int b=0; b < roadsigns.size(); ++b) {
    float rigidityvalue = 0.0f;

    vec2f ftry = vec2f(
      roadsigns[b].x * scale_hz,
      roadsigns[b].y * scale_hz);
    vec2f tilemin = vec2f(
      tileptr->posx * tilesize * scale_hz,
      tileptr->posy * tilesize * scale_hz);
    vec2f tilemax = vec2f(
      (tileptr->posx * tilesize + tilesize) * scale_hz,
      (tileptr->posy * tilesize + tilesize) * scale_hz);

    if (ftry.x >= tilemin.x && ftry.x <= tilemax.x && ftry.y >= tilemin.y && ftry.y <= tilemax.y) {
      tileptr->roadsignset[b].inst.clear();
      tileptr->roadsignset[b].inst.push_back(PTerrainFoliage());
      tileptr->roadsignset[b].inst.back().pos.x    = ftry.x;
      tileptr->roadsignset[b].inst.back().pos.y    = ftry.y;
      tileptr->roadsignset[b].inst.back().pos.z    = getHeight(ftry.x, ftry.y);
      tileptr->roadsignset[b].inst.back().ang      = roadsigns[b].deg;
      tileptr->roadsignset[b].inst.back().scale    = roadsigns[b].scale;

      rigidityvalue = rigidity.getRigidity(roadsigns[b].sprite->getName());
      if (rigidityvalue != 0.0f) {
        tileptr->roadsignset[b].inst.back().rigidity = rigidityvalue;
        tileptr->straight.push_back(tileptr->roadsignset[b].inst.back());
      }

      //ramfile1.clear();
      //ramfile2.clear();

      tileptr->roadsignset[b].numvert = 0;
      tileptr->roadsignset[b].numelem = 0;

      float angincr = PI / 1.0f;

      vbo = new float[tileptr->roadsignset[b].inst.size() * 4 * 5];
      unsigned short* ibo = new unsigned short[tileptr->roadsignset[b].inst.size() * 6];

      PVert_tv* pvt = (PVert_tv*)vbo;

      for (unsigned int j=0; j<tileptr->roadsignset[b].inst.size(); j++)
      {
        for (float anga = 0.0f; anga < PI - 0.01f; anga += angincr)
        {
          float interang = tileptr->roadsignset[b].inst[j].ang + anga;
          int stv = tileptr->roadsignset[b].numvert;
          PVert_tv tmpv;

          tmpv.xyz = tileptr->roadsignset[b].inst[j].pos +
            vec3f(cos(interang)*HMULT,sin(interang)*HMULT,0.0f) *
            tileptr->roadsignset[b].inst[j].scale;
          tmpv.st = vec2f(1.0f,0.0f);
          //ramfile1.write(&tmpv,sizeof(PVert_tv));
          pvt[tileptr->roadsignset[b].numvert] = tmpv;
          tileptr->roadsignset[b].numvert++;

          tmpv.xyz = tileptr->roadsignset[b].inst[j].pos +
            vec3f(-cos(interang)*HMULT,-sin(interang)*HMULT,0.0f) *
            tileptr->roadsignset[b].inst[j].scale;
          tmpv.st = vec2f(0.0f,0.0f);
          //ramfile1.write(&tmpv,sizeof(PVert_tv));
          pvt[tileptr->roadsignset[b].numvert] = tmpv;
          tileptr->roadsignset[b].numvert++;

          tmpv.xyz = tileptr->roadsignset[b].inst[j].pos +
            vec3f(-cos(interang)*HMULT,-sin(interang)*HMULT,VMULT) *
            tileptr->roadsignset[b].inst[j].scale;
          tmpv.st = vec2f(0.0f,1.0f/*-1.0f/32.0f*/);
          //ramfile1.write(&tmpv,sizeof(PVert_tv));
          pvt[tileptr->roadsignset[b].numvert] = tmpv;
          tileptr->roadsignset[b].numvert++;

          tmpv.xyz = tileptr->roadsignset[b].inst[j].pos +
            vec3f(cos(interang)*HMULT,sin(interang)*HMULT,VMULT) *
            tileptr->roadsignset[b].inst[j].scale;
          tmpv.st = vec2f(1.0f,1.0f/*-1.0f/32.0f*/);
          //ramfile1.write(&tmpv,sizeof(PVert_tv));
          pvt[tileptr->roadsignset[b].numvert] = tmpv;
          tileptr->roadsignset[b].numvert++;

          int ind;
          ind = stv + 0;
          //ramfile2.write(&ind,sizeof(uint32));
          ibo[tileptr->roadsignset[b].numelem] = ind;
          tileptr->roadsignset[b].numelem++;
          ind = stv + 1;
          //ramfile2.write(&ind,sizeof(uint32));
          ibo[tileptr->roadsignset[b].numelem] = ind;
          tileptr->roadsignset[b].numelem++;
          ind = stv + 2;
          //ramfile2.write(&ind,sizeof(uint32));
          ibo[tileptr->roadsignset[b].numelem] = ind;
          tileptr->roadsignset[b].numelem++;
          ind = stv + 0;
          //ramfile2.write(&ind,sizeof(uint32));
          ibo[tileptr->roadsignset[b].numelem] = ind;
          tileptr->roadsignset[b].numelem++;
          ind = stv + 2;
          //ramfile2.write(&ind,sizeof(uint32));
          ibo[tileptr->roadsignset[b].numelem] = ind;
          tileptr->roadsignset[b].numelem++;
          ind = stv + 3;
          //ramfile2.write(&ind,sizeof(uint32));
          ibo[tileptr->roadsignset[b].numelem] = ind;
          tileptr->roadsignset[b].numelem++;
        }
      }

      if (tileptr->roadsignset[b].numelem)
      {
        /*tileptr->roadsignset[b].buff[0].create(ramfile1.getSize(),
            PVBuffer::VertexContent, PVBuffer::StaticUsage, ramfile1.getData());
          tileptr->roadsignset[b].buff[1].create(ramfile2.getSize(),
            PVBuffer::IndexContent, PVBuffer::StaticUsage, ramfile2.getData());*/
        tileptr->roadsignset[b].vao = new VAO(
          vbo, tileptr->roadsignset[b].numvert * 5 * sizeof(float),
          ibo, tileptr->roadsignset[b].numelem * sizeof(unsigned short)
        );
      }

      delete[] ibo;
      delete[] vbo;
    }
  }

  return tileptr;
}


void PTerrain::render(const glm::vec3 &campos, const glm::mat4 &camorim, PTexture* tex_detail, const vec3f& fog_color, float fog_density, const glm::mat4& mv, const glm::mat4& p)
{
  //float blah = camorim.row[0][0]; blah = blah; // unused

  // increase all lru counters
  for (std::list<PTerrainTile>::iterator iter = tile.begin();
    iter != tile.end(); ++iter) ++iter->lru_counter;

  // get frustum
  /*frustumf frust;
  {
    mat44f mat_mv, mat_p, mat_c;

    glGetFloatv(GL_MODELVIEW_MATRIX, mat_mv);
    glGetFloatv(GL_PROJECTION_MATRIX, mat_p);

    mat_c = mat_mv.concatenate(mat_p);

    frust.construct(mat_c);
  }*/

  int ctx = (int)(campos.x * scale_tile_inv);
  if (campos.x < 0.0) --ctx;
  int cty = (int)(campos.y * scale_tile_inv);
  if (campos.y < 0.0) --cty;

  int mintx = ctx - 3,
    maxtx = ctx + 4,
    minty = cty - 3,
    maxty = cty + 4;

  // Determine list of tiles to draw

  std::list<PTerrainTile *> drawtile;

  for (int ty = minty; ty < maxty; ++ty) {
    for (int tx = mintx; tx < maxtx; ++tx) {
      drawtile.push_back(getTile(tx,ty));
    }
  }

  // Draw terrain
  glm::vec3 tex_gen_parameters = glm::vec3(0.0f, 0.0f, scale_tile_inv);

  sp_tile->use();

  glm::vec3 f_color(fog_color[0], fog_color[1], fog_color[2]);
  sp_tile->uniform("fog_density", fog_density);
  sp_tile->uniform("fog_color", f_color);

  glActiveTexture(GL_TEXTURE1);
  //glEnable(GL_TEXTURE_2D);
  tex_detail->bind();
  sp_tile->uniform("detail", 1);
  glActiveTexture(GL_TEXTURE0);

  sp_tile->uniform("p", p);
  sp_tile->uniform("mv", mv);

  for (std::list<PTerrainTile *>::iterator t = drawtile.begin(); t != drawtile.end(); t++) {
    //if (frust.isAABBOutside(tileptr->mins, tileptr->maxs))
    //    glColor3f(1,0,0);
    //else
    //    glColor3f(1,1,1);

    tex_gen_parameters.x = (float) (- (*t)->posx);
    tex_gen_parameters.y = (float) (- (*t)->posy);

    // Texture
    glActiveTexture(GL_TEXTURE0);
    (*t)->tex.bind();
    sp_tile->uniform("tile", 0);

    // Vertex buffers
    (*t)->vao->bind();
    sp_tile->attrib("position", 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT), 0);
    sp_tile->uniform("tex_gen_parameters", tex_gen_parameters);

    glDrawElements(GL_TRIANGLE_STRIP, numinds, GL_UNSIGNED_SHORT, 0);

    (*t)->vao->unbind();
  }
  sp_tile->unuse();

  // Don't apply terrain detail texture to foliage.
  // http://sourceforge.net/p/trigger-rally/discussion/527953/thread/b53361ba/
  glActiveTexture(GL_TEXTURE1);
  //glDisable(GL_TEXTURE_2D);
  glActiveTexture(GL_TEXTURE0);

  // Draw foliage
  #if 1
  glDisable(GL_CULL_FACE);

  sp_terrain->use();
  sp_terrain->uniform("p", p);
  sp_terrain->uniform("mv", mv);
  for (unsigned int b = 0; b < foliageband.size(); b++) {
    glActiveTexture(GL_TEXTURE0);
    foliageband[b].sprite_tex->bind();
    sp_terrain->uniform("image", 0);

    for (std::list<PTerrainTile *>::iterator t = drawtile.begin(); t != drawtile.end(); t++) {

      if ((*t)->foliage[b].numelem) {
        (*t)->foliage[b].vao->bind();

        sp_terrain->attrib("tex_coord", 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 0);
        sp_terrain->attrib("position", 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 2 * sizeof(GL_FLOAT));
        glDrawElements(GL_TRIANGLES, (*t)->foliage[b].numelem, GL_UNSIGNED_SHORT, 0);
        (*t)->foliage[b].vao->unbind();
      }

      #if 0
      for (std::vector<PTerrainFoliage>::iterator f = (*t)->foliage.begin(); f != (*t)->foliage.end(); f++) {

        #if 0
        glBegin(GL_LINES);
        vec3f pos = f->pos;
        glVertex3fv(pos);
        pos += vec3f(0.0f, 0.0f, 2.0f);
        glVertex3fv(pos);
        glEnd();
        #endif

        #if 0
        if (!f->tfb->model) continue;

        glPushMatrix();
        vec3f &pos = f->pos;
        glTranslatef(pos.x, pos.y, pos.z);
        glScalef(f->tfb->modelscale, f->tfb->modelscale, f->tfb->modelscale);
        ssRender.drawModel(*f->tfb->model, ssEffect, ssTexture);
        glPopMatrix();
        #endif
      }
      #endif
    }
  }

  // Using same shader
  // draw road signs
  for (unsigned int b=0; b < roadsigns.size(); ++b)
  {
    roadsigns[b].sprite->bind();

    for (std::list<PTerrainTile *>::iterator t = drawtile.begin(); t != drawtile.end(); t++) {
      if ((*t)->roadsignset[b].numelem)
      {
        (*t)->roadsignset[b].vao->bind();
        sp_terrain->attrib("tex_coord", 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 0);
        sp_terrain->attrib("position", 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 2 * sizeof(GL_FLOAT));

        glDrawElements(GL_TRIANGLES, (*t)->roadsignset[b].numelem, GL_UNSIGNED_SHORT, 0);

        (*t)->roadsignset[b].vao->unbind();
      }
    }
  }
  sp_terrain->unuse();

  #endif

  glEnable(GL_CULL_FACE);
}

/*
 * TODO: This function looks overengineered. That's not how you should draw a shadow.
 * I'll keep in until I implement proper shadows.
 */
void PTerrain::drawShadow(float x, float y, float scale, float angle, PTexture* tex_shadow, const glm::mat4& mv, const glm::mat4& p)
{
  float *hmd = &hmap[0];
  int cx = totsize;
  int cy = totsize;

  x *= scale_hz_inv;
  y *= scale_hz_inv;

  scale *= 0.5f;

  int miny = (int)(y - scale);
  if ((y - scale) < 0.0f) miny--;
  int maxy = (int)(y + scale) + 1;
  if ((y + scale) < 0.0f) maxy--;
  int minx = (int)(x - scale);
  if ((x - scale) < 0.0f) minx--;
  int maxx = (int)(x + scale) + 2;
  if ((x + scale) < 0.0f) maxx--;

  glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.0f));
  t = glm::rotate(t, angle, glm::vec3(0.0f, 0.0f, 1.0f));
  t = glm::scale(t, glm::vec3(0.5f / scale, 0.5f / scale, 1.0f));
  t = glm::translate(t, glm::vec3(-x, -y, 0.0f));

  float* vbo = new float[(maxx-minx)*(maxy-miny+1)*5]; // N times M+1 vertices, each having 2+3 attributes

  int x_stride = maxx-minx;

  for (int y2=miny; y2<=maxy; y2++) {
    int yc = y2 & (cy-1); // Y modulo tile size
    int yc_cx = yc * cx;  // Offset for Y

    int y3 = y2 - miny;

    for (int x2=minx; x2<maxx; x2++) {
      int xc = x2 & (cx - 1);

      int x3 = x2 - minx;

      glm::vec4 tex_v = glm::vec4(x2, y2, 0.0f, 1.0f);
      tex_v = t * tex_v;

      vbo[(y3 * x_stride + x3)*5 + 0] = tex_v.x;
      vbo[(y3 * x_stride + x3)*5 + 1] = tex_v.y;

      vbo[(y3 * x_stride + x3)*5 + 2] = x2 * scale_hz;
      vbo[(y3 * x_stride + x3)*5 + 3] = y2 * scale_hz;
      vbo[(y3 * x_stride + x3)*5 + 4] = hmd[yc_cx + xc] + 0.05; // TODO: hack to avoid Z-fighting for shadow
    }
  }

  unsigned short* ibo = new unsigned short[(maxx-minx+1)*(maxy-miny)*2]; // N times M squares, each having 2 triangles with 3 vertices

  for (int y2 = 0; y2 < (maxy-miny); y2++) {
    for (int x2 = 0; x2 < (maxx-minx); x2++) {
      ibo[(y2 * (x_stride+1) + x2)*2 + 0] = (y2 + 1) * (x_stride) + (x2 + 0);
      ibo[(y2 * (x_stride+1) + x2)*2 + 1] = (y2 + 0) * (x_stride) + (x2 + 0);
    }
    ibo[(y2 * (x_stride+1) + (maxx-minx))*2 + 0] = 0; // Restart strip
    ibo[(y2 * (x_stride+1) + (maxx-minx))*2 + 1] = 0;
  }

  VAO vao(
    vbo, (maxx-minx)*(maxy-miny+1)*5*sizeof(float),
    ibo, (maxx-minx+1)*(maxy-miny)*2*sizeof(unsigned short)
  );

  vao.bind();
  sp_shadow->use();
  sp_shadow->attrib("tex_coord", 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 0);
  sp_shadow->attrib("position", 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GL_FLOAT), 2 * sizeof(float));

  glActiveTexture(GL_TEXTURE0);
  tex_shadow->bind();
  sp_shadow->uniform("shadow", 0);
  sp_shadow->uniform("mv", mv);
  sp_shadow->uniform("p", p);

  //glInterleavedArrays(GL_T2F_V3F, 5*sizeof(GL_FLOAT), vbo);
  glDrawElements(GL_TRIANGLE_STRIP, (maxx-minx+1)*(maxy-miny)*2, GL_UNSIGNED_SHORT, 0);

  delete[] ibo;
  delete[] vbo;

  sp_shadow->unuse();
}



