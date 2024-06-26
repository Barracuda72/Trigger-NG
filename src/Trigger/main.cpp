
// main.cpp

// Copyright 2004-2006 Jasmine Langridge, jas@jareiko.net
// License: GPL version 2 (see included gpl.txt)



#include "main.h"
#include "physfs_utils.h"

#include <SDL2/SDL_main.h>
#include <SDL2/SDL_thread.h>

#include <cctype>
#include <regex>

GLfloat MainApp::cfg_anisotropy = 1.0f;
bool MainApp::cfg_foliage = true;
bool MainApp::cfg_roadsigns = true;
bool MainApp::cfg_weather = true;

void MainApp::config()
{
    PUtil::setDebugLevel(DEBUGLEVEL_DEVELOPER);

    loadConfig();
    setScreenMode(cfg_video_cx, cfg_video_cy, cfg_video_fullscreen);
    calcScreenRatios();

    if (cfg_datadirs.empty())
        throw MakePException("Data directory paths are empty: check your trigger-rally.config file.");

    for (const std::string &datadir: cfg_datadirs)
        if (PHYSFS_mount(datadir.c_str(), NULL, 1) == 0)
        {
            PUtil::outLog() << "Failed to add PhysFS search directory \"" << datadir << "\"" << std::endl
                << "PhysFS: " << physfs_getErrorString() << std::endl;
        }
        else
        {
            PUtil::outLog() << "Main game data directory datadir=\"" << datadir << "\"" << std::endl;
            break;
        }

    if (cfg_copydefplayers)
        copyDefaultPlayers();

    best_times.loadAllTimes();
    player_unlocks = best_times.getUnlockData();

#ifndef NDEBUG
    PUtil::outLog() << "Player \"" << cfg_playername << "\" unlocks:\n";

    for (const auto &s: player_unlocks)
        PUtil::outLog() << '\t' << s << '\n';
#endif
}

void MainApp::load()
{
  psys_dirt = nullptr;

  audinst_engine = nullptr;
  audinst_wind = nullptr;
  audinst_gravel = nullptr;
  game = nullptr;

  // use PUtil, not boost
  //std::string buff = boost::str(boost::format("textures/splash/splash%u.jpg") % ((rand() % 3) + 1));
  //if (!(tex_splash_screen = getSSTexture().loadTexture(buff))) return false;

  if (!(tex_loading_screen = getSSTexture().loadTexture("/textures/splash/loading.png")))
    throw MakePException("Failed to load the Loading screen");

  if (!(tex_splash_screen = getSSTexture().loadTexture("/textures/splash/splash.jpg")))
    throw MakePException("Failed to load the Splash screen");

  appstate = AS_LOAD_1;

  loadscreencount = 3;

  splashtimeout = 0.0f;

  // Check that controls are available where requested
  // (can't be done in config because joy info not available)

  for (int i = 0; i < ActionCount; i++) {

    switch(ctrl.map[i].type) {
    case UserControl::TypeUnassigned:
      break;

    case UserControl::TypeKey:
      if (ctrl.map[i].key.sym <= 0 /* || ctrl.map[i].key.sym >= SDLK_LAST */) // `SDLK_LAST` unavailable in SDL2
        ctrl.map[i].type = UserControl::TypeUnassigned;
      break;

    case UserControl::TypeJoyButton:
      if (0 >= getNumJoysticks() || ctrl.map[i].joybutton.button >= getJoyNumButtons(0))
        ctrl.map[i].type = UserControl::TypeUnassigned;
      break;

    case UserControl::TypeJoyAxis:
      if (0 >= getNumJoysticks() || ctrl.map[i].joyaxis.axis >= getJoyNumAxes(0))
        ctrl.map[i].type = UserControl::TypeUnassigned;
      break;
    }
  }

  loadShadersAndVao();
}

namespace
{

///
/// @brief X-macro defining supported SDL keymaps.
/// @see http://wiki.libsdl.org/SDL_Keycode
///
#define STRING_TO_SDL_KEYMAP    \
    X(SDLK_UNKNOWN)             \
    X(SDLK_BACKSPACE)           \
    X(SDLK_TAB)                 \
    X(SDLK_RETURN)              \
    X(SDLK_ESCAPE)              \
    X(SDLK_SPACE)               \
    X(SDLK_EXCLAIM)             \
    X(SDLK_QUOTEDBL)            \
    X(SDLK_HASH)                \
    X(SDLK_DOLLAR)              \
    X(SDLK_PERCENT)             \
    X(SDLK_AMPERSAND)           \
    X(SDLK_QUOTE)               \
    X(SDLK_LEFTPAREN)           \
    X(SDLK_RIGHTPAREN)          \
    X(SDLK_ASTERISK)            \
    X(SDLK_PLUS)                \
    X(SDLK_COMMA)               \
    X(SDLK_MINUS)               \
    X(SDLK_PERIOD)              \
    X(SDLK_SLASH)               \
    X(SDLK_0)                   \
    X(SDLK_1)                   \
    X(SDLK_2)                   \
    X(SDLK_3)                   \
    X(SDLK_4)                   \
    X(SDLK_5)                   \
    X(SDLK_6)                   \
    X(SDLK_7)                   \
    X(SDLK_8)                   \
    X(SDLK_9)                   \
    X(SDLK_COLON)               \
    X(SDLK_SEMICOLON)           \
    X(SDLK_LESS)                \
    X(SDLK_EQUALS)              \
    X(SDLK_GREATER)             \
    X(SDLK_QUESTION)            \
    X(SDLK_AT)                  \
    X(SDLK_LEFTBRACKET)         \
    X(SDLK_BACKSLASH)           \
    X(SDLK_RIGHTBRACKET)        \
    X(SDLK_CARET)               \
    X(SDLK_UNDERSCORE)          \
    X(SDLK_BACKQUOTE)           \
    X(SDLK_a)                   \
    X(SDLK_b)                   \
    X(SDLK_c)                   \
    X(SDLK_d)                   \
    X(SDLK_e)                   \
    X(SDLK_f)                   \
    X(SDLK_g)                   \
    X(SDLK_h)                   \
    X(SDLK_i)                   \
    X(SDLK_j)                   \
    X(SDLK_k)                   \
    X(SDLK_l)                   \
    X(SDLK_m)                   \
    X(SDLK_n)                   \
    X(SDLK_o)                   \
    X(SDLK_p)                   \
    X(SDLK_q)                   \
    X(SDLK_r)                   \
    X(SDLK_s)                   \
    X(SDLK_t)                   \
    X(SDLK_u)                   \
    X(SDLK_v)                   \
    X(SDLK_w)                   \
    X(SDLK_x)                   \
    X(SDLK_y)                   \
    X(SDLK_z)                   \
    X(SDLK_DELETE)              \
    X(SDLK_CAPSLOCK)            \
    X(SDLK_F1)                  \
    X(SDLK_F2)                  \
    X(SDLK_F3)                  \
    X(SDLK_F4)                  \
    X(SDLK_F5)                  \
    X(SDLK_F6)                  \
    X(SDLK_F7)                  \
    X(SDLK_F8)                  \
    X(SDLK_F9)                  \
    X(SDLK_F10)                 \
    X(SDLK_F11)                 \
    X(SDLK_F12)                 \
    X(SDLK_PRINTSCREEN)         \
    X(SDLK_SCROLLLOCK)          \
    X(SDLK_PAUSE)               \
    X(SDLK_INSERT)              \
    X(SDLK_HOME)                \
    X(SDLK_PAGEUP)              \
    X(SDLK_END)                 \
    X(SDLK_PAGEDOWN)            \
    X(SDLK_RIGHT)               \
    X(SDLK_LEFT)                \
    X(SDLK_DOWN)                \
    X(SDLK_UP)                  \
    X(SDLK_NUMLOCKCLEAR)        \
    X(SDLK_KP_DIVIDE)           \
    X(SDLK_KP_MULTIPLY)         \
    X(SDLK_KP_MINUS)            \
    X(SDLK_KP_PLUS)             \
    X(SDLK_KP_ENTER)            \
    X(SDLK_KP_1)                \
    X(SDLK_KP_2)                \
    X(SDLK_KP_3)                \
    X(SDLK_KP_4)                \
    X(SDLK_KP_5)                \
    X(SDLK_KP_6)                \
    X(SDLK_KP_7)                \
    X(SDLK_KP_8)                \
    X(SDLK_KP_9)                \
    X(SDLK_KP_0)                \
    X(SDLK_KP_PERIOD)           \
    X(SDLK_APPLICATION)         \
    X(SDLK_POWER)               \
    X(SDLK_KP_EQUALS)           \
    X(SDLK_F13)                 \
    X(SDLK_F14)                 \
    X(SDLK_F15)                 \
    X(SDLK_F16)                 \
    X(SDLK_F17)                 \
    X(SDLK_F18)                 \
    X(SDLK_F19)                 \
    X(SDLK_F20)                 \
    X(SDLK_F21)                 \
    X(SDLK_F22)                 \
    X(SDLK_F23)                 \
    X(SDLK_F24)                 \
    X(SDLK_EXECUTE)             \
    X(SDLK_HELP)                \
    X(SDLK_MENU)                \
    X(SDLK_SELECT)              \
    X(SDLK_STOP)                \
    X(SDLK_AGAIN)               \
    X(SDLK_UNDO)                \
    X(SDLK_CUT)                 \
    X(SDLK_COPY)                \
    X(SDLK_PASTE)               \
    X(SDLK_FIND)                \
    X(SDLK_MUTE)                \
    X(SDLK_VOLUMEUP)            \
    X(SDLK_VOLUMEDOWN)          \
    X(SDLK_KP_COMMA)            \
    X(SDLK_KP_EQUALSAS400)      \
    X(SDLK_ALTERASE)            \
    X(SDLK_SYSREQ)              \
    X(SDLK_CANCEL)              \
    X(SDLK_CLEAR)               \
    X(SDLK_PRIOR)               \
    X(SDLK_RETURN2)             \
    X(SDLK_SEPARATOR)           \
    X(SDLK_OUT)                 \
    X(SDLK_OPER)                \
    X(SDLK_CLEARAGAIN)          \
    X(SDLK_CRSEL)               \
    X(SDLK_EXSEL)               \
    X(SDLK_KP_00)               \
    X(SDLK_KP_000)              \
    X(SDLK_THOUSANDSSEPARATOR)  \
    X(SDLK_DECIMALSEPARATOR)    \
    X(SDLK_CURRENCYUNIT)        \
    X(SDLK_CURRENCYSUBUNIT)     \
    X(SDLK_KP_LEFTPAREN)        \
    X(SDLK_KP_RIGHTPAREN)       \
    X(SDLK_KP_LEFTBRACE)        \
    X(SDLK_KP_RIGHTBRACE)       \
    X(SDLK_KP_TAB)              \
    X(SDLK_KP_BACKSPACE)        \
    X(SDLK_KP_A)                \
    X(SDLK_KP_B)                \
    X(SDLK_KP_C)                \
    X(SDLK_KP_D)                \
    X(SDLK_KP_E)                \
    X(SDLK_KP_F)                \
    X(SDLK_KP_XOR)              \
    X(SDLK_KP_POWER)            \
    X(SDLK_KP_PERCENT)          \
    X(SDLK_KP_LESS)             \
    X(SDLK_KP_GREATER)          \
    X(SDLK_KP_AMPERSAND)        \
    X(SDLK_KP_DBLAMPERSAND)     \
    X(SDLK_KP_VERTICALBAR)      \
    X(SDLK_KP_DBLVERTICALBAR)   \
    X(SDLK_KP_COLON)            \
    X(SDLK_KP_HASH)             \
    X(SDLK_KP_SPACE)            \
    X(SDLK_KP_AT)               \
    X(SDLK_KP_EXCLAM)           \
    X(SDLK_KP_MEMSTORE)         \
    X(SDLK_KP_MEMRECALL)        \
    X(SDLK_KP_MEMCLEAR)         \
    X(SDLK_KP_MEMADD)           \
    X(SDLK_KP_MEMSUBTRACT)      \
    X(SDLK_KP_MEMMULTIPLY)      \
    X(SDLK_KP_MEMDIVIDE)        \
    X(SDLK_KP_PLUSMINUS)        \
    X(SDLK_KP_CLEAR)            \
    X(SDLK_KP_CLEARENTRY)       \
    X(SDLK_KP_BINARY)           \
    X(SDLK_KP_OCTAL)            \
    X(SDLK_KP_DECIMAL)          \
    X(SDLK_KP_HEXADECIMAL)      \
    X(SDLK_LCTRL)               \
    X(SDLK_LSHIFT)              \
    X(SDLK_LALT)                \
    X(SDLK_LGUI)                \
    X(SDLK_RCTRL)               \
    X(SDLK_RSHIFT)              \
    X(SDLK_RALT)                \
    X(SDLK_RGUI)                \
    X(SDLK_MODE)                \
    X(SDLK_AUDIONEXT)           \
    X(SDLK_AUDIOPREV)           \
    X(SDLK_AUDIOSTOP)           \
    X(SDLK_AUDIOPLAY)           \
    X(SDLK_AUDIOMUTE)           \
    X(SDLK_MEDIASELECT)         \
    X(SDLK_WWW)                 \
    X(SDLK_MAIL)                \
    X(SDLK_CALCULATOR)          \
    X(SDLK_COMPUTER)            \
    X(SDLK_AC_SEARCH)           \
    X(SDLK_AC_HOME)             \
    X(SDLK_AC_BACK)             \
    X(SDLK_AC_FORWARD)          \
    X(SDLK_AC_STOP)             \
    X(SDLK_AC_REFRESH)          \
    X(SDLK_AC_BOOKMARKS)        \
    X(SDLK_BRIGHTNESSDOWN)      \
    X(SDLK_BRIGHTNESSUP)        \
    X(SDLK_DISPLAYSWITCH)       \
    X(SDLK_KBDILLUMTOGGLE)      \
    X(SDLK_KBDILLUMDOWN)        \
    X(SDLK_KBDILLUMUP)          \
    X(SDLK_EJECT)               \
    X(SDLK_SLEEP)

///
/// @brief Converts the string to a SDL keycode.
/// @param [in] s   The string to be converted.
/// @returns The keycode.
///
SDL_Keycode getSdlKeySym(const std::string &s)
{
#define X(SdlKey)   if (s == #SdlKey) return SdlKey;
    STRING_TO_SDL_KEYMAP
#undef X
    return SDLK_HELP;
}

}

