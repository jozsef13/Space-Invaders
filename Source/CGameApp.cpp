//-----------------------------------------------------------------------------
// File: CGameApp.cpp
//
// Desc: Game Application class, this is the central hub for all app processing
//
// Original design by Adam Hoult & Gary Simmons. Modified by Mihai Popescu.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// CGameApp Specific Includes
//-----------------------------------------------------------------------------
#include "CGameApp.h"
#include <cstdlib> 
#include <ctime> 

extern HINSTANCE g_hInst;
bool		p1Shoot = false;
bool		p2Shoot = false;
static UINT fTimer;
//-----------------------------------------------------------------------------
// CGameApp Member Functions
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Name : CGameApp () (Constructor)
// Desc : CGameApp Class Constructor
//-----------------------------------------------------------------------------
CGameApp::CGameApp()
{
	// Reset / Clear all required values
	m_hWnd			= NULL;
	m_hIcon			= NULL;
	m_hMenu			= NULL;
	m_pBBuffer		= NULL;
	m_pPlayer		= NULL;
	m_pPlayer2		= NULL;
	m_scoreP1		= NULL;
	m_scoreP2		= NULL;
	m_wonSprite = NULL;
	m_lostSprite = NULL;
	m_LastFrameRate = 0;
}

//-----------------------------------------------------------------------------
// Name : ~CGameApp () (Destructor)
// Desc : CGameApp Class Destructor
//-----------------------------------------------------------------------------
CGameApp::~CGameApp()
{
	// Shut the engine down
	ShutDown();
}

//-----------------------------------------------------------------------------
// Name : InitInstance ()
// Desc : Initialises the entire Engine here.
//-----------------------------------------------------------------------------
bool CGameApp::InitInstance( LPCTSTR lpCmdLine, int iCmdShow )
{
	// Create the primary display device
	if (!CreateDisplay()) { ShutDown(); return false; }

	// Build Objects
	if (!BuildObjects()) 
	{ 
		MessageBox( 0, _T("Failed to initialize properly. Reinstalling the application may solve this problem.\nIf the problem persists, please contact technical support."), _T("Fatal Error"), MB_OK | MB_ICONSTOP);
		ShutDown(); 
		return false; 
	}

	// Set up all required game states
	SetupGameState();

	// Success!
	return true;
}

//-----------------------------------------------------------------------------
// Name : CreateDisplay ()
// Desc : Create the display windows, devices etc, ready for rendering.
//-----------------------------------------------------------------------------
bool CGameApp::CreateDisplay()
{
	HMONITOR hmon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = { sizeof(mi) };
	if (!GetMonitorInfo(hmon, &mi)) return NULL;
	m_screenSize = Vec2(mi.rcMonitor.right, mi.rcMonitor.bottom);

	LPTSTR			WindowTitle		= _T("GameFramework");
	LPCSTR			WindowClass		= _T("GameFramework_Class");
	USHORT			Width			= mi.rcMonitor.right;
	USHORT			Height			= mi.rcMonitor.bottom;
	RECT			rc;
	WNDCLASSEX		wcex;


	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= CGameApp::StaticWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= g_hInst;
	wcex.hIcon			= LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= WindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON));

	if(RegisterClassEx(&wcex)==0)
		return false;

	// Retrieve the final client size of the window
	::GetClientRect( m_hWnd, &rc );
	m_nViewX		= rc.left;
	m_nViewY		= rc.top;
	m_nViewWidth	= rc.right - rc.left;
	m_nViewHeight	= rc.bottom - rc.top;

	m_hWnd = CreateWindow(WindowClass, WindowTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, Width, Height, NULL, NULL, g_hInst, this);

	if (!m_hWnd)
		return false;

	// Show the window
	ShowWindow(m_hWnd, SW_SHOW);

	// Success!!
	return true;
}

//-----------------------------------------------------------------------------
// Name : BeginGame ()
// Desc : Signals the beginning of the physical post-initialisation stage.
//		From here on, the game engine has control over processing.
//-----------------------------------------------------------------------------
int CGameApp::BeginGame()
{
	MSG		msg;

	// Start main loop
	while(true) 
	{
		// Did we recieve a message, or are we idling ?
		if ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) 
		{
			if (msg.message == WM_QUIT) break;
			TranslateMessage( &msg );
			DispatchMessage ( &msg );
		} 
		else 
		{
			// Advance Game Frame.
			FrameAdvance();

		} // End If messages waiting
	
	} // Until quit message is receieved

	return 0;
}
	
