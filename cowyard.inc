/**--------------------------------------------------------------------------**\
					==============================
					          cowyard.inc
					==============================
- Description:
	Creates an area where some cows can move freely.
	Cows can be killed with guns.
	If a gun is shot inside the area or a cow is killed,
	the cows will move faster for some time.

- Author:
	SimonItaly (http://forum.sa-mp.com/member.php?u=84139)

- Requires:
    SA:MP 0.3.7 RC2 or higher version

- Dependencies:
	Incognito's Streamer (plugin)
	Y_Less' Foreach (include)

- Thanks:
	Incognito - streamer
	Y_Less    - foreach
	Matite    - cow object
	Kalcor    - SA:MP 0.3.7 RC2

- Possible defines:
    CREATE_YARD_BORDERS
        if defined, four bollard objects will be created
        in the corners of the yard.
        
    DISABLE_COW_SHOOT
        if defined, disable killing cows

\**--------------------------------------------------------------------------**/

#if !defined GetPlayerPoolSize
	#error SA:MP version >= 0.3.7 RC2 is required.
#endif

#if !defined STREAMER_TYPE_OBJECT
	#error Incognito's Streamer plugin is required.
#endif

#if !defined foreach
	#error Y_Less' foreach include is required.
#endif

#if defined _cowyard_included
  #endinput
#endif
#define _cowyard_included

//******************************************************************************

#define MAX_COW_YARDS                           10
#define MAX_COWS_PER_YARD                       50

#define slowSpeed								0.5
#define fastSpeed								5.0

#define cowModel                                19833
#define cowOffset        						0.4

#define cowCooldown								10 * 1000

#define borderModel                             1319

//******************************************************************************

enum E_COWYARD
{
	yObject[MAX_COWS_PER_YARD],
	bool:yDead[MAX_COWS_PER_YARD]
}

static
	yardCows[MAX_COW_YARDS][E_COWYARD];

static
	yardArea[MAX_COW_YARDS],
	yardTimer[MAX_COW_YARDS],
	yardAlert[MAX_COW_YARDS],
	Float:yardCurSpeed[MAX_COW_YARDS];

#if defined CREATE_YARD_BORDERS
	static
		yardBorders[MAX_COW_YARDS][4];
#endif

static
	Iterator:CowYards<MAX_COW_YARDS>;

static const
	cowYardMagic = 849143371;                   //Bad practice?

//******************************************************************************

stock Float:AngleBetweenPoints(Float:X, Float:Y, Float:PointX, Float:PointY)
{
    new Float:Angle;
    if(X > PointX && Y > PointY)
        Angle = floatabs(atan2(floatsub(PointX, X), floatsub(PointY, Y)));
    if(X > PointX && Y <= PointY)
        Angle = floatadd(floatabs(atan2(floatsub(Y, PointY), floatsub(PointX, X))), 270.0);
    if(X <= PointX && Y > PointY)
        Angle = floatadd(floatabs(atan2(floatsub(PointY, Y), floatsub(X, PointX))), 90.0);
    if(X <= PointX && Y <= PointY)
        Angle = floatadd(floatabs(atan2(floatsub(X, PointX), floatsub(Y, PointY))), 180.0);

    return Angle >= 360.0 ? floatsub(Angle, 360.0) : Angle;
}

stock Float:frandom(Float:max, Float:min = 0.0, dp = 4)
{
    new Float:mul = floatpower(10.0, dp),
	    imin = floatround(min * mul),
	    imax = floatround(max * mul);
    return float(random(imax - imin) + imin) / mul;
}

//******************************************************************************

