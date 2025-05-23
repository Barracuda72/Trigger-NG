
// game.cpp

// Copyright 2004-2006 Jasmine Langridge, jas@jareiko.net
// License: GPL version 2 (see included gpl.txt)

#include "main.h"

// size of the checkpoints
#define CODRIVER_CHECKPOINT_RADIUS 20
#define CHECKPOINT_RADIUS 30

// default values for the simulation
#define DEF_GRAVITY vec3f(0, 0, -9.81)
#define DEF_START_POS vec3f::zero();
#define DEF_START_ORI quatf::identity();
#define DEF_NUM_LAPS 1
#define DEF_TARGET_TIME 754.567f
#define DEF_CLOUD_SCROLLRATE 0.001
#define DEF_FOG_COLOR vec3f(1,1,1)
#define DEF_FOG_DENSITY 0.01
#define DEF_FOG_DENSITY_SKY 0.8
#define DEF_RAIN 0
#define DEF_SNOWFALL 0
#define DEF_WATER_ENABLED false
#define DEF_WATER_HEIGHT 0
#define DEF_WATER_USERALPHA false
#define DEF_WATER_ALPHA 1
#define DEF_WATER_FIXEDALPHA false

#define DEF_CD_CHECKPT_ORDERED true

// When the race finishes, how much time to wait before quitting
#define ENDGAME_TIMER 5


TriggerGame::TriggerGame(MainApp *parent):
	app(parent),
	sim(nullptr),
	randomseed(0),
	terrain(nullptr),
	cdvoice(app->getCodriverWords(), app->getCodriverVolume()),
	cdsigns(app->getCodriverSigns(), app->getCodriverUserConfig()),
	rigidity()
{}

TriggerGame::~TriggerGame()
{
	if (sim) delete sim;
	if (terrain) delete terrain;
}

///
/// @brief Loads vehicle types for the game.
/// @details This file scans the directory "/data/vehicles".
/// @returns Whether or not the loading was successful.
/// @retval true            Mostly OK.
/// @retval false           Problems loading the vehicles.
/// @todo This shouldn't be called over and over for each game.
/// @todo Should throw a PException if there are no vehicles?
///
bool TriggerGame::loadVehicles()
{
    if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
        PUtil::outLog() << "Loading vehicle information from \"/vehicles\"\n";

    // if no simulation instance exists, create a new one
    if (sim == nullptr)
        sim = new PSim();

    // find names of all vehicle types
    const std::list<std::string> vehiclefiles = PUtil::findFiles("/vehicles", ".vehicle");

    // if there is any vehicle
    if (!vehiclefiles.empty())
    {
        for (const std::string &vefi: vehiclefiles)
        {
            PUtil::outLog() << "Found vehicle: \"" << vefi << "\"\n";

            // load it
            PVehicleType *vt = sim->loadVehicleType(vefi, app->getSSModel());

            // if the vehicle is locked
            if (app->isVehicleLocked(vefi) && !app->isUnlockedByPlayer(vefi))
            {
                PUtil::outLog() << "Vehicle \"" << vefi << "\" is locked\n";
                vt->setLocked(true);
            }
            else
            {
                vt->setLocked(false);
            }

            // push it
            if (vt != nullptr)
                vehiclechoices.push_back(vt);
            else
                PUtil::outLog() << "Warning: failed to load vehicle from \"" << vefi << "\"\n";
        }
    }
    // if no vehicle is available
    else
    {
        PUtil::outLog() << "Error: there is no vehicle information" << std::endl;
        return false;
    }

    return true;
}

///
/// @brief load a level from his filename
/// @returns Whether or not the loading was successful.
/// @retval true  = mostly ok
///         false = some problem occured
///