//-----------------------------------------------------------------------------
// Name : ShutDown ()
// Desc : Shuts down the game engine, and frees up all resources.
//-----------------------------------------------------------------------------
bool CGameApp::ShutDown()
{
	// Release any previously built objects
	ReleaseObjects ( );
	
	// Destroy menu, it may not be attached
	if ( m_hMenu ) DestroyMenu( m_hMenu );
	m_hMenu		 = NULL;

	// Destroy the render window
	SetMenu( m_hWnd, NULL );
	if ( m_hWnd ) DestroyWindow( m_hWnd );
	m_hWnd		  = NULL;
	
	// Shutdown Success
	return true;
}

//-----------------------------------------------------------------------------
// Name : StaticWndProc () (Static Callback)
// Desc : This is the main messge pump for ALL display devices, it captures
//		the appropriate messages, and routes them through to the application
//		class for which it was intended, therefore giving full class access.
// Note : It is VITALLY important that you should pass your 'this' pointer to
//		the lpParam parameter of the CreateWindow function if you wish to be
//		able to pass messages back to that app object.
//-----------------------------------------------------------------------------
LRESULT CALLBACK CGameApp::StaticWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	// If this is a create message, trap the 'this' pointer passed in and store it within the window.
	if ( Message == WM_CREATE ) SetWindowLong( hWnd, GWL_USERDATA, (LONG)((CREATESTRUCT FAR *)lParam)->lpCreateParams);

	// Obtain the correct destination for this message
	CGameApp *Destination = (CGameApp*)GetWindowLong( hWnd, GWL_USERDATA );
	
	// If the hWnd has a related class, pass it through
	if (Destination) return Destination->DisplayWndProc( hWnd, Message, wParam, lParam );
	
	// No destination found, defer to system...
	return DefWindowProc( hWnd, Message, wParam, lParam );
}

//-----------------------------------------------------------------------------
// Name : DisplayWndProc ()
// Desc : The display devices internal WndProc function. All messages being
//		passed to this function are relative to the window it owns.
//-----------------------------------------------------------------------------
LRESULT CGameApp::DisplayWndProc( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam )
{
	static UINT			fTimer;	

	// Determine message type
	switch (Message)
	{
		case WM_CREATE:
			break;
		
		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		
		case WM_SIZE:
			if ( wParam == SIZE_MINIMIZED )
			{
				// App is inactive
				m_bActive = false;
			
			} // App has been minimized
			else
			{
				// App is active
				m_bActive = true;

				// Store new viewport sizes
				m_nViewWidth  = LOWORD( lParam );
				m_nViewHeight = HIWORD( lParam );
		
			
			} // End if !Minimized

			break;

		case WM_LBUTTONDOWN:
			// Capture the mouse
			SetCapture( m_hWnd );
			GetCursorPos( &m_OldCursorPos );
			break;

		case WM_LBUTTONUP:
			// Release the mouse
			ReleaseCapture( );
			break;

		case WM_KEYDOWN:
			switch(wParam)
			{
			case VK_ESCAPE:
				PostQuitMessage(0);
				break;
			case 'B':
				m_pPlayer->Rotate(1);
				break;
			case 'R':
				m_pPlayer2->Rotate(2);
				break;
			}
			break;

		case WM_TIMER:
			switch (wParam)
			{
			case 1:
				if (!m_pPlayer->AdvanceExplosion()) 
					fTimer = SetTimer(m_hWnd, 1, 70, NULL);
				if (!m_pPlayer2->AdvanceExplosion())
					fTimer = SetTimer(m_hWnd, 1, 70, NULL);
				for (auto enem : m_enemies)
				{
					if (!enem->EnemyAdvanceExplosion())
						fTimer = SetTimer(m_hWnd, 1, 70, NULL);
				}
			}
			break;

		case WM_COMMAND:
			break;

		default:
			return DefWindowProc(hWnd, Message, wParam, lParam);

	} // End Message Switch
	
	return 0;
}