forward CreateCowYard(maxcows, Float:minx, Float:miny, Float:maxx, Float:maxy, Float:z, worldid, interiorid);
public CreateCowYard(maxcows, Float:minx, Float:miny, Float:maxx, Float:maxy, Float:z, worldid, interiorid)
{
	new yard = Iter_Free(CowYards);
	if(yard != -1)
	{
	    new Float:x, Float:y,
			Float:newx, Float:newy,
			Float:rz;
	    
	    yardCurSpeed[yard] = slowSpeed;
	    
	    maxcows = clamp(maxcows, 1, MAX_COWS_PER_YARD);
	    
	    //Borders
	    #if defined CREATE_YARD_BORDERS
			yardBorders[yard][0] = CreateDynamicObject(borderModel, minx, miny, z, 0.0, 0.0, 0.0, worldid, interiorid);
			yardBorders[yard][1] = CreateDynamicObject(borderModel, minx, maxy, z, 0.0, 0.0, 0.0, worldid, interiorid);
			yardBorders[yard][2] = CreateDynamicObject(borderModel, maxx, miny, z, 0.0, 0.0, 0.0, worldid, interiorid);
			yardBorders[yard][3] = CreateDynamicObject(borderModel, maxx, maxy, z, 0.0, 0.0, 0.0, worldid, interiorid);
		#endif
	    
	    z -= cowOffset;
	    
	    //Objects
		for(new i; i < maxcows; i++)
		{
		    x = frandom(maxx, minx);
			y = frandom(maxy, miny);
			
		    yardCows[yard][yObject][i] = CreateDynamicObject(cowModel, x, y, z, 0.0, 0.0, frandom(360.0), worldid, interiorid);

            newx = frandom(maxx, minx);
			newy = frandom(maxy, miny);

            rz = AngleBetweenPoints(x, y, newx, newy);

			SetDynamicObjectRot(yardCows[yard][yObject][i], 0.0, 0.0, rz + 180.0);

		    MoveDynamicObject(yardCows[yard][yObject][i], newx, newy, z, slowSpeed);
			
		    Streamer_SetIntData(STREAMER_TYPE_OBJECT, yardCows[yard][yObject][i], E_STREAMER_EXTRA_ID, yard + cowYardMagic);
		}
		
		//Area
		yardArea[yard] = CreateDynamicCuboid(minx, miny, z, maxx, maxy, z + 2.0, worldid, interiorid);
		Streamer_SetIntData(STREAMER_TYPE_AREA, yardArea[yard], E_STREAMER_EXTRA_ID, yard + cowYardMagic);
		
		yardTimer[yard] = 0;
		yardAlert[yard] = false;
		
		Iter_Add(CowYards, yard);
		
		return 1;
	}
	return 0;
}

forward DestroyCowYard(yard, next);
public DestroyCowYard(yard, next)
{
	if(Iter_Contains(CowYards, yard))
	{
	    //Borders
		#if defined CREATE_YARD_BORDERS
			for(new i; i < 4; i++)
			    DestroyDynamicObject(yardBorders[yard][i]);
	    #endif
	
	    //Objects
	    for(new i; i < MAX_COWS_PER_YARD && yardCows[yard][yObject][i] != 0; i++)
		{
		    DestroyDynamicObject(yardCows[yard][yObject][i]);
		    yardCows[yard][yObject][i] = 0;
		}
		
		//Area
		DestroyDynamicArea(yardArea[yard]);
	    
		if(yardTimer[yard] != 0)
		    KillTimer(yardTimer[yard]);
	    
	    Iter_SafeRemove(CowYards, yard, next);
	    
		return 1;
	}
	return 0;
}

//******************************************************************************

public OnDynamicObjectMoved(objectid)
{
	new extraid;
	extraid = Streamer_GetIntData(STREAMER_TYPE_OBJECT, objectid, E_STREAMER_EXTRA_ID);
	
	if(cowYardMagic <= extraid < cowYardMagic+MAX_COW_YARDS)
	{
	    extraid -= cowYardMagic;
	    
	    for(new i; i < MAX_COWS_PER_YARD && yardCows[extraid][yObject][i] != 0; i++)
	    {
	        if(objectid == yardCows[extraid][yObject][i])
	        {
	            if(!yardCows[extraid][yDead][i])
	            {
					MoveCow(objectid, extraid);
					break;
				}
			}
		}
	    return 1;
	}
	
	#if defined cow_OnDynamicObjectMoved
		cow_OnDynamicObjectMoved(objectid);
	#endif
	return 1;
}
#if defined _ALS_OnDynamicObjectMoved
	#undef OnDynamicObjectMoved
#else
	#define _ALS_OnDynamicObjectMoved
#endif
#define OnDynamicObjectMoved cow_OnDynamicObjectMoved
#if defined cow_OnDynamicObjectMoved
	forward cow_OnDynamicObjectMoved(objectid);
#endif

//******************************************************************************

public OnPlayerWeaponShot(playerid, weaponid, hittype, hitid, Float:fX, Float:fY, Float:fZ)
{
    new yardid = GetPlayerCowyard(playerid);
	if(yardid != -1)
	{
	    EscapeCows(yardid);
	}

	#if defined cow_OnPlayerWeaponShot
		cow_OnPlayerWeaponShot(playerid, weaponid, hittype, hitid, Float:fX, Float:fY, Float:fZ);
	#endif
	return 1;
}
#if defined _ALS_OnPlayerWeaponShot
	#undef OnPlayerWeaponShot
#else
	#define _ALS_OnPlayerWeaponShot
#endif
#define OnPlayerWeaponShot cow_OnPlayerWeaponShot
#if defined cow_OnPlayerWeaponShot
	forward cow_OnPlayerWeaponShot(playerid, weaponid, hittype, hitid, Float:fX, Float:fY, Float:fZ);
#endif

//------------------------------------------------------------------------------