bool TriggerGame::loadLevel(const std::string &filename)
{
  
	if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
		PUtil::outLog() << "Loading level \"" << filename << "\"\n";

	// create a new PSim if there is no one
	if (sim == nullptr)
		sim = new PSim();
  
	// set default values
	sim->setGravity(DEF_GRAVITY);
  
	start_pos = DEF_START_POS;
	start_ori = DEF_START_ORI;
  
	number_of_laps = DEF_NUM_LAPS;
	targettime = DEF_TARGET_TIME;
	
	weather.cloud.scrollrate = DEF_CLOUD_SCROLLRATE;
	weather.fog.color = DEF_FOG_COLOR;
	weather.fog.density = DEF_FOG_DENSITY;
	weather.fog.density_sky = DEF_FOG_DENSITY_SKY;
	weather.precip.rain = DEF_RAIN;
	weather.precip.snowfall = DEF_SNOWFALL;
  
	water.enabled = DEF_WATER_ENABLED;
	water.height = DEF_WATER_HEIGHT;
	water.useralpha = DEF_WATER_USERALPHA;
	water.alpha = DEF_WATER_ALPHA;
	water.fixedalpha = DEF_WATER_FIXEDALPHA;
  
	cdcheckpt_ordered = DEF_CD_CHECKPT_ORDERED;
  
	XMLDocument xmlfile;
	XMLElement *rootelem = PUtil::loadRootElement(xmlfile, filename, "level");
	if (!rootelem) return false;
  
	const char *val;
  
	val = rootelem->Attribute("comment");
	if (val) comment = val;
  
	// walk along the XML file and read datas
	for (XMLElement *walk = rootelem->FirstChildElement();
		walk; walk = walk->NextSiblingElement())
	{
    
		// terrain
		if (!strcmp(walk->Value(), "terrain"))
		{
			try
			{
				terrain = new PTerrain (walk, filename, app->getSSTexture (), rigidity,
          app->cfg.getFoliage(), app->cfg.getRoadsigns());
			}
			catch (PException &e)
			{
				PUtil::outLog () << "Terrain problem: " << e.what () << std::endl;
				return false;
			}
			sim->setTerrain(terrain);
		}
//
// TODO: delete this code soon
//
//#if 0
//    else if (!strcmp(walk->Value(), "vehicle")) {
//      PVehicle *vh = sim->createVehicle(walk, filename, app->getSSModel());
//      if (vh) {
//        //vehicle.push_back(vh);
//      } else {
//        PUtil::outLog() << "Warning: failed to load vehicle\n";
//      }
//    }
//    else if (!strcmp(walk->Value(), "vehicleoption")) {
//
//      val = walk->Attribute("type");
//      
//      if (val) {
//        PVehicleType *vt = sim->loadVehicleType(PUtil::assemblePath(val, filename), app->getSSModel());
//        if (vt) {
//          vehiclechoices.push_back(vt);
//        } else {
//          PUtil::outLog() << "Warning: failed to load vehicle option\n";
//        }
//      } else {
//        PUtil::outLog() << "Warning: vehicle option has no type\n";
//      }
//   }
//#endif

		// race (TODO: check race type... laps? once only?)
		else if (!strcmp(walk->Value(), "race"))
		{
			// level coordinates are multiplied by this multiplier
			vec2f coordscale = vec2f(1.0f, 1.0f);
			val = walk->Attribute("coordscale");
			if (val) sscanf(val, "%f , %f", &coordscale.x, &coordscale.y);
      
			// target time (seconds)
			val = walk->Attribute("targettime");
			if (val) targettime = atof(val);
     
			// Codriver checkpoint mode
			val = walk->Attribute("codrivercpmode");
			if (val != nullptr)
			{
				if (!strcmp(val, "ordered"))
					cdcheckpt_ordered = true;
				else if (!strcmp(val, "free"))
					cdcheckpt_ordered = false;
			}
      
			// number of laps
			val = walk->Attribute("laps");
			if (val != nullptr)
			{
				number_of_laps = std::stoi(val);

				if (number_of_laps < 1)
					number_of_laps = 1;
			}
      
			// Checkpoints
			for (XMLElement *walk2 = walk->FirstChildElement();
				walk2; walk2 = walk2->NextSiblingElement())
			{
       
				// if next element is a checkpoint
				if (!strcmp(walk2->Value(), "checkpoint"))
				{
					// coordinates
					val = walk2->Attribute("coords");

					vec2f coords;

					// read and save save them after applying the coord scale
					if (val)
					{
						if (sscanf(val, "%f , %f", &coords.x, &coords.y) == 2) {
							coords.x *= coordscale.x;
							coords.y *= coordscale.y;
						} else
							PUtil::outLog() << "Error reading checkpoint coords\n";
					} else {
						PUtil::outLog() << "Warning: checkpoint has no coords\n";
					}

					// pace notes (they are if is a codriver checkpoint)
					val = walk2->Attribute("notes");

					// if is a codriver checkpoint
					if (val != nullptr)
					{
						codrivercheckpt.push_back({{coords.x, coords.y, terrain->getHeight(coords.x, coords.y)}, val});
					}
					// if is a mandatory regular checkpoint
					else
					{
						checkpt.push_back(vec3f(coords.x, coords.y, terrain->getHeight(coords.x, coords.y)));
					}
				}
				// if is the start position 
				else if (!strcmp(walk2->Value(), "startposition")) 
				{
					// coordinates
					val = walk2->Attribute("pos");
					if (val) sscanf(val, "%f , %f , %f", &start_pos.x, &start_pos.y, &start_pos.z);
          
					// apply scale
					start_pos.x *= coordscale.x;
					start_pos.y *= coordscale.y;
          
					// set this as the position of the last checkpoint
					lastCkptPos = vec3f(start_pos.x, start_pos.y, terrain->getHeight(start_pos.x, start_pos.y) + 2.0f);
          
					// orientation
					val = walk2->Attribute("oridegrees");
					if (val)
					{
						float deg = atof(val);
						start_ori.fromZAngle(-RADIANS(deg));
						lastCkptOri = start_ori;
					}
          
					val = walk2->Attribute("ori");
					if (val) sscanf(val, "%f , %f , %f , %f", &start_ori.w, &start_ori.x, &start_ori.y, &start_ori.z);
				}
				// if it is a codriver checkpoint
				else if (!strcmp(walk2->Value(), "codrivercp")) 
				{
					vec2f coords(0.0f, 0.0f);
					std::string notes;

					// coordinates
					val = walk2->Attribute("coords");

					if (val != nullptr)
					{
						if (sscanf(val, "%f , %f", &coords.x, &coords.y) == 2)
						{
							coords.x *= coordscale.x;
							coords.y *= coordscale.y;
						}
						else
							PUtil::outLog() << "Error reading codriver checkpoint coords\n";
					}
					else
						PUtil::outLog() << "Warning: codriver checkpoint has no coords\n";

					// pace notes
					val = walk2->Attribute("notes");
	
					if (val != nullptr)
						notes = val;
					else
						PUtil::outLog() << "Warning: codriver checkpoint has no pace notes\n";

					codrivercheckpt.push_back({{coords.x, coords.y, terrain->getHeight(coords.x, coords.y)}, notes});
				}
			}
		}
		// weather conditions
		else if (!strcmp(walk->Value(), "weather"))
		{
			val = walk->Attribute("cloudtexture");
			if (val) weather.cloud.texname = PUtil::assemblePath(val, filename);
      
			val = walk->Attribute("cloudscrollrate");
			if (val) weather.cloud.scrollrate = atof(val);
      
			val = walk->Attribute("fogcolor");
			if (val) sscanf(val, "%f , %f , %f", &weather.fog.color.x, &weather.fog.color.y, &weather.fog.color.z);
	
			val = walk->Attribute("fogdensity");
			if (val) weather.fog.density = atof(val);
      
			val = walk->Attribute("fogdensitysky");
			if (val) weather.fog.density_sky = atof(val);
      
			val = walk->Attribute("rain");
			if (val && app->cfg.getWeather()) weather.precip.rain = atof(val);
	
			val = walk->Attribute("snowfall");
			if (val != nullptr && app->cfg.getWeather())
				weather.precip.snowfall = atof(val);
		}
		// water specifications
		else if (!strcmp(walk->Value(), "water"))
		{
			// if there is the water element means there is water
			water.enabled = true;
				
			// height
			val = walk->Attribute("height");

			if (val != nullptr)
				water.height = atof(val);
            
			// texture
			val = walk->Attribute("watertexture");
        
			if (val != nullptr)
				water.texname = val;
			else
				water.texname = "";
            
			// alpha
			val = walk->Attribute("alpha");
        
			if (val != nullptr)
			{
				water.useralpha = true;
				water.alpha = atof(val);
			}
			else
				water.useralpha = false;

			val = walk->Attribute("fixedalpha");

			if (val != nullptr && !strcmp(val, "yes"))
				water.fixedalpha = true;
		}
	}
  
	srand(1000);
  
	// if there are no checkpoints
	if (checkpt.size() == 0)
	{	
		int cpsize = 3;
    
		std::vector<vec2f> temp1;
    
		temp1.resize(cpsize);
    
		float ang = randm11 * PI;
    
		temp1[0] = vec2f(cosf(ang),sinf(ang)) * (100.0f + rand01 * 300.0f);
    
		for (int i=1; i<cpsize; i++)
		{
			ang += randm11 * (PI * 0.3f);
      
			temp1[i] = temp1[i-1] + vec2f(cosf(ang),sinf(ang)) * (100.0f + rand01 * 300.0f);
		}
    
		checkpt.resize(cpsize, vec3f::zero());
    
		for (int i=0; i<cpsize; i++) {
			vec2f coords = temp1[i];
			checkpt[i].pt = vec3f(coords.x, coords.y, terrain->getHeight(coords.x, coords.y));
		}
	}
  
	// activate vehicle brakes
	for (unsigned int i=0; i<vehicle.size(); i++) {
		vehicle[i]->ctrl.brake1 = 1.0f;
		vehicle[i]->ctrl.brake2 = 1.0f;
	}
  
	// do two seconds of simulations to have fine cars on ground
	sim->tick(2);
  
  /*
  for (int i=1; i<vehicle.size(); i++) {
    aid.push_back(AIDriver(i));
  }
  */ 
  
	// set some values
	coursetime = 0.0f;
	othertime = 3.0f;
	cptime = -4.0f;
	gamestate = Gamestate::countdown;
  
	return true;
}


