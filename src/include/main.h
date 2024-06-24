//
// Copyright (C) 2004-2006 Jasmine Langridge, jas@jareiko.net
// Copyright (C) 2017 Emanuele Sorce, emanuele.sorce@hotmail.com
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

#pragma once

#include <pengine.h>
#include <psim.h>
#include <unordered_map>

#include <glm/vec4.hpp>

#include <light.h>

// Forward declaration for TriggerGame to use
class MainApp;

///
/// @brief This struct stores a checkpoint
/// @details basically just a coordinate
///
struct CheckPoint {
	vec3f pt;

	CheckPoint(const vec3f &_pt) : pt(_pt) { }
};

///
/// @brief Holds codriver checkpoint information.
///
struct CodriverCP
{
	// Where this checkpoint is on the map
    vec3f pt;
    // What the codriver should say.
    std::string notes;

    CodriverCP(const vec3f &pt, const std::string &notes):
        pt(pt),
        notes(notes)
    {
    }
};

///
/// @brief The status of the race ending
///
enum class Gamefinish {
	// race not finished yet
	not_finished,
	// race passed
	pass,
	// race failed
	fail
};

///
/// @brief Current state of the game
///
enum class Gamestate {
	// the few seconds before start
	countdown,
	// during racing
	racing,
	// race ended
	finished
};
///
/// @brief Camera view mode
///
enum class CameraMode{
	chase = 0,
	bumper,
	side,
	hood,
	periscope,
	// disabled
	//piggyback,
	count
};

///
/// @brief class containing information about the race is being played
///
class TriggerGame {

	friend class MainApp;

public:

    // Sets the codriver checkpoints to "ordered" (true) or "free" (false).
    bool cdcheckpt_ordered = false;

private:
	MainApp *app;

	// simulation context
	PSim *sim;

	// current state of the race
	Gamestate gamestate;

	int randomseed;

	// the vehicles
	std::vector<PVehicle *> vehicle;

	// User's vehicle
	PVehicle *uservehicle;

	// the terrain
	PTerrain *terrain;

	// Checkpoints
	std::vector<CheckPoint> checkpt;
	// Codriver checkpoints
	std::vector<CodriverCP> codrivercheckpt;

	int number_of_laps = 1;

	// Codriver voice and sign set
	PCodriverVoice cdvoice;
	PCodriverSigns cdsigns;

public:

    const float offroadtime_penalty_multiplier = 2.5f;

    ///
    /// @brief Used for "real-time" counting of seconds, to scare the player.
    /// @details it read offroad time of the user vehicle
    ///
    float getOffroadTime() const
    {
        return uservehicle->offroadtime_total + (coursetime - uservehicle->offroadtime_begin);
    }

private:

	// Time passed since the race started
	float coursetime;
	// time variable used for pre and post race (i.e. Countdown)
	float othertime;
	// checkpoint time
	float cptime;
	// the time needed to win
	float targettime;

	// level comment string
	std::string comment;

	vec3f start_pos;
	quatf start_ori;

    // used to reset vehicle at last passed checkpoint
    vec3f lastCkptPos;
    quatf lastCkptOri;

	// Structure that stores the current weather
	struct {
		struct {
			std::string texname;
			float scrollrate;
		} cloud;
		struct {
			vec3f color;
			float density;
			float density_sky;
		} fog;
		struct {
			float rain;
			float snowfall;
		} precip;
	} weather;

    ///
    /// @brief Water information.
    /// @todo Maybe remove defaults, used in `game.cpp`.
    ///
    struct
    {
        bool        enabled     = false;    ///< Enables the water?
        float       height      = 0.0f;     ///< The height of the water.
        std::string texname     = "";       ///< Custom water texture.
        bool        useralpha   = false;    ///< Use alpha provided by user?
        bool        fixedalpha  = false;    ///< Use the same alpha for all the water?
        float       alpha       = 1.0f;     ///< Default user alpha value.
    } water;

public:
	std::vector<PVehicleType *> vehiclechoices;

public:
	TriggerGame(MainApp *parent);
	~TriggerGame();

	void resetAtCheckpoint(PVehicle *veh)
	{
		veh->doReset(lastCkptPos, lastCkptOri);
	}