//-----------------------------------------------------------------------------
// Name : BuildObjects ()
// Desc : Build our demonstration meshes, and the objects that instance them
//-----------------------------------------------------------------------------
bool CGameApp::BuildObjects()
{
	m_pBBuffer = new BackBuffer(m_hWnd, m_nViewWidth, m_nViewHeight);
	m_pPlayer = new CPlayer(m_pBBuffer, "data/ship1.bmp");
	m_pPlayer2 = new CPlayer(m_pBBuffer, "data/ship2.bmp");
	m_scoreP1 = new ScoreSprite(Vec2(190, 690), m_pBBuffer);
	m_wonSprite = new Sprite("data/winscreen.bmp", RGB(0xff, 0x00, 0xff));
	m_lostSprite = new Sprite("data/losescreen.bmp", RGB(0xff, 0x00, 0xff));

	m_scoreP2 = new ScoreSprite(Vec2(m_screenSize.x - 180, 690.0), m_pBBuffer);

	addEnemies(ceil((m_screenSize.x - 600) / 100) * 4);

	m_wonSprite->setBackBuffer(m_pBBuffer);
	m_lostSprite->setBackBuffer(m_pBBuffer);

	setPLives(3, 3);

	if(!m_imgBackground.LoadBitmapFromFile("data/Background.bmp", GetDC(m_hWnd)))
		return false;

	// Success!
	return true;
}

//-----------------------------------------------------------------------------
// Name : SetupGameState ()
// Desc : Sets up all the initial states required by the game.
//-----------------------------------------------------------------------------
void CGameApp::SetupGameState()
{
	m_pPlayer->Position() = Vec2(600, 700);
	m_pPlayer2->Position() = Vec2(900, 700);

	m_pPlayer->frameCounter() = 200;
	m_pPlayer2->frameCounter() = 200;

	m_wonSprite->mPosition = Vec2(int(m_screenSize.x / 2), int(m_screenSize.y / 2));
	m_lostSprite->mPosition = Vec2(int(m_screenSize.x / 2), int(m_screenSize.y / 2));

	m_gameState = GameState::ONGOING;
}

//-----------------------------------------------------------------------------
// Name : ReleaseObjects ()
// Desc : Releases our objects and their associated memory so that we can
//		rebuild them, if required, during our applications life-time.
//-----------------------------------------------------------------------------
void CGameApp::ReleaseObjects( )
{
	if(m_pPlayer != NULL)
	{
		delete m_pPlayer;
		m_pPlayer = NULL;
	}

	if (m_pPlayer2 != NULL)
	{
		delete m_pPlayer2;
		m_pPlayer2 = NULL;
	}

	if (m_scoreP1 != NULL) {
		delete m_scoreP1;
		m_scoreP1 = NULL;
	}

	if (m_scoreP2 != NULL) {
		delete m_scoreP2;
		m_scoreP2 = NULL;
	}

	if (m_wonSprite != NULL) {
		delete m_wonSprite;
		m_wonSprite = NULL;
	}

	if (m_lostSprite != NULL) {
		delete m_lostSprite;
		m_lostSprite = NULL;
	}

	while (!m_enemies.empty()) delete m_enemies.front(), m_enemies.pop_front();
	while (!bullets.empty()) delete bullets.front(), bullets.pop_front();
	while (!m_livesGreen.empty()) delete m_livesGreen.front(), m_livesGreen.pop_front();
	while (!m_livesRed.empty()) delete m_livesRed.front(), m_livesRed.pop_front();

	if (m_pBBuffer != NULL)
	{
		delete m_pBBuffer;
		m_pBBuffer = NULL;
	}
}

//-----------------------------------------------------------------------------
// Name : FrameAdvance () (Private)
// Desc : Called to signal that we are now rendering the next frame.
//-----------------------------------------------------------------------------
void CGameApp::FrameAdvance()
{
	static TCHAR FrameRate[ 50 ];
	static TCHAR TitleBuffer[ 255 ];

	// Advance the timer
	m_Timer.Tick( );

	// Skip if app is inactive
	if ( !m_bActive ) return;
	
	// Get / Display the framerate
	if ( m_LastFrameRate != m_Timer.GetFrameRate() )
	{
		m_LastFrameRate = m_Timer.GetFrameRate( FrameRate, 50 );
		sprintf_s( TitleBuffer, _T("Game : %s"), FrameRate );
		SetWindowText( m_hWnd, TitleBuffer );

	} // End if Frame Rate Altered

	// Poll & Process input devices
	ProcessInput();

	// Animate the game objects
	AnimateObjects();

	// Remove dead enemies
	removeDead();

	// Drawing the game objects
	DrawObjects();
}