///
/// @brief Create a new vehicle and push it in the simulation
/// @param type = the type of the vehicle to create
///
void TriggerGame::chooseVehicle(PVehicleType *type)
{ 
	// create the vehicle
	PVehicle *vh = sim->createVehicle(type, start_pos, start_ori /*, app->getSSModel()*/);
  
	// it is also the user vehicle
	uservehicle = vh;
  
	// if everything's ok push it in the vehicle list
	if (vh)
		vehicle.push_back(vh);
	else
		PUtil::outLog() << "Warning: failed to load vehicle\n";
}

///
/// @brief tick of a race
/// @details manage game statuses, time, checkpoint, physic simulators.
/// @param delta = time slice to run (in seconds)
///
void TriggerGame::tick(float delta)
{
	// Manage game states, and time counters accordingly
	switch (gamestate) {
		
		// Countdown before start
		case Gamestate::countdown:
			// count down is going
			othertime -= delta;
			// if countdown finishes
			if (othertime <= 0.0f)
			{
				// make the race start
				// this seconds are the one used when the race finishes (see "case GS_FINISHED:")
				othertime = ENDGAME_TIMER;
				gamestate = Gamestate::racing;
			}
			
			// In the countdown brakes must be on and user input ignored
			for (unsigned int i=0; i<vehicle.size(); i++)
			{
				vehicle[i]->ctrl.setZero();
				vehicle[i]->ctrl.brake1 = 1.0f;
				vehicle[i]->ctrl.brake2 = 1.0f;
			}
			break;
		
		// racing
		case Gamestate::racing:
			// time goes on
			coursetime += delta;
    
			// if the time finishes, and you have to stay in the time (for example in events) the race finishes
			if (coursetime + uservehicle->offroadtime_total * offroadtime_penalty_multiplier > targettime && app->lss.state == AM_TOP_EVT_PREP) {
				gamestate = Gamestate::finished;
			}
			break;
 
		// race finished
		case Gamestate::finished:
			// some seconds after the race finishes
			othertime -= delta;
    
			// brake up all vehicles
			for (unsigned int i=0; i<vehicle.size(); i++)
			{
				vehicle[i]->ctrl.brake1 = 1.0f;
				// also prevent throttle to avoid awful behaviour
				vehicle[i]->ctrl.throttle = 0.0f;
			}
			break;
	}
  
	// do the simulation
	sim->tick(delta);
  
	for (unsigned int i=0; i<vehicle.size(); i++)
	{
		const vec3f bodypos = vehicle[i]->body->getPosition();
		
		// line starting from the next checkpoint to the current position of the vehicle
    	vec2f diff = makevec2f(checkpt[vehicle[i]->nextcp].pt) - makevec2f(bodypos);

		// if the car was offroad in previous iteration
		static bool offroad_earlier = false;
		
		// if the car is currently offroad
		const bool offroad_now = !terrain->getRmapOnRoad(bodypos);

		//
		// TODO: the offroad penalty code is bad because it accumulates
		//  time for all cars (this will be a problem in the future if
		//  multiplayer races or AI drivers are implemented.)
		//
		
		// if it was offroad earlier
		if (offroad_earlier)
		{
			// but not now
			if (!offroad_now)
			{
				// update offroad values
				offroad_earlier     = false;
				vehicle[i]->offroadtime_end     = coursetime;
				vehicle[i]->offroadtime_total   += vehicle[i]->offroadtime_end - vehicle[i]->offroadtime_begin;
			}
		}
		// if it wasn't before but now
		else if (offroad_now)
		{
			// update offroad values
			offroad_earlier     = true;
			vehicle[i]->offroadtime_begin   = coursetime;
		}

		// if the vehicle get a checkpoint
		if (diff.lengthsq() < CHECKPOINT_RADIUS * CHECKPOINT_RADIUS)
		{
			//vehicle[i]->nextcp = (vehicle[i]->nextcp + 1) % checkpt.size();
			// update last checkpoint informations
			lastCkptPos = checkpt[vehicle[i]->nextcp].pt + vec3f(0.0f, 0.0f, 2.0f);
			lastCkptOri = vehicle[i]->body->getOrientation();
			
			cptime = coursetime;
			
			// if its last checkpoint
			if (++vehicle[i]->nextcp >= (int)checkpt.size())
			{
				// restart checkpoint counter
				vehicle[i]->nextcp = 0;
				// update lap counter
				++vehicle[i]->currentlap;
				// if it was last lap, enter game state GS_FINISHED (race finished)
				if (i == 0 && vehicle[i]->currentlap > number_of_laps) gamestate = Gamestate::finished;
			}
		}
    
		// if there are codriver checkpoints
		if (!codrivercheckpt.empty())
		{
			// if codriver checkpoint are in ordered mode
			if (cdcheckpt_ordered)
			{
				// distance to next codriver checkpoint
				diff = makevec2f(codrivercheckpt[vehicle[i]->nextcdcp].pt) - makevec2f(vehicle[i]->body->getPosition());

				// if the vehicle is enought close the checkpoint
				if (diff.lengthsq() < CODRIVER_CHECKPOINT_RADIUS * CODRIVER_CHECKPOINT_RADIUS)
				{
					// Codriver says the notes
					cdvoice.say(codrivercheckpt[vehicle[i]->nextcdcp].notes);
					// the related sign appears
					cdsigns.set(codrivercheckpt[vehicle[i]->nextcdcp].notes, coursetime);
					// update last checkpoint informations
					lastCkptPos = codrivercheckpt[vehicle[i]->nextcdcp].pt + vec3f(0.0f, 0.0f, 2.0f);
					lastCkptOri = vehicle[i]->body->getOrientation();

					// update next codriver checkpoint, if they are over start over again
					if (++vehicle[i]->nextcdcp >= (int)codrivercheckpt.size())
						vehicle[i]->nextcdcp = 0;
				}
			}
			// if they aren't in ordered mode
			else
			{
				// check for every codriver checkpoint
				for (std::size_t j=0; j < codrivercheckpt.size(); ++j)
				{
					// distance to the codriver checkpoint
					diff = makevec2f(codrivercheckpt[j].pt) - makevec2f(vehicle[i]->body->getPosition());

					// if it is close enought 
					if (diff.lengthsq() < CODRIVER_CHECKPOINT_RADIUS * CODRIVER_CHECKPOINT_RADIUS &&
						static_cast<int> (j + 1) != vehicle[i]->nextcdcp)
					{
						// say the notes
						cdvoice.say(codrivercheckpt[j].notes);
						// print the sign
						cdsigns.set(codrivercheckpt[j].notes, coursetime);
						// update last checkpoint
						lastCkptPos = codrivercheckpt[j].pt + vec3f(0.0f, 0.0f, 2.0f);
						lastCkptOri = vehicle[i]->body->getOrientation();
						
						// update next checkpoint
						vehicle[i]->nextcdcp = j + 1;
						break;
					}
				}
			}
		}
	}
  
  /*
  for (int i=0; i<aid.size(); i++) {
    PVehicle *vehic = vehicle[aid[i].vehic];
    
    vec2f diff = makevec2f(checkpt[vehic->nextcp].pt) - makevec2f(vehic->body->getPosition());
    float diffangle = -atan2(diff.y, diff.x);
    
    vec2f diff2 = makevec2f(checkpt[(vehic->nextcp+1)%checkpt.size()].pt) - makevec2f(checkpt[vehic->nextcp].pt);
    float diff2angle = -atan2(diff2.y, diff2.x);
    
    vec3f forw = makevec3f(vehic->body->getOrientationMatrix().row[0]);
    float forwangle = atan2(forw.y, forw.x);
    
    float correction = diffangle - forwangle + PI*0.5f + vehic->body->getAngularVel().z * -1.0f;
    
    #if 0
    float fact = diff.length() * 0.01f;
    if (fact > 0.3f) fact = 0.3f;
    correction += (diffangle - diff2angle) * fact;
    #endif
    
    if (correction >= PI) { correction -= PI*2.0f; if (correction >= PI) correction -= PI*2.0f; }
    if (correction < -PI) { correction += PI*2.0f; if (correction < -PI) correction += PI*2.0f; }
    
    vehic->ctrl.turn.z = correction * 2.0f + randm11 * 0.2f;
    
    vehic->ctrl.throttle = 1.0f - fabsf(correction) * 0.8f;
    if (vehic->ctrl.throttle < 0.1f)
      vehic->ctrl.throttle = 0.1f;
    
    vehic->ctrl.brake1 = 0.0f;
  }
  */
}

float TriggerGame::getOffroadTime() const
{
    return uservehicle->offroadtime_total + (coursetime - uservehicle->offroadtime_begin);
}

void TriggerGame::resetAtCheckpoint(PVehicle *veh)
{
    veh->doReset(lastCkptPos, lastCkptOri);
}

void TriggerGame::renderCodriverSigns(const glm::mat4& mv, const glm::mat4& p)
{
    cdsigns.render(coursetime, mv, p);
}

bool TriggerGame::isFinished() const
{
    return (gamestate == Gamestate::finished) && (othertime <= 0.0f);
}

bool TriggerGame::isRacing() const
{
    return gamestate == Gamestate::racing;
}

Gamefinish TriggerGame::getFinishState()
{
    if (gamestate != Gamestate::finished)
        return Gamefinish::not_finished;
    if (coursetime + uservehicle->offroadtime_total * offroadtime_penalty_multiplier <= targettime)
        return Gamefinish::pass;
    else
        return Gamefinish::fail;
}