	void renderCodriverSigns(const glm::mat4& mv, const glm::mat4& p)
	{
		cdsigns.render(coursetime, mv, p);
	}

	bool loadVehicles();

	bool loadLevel(const std::string &filename);

	void chooseVehicle(PVehicleType *type);

	void tick(float delta);

	bool isFinished() const
	{
		return (gamestate == Gamestate::finished) && (othertime <= 0.0f);
	}

	bool isRacing() const
	{
		return gamestate == Gamestate::racing;
	}

	Gamefinish getFinishState()
	{
		if (gamestate != Gamestate::finished)
			return Gamefinish::not_finished;
		if (coursetime + uservehicle->offroadtime_total * offroadtime_penalty_multiplier <= targettime)
			return Gamefinish::pass;
		else
			return Gamefinish::fail;
	}
};


#include "menu.h"


#define AS_LOAD_1           1
#define AS_LOAD_2           2
#define AS_LOAD_3           3
#define AS_LEVEL_SCREEN     10
#define AS_CHOOSE_VEHICLE   11
#define AS_IN_GAME          12
#define AS_END_SCREEN       13



struct TriggerLevel {
	std::string filename, name, description, comment, author, targettime, targettimeshort;

	float targettimefloat;

	PTexture *tex_minimap = nullptr;
	PTexture *tex_screenshot = nullptr;
};

///
/// @brief an Event with his levels
///
struct TriggerEvent {
	std::string filename, name, comment, author, totaltime;

	bool locked = false;
	UnlockData unlocks; ///< @see `HiScore1`

	// Note that levels are not linked to... they are
	// stored in the event because an event may have
	// "hidden" levels not otherwise available

	std::vector<TriggerLevel> levels;
};


class DirtParticleSystem : public PParticleSystem {
public:

	void tick(float delta)
	{
		PParticleSystem::tick(delta);

		for (unsigned int i=0; i<part.size(); i++)
		{
			PULLTOWARD(part[i].linvel, vec3f::zero(), delta * 25.0f);
		}
	}
};


struct RainDrop
{
	vec3f drop_pt, drop_vect;
	float life, prevlife;
};

struct SnowFlake
{
    vec3f drop_pt;
    vec3f drop_vect;
    float life;
    float prevlife;
};

struct UserControl {
	enum {
		TypeUnassigned,
		TypeKey,
		TypeJoyButton,
		TypeJoyAxis
	} type;
	union {
		struct {
			SDL_Keycode sym;
		} key;
		struct {
			int button;
		} joybutton;
		struct {
			int axis;
			float sign;
			float deadzone;
			float maxrange;
		} joyaxis; // more like half-axis, really
	};

	// from 0.0 to 1.0 depending on activation level
	float value;
};

///
/// @brief this class is the whole Trigger Rally game. Create a MainApp object is the only thing main() does
///
class MainApp : public PApp {
public:
	enum Speedunit {
		mph,
		kph
	};

	enum Speedstyle {
		analogue,
		hybrid
	};

	enum SnowFlakeType
	{
		point,
		square,
		textured
	};

	// TODO: these shouldn't be static+public, but the simplicity is worth it for now
	static GLfloat    cfg_anisotropy;     ///< Anisotropic filter quality.
	static bool       cfg_foliage;        ///< Foliage on/off flag.
	static bool       cfg_roadsigns;      ///< Road signs on/off flag.
	static bool       cfg_weather;        ///< Weather on/off flag.

private:

	int appstate;

	// TODO: use `aspect` instead of these?
	// TODO: type should be GLdouble instead of double
	double hratio; ///< Horizontal ratio.
	double vratio; ///< Vertical ratio.

	UnlockData player_unlocks; ///< Unlocks for current player, see `HiScore1`.

public:

    ///
    /// @brief Checks if the given data was unlocked by the current player.
    /// @param [in] udata       Unlock data to be checked.
    /// @retval true            The player has unlocked the data.
    /// @retval false           The player has not unlocked the data.
    ///
    bool isUnlockedByPlayer(const std::string &udata) const
    {
        return player_unlocks.count(udata) != 0;
    }