///
/// @brief Copies default players from data to user directory.
///
void MainApp::copyDefaultPlayers() const
{
    const std::string dppsearchdir = "/defplayers"; // Default Player Profiles Search Directory
    const std::string dppdestdir = "/players"; // Default Player Profiles Destination Directory

    char **rc = PHYSFS_enumerateFiles(dppsearchdir.c_str());

    for (char **fname = rc; *fname != nullptr; ++fname)
    {
        // reject files that are already in the user directory
        if (PHYSFS_exists((dppdestdir + '/' + *fname).c_str()))
        {
            PUtil::outLog() << "Skipping copy of default player \"" << *fname << "\"" << std::endl;
            continue;
        }

        // reject files without .PLAYER extension (lowercase)
        std::smatch mr; // Match Results
        std::regex pat(R"(^([\s\w]+)(\.player)$)"); // Pattern
        std::string fn(*fname); // Filename

        if (!std::regex_search(fn, mr, pat))
            continue;

        if (!PUtil::copyFile(dppsearchdir + '/' + *fname, dppdestdir + '/' + *fname))
            PUtil::outLog() << "Couldn't copy default player \"" << *fname << "\"" << std::endl;
    }

    PHYSFS_freeList(rc);
}

///
/// @brief Returns event that unlocks the vehicle
/// @param [in] vehiclename  Vehicle name
/// @returns Event name
///
std::string MainApp::getVehicleUnlockEvent(const std::string &vehiclename) const
{
    for (unsigned int i = 0; i < events.size(); i++) {
        for (UnlockData::const_iterator iter = events[i].unlocks.begin(); iter != events[i].unlocks.end(); ++iter) {
            if (*iter == vehiclename) {
                return events[i].name;
            }
        }
    }
    return std::string();
}