//-----------------------------------------------------------------------------
// Name : ProcessInput () (Private)
// Desc : Simply polls the input devices and performs basic input operations
//-----------------------------------------------------------------------------
void CGameApp::ProcessInput( )
{
	static UCHAR pKeyBuffer[ 256 ];
	ULONG		Direction = 0;
	ULONG		Direction1 = 0;
	POINT		CursorPos;
	float		X = 0.0f, Y = 0.0f;

	// Retrieve keyboard state
	if ( !GetKeyboardState( pKeyBuffer ) ) return;

	// Check the relevant keys
	if (!m_pPlayer->isDead)
	{
		if (pKeyBuffer[VK_UP] & 0xF0) Direction |= CPlayer::DIR_FORWARD;
		if (pKeyBuffer[VK_DOWN] & 0xF0) Direction |= CPlayer::DIR_BACKWARD;
		if (pKeyBuffer[VK_LEFT] & 0xF0) Direction |= CPlayer::DIR_LEFT;
		if (pKeyBuffer[VK_RIGHT] & 0xF0) Direction |= CPlayer::DIR_RIGHT;
		if (pKeyBuffer[VK_SPACE] & 0xF0 && m_pPlayer->frameCounter() >= 50)
		{
			fireBullet(m_pPlayer->Position(), Vec2(0, -250), 1);
			m_pPlayer->frameCounter() = 0;
		}
	}

	if (!m_pPlayer2->isDead)
	{
		if (pKeyBuffer[0x57] & 0xF0) Direction1 |= CPlayer::DIR_FORWARD;
		if (pKeyBuffer[0x53] & 0xF0) Direction1 |= CPlayer::DIR_BACKWARD;
		if (pKeyBuffer[0x41] & 0xF0) Direction1 |= CPlayer::DIR_LEFT;
		if (pKeyBuffer[0x44] & 0xF0) Direction1 |= CPlayer::DIR_RIGHT;
		if (pKeyBuffer['P'] & 0xF0 && m_pPlayer2->frameCounter() >= 50)
		{
			fireBullet(m_pPlayer2->Position(), Vec2(0, -250), 2);
			m_pPlayer2->frameCounter() = 0;
		}
	}
	
	// Move the player
	m_pPlayer->Move(Direction);
	m_pPlayer2->Move(Direction1);


	// Now process the mouse (if the button is pressed)
	if ( GetCapture() == m_hWnd )
	{
		// Hide the mouse pointer
		SetCursor( NULL );

		// Retrieve the cursor position
		GetCursorPos( &CursorPos );

		// Reset our cursor position so we can keep going forever :)
		SetCursorPos( m_OldCursorPos.x, m_OldCursorPos.y );

	} // End if Captured
}

//-----------------------------------------------------------------------------
// Name : AnimateObjects () (Private)
// Desc : Animates the objects we currently have loaded.
//-----------------------------------------------------------------------------
void CGameApp::AnimateObjects()
{
	updateGameState();
	switch (m_gameState)
	{
	case GameState::ONGOING:
		if (!m_pPlayer->isDead)
		{
			m_pPlayer->Update(m_Timer.GetTimeElapsed());
			m_pPlayer->frameCounter()++;
		}
		
		if (!m_pPlayer2->isDead)
		{
			m_pPlayer2->Update(m_Timer.GetTimeElapsed());
			m_pPlayer2->frameCounter()++;
		}
		
		for (auto bul : bullets)
		{
			bul->update(m_Timer.GetTimeElapsed());

			if (detectBulletCollision(bul) || bul->mPosition.y >= m_screenSize.y || bul->mPosition.y <= 0)
			{
				bullets.remove(bul);
				break;
			}
		}

		EnemyMove();
		
		for (auto enem : m_enemies) {
			enem->Update(m_Timer.GetTimeElapsed());
		}

		enemyFire();
		break;
	
	case GameState::WON:
		m_scoreP1->move(Vec2(m_screenSize.x / 2 - 150, m_screenSize.y / 2 + 200));
		m_scoreP2->move(Vec2(m_screenSize.x / 2 + 150, m_screenSize.y / 2 + 200));
		break;
	
	case GameState::LOST:
		m_scoreP1->move(Vec2(m_screenSize.x / 2 - 150, m_screenSize.y / 2 + 200));
		m_scoreP2->move(Vec2(m_screenSize.x / 2 + 150, m_screenSize.y / 2 + 200));
		break;
	}
}

