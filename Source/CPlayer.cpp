//-----------------------------------------------------------------------------
// File: CPlayer.cpp
//
// Desc: This file stores the player object class. This class performs tasks
//       such as player movement, some minor physics as well as rendering.
//
// Original design by Adam Hoult & Gary Simmons. Modified by Mihai Popescu.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// CPlayer Specific Includes
//-----------------------------------------------------------------------------
#include "CPlayer.h"
#include "CGameApp.h"


extern CGameApp g_App;
//-----------------------------------------------------------------------------
// Name : CPlayer () (Constructor)
// Desc : CPlayer Class Constructor
//-----------------------------------------------------------------------------
CPlayer::CPlayer(const BackBuffer *pBackBuffer, int x)
{
	//m_pSprite = new Sprite("data/planeimg.bmp", "data/planemask.bmp");
	if (x == 1)
	{
		m_pSprite = new Sprite("data/planeimgandmask.bmp", RGB(0xff, 0x00, 0xff));
		m_pSprite->setBackBuffer(pBackBuffer);
	}
	else
	{
		m_pSprite = new Sprite("data/planeimgandmaskk.bmp", RGB(0xff, 0x00, 0xff));
		m_pSprite->setBackBuffer(pBackBuffer);
	}
	
	m_eSpeedState = SPEED_STOP;
	m_fTimer = 0;
	isDead = false;

	bullet = new Sprite("data/bm.bmp", "data/bm.bmp");
	bullet->setBackBuffer(pBackBuffer);

	// Animation frame crop rectangle
	RECT r;
	r.left = 0;
	r.top = 0;
	r.right = 128;
	r.bottom = 128;

	m_pExplosionSprite	= new AnimatedSprite("data/explosion.bmp", "data/explosionmask.bmp", r, 16);
	m_pExplosionSprite->setBackBuffer( pBackBuffer );
	m_bExplosion		= false;
	m_iExplosionFrame	= 0;

	//m_bSprite = new Sprite("data/bullet.bmp", "data/bulletMask.bmp");
	//m_bSprite->setBackBuffer(pBackBuffer);
}

//-----------------------------------------------------------------------------
// Name : ~CPlayer () (Destructor)
// Desc : CPlayer Class Destructor
//-----------------------------------------------------------------------------
CPlayer::~CPlayer()
{
	delete m_pSprite;
	delete m_pExplosionSprite;
	delete m_bSprite;
}

void CPlayer::Update(float dt)
{
	// Update sprite
	m_pSprite->update(dt);


	// Get velocity
	double v = m_pSprite->mVelocity.Magnitude();

	// NOTE: for each async sound played Windows creates a thread for you
	// but only one, so you cannot play multiple sounds at once.
	// This creation/destruction of threads also leads to bad performance
	// so this method is not recommanded to be used in complex projects.

	// update internal time counter used in sound handling (not to overlap sounds)
	m_fTimer += dt;

	// A FSM is used for sound manager 
	switch(m_eSpeedState)
	{
	case SPEED_STOP:
		if(v > 35.0f)
		{
			m_eSpeedState = SPEED_START;
			PlaySound("data/jet-start.wav", NULL, SND_FILENAME | SND_ASYNC);
			m_fTimer = 0;
		}
		break;
	case SPEED_START:
		if(v < 25.0f)
		{
			m_eSpeedState = SPEED_STOP;
			PlaySound("data/jet-stop.wav", NULL, SND_FILENAME | SND_ASYNC);
			m_fTimer = 0;
		}
		else
			if(m_fTimer > 1.f)
			{
				PlaySound("data/jet-cabin.wav", NULL, SND_FILENAME | SND_ASYNC);
				m_fTimer = 0;
			}
		break;
	}

	// NOTE: For sound you also can use MIDI but it's Win32 API it is a bit hard
	// see msdn reference: http://msdn.microsoft.com/en-us/library/ms711640.aspx
	// In this case you can use a C++ wrapper for it. See the following article:
	// http://www.codeproject.com/KB/audio-video/midiwrapper.aspx (with code also)
}