///
/// @brief Load configurations from files
/// @todo Since C++11 introduced default members initializers, the defaults could
///  be set in the class declaration directly rather than in this function.
///
void MainApp::loadConfig()
{
  PUtil::outLog() << "Loading game configuration" << std::endl;

  // Set defaults

  cfg_playername = "Player";
  cfg_copydefplayers = true;

  cfg_video_cx = 640;
  cfg_video_cy = 480;
  cfg_video_fullscreen = false;

  cfg_drivingassist = 1.0f;
  cfg_enable_sound = true;
  cfg_enable_codriversigns = true;
  cfg_skip_saves = 5;
  cfg_volume_engine = 0.33f;
  cfg_volume_sfx = 1.0f;
  cfg_volume_codriver = 1.0f;
  cfg_anisotropy = 1.0f;
  cfg_foliage = true;
  cfg_roadsigns = true;
  cfg_weather = true;
  cfg_speed_unit = mph;
  cfg_speed_style = analogue;
  cfg_snowflaketype = SnowFlakeType::point;
  cfg_dirteffect = true;
  cfg_enable_fps = false;
  cfg_enable_ghost = false;

  cfg_datadirs.clear();

  hud_speedo_start_deg = MPH_ZERO_DEG;
  hud_speedo_mps_deg_mult = MPS_MPH_DEG_MULT;
  hud_speedo_mps_speed_mult = MPS_MPH_SPEED_MULT;

  ctrl.action_name[ActionForward] = std::string("forward");
  ctrl.action_name[ActionBack] = std::string("back");
  ctrl.action_name[ActionLeft] = std::string("left");
  ctrl.action_name[ActionRight] = std::string("right");
  ctrl.action_name[ActionHandbrake] = std::string("handbrake");
  ctrl.action_name[ActionRecover] = std::string("recover");
  ctrl.action_name[ActionRecoverAtCheckpoint] = std::string("recoveratcheckpoint");
  ctrl.action_name[ActionCamMode] = std::string("cammode");
  ctrl.action_name[ActionCamLeft] = std::string("camleft");
  ctrl.action_name[ActionCamRight] = std::string("camright");
  ctrl.action_name[ActionShowMap] = std::string("showmap");
  ctrl.action_name[ActionPauseRace] = std::string("pauserace");
  ctrl.action_name[ActionShowUi] = std::string("showui");
  ctrl.action_name[ActionShowCheckpoint] = std::string("showcheckpoint");
  ctrl.action_name[ActionNext] = std::string("next");

  for (int i = 0; i < ActionCount; i++) {
    ctrl.map[i].type = UserControl::TypeUnassigned;
    ctrl.map[i].value = 0.0f;
  }

  // Do config file management

  std::string cfgfilename = "trigger-rally-" PACKAGE_VERSION ".config";

  if (!PHYSFS_exists(cfgfilename.c_str())) {
#ifdef UNIX
    const std::vector<std::string> cfghidingplaces {
        "/usr/share/games/trigger-rally/"
    };

    for (const std::string &cfgpath: cfghidingplaces)
        if (PHYSFS_mount(cfgpath.c_str(), NULL, 1) == 0)
        {
            PUtil::outLog() << "Failed to add PhysFS search directory \"" <<
                cfgpath << "\"\nPhysFS: " << physfs_getErrorString() << std::endl;
        }
#endif
    PUtil::outLog() << "No user config file, copying over defaults" << std::endl;

    std::string cfgdefaults = "trigger-rally.config.defs";

    if (!PUtil::copyFile(cfgdefaults, cfgfilename)) {

      PUtil::outLog() << "Couldn't create user config file. Proceeding with defaults." << std::endl;

      cfgfilename = cfgdefaults;
    }
  }

  // Load actual settings from file

  XMLDocument xmlfile;

  XMLElement *rootelem = PUtil::loadRootElement(xmlfile, cfgfilename, "config");
  if (!rootelem) {
    PUtil::outLog() << "Error: Couldn't load configuration file" << std::endl;
#if TINYXML2_MAJOR_VERSION >= 6
    PUtil::outLog() << "TinyXML: " << xmlfile.ErrorStr() << std::endl;
#else
    PUtil::outLog() << "TinyXML: " << xmlfile.GetErrorStr1() << ' ' << xmlfile.GetErrorStr2() << std::endl;
#endif
    PUtil::outLog() << "Your data paths are probably not set up correctly" << std::endl;
    throw MakePException ("Boink");
  }

  const char *val;

  for (XMLElement *walk = rootelem->FirstChildElement();
    walk; walk = walk->NextSiblingElement()) {

    if (strcmp(walk->Value(), "player") == 0)
    {
        val = walk->Attribute("name");

        if (val != nullptr)
        {
            cfg_playername = val;
            best_times.setPlayerName(val);
        }

        val = walk->Attribute("copydefplayers");

        if (val != nullptr && std::string(val) == "no")
            cfg_copydefplayers = false;
        else
            cfg_copydefplayers = true;

        val = walk->Attribute("skipsaves");

        if (val != nullptr)
            cfg_skip_saves = std::stol(val);

        best_times.setSkipSaves(cfg_skip_saves);
    }
    else
    if (!strcmp(walk->Value(), "video")) {

        val = walk->Attribute("automatic");

        if (val != nullptr && std::string(val) == "yes")
            automaticVideoMode(true);
        else
            automaticVideoMode(false);

      val = walk->Attribute("width");
      if (val) cfg_video_cx = atoi(val);

      val = walk->Attribute("height");
      if (val) cfg_video_cy = atoi(val);

      val = walk->Attribute("fullscreen");
      if (val) {
        if (!strcmp(val, "yes"))
          cfg_video_fullscreen = true;
        else if (!strcmp(val, "no"))
          cfg_video_fullscreen = false;
      }

      val = walk->Attribute("requirergb");
      if (val) {
        if (!strcmp(val, "yes"))
          requireRGB(true);
        else if (!strcmp(val, "no"))
          requireRGB(false);
      }

      val = walk->Attribute("requirealpha");
      if (val) {
        if (!strcmp(val, "yes"))
          requireAlpha(true);
        else if (!strcmp(val, "no"))
          requireAlpha(false);
      }

      val = walk->Attribute("requiredepth");
      if (val) {
        if (!strcmp(val, "yes"))
          requireDepth(true);
        else if (!strcmp(val, "no"))
          requireDepth(false);
      }

      val = walk->Attribute("requirestencil");
      if (val) {
        if (!strcmp(val, "yes"))
          requireStencil(true);
        else if (!strcmp(val, "no"))
          requireStencil(false);
      }

      val = walk->Attribute("stereo");
      if (val) {
        if (!strcmp(val, "none"))
          setStereoMode(PApp::StereoNone);
        #ifndef GLES2
        else if (!strcmp(val, "quadbuffer"))
          setStereoMode(PApp::StereoQuadBuffer);
        #endif
        else if (!strcmp(val, "red-blue"))
          setStereoMode(PApp::StereoRedBlue);
        else if (!strcmp(val, "red-green"))
          setStereoMode(PApp::StereoRedGreen);
        else if (!strcmp(val, "red-cyan"))
          setStereoMode(PApp::StereoRedCyan);
        else if (!strcmp(val, "yellow-blue"))
          setStereoMode(PApp::StereoYellowBlue);
      }

      float sepMult = 1.0f;
      val = walk->Attribute("stereoswapeyes");
      if (val && !strcmp(val, "yes"))
        sepMult = -1.0f;

      val = walk->Attribute("stereoeyeseparation");
      if (val) {
        setStereoEyeSeperation(atof(val) * sepMult);
      }
    }
    else
    if (!strcmp(walk->Value(), "audio"))
    {
        val = walk->Attribute("enginevolume");

        if (val != nullptr)
            cfg_volume_engine = atof(val);

        val = walk->Attribute("sfxvolume");

        if (val != nullptr)
            cfg_volume_sfx = atof(val);

        val = walk->Attribute("codrivervolume");

        if (val != nullptr)
            cfg_volume_codriver = atof(val);
    }
    else
    if (!strcmp(walk->Value(), "graphics"))
    {
        val = walk->Attribute("anisotropy");

        if (val)
        {
            if (!strcmp(val, "off"))
            {
                cfg_anisotropy = 1.0f;
            }
            else
            if (!strcmp(val, "max"))
            {
                glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &cfg_anisotropy);
            }
            else // TODO: listen to the user, but don't trust him
            {
                cfg_anisotropy = atof(val);
                CLAMP_LOWER(cfg_anisotropy, 1.0f);
            }
        }

        val = walk->Attribute("foliage");

        if (val)
        {
            if (!strcmp(val, "no"))
                cfg_foliage = false;
            else // "yes"
                cfg_foliage = true;
        }

        val = walk->Attribute("roadsigns");

        if (val != nullptr)
        {
            if (strcmp(val, "no") == 0)
                cfg_roadsigns = false;
            else // yes
                cfg_roadsigns = true;
        }

        val = walk->Attribute("weather");

        if (val)
        {
            if (!strcmp(val, "no"))
                cfg_weather = false;
            else // "yes"
                cfg_weather = true;
        }

        val = walk->Attribute("snowflaketype");

        if (val)
        {
            if (!strcmp(val, "square"))
                cfg_snowflaketype = SnowFlakeType::square;
            else
            if (!strcmp(val, "textured"))
                cfg_snowflaketype = SnowFlakeType::textured;
            else // default
                cfg_snowflaketype = SnowFlakeType::point;
        }

        val = walk->Attribute("dirteffect");

        if (val)
        {
            if (!strcmp(val, "yes"))
                cfg_dirteffect = true;
            else
                cfg_dirteffect = false;
        }
    }
    else
    if (!strcmp(walk->Value(), "datadirectory"))
    {
        for (XMLElement *walk2 = walk->FirstChildElement(); walk2; walk2 = walk2->NextSiblingElement())
            if (!strcmp(walk2->Value(), "data"))
                cfg_datadirs.push_back(walk2->Attribute("path"));
    }
    else if (!strcmp(walk->Value(), "parameters")) {

      val = walk->Attribute("drivingassist");
      if (val) cfg_drivingassist = atof(val);

      val = walk->Attribute("enablesound");
      if (val) {
        if (!strcmp(val, "yes"))
          cfg_enable_sound = true;
        else if (!strcmp(val, "no"))
          cfg_enable_sound = false;
      }

        val = walk->Attribute("enablecodriversigns");

        if (val != nullptr)
        {
            if (strcmp(val, "yes") == 0)
                cfg_enable_codriversigns = true;
            else
            if (strcmp(val, "no") == 0)
                cfg_enable_codriversigns = false;
        }

      val = walk->Attribute("speedunit");
      if (val) {
        if (!strcmp(val, "mph")) {
            cfg_speed_unit = mph;
            hud_speedo_start_deg = MPH_ZERO_DEG;
            hud_speedo_mps_deg_mult = MPS_MPH_DEG_MULT;
            hud_speedo_mps_speed_mult = MPS_MPH_SPEED_MULT;
          }
        else if (!strcmp(val, "kph")) {
           cfg_speed_unit = kph;
           hud_speedo_start_deg = KPH_ZERO_DEG;
           hud_speedo_mps_deg_mult = MPS_KPH_DEG_MULT;
           hud_speedo_mps_speed_mult = MPS_KPH_SPEED_MULT;
         }
      }

      val = walk->Attribute("enablefps");
      if (val) {
        if (!strcmp(val, "yes"))
          cfg_enable_fps = true;
        else if (!strcmp(val, "no"))
          cfg_enable_fps = false;
      }

      val = walk->Attribute("enableghost");
      if (val) {
        if (!strcmp(val, "yes"))
          cfg_enable_ghost= true;
        else if (!strcmp(val, "no"))
          cfg_enable_ghost = false;
      }

      val = walk->Attribute("codriver");

      if (val != nullptr)
        cfg_codrivername = val;

      val = walk->Attribute("codriversigns");

        if (val != nullptr)
            cfg_codriversigns = val;

        val = walk->Attribute("codriversignslife");

        if (val != nullptr)
            cfg_codriveruserconfig.life = std::stof(val);

        val = walk->Attribute("codriversignsposx");

        if (val != nullptr)
            cfg_codriveruserconfig.posx = std::stof(val);

        val = walk->Attribute("codriversignsposy");

        if (val != nullptr)
            cfg_codriveruserconfig.posy = std::stof(val);

        val = walk->Attribute("codriversignsscale");

        if (val != nullptr)
            cfg_codriveruserconfig.scale = std::stof(val);

    } else if (!strcmp(walk->Value(), "controls")) {

      for (XMLElement *walk2 = walk->FirstChildElement();
        walk2; walk2 = walk2->NextSiblingElement()) {

        if (!strcmp(walk2->Value(), "keyboard")) {

          val = walk2->Attribute("enable");
          if (val && !strcmp(val, "no"))
            continue;

          for (XMLElement *walk3 = walk2->FirstChildElement();
            walk3; walk3 = walk3->NextSiblingElement()) {

            if (!strcmp(walk3->Value(), "key")) {

              val = walk3->Attribute("action");

              int a;
              for (a = 0; a < ActionCount; a++)
                if (ctrl.action_name[a] == val) break;

              if (a >= ActionCount) {
                PUtil::outLog() << "Config ctrls: Unknown action \"" << val << "\"" << std::endl;
                continue;
              }
              /*
              // TODO: implement string to keycode mapping
              val = walk3->Attribute("code");
              if (!val) {
                PUtil::outLog() << "Config ctrls: Key has no code" << std::endl;
                continue;
              }
              */

              val = walk3->Attribute("id");

              if (!val)
              {
                  PUtil::outLog() << "Config ctrls: Key has no ID" << std::endl;
                  continue;
              }

              ctrl.map[a].type = UserControl::TypeKey;
              //ctrl.map[a].key.sym = (SDLKey) atoi(val);
              ctrl.map[a].key.sym = getSdlKeySym(val);
            }
          }

        } else if (!strcmp(walk2->Value(), "joystick")) {

          val = walk2->Attribute("enable");
          if (val && !strcmp(val, "no"))
            continue;

          for (XMLElement *walk3 = walk2->FirstChildElement();
            walk3; walk3 = walk3->NextSiblingElement()) {

            if (!strcmp(walk3->Value(), "button")) {

              val = walk3->Attribute("action");

              int a;
              for (a = 0; a < ActionCount; a++)
                if (ctrl.action_name[a] == val) break;

              if (a >= ActionCount) {
                PUtil::outLog() << "Config ctrls: Unknown action \"" << val << "\"" << std::endl;
                continue;
              }

              val = walk3->Attribute("index");
              if (!val) {
                PUtil::outLog() << "Config ctrls: Joy button has no index" << std::endl;
                continue;
              }

              ctrl.map[a].type = UserControl::TypeJoyButton;
              ctrl.map[a].joybutton.button = atoi(val);

            } else if (!strcmp(walk3->Value(), "axis")) {

              val = walk3->Attribute("action");

              int a;
              for (a = 0; a < ActionCount; a++)
                if (ctrl.action_name[a] == val) break;

              if (a >= ActionCount) {
                PUtil::outLog() << "Config ctrls: Unknown action \"" << val << "\"" << std::endl;
                continue;
              }

              val = walk3->Attribute("index");
              if (!val) {
                PUtil::outLog() << "Config ctrls: Joy axis has no index" << std::endl;
                continue;
              }

              int index = atoi(val);

              bool positive;

              val = walk3->Attribute("direction");
              if (!val) {
                PUtil::outLog() << "Config ctrls: Joy axis has no direction" << std::endl;
                continue;
              }
              if (!strcmp(val, "+"))
                positive = true;
              else if (!strcmp(val, "-"))
                positive = false;
              else {
                PUtil::outLog() << "Config ctrls: Joy axis direction \"" << val <<
                  "\" is neither \"+\" nor \"-\"" << std::endl;
                continue;
              }

              ctrl.map[a].type = UserControl::TypeJoyAxis;
              ctrl.map[a].joyaxis.axis = index;
              ctrl.map[a].joyaxis.sign = positive ? 1.0f : -1.0f;
              ctrl.map[a].joyaxis.deadzone = 0.0f;
              ctrl.map[a].joyaxis.maxrange = 1.0f;

              val = walk3->Attribute("deadzone");
              if (val) ctrl.map[a].joyaxis.deadzone = atof(val);

              val = walk3->Attribute("maxrange");
              if (val) ctrl.map[a].joyaxis.maxrange = atof(val);
            }
          }
        }
      }
    }
  }
}

