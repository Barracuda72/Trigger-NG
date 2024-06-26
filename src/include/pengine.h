
// pengine.h [pengine]

// Copyright 2004-2006 Jasmine Langridge, jas@jareiko.net
// License: GPL version 2 (see included gpl.txt)


#ifndef PENGINE_H_INCLUDED
#define PENGINE_H_INCLUDED

#include <stdlib.h>

#include <iostream>

#include <vector>
#include <list>
#include <string>
#include <utility>


// "SDL.h" is supposed to be the portable way, but it
// doesn't seem to work in some circumstances...
//#include "SDL.h"
#include <SDL2/SDL.h>


// The PhysicsFS game file system
#include <physfs.h>

#ifdef GLES2
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengles2.h>
#include <SDL2/SDL_opengles2_gl2ext.h>
#else
#include <GL/glew.h>
//#include <GL/glext.h>
#endif

#include <tinyxml2.h>
using namespace tinyxml2;

// Some maths utils and macros
#include "vmath.h"

#ifdef __ANDROID__
/*
 * Simple buffer class to redirect std::cout into android log
 */
#include <android/log.h>
#include <streambuf>

class AndroidLogger : public std::streambuf
{
public:
    static const int buffer_size = 1024;

    AndroidLogger() { setp(buffer, buffer + buffer_size - 2); }

private:
    int overflow(int c)
    {
        *pptr() = traits_type::to_char_type(c);
        pbump(1);
        sync();
        return 0;
    }

    int sync()
    {
        int n = pptr() - pbase(); // N is always less than buffer_size - 1
        if (n > 0) {
            buffer[n] = 0;
            __android_log_write(ANDROID_LOG_INFO, "std", buffer);
            pbump(-n);
        }
        return 0;
    }

    char buffer[buffer_size];
};
#endif

class PUtil;

class PApp;
class PSubsystem;
class   PSSRender;
class     PParticleType;
struct    PParticle_s;
class   PSSTexture;
class     PImage;
class     PTexture;
class   PSSAudio;
class     PAudioSample;
class     PAudioInstance;
class   PSSEffect;
class     PEffect;
class   PSSModel;
class     PFace;
class     PMesh;
class     PModel;
class   PTerrain;

class PException;
class   PUserException;
class   PFileException;
class   PParseException;

class PCodriverVoice;

#include "exception.h"


// Utility

#define DEBUGLEVEL_CRITICAL     0
#define DEBUGLEVEL_ENDUSER      10
#define DEBUGLEVEL_TEST         20
#define DEBUGLEVEL_DEVELOPER    30

/// Enables an MS-style macro for use with unused function parameters.
#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(p)   static_cast<void> (p)
#endif

///
/// @brief Helper structure for storing dirt information.
///
struct dirtinfo
{
    float startsize = 0.1f;
    float endsize   = 0.5f;
    float decay     = 6.0f;

    dirtinfo() = default;

    dirtinfo(float startsize, float endsize, float decay):
        startsize(startsize),
        endsize(endsize),
        decay(decay)
    {
    }
};

///
/// @brief Helper structure for storing RGB colors.
///
struct rgbcolor
{
    uint8_t r=0;    ///< Red.
    uint8_t g=0;    ///< Green.
    uint8_t b=0;    ///< Blue.

    rgbcolor() = default;

    rgbcolor(uint8_t r, uint8_t g, uint8_t b):
        r(r),
        g(g),
        b(b)
    {
    }

    bool operator == (const rgbcolor &rhs) const
    {
        if (r == rhs.r &&
            g == rhs.g &&
            b == rhs.b)
            return true;

        return false;
    }

    bool operator != (const rgbcolor &rhs) const
    {
        if (r != rhs.r ||
            g != rhs.g ||
            b != rhs.b)
            return true;

        return false;
    }
};

#include "terrainmap.h"

///
/// @brief Class which contains helper functions
/// @todo maybe set these functions in a more ordered way?
///
class PUtil {
private:
  PUtil() { } // cannot be constructed

  static int deblev;

public:
  // Output streams
  static void initLog() {
#ifdef __ANDROID__
      std::cout.rdbuf(new AndroidLogger());
#endif
  }

  static std::ostream &outLog() { return std::cout; }

  // Debug level
  static bool isDebugLevel(int debugLevel) { return deblev >= debugLevel; }
  static void setDebugLevel(int debugLevel) { deblev = debugLevel; }

  // TODO: these two functions are probably misplaced here
  static TerrainType decideRoadSurface(const rgbcolor &c);
  static float decideFrictionCoef(TerrainType tt);
  static float decideResistance(TerrainType tt);
  static const char * getTerrainInfo(TerrainType tt);
  static dirtinfo getDirtInfo(TerrainType tt);
  static rgbcolor getTerrainColor(TerrainType tt);

  /*! Get token and value from a string line. The token is the string
   * before first space. The value, is the remaining string
   * \return true if could extract token and value */
  static bool getToken(std::string line, std::string& tok, std::string& value);

  static char* fgets2(char *s, int size, PHYSFS_file *pfile);

  // Given "data/blah/pic.jpg" will return "data/blah/"
  static std::string extractPathFromFilename(const std::string &filename);

  static std::string assemblePath(const std::string &relativefile, const std::string &parentfile);

  // Load XML file and return the root element of given name (failure: null)
  static XMLElement *loadRootElement(XMLDocument &doc, const std::string &filename, const char *rootName);

  static bool copyFile(const std::string &fileFrom, const std::string &fileTo);
  static std::list<std::string> findFiles(const std::string &basedir, const std::string &extension);

  static std::string formatInt(int value, int width);
  static std::string formatInt(int value);

  static std::string formatTime(float seconds);
  static std::string formatTimeShort(float seconds);

  // RWops created must be freed by using SDL freesrc on load
  static SDL_RWops *allocPhysFSops(PHYSFS_file *pfile);
};



#include "hiscore1.h"
#include "app.h"
#include "subsys.h"
#include "audio.h"
#include "render.h"
#include "codriver.h"


#endif // PENGINE_H_INCLUDED