    ///
    /// @brief Checks if the given vehicle is locked.
    /// @param [in] vefi        Filename of the vehicle.
    /// @retval true            Vehicle is marked as locked.
    /// @retval false           Vehicle is not marked as locked.
    ///
    bool isVehicleLocked(const std::string &vefi) const
    {
        XMLDocument xmlfile;
        XMLElement *rootelem = PUtil::loadRootElement(xmlfile, vefi, "vehicle");

        if (rootelem == nullptr)
        {
            PUtil::outLog() << "Couldn't read vehicle \"" << vefi << "\"" << std::endl;
            return false;
        }

        const char *val = rootelem->Attribute("locked");

        if (val != nullptr && std::string(val) == "yes")
            return true;

        return false;
    }

private:

	// Config settings

	std::string cfg_playername;
	bool cfg_copydefplayers;

	int cfg_video_cx, cfg_video_cy;
	bool cfg_video_fullscreen;

	float cfg_drivingassist;
	bool cfg_enable_sound;
	bool cfg_enable_codriversigns;
	bool cfg_enable_fps;

	long int cfg_skip_saves;

	/// Basic volume control.
	float cfg_volume_engine       = 0.33f;    ///< Engine.
	float cfg_volume_sfx          = 1.00f;    ///< Sound effects (wind, gear change, crash, skid, etc.)
	float cfg_volume_codriver     = 1.00f;    ///< Codriver voice.

	/// Search paths for the data directory, as read from the configuration.
	std::list<std::string> cfg_datadirs;

	/// Name of the codriver whose words to load.
	/// Must be a valid directory in /data/sounds/codriver/.
	std::string cfg_codrivername;

	/// Name of the codriver icon set to load.
	/// Must be a valid directory in /data/textures/CodriverSigns/.
	std::string cfg_codriversigns;

	/// User settings for codriver signs: position, scale, fade start time.
	PCodriverUserConfig cfg_codriveruserconfig;

	Speedunit cfg_speed_unit;
	Speedstyle cfg_speed_style;
	float hud_speedo_start_deg;
	float hud_speedo_mps_deg_mult;
	float hud_speedo_mps_speed_mult;

	SnowFlakeType cfg_snowflaketype = SnowFlakeType::point;

	bool cfg_dirteffect = true;

	enum Action {
		ActionForward,
		ActionBack,
		ActionLeft,
		ActionRight,
		ActionHandbrake,
		ActionRecover,
		ActionRecoverAtCheckpoint,
		ActionCamMode,
		ActionCamLeft,
		ActionCamRight,
		ActionShowMap,
		ActionShowUi,
		ActionShowCheckpoint,
		ActionPauseRace,
		ActionNext,
		ActionCount
	};

	struct {
		std::string action_name[ActionCount];
		UserControl map[ActionCount];
	} ctrl;

	//

	float splashtimeout;

	//

	std::vector<TriggerLevel> levels;
	std::vector<TriggerEvent> events;

	std::string getVehicleUnlockEvent(const std::string &vehiclename) const;

	// for level screen
	Gui gui;

public:

	LevelState lss;

private:

	HISCORE1_SORT hs_sort_method = HISCORE1_SORT::BY_TOTALTIME_ASC;
	RaceData race_data;
	std::vector<TimeEntry> current_times;

	TriggerGame *game;

	PVehicleType *vt_tank;

	PTexture *tex_fontSourceCodeBold,
			*tex_fontSourceCodeOutlined,
			*tex_fontSourceCodeShadowed;

	PTexture *tex_detail,
			*tex_sky[1],
			*tex_water,
			*tex_waterdefault,
			*tex_dirt,
			*tex_snowflake,
			*tex_shadow,
			*tex_hud_revs,
			*tex_hud_revneedle,
			*tex_hud_life,
			*tex_hud_offroad,
			*tex_loading_screen,
			*tex_splash_screen,
			*tex_end_screen,
			*tex_race_no_screenshot,
			*tex_race_no_minimap,
			*tex_button_next,
			*tex_button_prev;

	std::unordered_map<std::string, PTexture *> tex_codriversigns;
	std::unordered_map<std::string, PAudioSample *> aud_codriverwords;