bool MainApp::loadLevel(TriggerLevel &tl)
{
  tl.name = "Untitled";
  tl.description = "(no description)";
  tl.comment = "";
  tl.author = "";
  tl.targettime = "";
  tl.targettimeshort = "";
  tl.targettimefloat = 0.0f;
  tl.tex_minimap = nullptr;
  tl.tex_screenshot = nullptr;

  XMLDocument xmlfile;
  XMLElement *rootelem = PUtil::loadRootElement(xmlfile, tl.filename, "level");
  if (!rootelem) {
    PUtil::outLog() << "Couldn't read level \"" << tl.filename << "\"" << std::endl;
    return false;
  }

  const char *val;

  val = rootelem->Attribute("name");
  if (val) tl.name = val;

  val = rootelem->Attribute("description");

  if (val != nullptr)
    tl.description = val;

  val = rootelem->Attribute("comment");
  if (val) tl.comment = val;
  val = rootelem->Attribute("author");
  if (val) tl.author = val;

  val = rootelem->Attribute("screenshot");

  if (val != nullptr)
    tl.tex_screenshot = getSSTexture().loadTexture(PUtil::assemblePath(val, tl.filename));

  val = rootelem->Attribute("minimap");

  if (val != nullptr)
    tl.tex_minimap = getSSTexture().loadTexture(PUtil::assemblePath(val, tl.filename));

  for (XMLElement *walk = rootelem->FirstChildElement();
    walk; walk = walk->NextSiblingElement()) {

    if (!strcmp(walk->Value(), "race")) {
      val = walk->Attribute("targettime");
      if (val)
      {
        tl.targettime = PUtil::formatTime(atof(val));
        tl.targettimeshort = PUtil::formatTimeShort(atof(val));
        tl.targettimefloat = atof(val);
      }
    }
  }

  return true;
}

bool MainApp::loadLevelsAndEvents()
{
  PUtil::outLog() << "Loading levels and events" << std::endl;

  // Find levels

  std::list<std::string> results = PUtil::findFiles("/maps", ".level");

  for (std::list<std::string>::iterator i = results.begin();
    i != results.end(); ++i) {

    TriggerLevel tl;
    tl.filename = *i;

    if (!loadLevel(tl)) continue;

    // Insert level in alphabetical order
    std::vector<TriggerLevel>::iterator j = levels.begin();
    while (j != levels.end() && j->name < tl.name) ++j;
    levels.insert(j, tl);
  }

  // Find events

  results = PUtil::findFiles("/events", ".event");

  for (std::list<std::string>::iterator i = results.begin();
    i != results.end(); ++i) {

    TriggerEvent te;

    te.filename = *i;

    XMLDocument xmlfile;
    XMLElement *rootelem = PUtil::loadRootElement(xmlfile, *i, "event");
    if (!rootelem) {
      PUtil::outLog() << "Couldn't read event \"" << *i << "\"" << std::endl;
      continue;
    }

    const char *val;

    val = rootelem->Attribute("name");
    if (val) te.name = val;
    val = rootelem->Attribute("comment");
    if (val) te.comment = val;
    val = rootelem->Attribute("author");
    if (val) te.author = val;

    val = rootelem->Attribute("locked");

    if (val != nullptr && strcmp(val, "yes") == 0)
        te.locked = true;
    else
        te.locked = false; // FIXME: redundant but clearer?

    float evtotaltime = 0.0f;

    for (XMLElement *walk = rootelem->FirstChildElement();
      walk; walk = walk->NextSiblingElement()) {

      if (strcmp(walk->Value(), "unlocks") == 0)
      {
          val = walk->Attribute("file");

          if (val == nullptr)
          {
              PUtil::outLog() << "Warning: Event has empty unlock" << std::endl;
              continue;
          }

          te.unlocks.insert(val);
      }
      else
      if (!strcmp(walk->Value(), "level")) {

        TriggerLevel tl;

        val = walk->Attribute("file");
        if (!val) {
          PUtil::outLog() << "Warning: Event level has no filename" << std::endl;
          continue;
        }
        tl.filename = PUtil::assemblePath(val, *i);

        if (loadLevel(tl))
        {
          te.levels.push_back(tl);
          evtotaltime += tl.targettimefloat;
        }

        PUtil::outLog() << tl.filename << std::endl;
      }
    }

    if (te.levels.size() <= 0) {
      PUtil::outLog() << "Warning: Event has no levels" << std::endl;
      continue;
    }

    te.totaltime = PUtil::formatTimeShort(evtotaltime);

    // Insert event in alphabetical order
    std::vector<TriggerEvent>::iterator j = events.begin();
    while (j != events.end() && j->name < te.name) ++j;
    events.insert(j, te);
  }

  return true;
}