void CPlayer::Draw()
{
	if (fireCooldown > 1) {
		fireCooldown--;
	}
	if(!m_bExplosion)
		m_pSprite->draw();
	else
		m_pExplosionSprite->draw();
}

void CPlayer::Move(ULONG ulDirection)
{
	if (m_pSprite->mPosition.x - m_pSprite->width()/2 <= 0)
	{
		m_pSprite->mVelocity.x = 0;
		m_pSprite->mPosition.x = m_pSprite->mPosition.x + 1;
	}
	else if (ulDirection & CPlayer::DIR_LEFT)
	{
		m_pSprite->mVelocity.x -= 5;
	}

	if (m_pSprite->mPosition.x + m_pSprite->width()/2 >= GetSystemMetrics(SM_CXSCREEN))
	{
		m_pSprite->mVelocity.x = 0;
		m_pSprite->mPosition.x = m_pSprite->mPosition.x - 1;
	}
	else if (ulDirection & CPlayer::DIR_RIGHT)
	{
		m_pSprite->mVelocity.x += 5;
	}

	if (m_pSprite->mPosition.y - m_pSprite->height()/2 <= 0)
	{
		m_pSprite->mVelocity.y = 0;
		m_pSprite->mPosition.y = m_pSprite->mPosition.y + 1;
	}
	else if (ulDirection & CPlayer::DIR_FORWARD)
	{
		m_pSprite->mVelocity.y -= 5;
	}

	if (m_pSprite->mPosition.y + m_pSprite->height()/2 >= GetSystemMetrics(SM_CYSCREEN) - m_pSprite->height()/2)
	{
		m_pSprite->mVelocity.y = 0;
		m_pSprite->mPosition.y = m_pSprite->mPosition.y - 1;
	}
	else if (ulDirection & CPlayer::DIR_BACKWARD)
	{
		m_pSprite->mVelocity.y += 5;
	}
		
}


Vec2& CPlayer::Position()
{
	return m_pSprite->mPosition;
}

Vec2& CPlayer::Velocity()
{
	return m_pSprite->mVelocity;
}

void CPlayer::Explode()
{
	m_pExplosionSprite->SetFrame(0);
	m_pExplosionSprite->mPosition = m_pSprite->mPosition;
	m_pSprite->mVelocity.x = 0;
	m_pSprite->mVelocity.y = 0;
	PlaySound("data/explosion.wav", NULL, SND_FILENAME | SND_ASYNC);
	m_bExplosion = true;
}

bool CPlayer::AdvanceExplosion()
{
	if(m_bExplosion)
	{
		m_pExplosionSprite->SetFrame(m_iExplosionFrame++);
		if(m_iExplosionFrame==m_pExplosionSprite->GetFrameCount())
		{
			isDead = true;
			m_bExplosion = false;
			m_iExplosionFrame = 0;
			m_eSpeedState = SPEED_STOP;
			return false;
		}
	}

	return true;
}

int CPlayer::playerIsDead()
{
	return isDead;
}

void CPlayer::Shoot(int x)
{
	if (fireCooldown < 25) 
	{
		bullet = new Sprite("data/b.bmp", "data/bm.bmp");
		bullet->setBackBuffer(g_App.m_pBBuffer);
		if (x == 1) {
			bullet->mPosition.y = m_pSprite->mPosition.y - m_pSprite->height() / 2;
			bullet->mPosition.x = m_pSprite->mPosition.x;
		}
		else {
			bullet->mPosition.y = m_pSprite->mPosition.y + m_pSprite->height() / 2;
			bullet->mPosition.x = m_pSprite->mPosition.x;
		}
		bullets.push_back(bullet);
		fireCooldown = 60;

	}

}

void CPlayer::fire(int x, int y) {
	for (Sprite* it : bullets) 
	{
		it->draw();
		it->mPosition.y += x;
		it->mVelocity.y = x;
		it->mPosition.x += y;
		it->mVelocity.x = y;
	}
}

int& CPlayer::frameCounter() {
	return m_pSprite->frameCounter;
}

int CPlayer::positionWidth()
{
	return m_pSprite->width();
}

int CPlayer::positionHeight()
{
	return m_pSprite->height();
}