//-----------------------------------------------------------------------------
// Name : DrawObjects () (Private)
// Desc : Draws the game objects
//-----------------------------------------------------------------------------
void CGameApp::DrawObjects()
{
	m_pBBuffer->reset();

	m_imgBackground.Paint(m_pBBuffer->getDC(), 0, 0);

	switch (m_gameState)
	{
	case GameState::START:
		break;
	case GameState::ONGOING:
		m_scoreP1->draw();
		m_scoreP2->draw();

		if (!m_pPlayer->isDead) m_pPlayer->Draw();

		if (!m_pPlayer2->isDead) m_pPlayer2->Draw();

		for (auto lg : m_livesGreen) lg->draw();

		for (auto lr : m_livesRed) lr->draw();

		for (auto bul : bullets)
		{
			bul->draw();
		}

		for (auto enem : m_enemies)
		{
			enem->Draw();
			enem->frameCounter()++;
		}
		break;
	case GameState::LOST:
		m_scoreP1->draw();
		m_scoreP2->draw();

		m_lostSprite->draw();
		break;
	case GameState::WON:
		m_scoreP1->draw();
		m_scoreP2->draw();

		m_wonSprite->draw();
		break;
	default:
		break;
	}
	
	m_pBBuffer->present();
}

void CGameApp::addEnemies(int noEnemies)
{
	Vec2 position = Vec2(300, 50);
	srand(time(NULL));
	int ok = 1;

	for (int it = 0; it != noEnemies; ++it) {
		if (ok > 3) ok = 1;

		switch (ok)
		{
		case 1:
			m_enemies.push_back(new CPlayer(m_pBBuffer, "data/alien1p1.bmp"));
			break;
		case 2:
			m_enemies.push_back(new CPlayer(m_pBBuffer, "data/alien2p1.bmp"));
			break;
		case 3:
			m_enemies.push_back(new CPlayer(m_pBBuffer, "data/alien3p1.bmp"));
			break;
		}
		
		m_enemies.back()->Position() = position;
		m_enemies.back()->Velocity() = Vec2(0, 0);
		m_enemies.back()->frameCounter() = rand() % 1000;

		position.x += 100;
		if (position.x > m_screenSize.x - 300) {
			ok++;
			position.x = 300;
			position.y += 100;
		}
	}
}


void CGameApp::removeDead()
{

	if (!m_pPlayer->getLives() && !m_pPlayer->hasExploded())
	{
		fTimer = SetTimer(m_hWnd, 1, 70, NULL);
		m_pPlayer->Explode();
	}

	if (!m_pPlayer2->getLives() && !m_pPlayer2->hasExploded())
	{
		fTimer = SetTimer(m_hWnd, 1, 70, NULL);
		m_pPlayer2->Explode();
	}

	for (auto enem : m_enemies)
	{
		if (enem->isDead)
		{
			m_enemies.remove(enem);
			break;
		}
	}
}

bool CGameApp::Collision(CPlayer* p1, CPlayer* p2)
{
	RECT r;
	r.left = p1->Position().x - p1->getSize().x / 2;
	r.right = p1->Position().x + p1->getSize().x / 2;
	r.top = p1->Position().y - p1->getSize().y / 2;
	r.bottom = p1->Position().y + p1->getSize().y / 2;

	RECT r2;
	r2.left = p2->Position().x - p2->getSize().x / 2;
	r2.right = p2->Position().x + p2->getSize().x / 2;
	r2.top = p2->Position().y - p2->getSize().y / 2;
	r2.bottom = p2->Position().y + p2->getSize().y / 2;


	if (r.right > r2.left && r.left < r2.right && r.bottom>r2.top && r.top < r2.bottom) {
		return true;
	}

	return false;
}

bool CGameApp::detectBulletCollision(const Sprite* bullet)
{

	if (bulletCollision(*bullet, *m_pPlayer) && bullet->team == 3 && !m_pPlayer->hasExploded() && m_livesGreen.size() > 0)
	{
		m_pPlayer->takeDamage();
		
		delete m_livesGreen.back();
		m_livesGreen.pop_back();
		
		return true;
	}

	if (bulletCollision(*bullet, *m_pPlayer2) && bullet->team == 3 && !m_pPlayer2->hasExploded() && m_livesRed.size() > 0)
	{
		m_pPlayer2->takeDamage();
		
		delete m_livesRed.back();
		m_livesRed.pop_back();
		
		return true;
	}

	for (auto enem : m_enemies)
	{
		if (bulletCollision(*bullet, *enem) && bullet->team != 3)
		{
			if (enem->enemyHasExploded())
			{
				return false;
			}
			
			if (bullet->team == 1)
			{
				m_scoreP1->updateScore(100);
			}
			else if (bullet->team == 2)
			{
				m_scoreP2->updateScore(100);
			}
			else
			{
				return false;
			}
			fTimer = SetTimer(m_hWnd, 1, 70, NULL);
			enem->EnemyExplode();
			return true;
		}
	}

	return false;
}