#if !defined DISABLE_COW_SHOOT
	public OnPlayerShootDynamicObject(playerid, weaponid, objectid, Float:x, Float:y, Float:z)
	{
	    new extraid = Streamer_GetIntData(STREAMER_TYPE_OBJECT, objectid, E_STREAMER_EXTRA_ID);

		if(cowYardMagic <= extraid < cowYardMagic+MAX_COW_YARDS)
		{
		    extraid -= cowYardMagic;

		    for(new i; i < MAX_COWS_PER_YARD && yardCows[extraid][yObject][i] != 0; i++)
			{
			    if(objectid == yardCows[extraid][yObject][i])
			    {
			        if(!yardCows[extraid][yDead][i])
		            {
			    		KillCow(extraid, i);
			    		EscapeCows(extraid);
			    	}
			    	return 1;
				}
			}

		    return 1;
		}

		#if defined cow_OnPlayerShootDynamicObject
			cow_OnPlayerShootDynamicObject(playerid, weaponid, objectid, Float:x, Float:y, Float:z);
		#endif
		return 1;
	}
	#if defined _ALS_OnPlayerShootDynamicObject
		#undef OnPlayerShootDynamicObject
	#else
		#define _ALS_OnPlayerShootDynamicObject
	#endif
	#define OnPlayerShootDynamicObject cow_OnPlayerShootDynamicObject
	#if defined cow_OnPlayerShootDynamicObject
		forward cow_OnPlayerShootDynamicObject(playerid, weaponid, objectid, Float:x, Float:y, Float:z);
	#endif
#endif

//------------------------------------------------------------------------------

stock EscapeCows(yardid)
{
    if(yardTimer[yardid] != 0)
	    KillTimer(yardTimer[yardid]);

	if(!yardAlert[yardid])
    	MoveCows(yardid, fastSpeed);

    yardTimer[yardid] = SetTimerEx("MoveCows", cowCooldown, 0, "if", yardid, slowSpeed);

    yardAlert[yardid] = true;
}

//******************************************************************************

stock GetPlayerCowyard(playerid)
{
	new extraid,
	    areaid = GetPlayerDynamicArea(playerid);
	
	if(areaid)
	{
		extraid = Streamer_GetIntData(STREAMER_TYPE_AREA, areaid, E_STREAMER_EXTRA_ID);

		if(cowYardMagic <= extraid < cowYardMagic+MAX_COW_YARDS)
		{
		    extraid -= cowYardMagic;

		    return extraid;
		}
	}
	
	return -1;
}

stock GetPlayerDynamicArea(playerid)
{
    new areas[1];
    GetPlayerDynamicAreas(playerid, areas, 1);
    return areas[0];
}

forward MoveCows(yardid, Float:speed);
public MoveCows(yardid, Float:speed)
{
	if(Iter_Contains(CowYards, yardid))
	{
		yardCurSpeed[yardid] = speed;

        yardAlert[yardid] = false;

		for(new i; i < MAX_COWS_PER_YARD && yardCows[yardid][yObject][i] != 0; i++)
		{
		    if(!yardCows[yardid][yDead][i])
		    	MoveCow(yardCows[yardid][yObject][i], yardid);
		}
	}
}

stock MoveCow(objectid, yardid)
{
	new Float:minx, Float:miny,
	    Float:maxx, Float:maxy,
	    Float:x, Float:y, Float:z,
		Float:newx, Float:newy,
		Float:rz;

	Streamer_GetFloatData(STREAMER_TYPE_AREA, yardArea[yardid], E_STREAMER_MIN_X, minx);
	Streamer_GetFloatData(STREAMER_TYPE_AREA, yardArea[yardid], E_STREAMER_MAX_X, maxx);
	Streamer_GetFloatData(STREAMER_TYPE_AREA, yardArea[yardid], E_STREAMER_MIN_X, miny);
	Streamer_GetFloatData(STREAMER_TYPE_AREA, yardArea[yardid], E_STREAMER_MAX_Y, maxy);

	GetDynamicObjectPos(objectid, x, y, z);

	newx = frandom(maxx, minx);
	newy = frandom(maxy, miny);

    rz = AngleBetweenPoints(x, y, newx, newy);

    SetDynamicObjectRot(objectid, 0.0, 0.0, rz + 180.0);

	MoveDynamicObject(objectid, newx, newy, z, yardCurSpeed[yardid]);
	
	//Debug
	//CreateDynamicObject(2114, newx, newy, z, 0.0, 0.0, 0.0);
}

stock KillCow(yardid, cowid)
{
    yardCows[yardid][yDead][cowid] = true;
    
    new Float:x, Float:y, Float:z,
        Float:_dump, Float:rz,
		objectid = yardCows[yardid][yObject][cowid];
    
    GetDynamicObjectPos(objectid, x, y, z);
    GetDynamicObjectRot(objectid, _dump, _dump, rz);
    
	StopDynamicObject(objectid);
	MoveDynamicObject(objectid, x, y, z + 0.01, 1.0, 0.0, random(2) ? 90.0 : 270.0, rz);
}