///
/// @TODO: should also load all vehicles here, then if needed filter which
///  of them should be made available to the player -- it makes no sense
///  to reload vehicles for each race, over and over again
///
bool MainApp::loadAll()
{
  if (!(tex_fontSourceCodeBold = getSSTexture().loadTexture("/textures/font-SourceCodeProBold.png")))
    return false;

  if (!(tex_fontSourceCodeOutlined = getSSTexture().loadTexture("/textures/font-SourceCodeProBoldOutlined.png")))
    return false;

  if (!(tex_fontSourceCodeShadowed = getSSTexture().loadTexture("/textures/font-SourceCodeProBoldShadowed.png")))
    return false;

  if (!(tex_end_screen = getSSTexture().loadTexture("/textures/splash/endgame.jpg"))) return false;

  if (!(tex_hud_life = getSSTexture().loadTexture("/textures/life_helmet.png"))) return false;

  if (!(tex_detail = getSSTexture().loadTexture("/textures/detail.jpg"))) return false;
  if (!(tex_dirt = getSSTexture().loadTexture("/textures/dust.png"))) return false;
  if (!(tex_shadow = getSSTexture().loadTexture("/textures/shadow.png", true, true))) return false;

  if (!(tex_hud_revneedle = getSSTexture().loadTexture("/textures/rev_needle.png"))) return false;

  if (!(tex_hud_revs = getSSTexture().loadTexture("/textures/dial_rev.png"))) return false;

  if (!(tex_hud_offroad = getSSTexture().loadTexture("/textures/offroad.png"))) return false;

  if (!(tex_race_no_screenshot = getSSTexture().loadTexture("/textures/no_screenshot.png"))) return false;

  if (!(tex_race_no_minimap = getSSTexture().loadTexture("/textures/no_minimap.png"))) return false;

  if (!(tex_button_next = getSSTexture().loadTexture("/textures/button_next.png"))) return false;
  if (!(tex_button_prev = getSSTexture().loadTexture("/textures/button_prev.png"))) return false;

  if (!(tex_waterdefault = getSSTexture().loadTexture("/textures/water/default.png"))) return false;

  if (!(tex_snowflake = getSSTexture().loadTexture("/textures/snowflake.png"))) return false;

  if (!(tex_damage_front_left = getSSTexture().loadTexture("/textures/damage_front_left.png"))) return false;
  if (!(tex_damage_front_right = getSSTexture().loadTexture("/textures/damage_front_right.png"))) return false;
  if (!(tex_damage_rear_left = getSSTexture().loadTexture("/textures/damage_rear_left.png"))) return false;
  if (!(tex_damage_rear_right = getSSTexture().loadTexture("/textures/damage_rear_right.png"))) return false;

    if (cfg_enable_codriversigns && !cfg_codriversigns.empty())
    {
        const std::string origdir(std::string("/textures/CodriverSigns/") + cfg_codriversigns);

        char **rc = PHYSFS_enumerateFiles(origdir.c_str());

        for (char **fname = rc; *fname != nullptr; ++fname)
        {
            PTexture *tex_cdsign = getSSTexture().loadTexture(origdir + '/' + *fname);

            if (tex_cdsign != nullptr) // failed loads are ignored
            {
                // remove the extension from the filename
                std::smatch mr; // Match Results
                std::regex pat(R"(^(\w+)(\..+)$)"); // Pattern
                std::string fn(*fname); // Filename

                if (!std::regex_search(fn, mr, pat))
                    continue;

                std::string basefname = mr[1];

                // make the base filename lowercase
                for (char &c: basefname)
                    c = std::tolower(static_cast<unsigned char> (c));

                tex_codriversigns[basefname] = tex_cdsign;
                //PUtil::outLog() << "Loaded codriver sign for: \"" << basefname << '"' << std::endl;
            }
        }

        PHYSFS_freeList(rc);
    }

  if (cfg_enable_sound) {
    if (!(aud_engine = getSSAudio().loadSample("/sounds/engine.wav", false))) return false;
    if (!(aud_wind = getSSAudio().loadSample("/sounds/wind.wav", false))) return false;
    if (!(aud_shiftup = getSSAudio().loadSample("/sounds/shiftup.wav", false))) return false;
    if (!(aud_shiftdown = getSSAudio().loadSample("/sounds/shiftdown.wav", false))) return false;
    if (!(aud_gravel = getSSAudio().loadSample("/sounds/gravel.wav", false))) return false;
    if (!(aud_crash1 = getSSAudio().loadSample("/sounds/bang.wav", false))) return false;

    if (!cfg_codrivername.empty() && cfg_codrivername != "mime")
    {
        const std::string origdir(std::string("/sounds/codriver/") + cfg_codrivername);

        char **rc = PHYSFS_enumerateFiles(origdir.c_str());

        for (char **fname = rc; *fname != nullptr; ++fname)
        {
            PAudioSample *aud_cdword = getSSAudio().loadSample(origdir + '/' + *fname, false);

            if (aud_cdword != nullptr) // failed loads are ignored
            {
                // remove the extension from the filename
                std::smatch mr; // Match Results
                std::regex pat(R"(^(\w+)(\..+)$)"); // Pattern
                std::string fn(*fname); // Filename

                if (!std::regex_search(fn, mr, pat))
                    continue;

                std::string basefname = mr[1];

                // make the base filename lowercase
                for (char &c: basefname)
                    c = std::tolower(static_cast<unsigned char> (c));

                aud_codriverwords[basefname] = aud_cdword;
                //PUtil::outLog() << "Loaded codriver word for: \"" << basefname << '"' << std::endl;
            }
        }

        PHYSFS_freeList(rc);
    }
  }

  if (!gui.loadColors("/menu.colors"))
    PUtil::outLog() << "Couldn't load (all) menu colors, continuing with defaults" << std::endl;

  gui.loadVaoShader();

  if (!loadLevelsAndEvents()) {
    PUtil::outLog() << "Couldn't load levels/events" << std::endl;
    return false;
  }

  //quatf tempo;
  //tempo.fromThreeAxisAngle(vec3f(-0.38, -0.38, 0.0));
  //vehic->getBody().setOrientation(tempo);

  campos = campos_prev = vec3f(-15.0,0.0,30.0);
  //camori.fromThreeAxisAngle(vec3f(-1.0,0.0,1.5));
  camori = quatf::identity();

  camvel = vec3f::zero();

  cloudscroll = 0.0f;

  cprotate = 0.0f;

  cameraview = CameraMode::chase;
  camera_user_angle = 0.0f;

  showmap = true;

  pauserace = false;

  showui = true;

  showcheckpoint = true;

  crashnoise_timeout = 0.0f;

    if (cfg_dirteffect)
    {
        psys_dirt = new DirtParticleSystem();
        psys_dirt->setColorStart(0.5f, 0.4f, 0.2f, 1.0f);
        psys_dirt->setColorEnd(0.5f, 0.4f, 0.2f, 0.0f);
        psys_dirt->setSize(0.1f, 0.5f);
        psys_dirt->setDecay(6.0f);
        psys_dirt->setTexture(tex_dirt);
        psys_dirt->setBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
        psys_dirt = nullptr;

  //

  choose_type = 0;

  choose_spin = 0.0f;

  return true;
}

void MainApp::loadShadersAndVao()
{
    rpm_dial_vao = new VAO(
        rpm_dial_vbo, 5 * 4 * sizeof(float),
        rpm_dial_ibo, 4 * sizeof(unsigned short)
    );

    map_marker_vao = new VAO(
        map_marker_vbo, 5 * 3 * sizeof(float),
        map_marker_ibo, 6 * sizeof(unsigned short)
    );

    bckgnd_vao = new VAO(
        bckgnd_vbo, 5 * 4 * sizeof(GL_FLOAT),
        bckgnd_ibo, 4 * sizeof(unsigned short)
    );

    snow_vao = new VAO(
        snow_vbo, 5 * 4 * sizeof(GL_FLOAT),
        snow_ibo, 4 * sizeof(unsigned short)
    );

    map_vao = new VAO(
        map_vbo, 8 * sizeof(float),
        map_ibo, 4 * sizeof(unsigned short)
    );

    offroad_vao = new VAO(
        offroad_vbo, 16 * sizeof(float),
        offroad_ibo, 4 * sizeof(unsigned short)
    );

    damage_vao = new VAO(
        damage_vbo, 16 * sizeof(float),
        damage_ibo, 4 * sizeof(unsigned short)
    );

    buildSkyVao();
    buildChkptVao();

    sp_map_marker = new ShaderProgram("map_marker");
    sp_rpm_dial = new ShaderProgram("rpm_dial");
    sp_rpm_needle = new ShaderProgram("rpm_needle");
    sp_water = new ShaderProgram("water");
    sp_bckgnd = new ShaderProgram("bckgnd");
    sp_rain = new ShaderProgram("rain");
    sp_snow = new ShaderProgram("snow");
    sp_chkpt = new ShaderProgram("chkpt");
    sp_map = new ShaderProgram("map");
    sp_sky = new ShaderProgram("sky");
    sp_offroad = new ShaderProgram("offroad_sign");
    sp_damage = new ShaderProgram("damage");
}

void MainApp::unload()
{
  delete sp_map_marker;
  delete sp_rpm_dial;
  delete sp_rpm_needle;
  delete sp_water;
  delete sp_bckgnd;
  delete sp_rain;
  delete sp_snow;
  delete sp_chkpt;
  delete sp_map;
  delete sp_sky;
  delete sp_offroad;
  delete sp_damage;

  delete map_marker_vao;
  delete rpm_dial_vao;
  delete bckgnd_vao;
  delete snow_vao;
  delete sky_vao;
  delete chkpt_vao;
  delete map_vao;
  delete offroad_vao;
  delete damage_vao;

  endGame(Gamefinish::not_finished);

  delete psys_dirt;
}

///
/// @brief Prepare to start a new game (a race)
/// @param filename = filename of the level (track) to load
///
bool MainApp::startGame(const std::string &filename)
{
  PUtil::outLog() << "Starting level \"" << filename << "\"" << std::endl;

  // mouse is grabbed during the race
  grabMouse(true);

  // the game
  game = new TriggerGame(this);

  // load vehicles
  if (!game->loadVehicles())
  {
      PUtil::outLog() << "Error: failed to load vehicles" << std::endl;
      return false;
  }

  // load the vehicle
  if (!game->loadLevel(filename)) {
    PUtil::outLog() << "Error: failed to load level" << std::endl;
    return false;
  }

  // useful datas
  race_data.playername  = cfg_playername; // TODO: move to a better place
  race_data.mapname     = filename;
  choose_type = 0;

  // if there is more than a vehicle to choose from, choose it
  if (game->vehiclechoices.size() > 1) {
    appstate = AS_CHOOSE_VEHICLE;
  } else {
    game->chooseVehicle(game->vehiclechoices[choose_type]);
    if (cfg_enable_ghost)
      ghost.recordStart(filename, game->vehiclechoices[choose_type]->getName());

    if (lss.state == AM_TOP_LVL_PREP)
    {
        const float bct = best_times.getBestClassTime(
            filename,
            game->vehicle.front()->type->proper_class);

        if (bct >= 0.0f)
            game->targettime = bct;
    }

    initAudio();
    appstate = AS_IN_GAME;
  }

  // load the sky texture
  tex_sky[0] = nullptr;

  if (game->weather.cloud.texname.length() > 0)
    tex_sky[0] = getSSTexture().loadTexture(game->weather.cloud.texname);

  // if there is none load default
  if (tex_sky[0] == nullptr) {
    tex_sky[0] = getSSTexture().loadTexture("/textures/sky/blue.jpg");

    if (tex_sky[0] == nullptr) tex_sky[0] = tex_detail; // last fallback...
  }

  // load water texture
  tex_water = nullptr;

  if (!game->water.texname.empty())
    tex_water = getSSTexture().loadTexture(game->water.texname);

  // if there is none load water default
  if (tex_water == nullptr)
    tex_water = tex_waterdefault;

  fpstime = 0.0f;
  fpscount = 0;
  fps = 0.0f;

  return true;
}

///
/// @brief Turns game sound effects on or off.
/// @note Codriver voice unaffected.
/// @param to       State to switch to (true is on, false is off).
///
void MainApp::toggleSounds(bool to)
{
    if (cfg_enable_sound)
    {
        if (audinst_engine != nullptr)
        {
            if (!to)
            {
                audinst_engine->setGain(0.0f);
                audinst_engine->play();
            }
        }

        if (audinst_wind != nullptr)
        {
            if (!to)
            {
                audinst_wind->setGain(0.0f);
                audinst_wind->play();
            }
        }

        if (audinst_gravel != nullptr)
        {
            if (!to)
            {
                audinst_gravel->setGain(0.0f);
                audinst_gravel->play();
            }
        }
    }
}

///
/// @brief Initialize game sounds instances
///
void MainApp::initAudio()
{
  if (cfg_enable_sound) {
	// engine sound
    audinst_engine = new PAudioInstance(aud_engine, true);
    audinst_engine->setGain(0.0);
    audinst_engine->play();

	// wind sound
    audinst_wind = new PAudioInstance(aud_wind, true);
    audinst_wind->setGain(0.0);
    audinst_wind->play();

	// terrain sound
    audinst_gravel = new PAudioInstance(aud_gravel, true);
    audinst_gravel->setGain(0.0);
    audinst_gravel->play();
  }
}

void MainApp::endGame(Gamefinish state)
{
  float coursetime = (state == Gamefinish::not_finished) ? 0.0f :
    game->coursetime + game->uservehicle->offroadtime_total * game->offroadtime_penalty_multiplier;

    if (state != Gamefinish::not_finished && lss.state != AM_TOP_EVT_PREP)
    {
        race_data.carname   = game->vehicle.front()->type->proper_name;
        race_data.carclass  = game->vehicle.front()->type->proper_class;
        race_data.totaltime = game->coursetime + game->uservehicle->offroadtime_total * game->offroadtime_penalty_multiplier;
        race_data.maxspeed  = 0.0f; // TODO: measure this too
        //PUtil::outLog() << race_data;
        current_times = best_times.insertAndGetCurrentTimesHL(race_data);
        best_times.skipSavePlayer();

        // show the best times
        if (lss.state == AM_TOP_LVL_PREP)
            lss.state = AM_TOP_LVL_TIMES;
        else
        if (lss.state == AM_TOP_PRAC_SEL_PREP)
            lss.state = AM_TOP_PRAC_TIMES;
    }

  if (cfg_enable_ghost && state != Gamefinish::not_finished) {
    ghost.recordStop(race_data.totaltime);
  }

  if (audinst_engine) {
    delete audinst_engine;
    audinst_engine = nullptr;
  }

  if (audinst_wind) {
    delete audinst_wind;
    audinst_wind = nullptr;
  }

  if (audinst_gravel) {
    delete audinst_gravel;
    audinst_gravel = nullptr;
  }

  for (unsigned int i=0; i<audinst.size(); i++) {
    delete audinst[i];
  }
  audinst.clear();

  if (game) {
    delete game;
    game = nullptr;
  }

  finishRace(state, coursetime);
}

///
/// @brief Calculate screen ratios from the current screen width and height.
/// @details Sets `hratio` and `vratio` member data in accordance to the values of
///  `getWidth()` (screen width) and `getHeight()` (screen height).
///  This data is important for proper scaling on widescreen monitors.
///
void MainApp::calcScreenRatios()
{
    const int cx = getWidth();
    const int cy = getHeight();

    if (cx > cy)
    {
        hratio = static_cast<double> (cx) / cy;
        vratio = 1.0;
    }
    else
    if (cx < cy)
    {
        hratio = 1.0;
        vratio = static_cast<double> (cy) / cx;
    }
    else
    {
        hratio = 1.0;
        vratio = 1.0;
    }
}

void MainApp::tick(float delta)
{
    getSSAudio().tick();

  switch (appstate) {
  case AS_LOAD_1:
    splashtimeout -= delta;
    if (--loadscreencount <= 0)
      appstate = AS_LOAD_2;
    break;
  case AS_LOAD_2:
    splashtimeout -= delta;
    if (!loadAll()) {
      requestExit();
      return;
    }
    appstate = AS_LOAD_3;
    break;
  case AS_LOAD_3:
    splashtimeout -= delta;
    if (splashtimeout <= 0.0f)
      levelScreenAction(AA_INIT, 0);
    break;

  case AS_LEVEL_SCREEN:
    tickStateLevel(delta);
    break;

  case AS_CHOOSE_VEHICLE:
    tickStateChoose(delta);
    break;

  case AS_IN_GAME:
      if (!pauserace)
        tickStateGame(delta);
    break;

  case AS_END_SCREEN:
    splashtimeout += delta * 0.04f;
    if (splashtimeout >= 1.0f)
      requestExit();
    break;
  }
}

void MainApp::tickStateChoose(float delta)
{
  choose_spin += delta * 2.0f;
}

void MainApp::tickCalculateFps(float delta)
{
  fpstime += delta;
  fpscount++;

  if (fpstime >= 0.1) {
    fps = fpscount / fpstime;
    fpstime = 0.0f;
    fpscount = 0;
  }
}

void MainApp::tickStateGame(float delta)
{
  PVehicle *vehic = game->vehicle[0];

  if (game->isFinished())
  {
    endGame(game->getFinishState());
    return;
  }

  cloudscroll = fmodf(cloudscroll + delta * game->weather.cloud.scrollrate, 1.0f);

  cprotate = fmodf(cprotate + delta * 1.0f, 1000.0f);

  // Do input/control processing

  for (int a = 0; a < ActionCount; a++) {

    switch(ctrl.map[a].type) {
    case UserControl::TypeUnassigned:
      break;

    case UserControl::TypeKey:
      ctrl.map[a].value = keyDown(SDL_GetScancodeFromKey(ctrl.map[a].key.sym)) ? 1.0f : 0.0f;
      break;

    case UserControl::TypeJoyButton:
      ctrl.map[a].value = getJoyButton(0, ctrl.map[a].joybutton.button) ? 1.0f : 0.0f;
      break;

    case UserControl::TypeJoyAxis:
      ctrl.map[a].value = ctrl.map[a].joyaxis.sign *
        getJoyAxis(0, ctrl.map[a].joyaxis.axis);

      RANGEADJUST(ctrl.map[a].value, ctrl.map[a].joyaxis.deadzone, ctrl.map[a].joyaxis.maxrange, 0.0f, 1.0f);

      CLAMP_LOWER(ctrl.map[a].value, 0.0f);
      break;
    }
  }

  // Bit of a hack for turning, because you simply can't handle analogue
  // and digital steering the same way, afaics

  if (ctrl.map[ActionLeft].type == UserControl::TypeJoyAxis ||
    ctrl.map[ActionRight].type == UserControl::TypeJoyAxis) {

    // Analogue mode

    vehic->ctrl.turn.z = 0.0f;
    vehic->ctrl.turn.z -= ctrl.map[ActionLeft].value;
    vehic->ctrl.turn.z += ctrl.map[ActionRight].value;

  } else {

    // Digital mode

    static float turnaccel = 0.0f;

    if (ctrl.map[ActionLeft].value > 0.0f) {
      if (turnaccel > -0.0f) turnaccel = -0.0f;
      turnaccel -= 8.0f * delta;
      vehic->ctrl.turn.z += turnaccel * delta;
    } else if (ctrl.map[ActionRight].value > 0.0f) {
      if (turnaccel < 0.0f) turnaccel = 0.0f;
      turnaccel += 8.0f * delta;
      vehic->ctrl.turn.z += turnaccel * delta;
    } else {
      PULLTOWARD(turnaccel, 0.0f, delta * 5.0f);
      PULLTOWARD(vehic->ctrl.turn.z, 0.0f, delta * 5.0f);
    }
  }

  // Computer aided steering
  if (vehic->forwardspeed > 1.0f)
    vehic->ctrl.turn.z -= vehic->body->getAngularVel().z * cfg_drivingassist / (1.0f + vehic->forwardspeed);


  float throttletarget = 0.0f;
  float braketarget = 0.0f;

  if (ctrl.map[ActionForward].value > 0.0f) {
    if (vehic->wheel_angvel > -10.0f)
      throttletarget = ctrl.map[ActionForward].value;
    else
      braketarget = ctrl.map[ActionForward].value;
  }
  if (ctrl.map[ActionBack].value > 0.0f) {
    if (vehic->wheel_angvel < 10.0f)
      throttletarget = -ctrl.map[ActionBack].value;
    else
      braketarget = ctrl.map[ActionBack].value;
  }

  PULLTOWARD(vehic->ctrl.throttle, throttletarget, delta * 15.0f);
  PULLTOWARD(vehic->ctrl.brake1, braketarget, delta * 25.0f);

  vehic->ctrl.brake2 = ctrl.map[ActionHandbrake].value;


  //PULLTOWARD(vehic->ctrl.aim.x, 0.0, delta * 2.0);
  //PULLTOWARD(vehic->ctrl.aim.y, 0.0, delta * 2.0);

  game->tick(delta);

  // Record ghost car (assumes first vehicle is player vehicle)
  if (cfg_enable_ghost && game->vehicle[0]) {
    ghost.recordSample(delta, game->vehicle[0]->part[0]);
  }

    if (cfg_dirteffect)
    {

#define BRIGHTEN_ADD        0.20f

  for (unsigned int i=0; i<game->vehicle.size(); i++) {
    for (unsigned int j=0; j<game->vehicle[i]->part.size(); j++) {
      //const vec3f bodydirtpos = game->vehicle[i]->part[j].ref_world.getPosition();
      const vec3f bodydirtpos = game->vehicle[i]->body->getPosition();
      const dirtinfo bdi = PUtil::getDirtInfo(game->terrain->getRoadSurface(bodydirtpos));

    if (bdi.startsize >= 0.30f && game->vehicle[i]->forwardspeed > 23.0f)
    {
        if (game->vehicle[i]->canHaveDustTrail())
        {
            const float sizemult = game->vehicle[i]->forwardspeed * 0.035f;
            const vec3f bodydirtvec = {0, 0, 1}; // game->vehicle[i]->body->getLinearVelAtPoint(bodydirtpos);
            vec3f bodydirtcolor = game->terrain->getCmapColor(bodydirtpos);

            bodydirtcolor.x += BRIGHTEN_ADD;
            bodydirtcolor.y += BRIGHTEN_ADD;
            bodydirtcolor.z += BRIGHTEN_ADD;

            CLAMP(bodydirtcolor.x, 0.0f, 1.0f);
            CLAMP(bodydirtcolor.y, 0.0f, 1.0f);
            CLAMP(bodydirtcolor.z, 0.0f, 1.0f);
            psys_dirt->setColorStart(bodydirtcolor.x, bodydirtcolor.y, bodydirtcolor.z, 1.0f);
            psys_dirt->setColorEnd(bodydirtcolor.x, bodydirtcolor.y, bodydirtcolor.z, 0.0f);
            psys_dirt->setSize(bdi.startsize * sizemult, bdi.endsize * sizemult);
            psys_dirt->setDecay(bdi.decay);
            psys_dirt->addParticle(bodydirtpos, bodydirtvec);
        }
    }
    else
      for (unsigned int k=0; k<game->vehicle[i]->part[j].wheel.size(); k++) {
        if (rand01 * 20.0f < game->vehicle[i]->part[j].wheel[k].dirtthrow)
        {
            const vec3f dirtpos = game->vehicle[i]->part[j].wheel[k].dirtthrowpos;
            const vec3f dirtvec = game->vehicle[i]->part[j].wheel[k].dirtthrowvec;
            const dirtinfo di = PUtil::getDirtInfo(game->terrain->getRoadSurface(dirtpos));
            vec3f dirtcolor = game->terrain->getCmapColor(dirtpos);

            dirtcolor.x += BRIGHTEN_ADD;
            dirtcolor.y += BRIGHTEN_ADD;
            dirtcolor.z += BRIGHTEN_ADD;
            CLAMP(dirtcolor.x, 0.0f, 1.0f);
            CLAMP(dirtcolor.y, 0.0f, 1.0f);
            CLAMP(dirtcolor.z, 0.0f, 1.0f);
            psys_dirt->setColorStart(dirtcolor.x, dirtcolor.y, dirtcolor.z, 1.0f);
            psys_dirt->setColorEnd(dirtcolor.x, dirtcolor.y, dirtcolor.z, 0.0f);
            psys_dirt->setSize(di.startsize, di.endsize);
            psys_dirt->setDecay(di.decay);
            psys_dirt->addParticle(dirtpos, dirtvec /*+ vec3f::rand() * 10.0f*/);
        }
      }
    }
  }

  #undef BRIGHTEN_ADD

    }

  float angtarg = 0.0f;
  angtarg -= ctrl.map[ActionCamLeft].value;
  angtarg += ctrl.map[ActionCamRight].value;
  angtarg *= PI*0.75f;

  PULLTOWARD(camera_user_angle, angtarg, delta * 4.0f);

  quatf tempo;
  //tempo.fromThreeAxisAngle(vec3f(-1.3,0.0,0.0));

  // allow temporary camera view changes for this frame
  CameraMode cameraview_mod = cameraview;

  if (game->gamestate == Gamestate::finished) {
    cameraview_mod = CameraMode::chase;
    static float spinner = 0.0f;
    spinner += 1.4f * delta;
    tempo.fromThreeAxisAngle(vec3f(-PI*0.5f,0.0f,spinner));
  } else {
    tempo.fromThreeAxisAngle(vec3f(-PI*0.5f,0.0f,0.0f));
  }

  renderowncar = (cameraview_mod != CameraMode::hood && cameraview_mod != CameraMode::bumper);

  campos_prev = campos;

  //PReferenceFrame *rf = &vehic->part[2].ref_world;
  PReferenceFrame *rf = &vehic->getBody();

  vec3f forw = makevec3f(rf->getOrientationMatrix().row[0]);
  float forwangle = atan2(forw.y, forw.x);

  mat44f cammat;

  switch (cameraview_mod) {

	default:
	case CameraMode::chase: {
    quatf temp2;
    temp2.fromZAngle(forwangle + camera_user_angle);

    quatf target = tempo * temp2;

    if (target.dot(camori) < 0.0f) target = target * -1.0f;

    PULLTOWARD(camori, target, delta * 3.0f);

    camori.normalize();

    cammat = camori.getMatrix();
    cammat = cammat.transpose();
    //campos = rf->getPosition() + makevec3f(cammat.row[2]) * 100.0;
    campos = rf->getPosition() +
      makevec3f(cammat.row[1]) * 1.6f +
      makevec3f(cammat.row[2]) * 5.0f;
    } break;

	case CameraMode::bumper: {
    quatf temp2;
    temp2.fromZAngle(camera_user_angle);

    quatf target = tempo * temp2 * rf->ori;

    if (target.dot(camori) < 0.0f) target = target * -1.0f;

    PULLTOWARD(camori, target, delta * 25.0f);

    camori.normalize();

    cammat = camori.getMatrix();
    cammat = cammat.transpose();
    const mat44f &rfmat = rf->getInverseOrientationMatrix();
    //campos = rf->getPosition() + makevec3f(cammat.row[2]) * 100.0;
    campos = rf->getPosition() +
      makevec3f(rfmat.row[1]) * 1.7f +
      makevec3f(rfmat.row[2]) * 0.4f;
    } break;

    // Right wheel
	case CameraMode::side: {
    quatf temp2;
    temp2.fromZAngle(camera_user_angle);

    quatf target = tempo * temp2 * rf->ori;

    if (target.dot(camori) < 0.0f) target = target * -1.0f;

    //PULLTOWARD(camori, target, delta * 25.0f);
    camori = target;

    camori.normalize();

    cammat = camori.getMatrix();
    cammat = cammat.transpose();
    const mat44f &rfmat = rf->getInverseOrientationMatrix();
    //campos = rf->getPosition() + makevec3f(cammat.row[2]) * 100.0;
    campos = rf->getPosition() +
      makevec3f(rfmat.row[0]) * 1.1f +
      makevec3f(rfmat.row[1]) * 0.3f +
      makevec3f(rfmat.row[2]) * 0.1f;
    } break;

	case CameraMode::hood: {
    quatf temp2;
    temp2.fromZAngle(camera_user_angle);

    quatf target = tempo * temp2 * rf->ori;

    if (target.dot(camori) < 0.0f) target = target * -1.0f;

    //PULLTOWARD(camori, target, delta * 25.0f);
    camori = target;

    camori.normalize();

    cammat = camori.getMatrix();
    cammat = cammat.transpose();
    const mat44f &rfmat = rf->getInverseOrientationMatrix();
    //campos = rf->getPosition() + makevec3f(cammat.row[2]) * 100.0;
    campos = rf->getPosition() +
      makevec3f(rfmat.row[1]) * 0.50f +
      makevec3f(rfmat.row[2]) * 0.85f;
    } break;

    // Periscope view
	case CameraMode::periscope:{
    quatf temp2;
    temp2.fromZAngle(camera_user_angle);

    quatf target = tempo * temp2 * rf->ori;

    if (target.dot(camori) < 0.0f) target = target * -1.0f;

    PULLTOWARD(camori, target, delta * 25.0f);

    camori.normalize();

    cammat = camori.getMatrix();
    cammat = cammat.transpose();
    const mat44f &rfmat = rf->getInverseOrientationMatrix();
    //campos = rf->getPosition() + makevec3f(cammat.row[2]) * 100.0;
    campos = rf->getPosition() +
      makevec3f(rfmat.row[1]) * 1.7f +
      makevec3f(rfmat.row[2]) * 5.0f;
    } break;

    // Piggyback (fixed chase)
    //
    // TODO: broken because of "world turns upside down" bug
	//		the problem is in noseangle
    /*
	case CameraMode::piggyback:
	{
		vec3f nose = makevec3f(rf->getOrientationMatrix().row[1]);
		float noseangle = atan2(nose.z, nose.y);

		quatf temp2,temp3,temp4;
		temp2.fromZAngle(forwangle + camera_user_angle);
		//temp3.fromXAngle(noseangle);
		temp3.fromXAngle
		(
			atan2
			(
				rf->getWorldToLocPoint(rf->getPosition()).z,
				rf->getWorldToLocPoint(rf->getPosition()).x
				//(rf->getLocToWorldPoint(vec3f(1,0,0))-rf->getPosition()).x,
				//(rf->getLocToWorldPoint(vec3f(0,1,0))-rf->getPosition()).y
			)
		);

		temp4 = temp3;// * temp2;

		quatf target = tempo * temp4;

		if (target.dot(camori) < 0.0f)
			target = target * -1.0f;
		//if (camori.dot(target) < 0.0f) camori = camori * -1.0f;

		PULLTOWARD(camori, target, delta * 3.0f);

		camori.normalize();

		cammat = camori.getMatrix();
		cammat = cammat.transpose();
		//campos = rf->getPosition() + makevec3f(cammat.row[2]) * 100.0;
		campos = rf->getPosition() +
			makevec3f(cammat.row[1]) * 1.6f +
			makevec3f(cammat.row[2]) * 6.5f;
	}
	break;
	*/
  }

  forw = makevec3f(cammat.row[0]);
  camera_angle = atan2(forw.y, forw.x);

  vec2f diff = makevec2f(game->checkpt[vehic->nextcp].pt) - makevec2f(vehic->body->getPosition());
  nextcpangle = -atan2(diff.y, diff.x) - forwangle + PI*0.5f;

  if (cfg_enable_sound) {
    SDL_Haptic *haptic = nullptr;

    if (getNumJoysticks() > 0)
      haptic = getJoyHaptic(0);

    audinst_engine->setGain(cfg_volume_engine);
    audinst_engine->setPitch(vehic->getEngineRPM() / 9000.0f);

    float windlevel = fabsf(vehic->forwardspeed) * 0.6f;

    audinst_wind->setGain(windlevel * 0.03f * cfg_volume_sfx);
    audinst_wind->setPitch(windlevel * 0.02f + 0.9f);

    float skidlevel = vehic->getSkidLevel();

    audinst_gravel->setGain(skidlevel * 0.1f * cfg_volume_sfx);
    audinst_gravel->setPitch(1.0f);//vehic->getEngineRPM() / 7500.0f);

    if(haptic != nullptr && skidlevel > 500.0f)
      SDL_HapticRumblePlay(haptic, skidlevel * 0.0001f, MAX(1000, (unsigned int)(skidlevel * 0.05f)));

    if (vehic->getFlagGearChange()) {
       switch (vehic->iengine.getShiftDirection())
       {
         case 1: // Shift up
         {
             audinst.push_back(new PAudioInstance(aud_shiftup));
             audinst.back()->setPitch(0.7f + randm11*0.02f);
             audinst.back()->setGain(1.0f * cfg_volume_sfx);
             audinst.back()->play();
             break;
         }
         case -1: // Shift down
         {
             audinst.push_back(new PAudioInstance(aud_shiftdown));
             audinst.back()->setPitch(0.8f + randm11*0.12f);
             audinst.back()->setGain(1.0f * cfg_volume_sfx);
             audinst.back()->play();
             break;
         }
         default: // Shift flag but neither up nor down?
         {
           break;
         }
       }
    }

    if (crashnoise_timeout <= 0.0f) {
      float crashlevel = vehic->getCrashNoiseLevel();
      if (crashlevel > 0.0f) {
        audinst.push_back(new PAudioInstance(aud_crash1));
        audinst.back()->setPitch(1.0f + randm11*0.02f);
        audinst.back()->setGain(logf(1.0f + crashlevel) * cfg_volume_sfx);
        audinst.back()->play();

        if (haptic != nullptr)
          SDL_HapticRumblePlay(haptic, crashlevel * 0.2f, MAX(1000, (unsigned int)(crashlevel * 20.0f)));
      }
      crashnoise_timeout = rand01 * 0.1f + 0.01f;
    } else {
      crashnoise_timeout -= delta;
    }

    for (unsigned int i=0; i<audinst.size(); i++) {
      if (!audinst[i]->isPlaying()) {
        delete audinst[i];
        audinst.erase(audinst.begin() + i);
        i--;
        continue;
      }
    }
  }

  if (psys_dirt != nullptr)
    psys_dirt->tick(delta);

#define RAIN_START_LIFE         0.6f
#define RAIN_POS_RANDOM         15.0f
#define RAIN_VEL_RANDOM         2.0f

  vec3f camvel = (campos - campos_prev) * (1.0f / delta);

  {
  const vec3f def_drop_vect(2.5f,0.0f,17.0f);

  // randomised number of drops calculation
  float numdrops = game->weather.precip.rain * delta;
  int inumdrops = (int)numdrops;
  if (rand01 < numdrops - inumdrops) inumdrops++;
  for (int i=0; i<inumdrops; i++) {
    rain.push_back(RainDrop());
    rain.back().drop_pt = vec3f(campos.x,campos.y,0);
    rain.back().drop_pt += camvel * RAIN_START_LIFE;
    rain.back().drop_pt += vec3f::rand() * RAIN_POS_RANDOM;
    rain.back().drop_pt.z = game->terrain->getHeight(rain.back().drop_pt.x, rain.back().drop_pt.y);

    if (game->water.enabled && rain.back().drop_pt.z < game->water.height)
        rain.back().drop_pt.z = game->water.height;

    rain.back().drop_vect = def_drop_vect + vec3f::rand() * RAIN_VEL_RANDOM;
    rain.back().life = RAIN_START_LIFE;
  }

  // update life and delete dead raindrops
  unsigned int j=0;
  for (unsigned int i = 0; i < rain.size(); i++) {
    if (rain[i].life <= 0.0f) continue;
    rain[j] = rain[i];
    rain[j].prevlife = rain[j].life;
    rain[j].life -= delta;
    if (rain[j].life < 0.0f)
      rain[j].life = 0.0f; // will be deleted next time round
    j++;
  }
  rain.resize(j);
  }

#define SNOWFALL_START_LIFE     6.5f
#define SNOWFALL_POS_RANDOM     110.0f
#define SNOWFALL_VEL_RANDOM     0.8f

  // snowfall logic; this is rain logic CPM'd (Copied, Pasted and Modified) -- A.B.
  {
    const vec3f def_drop_vect(1.3f, 0.0f, 6.0f);

  // randomised number of flakes calculation
  float numflakes = game->weather.precip.snowfall * delta;
  int inumflakes = (int)numflakes;
  if (rand01 < numflakes - inumflakes) inumflakes++;
  for (int i=0; i<inumflakes; i++) {
    snowfall.push_back(SnowFlake());
    snowfall.back().drop_pt = vec3f(campos.x,campos.y,0);
    snowfall.back().drop_pt += camvel * SNOWFALL_START_LIFE / 2;
    snowfall.back().drop_pt += vec3f::rand() * SNOWFALL_POS_RANDOM;
    snowfall.back().drop_pt.z = game->terrain->getHeight(snowfall.back().drop_pt.x, snowfall.back().drop_pt.y);

    if (game->water.enabled && snowfall.back().drop_pt.z < game->water.height)
        snowfall.back().drop_pt.z = game->water.height;

    snowfall.back().drop_vect = def_drop_vect + vec3f::rand() * SNOWFALL_VEL_RANDOM;
    snowfall.back().life = SNOWFALL_START_LIFE * rand01;
  }

  // update life and delete dead snowflakes
  unsigned int j=0;
  for (unsigned int i = 0; i < snowfall.size(); i++) {
    if (snowfall[i].life <= 0.0f) continue;
    snowfall[j] = snowfall[i];
    snowfall[j].prevlife = snowfall[j].life;
    snowfall[j].life -= delta;
    if (snowfall[j].life < 0.0f)
      snowfall[j].life = 0.0f; // will be deleted next time round
    j++;
  }
  snowfall.resize(j);
  }

  // update stuff for SSRender

  cam_pos = campos;
  cam_orimat = cammat;
  cam_linvel = camvel;

  tickCalculateFps(delta);
}

// TODO: mark instant events with flags, deal with them in tick()
// this will get rid of the silly doubling up between keyEvent and joyButtonEvent
// and possibly mouseButtonEvent in future

void MainApp::keyEvent(const SDL_KeyboardEvent &ke)
{
  if (ke.type == SDL_KEYDOWN) {

    if (ke.keysym.sym == SDLK_F12) {
      saveScreenshot();
      return;
    }

    switch (appstate) {
    case AS_LOAD_1:
    case AS_LOAD_2:
      // no hitting escape allowed... end screen not loaded!
      return;
    case AS_LOAD_3:
      levelScreenAction(AA_INIT, 0);
      return;
    case AS_LEVEL_SCREEN:
      handleLevelScreenKey(ke);
      return;
    case AS_CHOOSE_VEHICLE:

      if (ctrl.map[ActionLeft].type == UserControl::TypeKey &&
        ctrl.map[ActionLeft].key.sym == ke.keysym.sym) {
        if (--choose_type < 0)
          choose_type = (int)game->vehiclechoices.size()-1;
        return;
      }
      if ((ctrl.map[ActionRight].type == UserControl::TypeKey &&
        ctrl.map[ActionRight].key.sym == ke.keysym.sym) ||
        (ctrl.map[ActionNext].type == UserControl::TypeKey &&
        ctrl.map[ActionNext].key.sym == ke.keysym.sym)) {
        if (++choose_type >= (int)game->vehiclechoices.size())
          choose_type = 0;
        return;
      }

      switch (ke.keysym.sym) {
      case SDLK_RETURN:
      case SDLK_KP_ENTER:
      {
        enterGame();
        return;
      }
      case SDLK_ESCAPE:
        endGame(Gamefinish::not_finished);
        return;
      default:
        break;
      }
      break;
    case AS_IN_GAME:

      if (ctrl.map[ActionRecover].type == UserControl::TypeKey &&
        ctrl.map[ActionRecover].key.sym == ke.keysym.sym) {
        game->vehicle[0]->doReset();
        return;
      }
      if (ctrl.map[ActionRecoverAtCheckpoint].type == UserControl::TypeKey &&
        ctrl.map[ActionRecoverAtCheckpoint].key.sym == ke.keysym.sym)
      {
          game->resetAtCheckpoint(game->vehicle[0]);
          return;
      }
      if (ctrl.map[ActionCamMode].type == UserControl::TypeKey &&
        ctrl.map[ActionCamMode].key.sym == ke.keysym.sym) {
        cameraview = static_cast<CameraMode>((static_cast<int>(cameraview) + 1) % static_cast<int>(CameraMode::count));
        camera_user_angle = 0.0f;
        return;
      }
      if (ctrl.map[ActionShowMap].type == UserControl::TypeKey &&
        ctrl.map[ActionShowMap].key.sym == ke.keysym.sym) {
        showmap = !showmap;
        return;
      }
      if (ctrl.map[ActionPauseRace].type == UserControl::TypeKey &&
        ctrl.map[ActionPauseRace].key.sym == ke.keysym.sym)
      {
          toggleSounds(pauserace);
          pauserace = !pauserace;
          return;
      }
      if (ctrl.map[ActionShowUi].type == UserControl::TypeKey &&
        ctrl.map[ActionShowUi].key.sym == ke.keysym.sym) {
        showui = !showui;
        return;
      }

      if (ctrl.map[ActionShowCheckpoint].type == UserControl::TypeKey &&
        ctrl.map[ActionShowCheckpoint].key.sym == ke.keysym.sym) {
            showcheckpoint = !showcheckpoint;
            return;
      }


      switch (ke.keysym.sym) {
      case SDLK_ESCAPE:
          endGame(game->getFinishState());
          pauserace = false;
/*
          if (game->getFinishState() == GF_PASS)
            endGame(GF_PASS);
          else // GF_FAIL or GF_NOT_FINISHED
            endGame(GF_FAIL);
*/
        return;
      default:
        break;
      }
      break;
    case AS_END_SCREEN:
      requestExit();
      return;
    }

    switch (ke.keysym.sym) {
    case SDLK_ESCAPE:
      quitGame();
      return;
    default:
      break;
    }
  }
}

void MainApp::enterGame()
{
    if (!game->vehiclechoices[choose_type]->getLocked()) {
        initAudio();
        game->chooseVehicle(game->vehiclechoices[choose_type]);
        if (cfg_enable_ghost)
          ghost.recordStart(race_data.mapname, game->vehiclechoices[choose_type]->getName());

        if (lss.state == AM_TOP_LVL_PREP)
        {
            const float bct = best_times.getBestClassTime(
                race_data.mapname,
                game->vehicle.front()->type->proper_class);

            if (bct >= 0.0f)
                game->targettime = bct;
        }

        appstate = AS_IN_GAME;
    }
}

void MainApp::mouseMoveEvent(int dx, int dy)
{
  //PVehicle *vehic = game->vehicle[0];

  //vehic->ctrl.tank.turret_turn.x += dx * -0.002;
  //vehic->ctrl.tank.turret_turn.y += dy * 0.002;

  //vehic->ctrl.turn.x += dy * 0.005;
  //vehic->ctrl.turn.y += dx * -0.005;

  dy = dy;

  if (appstate == AS_IN_GAME) {
    PVehicle *vehic = game->vehicle[0];
    vehic->ctrl.turn.z += dx * 0.01f;
  }
}

void MainApp::joyButtonEvent(int which, int button, bool down)
{
  if (which == 0 && down) {

    switch (appstate) {
    case AS_CHOOSE_VEHICLE:

      if (ctrl.map[ActionLeft].type == UserControl::TypeJoyButton &&
        ctrl.map[ActionLeft].joybutton.button == button) {
        if (--choose_type < 0)
          choose_type = (int)game->vehiclechoices.size()-1;
        return;
      }
      if ((ctrl.map[ActionRight].type == UserControl::TypeJoyButton &&
        ctrl.map[ActionRight].joybutton.button == button) ||
        (ctrl.map[ActionNext].type == UserControl::TypeJoyButton &&
        ctrl.map[ActionNext].joybutton.button == button)) {
        if (++choose_type >= (int)game->vehiclechoices.size())
          choose_type = 0;
        return;
      }

      break;

    case AS_IN_GAME:

      if (ctrl.map[ActionRecover].type == UserControl::TypeJoyButton &&
        ctrl.map[ActionRecover].joybutton.button == button) {
        game->vehicle[0]->doReset();
        return;
      }
      if (ctrl.map[ActionRecoverAtCheckpoint].type == UserControl::TypeJoyButton &&
        ctrl.map[ActionRecoverAtCheckpoint].joybutton.button == button)
      {
          game->resetAtCheckpoint(game->vehicle[0]);
          return;
      }
      if (ctrl.map[ActionCamMode].type == UserControl::TypeJoyButton &&
        ctrl.map[ActionCamMode].joybutton.button == button) {
		cameraview = static_cast<CameraMode>((static_cast<int>(cameraview) + 1) % static_cast<int>(CameraMode::count));
        camera_user_angle = 0.0f;
        return;
      }
      if (ctrl.map[ActionShowMap].type == UserControl::TypeJoyButton &&
        ctrl.map[ActionShowMap].joybutton.button == button) {
        showmap = !showmap;
        return;
      }
      if (ctrl.map[ActionPauseRace].type == UserControl::TypeJoyButton &&
        ctrl.map[ActionPauseRace].joybutton.button == button)
        {
            toggleSounds(pauserace);
            pauserace = !pauserace;
            return;
        }
      if (ctrl.map[ActionShowUi].type == UserControl::TypeJoyButton &&
        ctrl.map[ActionShowUi].joybutton.button == button) {
        showui = !showui;
        return;
      }
    }
  }
}

bool MainApp::joyAxisEvent(int which, int axis, float value, bool down)
{
  if (which == 0) {

    switch (appstate) {
    case AS_CHOOSE_VEHICLE:

      if (ctrl.map[ActionLeft].type == UserControl::TypeJoyAxis &&
        ctrl.map[ActionLeft].joyaxis.axis == axis &&
        ctrl.map[ActionLeft].joyaxis.sign * value > 0.5) {
        if (!down)
          if (--choose_type < 0)
            choose_type = (int)game->vehiclechoices.size()-1;
        return true;
      }
      else if (ctrl.map[ActionRight].type == UserControl::TypeJoyAxis &&
        ctrl.map[ActionRight].joyaxis.axis == axis &&
        ctrl.map[ActionRight].joyaxis.sign * value > 0.5) {
        if (!down)
          if (++choose_type >= (int)game->vehiclechoices.size())
            choose_type = 0;
        return true;
      }
      else if ((ctrl.map[ActionLeft].type == UserControl::TypeJoyAxis &&
        ctrl.map[ActionLeft].joyaxis.axis == axis &&
        ctrl.map[ActionLeft].joyaxis.sign * value <= 0.5) ||
        (ctrl.map[ActionRight].type == UserControl::TypeJoyAxis &&
        ctrl.map[ActionRight].joyaxis.axis == axis &&
        ctrl.map[ActionRight].joyaxis.sign * value <= 0.5)) {
          return false;
      }

      break;
    }
  }
  return down;
}

int main(int argc, char *argv[])
{
    PUtil::initLog();
    return MainApp("Trigger Rally-NG", ".trigger-rally").run(argc, argv);
}
