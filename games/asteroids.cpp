/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015, 2016 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * asteroids.cpp
 * Single-player space exploration with asteroids!
 */

#include <sys/keycodes.h>
#include <sys/termmode.h>

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <timespec.h>
#include <unistd.h>

#include <dispd.h>

static inline float RandomFloat()
{
	return arc4random() / (float) UINT32_MAX;
}

static inline float RandomFloat(float min, float max)
{
	return min + RandomFloat() * (max - min);
}

static inline float DegreeToRadian(float degree)
{
	return degree / 180 * M_PI;
}

static inline float RandomAngle()
{
	return RandomFloat() * DegreeToRadian(360);
}

static inline uint32_t MakeColor(uint8_t r, uint8_t g, uint8_t b)
{
	return b << 0UL | g << 8UL | r << 16UL;
}

static const size_t STARFIELD_WIDTH = 512UL;
static const size_t STARFIELD_HEIGHT = 512UL;
static uint32_t starfield[STARFIELD_WIDTH * STARFIELD_HEIGHT];

void GenerateStarfield(uint32_t* bitmap, size_t width, size_t height)
{
	size_t numpixels = width * height;

	for ( size_t i = 0; i < numpixels; i++ )
	{
		uint8_t color = 0;
		uint8_t randval;
		arc4random_buf(&randval, sizeof(randval));
		if ( randval == 5 || randval == 42 || randval == 101 )
			arc4random_buf(&color, sizeof(color));
		bitmap[i] = MakeColor(color, color, color);
	}
}

static const size_t MAXKEYNUM = 512UL;
static bool keysdown[MAXKEYNUM] = { false };
static bool keyspending[MAXKEYNUM] = { false };
static struct timespec key_handled_last[MAXKEYNUM];

bool pop_is_key_just_down(int abskbkey)
{
	assert(0 <= abskbkey);
	if ( MAXKEYNUM <= (size_t) abskbkey )
		return false;
	if ( keyspending[abskbkey] )
	{
		keyspending[abskbkey] = false;
		clock_gettime(CLOCK_MONOTONIC, &key_handled_last[abskbkey]);
		return true;
	}
	if ( !keysdown[abskbkey] )
		return false;
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	struct timespec elapsed = timespec_sub(now, key_handled_last[abskbkey]);
	struct timespec repress_delay = timespec_make(0, 100 * 1000 * 1000);
	if ( timespec_lt(elapsed, repress_delay) )
		return false;
	clock_gettime(CLOCK_MONOTONIC, &key_handled_last[abskbkey]);
	return true;
}

void FetchKeyboardInput()
{
	// Read the keyboard input from the user.
	const unsigned termmode = TERMMODE_KBKEY
	                        | TERMMODE_SIGNAL
	                        | TERMMODE_NONBLOCK;
	if ( settermmode(0, termmode) )
		error(1, errno, "settermmode");
	uint32_t codepoint;
	ssize_t numbytes;
	while ( 0 < (numbytes = read(0, &codepoint, sizeof(codepoint))) )
	{
		int kbkey = KBKEY_DECODE(codepoint);
		if( !kbkey )
			continue;
		int abskbkey = (kbkey < 0) ? -kbkey : kbkey;
		if ( MAXKEYNUM <= (size_t) abskbkey )
			continue;
		bool is_key_down_event = 0 < kbkey;
		if ( !keysdown[abskbkey] && is_key_down_event )
			keyspending[abskbkey] = true;
		keysdown[abskbkey] = is_key_down_event;
	}
}

static size_t xres;
static size_t yres;
static size_t bpp;
static size_t linesize;
static size_t framesize;
static uint32_t* buf;
static bool gamerunning;
static unsigned long framenum;

void DrawLine(uint32_t color, long x0, long y0, long x1, long y1)
{
	long dx = labs(x1-x0);
	long sx = x0 < x1 ? 1 : -1;
	long dy = labs(y1-y0);
	long sy = y0 < y1 ? 1 : -1;
	long err = (dx>dy ? dx : -dy)/2L;
	long e2;
	while ( true )
	{
		if ( 0 <= x0 && (size_t) x0 < xres && 0 <= y0 && (size_t) y0 < yres )
		{
			size_t index = y0 * linesize + x0;
			buf[index] = color;
		}
		if ( x0 == x1 && y0 == y1 )
			break;
		e2 = err;
		if ( e2 > -dx )
		{
			err -= dy;
			x0 += sx;
		}
		if ( e2 < dy )
		{
			err += dx;
			y0 += sy;
		}
	}
}