bool CGameApp::bulletCollision(const Sprite& bullet, CPlayer& p1)
{
	if (bullet.mPosition.x >= p1.Position().x - (p1.getSize().x / 2))
		if (bullet.mPosition.x <= p1.Position().x + (p1.getSize().x / 2))
			if (bullet.mPosition.y >= p1.Position().y - (p1.getSize().y / 2))
				if (bullet.mPosition.y <= p1.Position().y + (p1.getSize().y / 2))
					return true;

	return false;

}

void CGameApp::fireBullet(const Vec2 position, const Vec2 velocity, int x)
{
	if (x == 1)
	{
		bullets.push_back(new Sprite("data/bullet.bmp", RGB(0xff, 0x00, 0xff)));
	}
	else if (x == 2)
	{
		bullets.push_back(new Sprite("data/bullet.bmp", RGB(0xff, 0x00, 0xff)));
	}
	else
	{
		bullets.push_back(new Sprite("data/bulletaliens.bmp", RGB(0xff, 0x00, 0xff)));
	}

	bullets.back()->setBackBuffer(m_pBBuffer);
	bullets.back()->mPosition = position;
	bullets.back()->mVelocity = velocity;
	bullets.back()->team = x;

	if (velocity.y < 0)
	{
		bullets.back()->mPosition.y -= 75;
	}
	else
	{
		bullets.back()->mPosition.y += 75;
	}
}

void CGameApp::enemyFire()
{
	srand(time(NULL));

	for (auto enem : m_enemies)
	{
		if (enem->frameCounter() == 1500)
		{
			enem->frameCounter() = rand() % 1000;
			fireBullet(enem->Position(), Vec2(0, 100), 3);
		}
	}
}

void CGameApp::EnemyMove()
{
	frameCounter++;

	if (frameCounter == 3200)
	{
		frameCounter = 0;
	}

	for (auto enem : m_enemies) {
		if (frameCounter <= 700) {
			enem->Velocity() = Vec2(-20, 0);
		}
		else if (frameCounter <= 900) {
			enem->Velocity() = Vec2(0, 20);
		}
		else if (frameCounter <= 2300) {
			enem->Velocity() = Vec2(20, 0);
		}
		else if (frameCounter <= 2500) {
			enem->Velocity() = Vec2(0, -20);
		}
		else if (frameCounter <= 3200) {
			enem->Velocity() = Vec2(-20, 0);
		}
	}
}

void CGameApp::setPLives(int livesP1, int livesP2)
{
	m_pPlayer->setLives(livesP1);
	m_pPlayer2->setLives(livesP2);

	Vec2 greenPos(30, 770);
	Vec2 redPos(m_screenSize.x - 50, 770.0);
	Vec2 increment(30, 0);

	for (int it = 0; it != livesP1; ++it) {
		m_livesGreen.push_back(new Sprite("data/heart.bmp", RGB(0xff, 0x00, 0xff)));
		auto& lastGreen = m_livesGreen.back();

		lastGreen->mPosition = greenPos;
		lastGreen->mVelocity = Vec2(0, 0);
		lastGreen->setBackBuffer(m_pBBuffer);

		greenPos += increment;
	}

	for (int it = 0; it != livesP2; ++it) {
		m_livesRed.push_back(new Sprite("data/heart.bmp", RGB(0xff, 0x00, 0xff)));
		auto& lastRed = m_livesRed.back();

		lastRed->mPosition = redPos;
		lastRed->mVelocity = Vec2(0, 0);
		lastRed->setBackBuffer(m_pBBuffer);

		redPos -= increment;
	}
}

void CGameApp::updateGameState()
{
	if (m_pPlayer->isDead && m_pPlayer2->isDead && m_gameState == ONGOING) {
		m_gameState = LOST;
	}
	else if (!m_enemies.size() && m_gameState == ONGOING) {
		m_gameState = WON;
	}
	else if (m_gameState == ONGOING) {
		m_gameState = ONGOING;
	}
}