	DirtParticleSystem *psys_dirt;

	// Tones
	PAudioSample *aud_engine,
				*aud_wind,
				*aud_shiftup,
				*aud_shiftdown,
				*aud_gravel,
				*aud_crash1;

	// Audio instances
	PAudioInstance *audinst_engine, *audinst_wind, *audinst_gravel;

	std::vector<PAudioInstance *> audinst;

	float cloudscroll;

	vec3f campos, campos_prev;
	quatf camori;

	vec3f camvel;

	float nextcpangle;

	float cprotate;

	// what view mode the camera is
	CameraMode cameraview;
	float camera_angle;
	float camera_user_angle;

	// If with the the vehicle should be rendered (depends on cameraview)
	bool renderowncar;

	bool showmap;

	bool pauserace;

	bool showui;

	bool showcheckpoint;

	float crashnoise_timeout;

	std::vector<RainDrop> rain;
	std::vector<SnowFlake> snowfall;

	//

	int loadscreencount;

	float choose_spin;

	int choose_type;

	// Time and count to calculate frames per second
	float fpstime;
	unsigned int fpscount;
	float fps;
	void tickCalculateFps(float delta);

protected:

	void renderWater(const glm::mat4 &mv, const glm::mat4& p);
	void renderSky(const glm::mat4 &cammat, const glm::mat4& p);

	bool startGame(const std::string &filename);
	void toggleSounds(bool to);
	void initAudio();
	void endGame(Gamefinish state);
	void enterGame();

	void quitGame()
	{
		endGame(Gamefinish::not_finished);
		splashtimeout = 0.0f;
		appstate = AS_END_SCREEN;
	}

	void levelScreenAction(int action, int index);
	void handleLevelScreenKey(const SDL_KeyboardEvent &ke);
	void finishRace(Gamefinish state, float coursetime);

public:
	MainApp(const std::string &title, const std::string &name):

    PApp(title, name)
	{
	}
	//MainApp::~MainApp(); // should not have destructor, use unload

	float getCodriverVolume() const
	{
		return cfg_volume_codriver;
	}

	void config();
	void load();
	void unload();

	void copyDefaultPlayers() const;
	void loadConfig();
	bool loadAll();
	void loadShadersAndVao();
	bool loadLevelsAndEvents();
	bool loadLevel(TriggerLevel &tl);

	void calcScreenRatios();

	void tick(float delta);

	void resize();
	void render(float eyetranslation);

	void renderTexturedFullscreenQuad(const glm::mat4& mv, const glm::mat4& p);
	void renderTexturedFullscreenQuad(const glm::mat4& p);
	void renderStateLoading(float eyetranslation);
	void renderStateEnd(float eyetranslation);
	void tickStateLevel(float delta);
	void renderStateLevel(float eyetranslation);
	void tickStateChoose(float delta);
	void renderStateChoose(float eyetranslation);
	void tickStateGame(float delta);
	void renderStateGame(float eyetranslation);
	void renderVehicle(PVehicle* vehic, const glm::mat4& mv, const glm::mat4& p);
	void renderVehicleType(PVehicleType* vtype, const glm::mat4& mv, const glm::mat4& p);
	void renderRpmDial(float rpm, const glm::mat4& p);
	void renderMap(int nextcp, const glm::mat4& p);
	void renderMapMarker(const glm::vec2& vpos, float angle, const glm::vec4& col, float sc, const glm::mat4& mv, const glm::mat4& p);
	void renderRain(const glm::mat4& mv, const glm::mat4& p);
	void renderSnow(const glm::mat4& mv, const glm::mat4& p);
	void renderCheckpoints(int nextcp, const glm::mat4& mv, const glm::mat4& p);
	void buildSkyVao();
	void buildChkptVao();

	void keyEvent(const SDL_KeyboardEvent &ke);
	void mouseMoveEvent(int dx, int dy);
	void cursorMoveEvent(int posx, int posy);
	void mouseButtonEvent(const SDL_MouseButtonEvent &mbe);
	void joyButtonEvent(int which, int button, bool down);
	bool joyAxisEvent(int which, int axis, float value, bool down);