class Vector
{
public:
	float x;
	float y;

public:
	Vector(float x = 0.0f, float y = 0.0f) : x(x), y(y) { }

	Vector& operator=(const Vector& rhs)
	{
		if ( this != &rhs )
		{
			x = rhs.x;
			y = rhs.y;
		}
		return *this;
	}

	Vector& operator+=(const Vector& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	Vector& operator-=(const Vector& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}

	Vector& operator*=(float scalar)
	{
		x *= scalar;
		y *= scalar;
		return *this;
	}

	Vector& operator/=(float scalar)
	{
		x /= scalar;
		y /= scalar;
		return *this;
	}

	const Vector operator+(const Vector& other) const
	{
		Vector ret(*this); ret += other; return ret;
	}

	const Vector operator-(const Vector& other) const
	{
		Vector ret(*this); ret -= other; return ret;
	}

	const Vector operator*(float scalar) const
	{
		Vector ret(*this); ret *= scalar; return ret;
	}

	const Vector operator/(float scalar) const
	{
		Vector ret(*this); ret /= scalar; return ret;
	}

	bool operator==(const Vector& other) const
	{
		return x == other.x && y == other.y;
	}

	bool operator!=(const Vector& other) const
	{
		return !(*this == other);
	}

	float Dot(const Vector& other) const
	{
		return x * other.x + y * other.y;
	}

	float SquaredSize() const
	{
		return x*x + y*y;
	}

	float Size() const
	{
		return sqrtf(SquaredSize());
	}

	float DistanceTo(const Vector& other) const
	{
		return (other - *this).Size();
	}

	const Vector Normalize() const
	{
		float size = Size();
		if ( size == 0.0 )
			size = 1.0f;
		return *this / size;
	}

	const Vector Rotate(float radians) const
	{
		float sinr = sinf(radians);
		float cosr = cosf(radians);
		float newx = x * cosr - y * sinr;
		float newy = x * sinr + y * cosr;
		return Vector(newx, newy);
	}

	const Vector RotateAround(float radians, const Vector& off) const
	{
		return Vector(*this - off).Rotate(radians) + off;
	}
};

bool AboveLine(const Vector& a, const Vector& b, const Vector& p)
{
	Vector ba = b - a;
	Vector bahat = Vector(ba.y, -ba.x);
	Vector bp = p - a;
	return 0.0 <= bahat.Dot(bp);
}

bool InsideTriangle(const Vector& a, const Vector& b, const Vector& c,
                    const Vector& p)
{
	return !AboveLine(a, b, p) && !AboveLine(b, c, p) && !AboveLine(c, a, p);
}

class Object;
class Actor;
class Spaceship;

static Object* firstobject = NULL;
static Object* lastobject = NULL;
static Vector screenoff;
static Spaceship* playership = NULL;

class Object
{
public:
	Object()
	{
		gcborn = false;
		gcdead = false;
		if ( !firstobject )
		{
			firstobject = lastobject = this;
			prevobj = nextobj = NULL;
		}
		else
		{
			lastobject->nextobj = this;
			this->prevobj = lastobject;
			this->nextobj = NULL;
			lastobject = this;
		}
	};

	virtual ~Object()
	{
		if ( !prevobj )
			firstobject = nextobj;
		else
			prevobj->nextobj = nextobj;
		if ( !nextobj )
			lastobject = prevobj;
		else
			nextobj->prevobj = prevobj;
	}

public:
	virtual bool IsA(const char* classname)
	{
		return !strcmp(classname, "Object");
	}
	virtual void PreFrame() { }
	virtual void OnFrame(float /*deltatime*/) { }
	virtual void PostFrame(float /*deltatime*/) { }
	virtual void Render() { }

private:
	bool gcborn;
	bool gcdead;
	Object* prevobj;
	Object* nextobj;

public:
	bool GCIsBorn() const { return gcborn; }
	bool GCIsDead() const { return gcdead; }
	bool GCIsAlive() const { return GCIsBorn() && !GCIsDead(); }
	void GCDie() { gcdead = true; }
	void GCBirth() { gcborn = true; }
	Object* NextObj() const { return nextobj; }

};

class Actor : public Object
{
public:
	Actor() { mass = 1.0; }
	virtual ~Actor() { }

public:
	virtual void PreFrame()
	{
		force = Vector(0, 0);
		otherforce = Vector(0, 0);
	}

