#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>
#include <ctime>

#pragma comment (lib,"Gdiplus.lib")

using namespace Gdiplus;

// Global Variables
HINSTANCE hInst;
HWND hWnd;
bool isRunning = true;

struct Bullet {
    float x, y;
    float speed = 50.0f;  // Increase bullet speed
};

struct Enemy {
    float x, y;
    float speed = 2.0f;  // New variable to move enemies downwards
    bool alive = true;
};

std::vector<Bullet> bullets;
std::vector<Enemy> enemies;

// Player Variables
float playerX = 400.0f;
float playerY = 500.0f;
const int playerSpeed = 50;  // Increase player movement speed

// Images
Image* spaceshipImage = nullptr;
Image* alienImage = nullptr;
Image* backgroundImage = nullptr;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void LoadAssets() {
    spaceshipImage = new Image(L"spaceship.png");
    alienImage = new Image(L"alien.png");
    backgroundImage = new Image(L"background.png");
}

void ReleaseAssets() {
    delete spaceshipImage;
    delete alienImage;
    delete backgroundImage;
}

void Update() {
    // Update bullets
    for (auto& bullet : bullets) {
        bullet.y -= bullet.speed;
    }

    // Update enemies and check for collisions with bullets
    for (auto& enemy : enemies) {
        if (enemy.alive) {
            enemy.y += enemy.speed;  // Move enemy downwards

            for (auto& bullet : bullets) {
                float dx = bullet.x - enemy.x;
                float dy = bullet.y - enemy.y;
                float distance = std::sqrt(dx * dx + dy * dy);

                if (distance < 20.0f) {  // Collision radius
                    enemy.alive = false;
                    bullet.y = -1000;  // Remove bullet by moving it off-screen
                }
            }
        }
    }

    // Remove off-screen bullets and inactive enemies
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) { return b.y < 0; }), bullets.end());
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& e) { return !e.alive || e.y > 600; }), enemies.end());
}

void Render(HDC hdc) {
    Graphics graphics(hdc);

    // Draw background
    if (backgroundImage) {
        graphics.DrawImage(backgroundImage, 0, 0, 800, 600);
    }

    // Draw player
    if (spaceshipImage) {
        graphics.DrawImage(spaceshipImage, static_cast<int>(playerX), static_cast<int>(playerY), 40, 40);
    }

    // Draw bullets
    SolidBrush bulletBrush(Color(255, 255, 0, 0));
    for (auto& bullet : bullets) {
        graphics.FillEllipse(&bulletBrush, static_cast<int>(bullet.x), static_cast<int>(bullet.y), 5, 10);
    }

    // Draw enemies
    if (alienImage) {
        for (auto& enemy : enemies) {
            if (enemy.alive) {
                graphics.DrawImage(alienImage, static_cast<int>(enemy.x), static_cast<int>(enemy.y), 30, 30);
            }
        }
    }
}

void SpawnEnemy() {
    enemies.push_back({ float(rand() % 800), 0 });  // Start enemies from the top (y = 0)
}

void ShootBullet() {
    bullets.push_back({ playerX + 15.0f, playerY });
}

void RenderDoubleBuffer(HDC hdc) {
    // Create a compatible memory device context
    HDC memDC = CreateCompatibleDC(hdc);

    // Create a compatible bitmap for double buffering
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, 800, 600);
    SelectObject(memDC, memBitmap);

    // Fill the background of the memory DC
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
    RECT rect = { 0, 0, 800, 600 };
    FillRect(memDC, &rect, hBrush);
    DeleteObject(hBrush);

    // Use GDI+ to render to the memory DC
    Graphics graphics(memDC);
    Render(memDC);

    // Copy the memory DC to the actual device context
    BitBlt(hdc, 0, 0, 800, 600, memDC, 0, 0, SRCCOPY);

    // Cleanup
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    hInst = hInstance;
    MSG msg;

    // Register Window Class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"MainWndClass";
    wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    RegisterClassEx(&wcex);

    // Create Window
    hWnd = CreateWindow(L"MainWndClass", L"2D Shooter Game", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // Load game assets
    LoadAssets();
    srand(static_cast<unsigned>(time(0)));

    // Set up timers
    SetTimer(hWnd, 1, 16, nullptr);  // Timer for the game loop (roughly 60 FPS)
    SetTimer(hWnd, 2, 800, nullptr);  // Timer to spawn enemies more frequently (800ms)

    // Game loop
    while (isRunning) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                isRunning = false;
            }
            else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    // Clean up GDI+ resources
    ReleaseAssets();
    GdiplusShutdown(gdiplusToken);
    KillTimer(hWnd, 1);
    KillTimer(hWnd, 2);

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_LEFT:
            playerX -= playerSpeed;
            break;
        case VK_RIGHT:
            playerX += playerSpeed;
            break;
        case VK_SPACE:
            ShootBullet();
            break;
        }
        break;

    case WM_TIMER:
        if (wParam == 1) {  // Game update timer
            Update();
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        if (wParam == 2) {  // Enemy spawn timer
            SpawnEnemy();
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RenderDoubleBuffer(hdc);  // Render using double buffering
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