    std::unordered_map<std::string, PAudioSample *> getCodriverWords() const
    {
        return aud_codriverwords;
    }

    std::unordered_map<std::string, PTexture *> getCodriverSigns() const
    {
        return tex_codriversigns;
    }

    PCodriverUserConfig getCodriverUserConfig() const
    {
        return cfg_codriveruserconfig;
    }
private:
    const vec4f checkpoint_col[3] =
    {
        vec4f(1.0f, 0.0f, 0.0f, 0.8f),  // 0 = next checkpoint
        vec4f(0.7f, 0.7f, 0.1f, 0.6f),  // 1 = checkpoint after next
        vec4f(0.2f, 0.8f, 0.2f, 0.4f)  // 2 = all other checkpoints
    };

    ShaderProgram* sp_rpm_needle;
    ShaderProgram* sp_rpm_dial;
    ShaderProgram* sp_map_marker;
    ShaderProgram* sp_water;
    ShaderProgram* sp_bckgnd;
    ShaderProgram* sp_snow;
    ShaderProgram* sp_rain;
    ShaderProgram* sp_chkpt;
    ShaderProgram* sp_map;
    ShaderProgram* sp_sky;
    ShaderProgram* sp_offroad;

    VAO* map_marker_vao;
    VAO* rpm_dial_vao;
    VAO* bckgnd_vao;
    VAO* snow_vao;
    VAO* sky_vao;
    VAO* chkpt_vao;
    VAO* map_vao;
    VAO* offroad_vao;
    size_t sky_size = 0;
    size_t chkpt_size = 0;

    /// Map marker
    // 2f position, 1f alpha
    const float map_marker_vbo[15] = {
        0.0f, 0.0f,  1.0f,
        1.0f, 0.0f,  0.0f,
        0.0f, 1.0f,  0.0f,
       -1.0f, 0.0f,  0.0f,
        0.0f,-1.0f,  0.0f,
    };
    // Fan
    const unsigned short map_marker_ibo[6] = {
        0, 1, 2, 3, 4, 1,
    };

    /// Map
    float map_vbo[8] = {
        -1.0f,  1.0f,
        -1.0f, -1.0f,
        1.0f,  1.0f,
        1.0f, -1.0f,
    };
    unsigned short map_ibo[4] = {
        0, 1, 2, 3,
    };

    /// RPM dial
    // GL_T2F_V3F
    const float rpm_dial_vbo[20] = {
      0.0f,1.0f, -1.0f, 1.0f, 0.0f,
      0.0f,0.0f, -1.0f,-1.0f, 0.0f,
      1.0f,1.0f,  1.0f, 1.0f, 0.0f,
      1.0f,0.0f,  1.0f,-1.0f, 0.0f,
    };

    const unsigned short rpm_dial_ibo[4] = {
        0, 1, 2, 3,
    };

    /// Background
    float bckgnd_vbo[20] = {
        0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
        0.0f, 0.0f,  -1.0f,-1.0f, 0.0f,
        1.0f, 1.0f,   1.0f, 1.0f, 0.0f,
        1.0f, 0.0f,   1.0f,-1.0f, 0.0f,
    };

    unsigned short bckgnd_ibo[4] = {
        0, 1, 2, 3,
    };

    /// Snow
    // GL_T2F_V3F
    // Texture coordinates will be ignored if texturing is disabled
    float snow_vbo[20] = {
        1.0f, 1.0f,  0.0f, 0.0f, 0.0f,
        0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
        1.0f, 0.0f,  1.0f, 1.0f, 0.0f,
        0.0f, 0.0f,  1.0f, 1.0f, 1.0f,
    };

    unsigned short snow_ibo[4] = {
        0, 1, 2, 3,
    };

    /// Offroad sign
    float offroad_vbo[16] = {
        0.0f,   1.0f,  -1.0f,   1.0f,
        0.0f,   0.0f,  -1.0f,  -1.0f,
        1.0f,   1.0f,   1.0f,   1.0f,
        1.0f,   0.0f,   1.0f,  -1.0f,
    };

    unsigned short offroad_ibo[4] = {
        0, 1, 2, 3,
    };

    Light default_light;
    Material default_material;
};