	virtual void OnFrame(float deltatime)
	{
		Think(deltatime);
	}

	virtual void PostFrame(float deltatime)
	{
		Move(deltatime);
	}

	virtual bool IsA(const char* classname)
	{
		return !strcmp(classname, "Actor") || Object::IsA(classname);
	}
	virtual void Move(float deltatime);
	virtual void Think(float /*deltatime*/) { }
	virtual void Render() { }

public:
	Vector pos;
	Vector vel;
	Vector acc;
	Vector force;
	Vector otherforce;
	float mass;

};

void Actor::Move(float deltatime)
{
	acc = (force + otherforce) / mass;
	vel += acc * deltatime;
	pos += vel * deltatime;
}

enum AsteroidType
{
	TYPE_NORMAL,
	TYPE_CRYSTAL,
	TYPE_NOT_CRYSTAL,
};

class Asteroid : public Actor
{
public:
	Asteroid(Vector pos, Vector vel, float size, AsteroidType type = TYPE_NORMAL);
	virtual ~Asteroid() { }
	virtual bool IsA(const char* classname)
	{
		return !strcmp(classname, "Asteroid") || Actor::IsA(classname);
	}
	virtual void Move(float deltatime);
	virtual void Render();

private:
	Vector Point(size_t id);

public:
	bool InsideMe(const Vector& p);
	void OnHit();
	float Size() const { return size; }
	AsteroidType Type() const { return type; }

private:
	static const size_t MIN_POLYS = 5;
	static const size_t MAX_POLYS = 12;
	static constexpr float MAX_TURN_SPEED = 50.0f;
	size_t numpolygons;
	float slice;
	float polydists[MAX_POLYS+1];
	float size;
	float angle;
	float turnspeed;
	AsteroidType type;

};

Asteroid::Asteroid(Vector pos, Vector vel, float size, AsteroidType type)
{
	this->pos = pos;
	this->vel = vel;
	this->size = size;
	this->type = type;
	float MASS_PER_UNIT = 1.0;
	this->mass = MASS_PER_UNIT * 4.0f / 3.0f * size * size * size;
	if ( type == TYPE_NORMAL )
	{
		const float CRYSTAL_CHANCE = 0.1f;
		const float MAX_SIZE = 64.0;
		if ( RandomFloat() < CRYSTAL_CHANCE && size < MAX_SIZE )
			this->type = TYPE_CRYSTAL;
	}
	angle = 0.0f;
	turnspeed = DegreeToRadian(MAX_TURN_SPEED) * (RandomFloat() * 2.0f - 1.0f);
	numpolygons = MIN_POLYS + arc4random_uniform(MAX_POLYS - MIN_POLYS);
	slice = DegreeToRadian(360.0f) / (float) numpolygons;
	for ( size_t i = 0; i < numpolygons; i++ )
	{
		polydists[i] = (RandomFloat() + 1.0) * size / 2.0;
	}
	polydists[numpolygons] = polydists[0];
}

void Asteroid::Move(float deltatime)
{
	Actor::Move(deltatime);
	angle += turnspeed * deltatime;
}

Vector Asteroid::Point(size_t i)
{
	float rot = i * slice + angle;
	return Vector(polydists[i], 0.0).Rotate(rot);
}

bool Asteroid::InsideMe(const Vector& p)
{
	const Vector& center = pos;
	for ( size_t i = 0; i < numpolygons; i++ )
	{
		Vector from = Point(i) + pos;
		Vector to = Point(i+1) + pos;
		if ( InsideTriangle(from, to, center, p) )
			return true;
	}
	return false;
}

void Asteroid::Render()
{
	Vector screenpos = pos - screenoff;
	uint32_t color = MakeColor(200, 200, 200);
	if ( type == TYPE_CRYSTAL )
		color = MakeColor(100, 100, 255);
	for ( size_t i = 0; i < numpolygons; i++ )
	{
		Vector from = Point(i) + screenpos;
		Vector to = Point(i+1) + screenpos;
		DrawLine(color, from.x, from.y, to.x, to.y);
	}
}

void Asteroid::OnHit()
{
	if ( !GCIsAlive() )
		return;
	Vector axis = Vector(size/2.0f, 0.0f).Rotate(RandomAngle());
	float sizea = RandomFloat(size*0.3, size*0.7);
	float sizeb = RandomFloat(size*0.3, size*0.7);
	const float MINIMUM_SIZE = 6.0;
	const float MAX_ANGLE = DegreeToRadian(45);
	if ( MINIMUM_SIZE <= sizea )
	{
		Vector astvel = vel.Rotate(RandomFloat(0.0, MAX_ANGLE)) * 1.2;
		new Asteroid(pos + axis, astvel, sizea, type);
	}
	if ( MINIMUM_SIZE <= sizeb )
	{
		Vector astvel = vel.Rotate(RandomFloat(0.0, -MAX_ANGLE)) * 1.2;
		new Asteroid(pos - axis, astvel, sizeb, type);
	}
	GCDie();
}

class AsteroidField : public Actor
{
public:
	AsteroidField() { }
	virtual ~AsteroidField() { }
	virtual bool IsA(const char* classname)
	{
		return !strcmp(classname, "AsteroidField") || Actor::IsA(classname);
	}

public:
	virtual void Think(float deltatime);

};

void AsteroidField::Think(float /*deltatime*/)
{
	float spawndist = 1500.0f;
	float maxdist = 1.5 * spawndist;
	size_t minimumasteroids = 200;
	size_t numasteroids = 0;
	Vector center = ((Actor*)playership)->pos;
	for ( Object* obj = firstobject; obj; obj = obj->NextObj() )
	{
		if ( !obj->IsA("Asteroid") )
			continue;
		Asteroid* ast = (Asteroid*) obj;
		numasteroids++;
		float dist = ast->pos.DistanceTo(center);
		if ( spawndist < dist ) { ast->GCDie(); }
	}
	for ( ; numasteroids < minimumasteroids; numasteroids++ )
	{
		float dist = RandomFloat(spawndist, maxdist);
		Vector astpos = Vector(dist, 0.0f).Rotate(RandomAngle()) + center;
		float minsize = 4.0;
		float maxsize = 120.0f;
		float maxspeed = 80.0f;
		float size = RandomFloat(minsize, maxsize);
		float speed = RandomFloat() * maxspeed;
		Vector astvel = Vector(speed, 0.0).Rotate(RandomAngle());
		new Asteroid(astpos, astvel, size);
	}
}

class Missile : public Actor
{
public:
	Missile(Vector pos, Vector vel, Vector direction, float ttl);
	virtual bool IsA(const char* classname)
	{
		return !strcmp(classname, "Missile") || Actor::IsA(classname);
	}
	virtual void Think(float deltatime);
	virtual void Render();
	virtual ~Missile();

protected:
	float ttl;
	Vector direction;

};

Missile::Missile(Vector pos, Vector vel, Vector direction, float ttl)
{
	this->pos = pos;
	this->vel = vel;
	this->ttl = ttl;
	this->direction = direction;
}

void Missile::Think(float deltatime)
{
	ttl -= deltatime;
	if ( ttl < 0 ) { GCDie(); }
	for ( Object* obj = firstobject; obj; obj = obj->NextObj() )
	{
		if ( !obj->GCIsAlive() )
			continue;
		if ( !obj->IsA("Asteroid") )
			continue;
		Asteroid* ast = (Asteroid*) obj;
		if ( !ast->InsideMe(pos) )
			continue;
		ast->OnHit();
		GCDie();
	}
}

void Missile::Render()
{
	Vector screenpos = pos - screenoff;
	uint32_t shipcolor = MakeColor(31, 255, 31);
	const float MISSILE_LEN = 5.0f;
	Vector from = screenpos;
	Vector to = screenpos + direction * (MISSILE_LEN / direction.Size());
	DrawLine(shipcolor, from.x, from.y, to.x, to.y);
}

Missile::~Missile()
{
}

class Attractor : public Actor
{
public:
	Attractor(Vector pos, Vector vel, float growtomass, float rate);
	virtual ~Attractor() { }
	virtual bool IsA(const char* classname)
	{
		return !strcmp(classname, "Attractor") || Actor::IsA(classname);
	}
	virtual void Think(float deltatime);
	virtual void Render();

public:
	float size;
	float growtomass;
	float rate;
	float accel;
	float age;

};

Attractor::Attractor(Vector pos, Vector vel, float growtomass, float rate)
{
	this->pos = pos;
	this->vel = vel;
	this->growtomass = growtomass;
	this->rate = rate;
	this->size = 1.0f;
	this->mass = 1.0f;
	this->rate = 0.0;
	this->accel = 20000.0;
	this->age = 0.0f;
}

void Attractor::Think(float deltatime)
{
	growtomass = 20000000.0;
	rate += deltatime * accel;
	mass += deltatime * rate;
	size += 5 * deltatime;
	age += deltatime;
	float sofar = 2.5 * age;
	mass = sofar*sofar*sofar*sofar;
	size = age*age;
	if ( 1.5*60 <= age )
	{
		GCDie();
		return;
	}
	for ( Object* obj = firstobject; obj; obj = obj->NextObj() )
	{
		if ( obj == this )
			continue;
		if ( !obj->GCIsAlive() )
			continue;
		if ( !obj->IsA("Actor") )
			continue;
		if ( obj->IsA("Attractor") )
			continue;
		//if ( obj->IsA("Spaceship") )
		//	continue;
		Actor* other = (Actor*) obj;
		Vector relative = pos - other->pos;
		float distsq = relative.SquaredSize();
		if ( distsq < size ) { distsq = size; }
		float forcesize = mass * other->mass / distsq;
		Vector force = relative.Normalize() * forcesize;
		other->force += force;
		this->force -= force;
		if ( distsq < size*size && other->IsA("Asteroid") )
		{
			Asteroid* ast = (Asteroid*) other;
			if ( RandomFloat() < 0.05f )
				ast->OnHit();
		}
	}
}

void Attractor::Render()
{
	const size_t NUM_SIDES = 128;
	float slice = DegreeToRadian(360.0f / NUM_SIDES);
	for ( size_t i = 0; i < NUM_SIDES; i++ )
	{
		Vector screenpos = pos - screenoff;
		uint32_t color = MakeColor(255, 0, 0);
		Vector from = screenpos + Vector(size, 0.0f).Rotate((i+0)*slice);
		Vector to   = screenpos + Vector(size, 0.0f).Rotate((i+1)*slice);
		DrawLine(color, from.x, from.y, to.x, to.y);
	}
}

class Firework : public Missile
{
public:
	Firework(Vector pos, Vector vel, Vector direction, float ttl);
	virtual bool IsA(const char* classname)
	{
		return !strcmp(classname, "Firework") || Actor::IsA(classname);
	}
	virtual void Think(float deltatime);

};

class Spaceship : public Actor
{
protected:
	Spaceship() { }

public:
	Spaceship(float shipangle,
	          Vector pos = Vector(0, 0),
	          Vector vel = Vector(0, 0),
	          Vector acc = Vector(0, 0));
	virtual ~Spaceship();

public:
	virtual bool IsA(const char* classname)
	{
		return !strcmp(classname, "Spaceship") || Actor::IsA(classname);
	}
	virtual void Think(float deltatime);
	virtual void Render();

public:
	void SetThrust(bool forward, bool backward);
	void SetTurn(bool turnleft, bool turnright);
	void SetFiring(bool missile, bool firework, bool attractor);

protected:
	bool turnleft;
	bool turnright;
	bool moveforward;
	bool movebackward;
	bool missile, firework, attractor;
	float shipangle;

};

Spaceship::Spaceship(float shipangle, Vector pos, Vector vel, Vector acc)
{
	this->shipangle = shipangle;
	this->pos = pos;
	this->vel = vel;
	this->acc = acc;
	this->mass = 1.0;
	turnleft = turnright = moveforward = movebackward = missile = \
	attractor = firework = false;
}

Spaceship::~Spaceship()
{
}

void Spaceship::Think(float deltatime)
{
	for ( Object* obj = firstobject; obj; obj = obj->NextObj() )
	{
		if ( !obj->GCIsAlive() )
			continue;
		if ( !obj->IsA("Asteroid") )
			continue;
		Asteroid* ast = (Asteroid*) obj;
		bool iscrystal = ast->Type() == TYPE_CRYSTAL;
		bool isinside = false;
		if ( iscrystal )
		{
			Vector relative = pos - ast->pos;
			float distsq = relative.SquaredSize();
			if ( distsq < 4.0 * 4.0 )
				isinside = true;
			if ( distsq < 100 )
				distsq = 100;
			float constant = 2000.0;
			float forcesize = constant * mass * ast->mass / distsq;
			Vector force = relative.Normalize() * forcesize;
			ast->force += force;
		}
		isinside = isinside || ast->InsideMe(pos);
		if ( isinside && iscrystal )
		{
			ast->GCDie();
			continue;
		}
		else if ( isinside && IsA("Botship") )
		{
			ast->OnHit();
			GCDie();
			return;
		}
		else if ( isinside )
		{
			ast->OnHit();
			pos.y = 16384 - pos.y;
			vel = Vector();
			break;
		}
	}
	const float turnspeed = 100.0;
	const float turnamount = turnspeed * deltatime;
	if ( turnleft )
		shipangle -= DegreeToRadian(turnamount);
	if ( turnright )
		shipangle += DegreeToRadian(turnamount);
	float shipaccelamount = 15.0;
	float shipaccel = 0.0;
	if ( moveforward )
		shipaccel += shipaccelamount;
	if ( movebackward )
		shipaccel -= shipaccelamount;
	force += Vector(shipaccel, 0.0).Rotate(shipangle) * mass;
	float shipspeed = vel.Size();
	float maxspeed = 50.0f;
	//if ( maxspeed < shipspeed )
	//	vel *= maxspeed / shipspeed;
	if ( maxspeed < shipspeed )
	{
		Vector backforce = vel.Normalize() * -shipaccelamount * mass;
		force += backforce;
	}
	if ( missile || firework || attractor )
	{
		float ttl = 8.0;
		float speed = 120.0;
		const Vector P3(16.0f, 0.0f);
		Vector spawnpos = pos + P3.Rotate(shipangle) * 1.1;
		Vector spawnvel = Vector(speed, 0.0).Rotate(shipangle);
		if ( missile )
			new Missile(spawnpos, vel + spawnvel, spawnvel, ttl);
		if ( firework )
			new Firework(spawnpos, vel + spawnvel, spawnvel, 0.0);
		if ( attractor )
			new Attractor(spawnpos, vel + spawnvel, 10000.0, 1.001);
	}
}

void Spaceship::Render()
{
	Vector screenpos = pos - screenoff;
	const Vector P1(-8.0f, 8.0f);
	const Vector P2(-8.0f, -8.0f);
	const Vector P3(16.0f, 0.0f);
	Vector p1 = P1.Rotate(shipangle) + screenpos;
	Vector p2 = P2.Rotate(shipangle) + screenpos;
	Vector p3 = P3.Rotate(shipangle) + screenpos;
	uint32_t shipcolor = MakeColor(200, 200, 200);
	if ( this == playership )
		shipcolor = MakeColor(255, 255, 255);
	DrawLine(shipcolor, p1.x, p1.y, p2.x, p2.y);
	DrawLine(shipcolor, p2.x, p2.y, p3.x, p3.y);
	DrawLine(shipcolor, p1.x, p1.y, p3.x, p3.y);
}

void Spaceship::SetThrust(bool forward, bool backward)
{
	this->moveforward = forward;
	this->movebackward = backward;
}

void Spaceship::SetTurn(bool turnleft, bool turnright)
{
	this->turnleft = turnleft;
	this->turnright = turnright;
}

void Spaceship::SetFiring(bool missile, bool firework, bool attractor)
{
	this->missile = missile;
	this->firework = firework;
	this->attractor = attractor;
}

class Botship : public Spaceship
{
public:
	Botship(float shipangle,
	          Vector pos = Vector(0, 0),
	          Vector vel = Vector(0, 0),
	          Vector acc = Vector(0, 0));
	virtual ~Botship();

public:
	virtual bool IsA(const char* classname)
	{
		return !strcmp(classname, "Botship") || Spaceship::IsA(classname);
	}
	virtual void Think(float deltatime);

private:
	float firedelay;

};

Botship::Botship(float shipangle, Vector pos, Vector vel, Vector acc)
{
	this->shipangle = shipangle;
	this->pos = pos;
	this->vel = vel;
	this->acc = acc;
	this->mass = 1.0;
	turnleft = false;
	turnright = false;
	moveforward = false;
	movebackward = false;
	missile = false;
	attractor = false;
	firework = false;
	firedelay = 0.0f;
}

Botship::~Botship()
{
}

void Botship::Think(float deltatime)
{
	if ( 0.0f < firedelay )
		firedelay -= deltatime;
	const float PLAYER_MAX_DIST = 512.0f;
	float playerdist = (playership->pos - pos).Size();
	bool needreturn = PLAYER_MAX_DIST < playerdist;
	Actor* target = NULL;
	bool movetotarget = false;
	Asteroid* asttarget = NULL;
	float targetsafety = 0.0f;
	for ( Object* obj = firstobject; !needreturn && obj; obj = obj->NextObj() )
	{
		if ( !obj->GCIsAlive() )
			continue;
		if ( !obj->IsA("Asteroid") )
			continue;
		Asteroid* ast = (Asteroid*) obj;
		if ( ast->Type() == TYPE_CRYSTAL )
			continue;
		Vector mypos = pos + vel * 2.0f;
		Vector astdir = ast->pos - mypos;
		float safety = astdir.Size() / (ast->vel.Size() * sqrtf(ast->Size()));
		if ( !asttarget || safety < targetsafety )
			asttarget = ast, targetsafety = safety;
	}
	target = asttarget;
	if ( needreturn )
		target = playership, movetotarget = true;
	moveforward = movebackward = missile = false;
	if ( (missile = asttarget && !needreturn && firedelay <= 0.0f) )
		firedelay = 0.3;
	if ( target )
	{
		// Estimate the location of the target.
		Vector targetdir = target->pos - pos;
		float missile_speed = 120.0;
		float firetime = targetdir.Size() / missile_speed;
		Vector projectedpos = target->pos + target->vel * firetime;
		Vector projecteddir = projectedpos - pos;

		// Further estimate the location of the target.
		float firetimeg2 = projecteddir.Size() / missile_speed;
		Vector projectedposg2 = target->pos + target->vel * firetimeg2;
		Vector projecteddirg2 = projectedposg2 - pos;

		// Calculate which direction to look in.
		Vector forward = Vector(1.0, 0.0).Rotate(shipangle);
		Vector forwardhat(-forward.y, forward.x);
		float dotproduct = forwardhat.Dot(projecteddirg2);
		turnright = 0.0f < dotproduct;
		turnleft = !turnright;
	}
	if ( target && movetotarget )
		moveforward = true;
	Spaceship::Think(deltatime);
}

Firework::Firework(Vector pos, Vector vel, Vector dir, float ttl) : Missile(pos, vel, dir, ttl)
{
}

void Firework::Think(float deltatime)
{
	ttl -= deltatime;
	if ( ttl < 0 )
	{
		// Explode in a shower of 8 missiles
		const float MISSILE_TTL = 3.0;
		const float MISSILE_SPEED = 8.0;
		const size_t NUM_MISSILES = 8;
		const Vector velocity = Vector(MISSILE_SPEED, 0);
		const float offsetangle = RandomAngle();
		const float angle = 2 * M_PI / NUM_MISSILES;
		for ( size_t i = 0; i < NUM_MISSILES; i++ )
		{
			Vector dir = velocity.Rotate(offsetangle + angle * i);
			new Missile(pos, vel + dir, dir, MISSILE_TTL);
		}
		GCDie();
		return;
	}
	for ( Object* obj = firstobject; obj; obj = obj->NextObj() )
	{
		if ( !obj->GCIsAlive() )
			continue;
		if ( !obj->IsA("Asteroid") )
			continue;
		Asteroid* ast = (Asteroid*) obj;
		if ( !ast->InsideMe(pos) )
			continue;
		// Fireworks taken out by asteroids before explosion.
		GCDie();
	}
}

struct timespec lastframeat;

void GameLogic()
{
	struct timespec now;
	do clock_gettime(CLOCK_MONOTONIC, &now);
	while ( timespec_eq(now, lastframeat) );
	struct timespec delta = timespec_sub(now, lastframeat);
	float deltatime = delta.tv_sec + delta.tv_nsec / 1E9f;
	lastframeat = now;
	float timescale = 3.0;
	deltatime *= timescale;
	Object* first = firstobject;
	Object* obj;
	for ( obj = first; obj; obj = obj->NextObj() )
		obj->GCBirth();
	bool key_a = pop_is_key_just_down(KBKEY_A);
	bool key_b = pop_is_key_just_down(KBKEY_B);
	bool key_space = pop_is_key_just_down(KBKEY_SPACE);
	bool key_lctrl = pop_is_key_just_down(KBKEY_LCTRL);
	playership->SetThrust(keysdown[KBKEY_UP], keysdown[KBKEY_DOWN]);
	playership->SetTurn(keysdown[KBKEY_LEFT], keysdown[KBKEY_RIGHT]);
	playership->SetFiring(key_space, key_lctrl, key_a);
	bool makebot = key_b;
	if ( makebot )
		new Botship(RandomAngle(), playership->pos, playership->vel);
	for ( obj = first; obj; obj = obj->NextObj() )
	{
		if ( !obj->GCIsBorn() )
			continue;
		obj->PreFrame();
	}
	for ( obj = first; obj; obj = obj->NextObj() )
	{
		if ( !obj->GCIsBorn() )
			continue;
		obj->OnFrame(deltatime);
	}
	for ( obj = first; obj; obj = obj->NextObj() )
	{
		if ( !obj->GCIsBorn() )
			continue;
		obj->PostFrame(deltatime);
	}
	for ( obj = first; obj; )
	{
		Object* todelete = obj;
		obj = obj->NextObj();
		if ( !todelete->GCIsDead() )
			continue;
		delete todelete;
	}
}

void Render()
{
	screenoff = playership->pos - Vector(xres/2.0, yres/2.0);
	size_t staroffx = (size_t) screenoff.x;
	size_t staroffy = (size_t) screenoff.y;
	for ( size_t y = 0; y < yres; y++ )
	{
		uint32_t* line = buf + y * linesize;
		for ( size_t x = 0; x < xres; x++ )
		{
			size_t fieldx = (x+staroffx) % STARFIELD_WIDTH;
			size_t fieldy = (y+staroffy) % STARFIELD_HEIGHT;
			size_t fieldindex = fieldy * STARFIELD_HEIGHT + fieldx;
			line[x] = starfield[fieldindex];
		}
	}

	for ( Object* obj = firstobject; obj; obj = obj->NextObj() )
	{
		obj->Render();
	}
}
void RunFrame(struct dispd_window* window)
{
	struct dispd_framebuffer* fb = dispd_begin_render(window);
	if ( !fb )
	{
		error(0, 0, "unable to begin rendering dispd window");
		gamerunning = false;
		return;
	}
	xres = dispd_get_framebuffer_width(fb);
	yres = dispd_get_framebuffer_height(fb);
	bpp = dispd_get_framebuffer_format(fb);
	linesize = dispd_get_framebuffer_pitch(fb) / (bpp / 8);
	framesize = dispd_get_framebuffer_pitch(fb) * yres;
	buf = (uint32_t*) dispd_get_framebuffer_data(fb);
	FetchKeyboardInput();
	GameLogic();
	Render();
	dispd_finish_render(fb);
}

void InitGame()
{
	clock_gettime(CLOCK_MONOTONIC, &lastframeat);
	GenerateStarfield(starfield, STARFIELD_WIDTH, STARFIELD_HEIGHT);
	playership = new Spaceship(0.0, Vector(0, 0), Vector(4.0f, 0));
	new AsteroidField;
}

static struct termios saved_tio;

static void restore_terminal_on_exit(void)
{
	tcsetattr(0, TCSAFLUSH, &saved_tio);
}

static void restore_terminal_on_signal(int signum)
{
	if ( signum == SIGTSTP )
	{
		struct termios tio;
		tcgetattr(0, &tio);
		tcsetattr(0, TCSAFLUSH, &saved_tio);
		raise(SIGSTOP);
		tcgetattr(0, &saved_tio);
		tcsetattr(0, TCSAFLUSH, &tio);
		return;
	}
	tcsetattr(0, TCSAFLUSH, &saved_tio);
	raise(signum);
}

int main(int argc, char* argv[])
{
	if ( !isatty(0) )
		error(1, errno, "standard input");
	if ( tcgetattr(0, &saved_tio) < 0 )
		error(1, errno, "tcsetattr: standard input");
	if ( atexit(restore_terminal_on_exit) != 0 )
		error(1, errno, "atexit");
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = restore_terminal_on_signal;
	sigaction(SIGTSTP, &sa, NULL);
	sa.sa_flags = SA_RESETHAND;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	if ( !dispd_initialize(&argc, &argv) )
		error(1, 0, "couldn't initialize dispd library");
	struct dispd_session* session = dispd_attach_default_session();
	if ( !session )
		error(1, 0, "couldn't attach to dispd default session");
	if ( !dispd_session_setup_game_rgba(session) )
		error(1, 0, "couldn't setup dispd rgba session");
	struct dispd_window* window = dispd_create_window_game_rgba(session);
	if ( !window )
		error(1, 0, "couldn't create dispd rgba window");

	InitGame();
	gamerunning = true;
	for ( framenum = 0; gamerunning; framenum++ )
		RunFrame(window);

	dispd_destroy_window(window);
	dispd_detach_session(session);

	return 0;
}
