#include <windows.h>
#include <gl/gl.h>
#include <math.h>
#include <iostream>
#include "ertugrul/subsystems/QuestManager.h"
#include "ertugrul/subsystems/CutsceneDirector.h"
#include "ertugrul/app/ObjModel.h"

#pragma comment(lib, "opengl32.lib")

ert::ObjModel treeModel;
ert::ObjModel heroModel;

// Kiritilgan 3D model bo'lmasa eski usulda chizish
void drawCube(float r, float g, float b, float sx, float sy, float sz) {
    glColor3f(r, g, b);
    glPushMatrix();
    glScalef(sx, sy, sz);
    glBegin(GL_QUADS);
    glVertex3f(-0.5f, 0.0f, 0.5f); glVertex3f(0.5f, 0.0f, 0.5f); glVertex3f(0.5f, 1.0f, 0.5f); glVertex3f(-0.5f, 1.0f, 0.5f);
    glVertex3f(-0.5f, 0.0f,-0.5f); glVertex3f(-0.5f, 1.0f,-0.5f); glVertex3f(0.5f, 1.0f,-0.5f); glVertex3f(0.5f, 0.0f,-0.5f);
    glVertex3f(-0.5f, 0.0f,-0.5f); glVertex3f(-0.5f, 0.0f, 0.5f); glVertex3f(-0.5f, 1.0f, 0.5f); glVertex3f(-0.5f, 1.0f,-0.5f);
    glVertex3f(0.5f, 0.0f,-0.5f); glVertex3f(0.5f, 1.0f,-0.5f); glVertex3f(0.5f, 1.0f, 0.5f); glVertex3f(0.5f, 0.0f, 0.5f);
    glVertex3f(-0.5f, 1.0f,-0.5f); glVertex3f(-0.5f, 1.0f, 0.5f); glVertex3f(0.5f, 1.0f, 0.5f); glVertex3f(0.5f, 1.0f,-0.5f);
    glEnd();
    glPopMatrix();
}

struct Vec3 { float x,y,z; };
Vec3 playerPos(0.0f, 0.5f, 0.0f);
float playerYaw = 0.0f;
float playerPitch = 0.0f;
float characterYaw = 0.0f;
bool keys[256] = {false};
POINT lastMousePos;
bool running = true;

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float camDist = 5.0f;
    float radYaw = playerYaw * 3.14159f / 180.0f;
    float radPitch = playerPitch * 3.14159f / 180.0f;

    glTranslatef(0, 0, -camDist);
    glRotatef(-playerPitch, 1, 0, 0);
    glRotatef(-playerYaw, 0, 1, 0);
    glTranslatef(-playerPos.x, -playerPos.y - 1.0f, -playerPos.z);

    // Checkered Floor (Katakkali Yer)
    glBegin(GL_QUADS);
    for(int i = -50; i < 50; i++) {
        for(int j = -50; j < 50; j++) {
            if((i + j) % 2 == 0) glColor3f(0.2f, 0.4f, 0.2f);
            else glColor3f(0.3f, 0.5f, 0.3f);
            
            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(i, 0.0f, j);
            glVertex3f(i, 0.0f, j + 1.0f);
            glVertex3f(i + 1.0f, 0.0f, j + 1.0f);
            glVertex3f(i + 1.0f, 0.0f, j);
        }
    }
    glEnd();

    // Daraxt (yoki Dushman)
    glPushMatrix();
    glTranslatef(10, 0, 10);
    glScalef(1.0f, 1.0f, 1.0f); // Default scale
    glColor3f(0.5f, 0.2f, 0.2f);
    treeModel.draw(); 
    glPopMatrix();

    // Qahramon (Ertugrul)
    glPushMatrix();
    glTranslatef(playerPos.x, playerPos.y, playerPos.z);
    glRotatef(characterYaw, 0, 1, 0);
    glScalef(1.0f, 1.0f, 1.0f); // Default scale
    glColor3f(0.8f, 0.6f, 0.2f); // Tilla/Jigarrang tus
    heroModel.draw(); 
    glPopMatrix();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_KEYDOWN) keys[wp] = true;
    else if (msg == WM_KEYUP) keys[wp] = false;
    else if (msg == WM_MOUSEMOVE) {
        int x = LOWORD(lp); int y = HIWORD(lp);
        playerYaw -= (x - lastMousePos.x) * 0.2f;
        playerPitch -= (y - lastMousePos.y) * 0.2f;
        lastMousePos.x = x; lastMousePos.y = y;
    }
    else if (msg == WM_SIZE) {
        int width = LOWORD(lp);
        int height = HIWORD(lp);
        if (height == 0) height = 1;
        glViewport(0, 0, width, height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float aspect = (float)width / (float)height;
        float fH = tanf(45.0f / 360.0f * 3.14159f) * 0.1f;
        glFrustum(-fH * aspect, fH * aspect, -fH, fH, 0.1f, 1000.0f);
        glMatrixMode(GL_MODELVIEW);
    }
    else if (msg == WM_CLOSE) running = false;
    else return DefWindowProc(hwnd, msg, wp, lp);
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    // === MANTIQIY QISM ===
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    std::cout << "KONSOL: Tizimlar ishga tushmoqda...\n";
    
    ert::QuestManager::get().loadEpisodes("data/episodes/episodes_v2.json");
    ert::QuestManager::get().printAllEpisodes();
    ert::QuestManager::get().startEpisode("EP001");
    ert::CutsceneDirector::get().playScene("e1_intro");

    // === GRAFIK QISM ===
    heroModel.load("assets/models/ottoman/ottoman.obj");
    treeModel.load("assets/models/crusader/crusader.obj");

    WNDCLASS wc = {0}; wc.lpfnWndProc = WndProc; wc.hInstance = hInst; wc.lpszClassName = "Game";
    RegisterClass(&wc);
    HWND hwnd = CreateWindow("Game", "Blades of Anatolia (C++ Version)", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 800, 600, 0, 0, hInst, 0);
    ShowCursor(FALSE); // Mishka kursorini yashirish
    HDC hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32, 0,0,0,0,0,0, 0,0,0,0,0,0,0, 24, 8, 0, PFD_MAIN_PLANE, 0, 0,0,0 };
    SetPixelFormat(hdc, ChoosePixelFormat(hdc, &pfd), &pfd);
    HGLRC hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);
    glEnable(GL_DEPTH_TEST);

    // ===================================
    // KENGAYTIRILGAN 3D RENDER (Lighting)
    // ===================================
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE); // Normal vektorni normalizatsiya qilish (RANG UCHUN)
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat light_pos[] = { 10.0f, 20.0f, 10.0f, 1.0f };
    GLfloat light_amb[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light_dif[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_dif);
    
    // Osmon rangi
    glClearColor(0.5f, 0.7f, 1.0f, 1.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float fH = tanf(45.0f / 360.0f * 3.14159f) * 0.1f;
    glFrustum(-fH * (800.0f/600.0f), fH * (800.0f/600.0f), -fH, fH, 0.1f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);

    // 2D Text uchun shrift tayyorlash
    GLuint fontBase = glGenLists(96);
    HFONT hFont = CreateFontA(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE|DEFAULT_PITCH, "Arial");
    SelectObject(hdc, hFont);
    wglUseFontBitmaps(hdc, 32, 96, fontBase);

    GetCursorPos(&lastMousePos); ScreenToClient(hwnd, &lastMousePos);
    DWORD lastTime = GetTickCount();

    auto drawText2D = [&](const std::string& text, float x, float y) {
        glDisable(GL_LIGHTING);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        // Ekranga qarab to'g'irlaymiz
        RECT rect; GetClientRect(hwnd, &rect);
        glOrtho(0, rect.right, rect.bottom, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glColor3f(1.0f, 1.0f, 0.0f); // Sariq matn
        glRasterPos2f(x, y);
        glPushAttrib(GL_LIST_BIT);
        glListBase(fontBase - 32);
        glCallLists(text.length(), GL_UNSIGNED_BYTE, text.c_str());
        glPopAttrib();
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_LIGHTING);
    };

    while (running) {
        MSG msg; while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }
        DWORD current = GetTickCount(); float dt = (current - lastTime) / 1000.0f; lastTime = current;

        float speed = 5.0f * dt;
        // playerYaw teskari ishora bilan olinishi kerak, shunda W kamera qaragan tomonga yuradi
        float camRad = -playerYaw * 3.14159f / 180.0f;
        Vec3 fwd{sinf(camRad), 0.0f, -cosf(camRad)};
        Vec3 right{cosf(camRad), 0.0f, sinf(camRad)};

        Vec3 moveDir{0.0f, 0.0f, 0.0f};
        bool moving = false;

        // Xatolik tuzatildi: endi W to'g'ri oldinga, S orqaga
        if (keys['W']) { moveDir.x -= fwd.x; moveDir.z -= fwd.z; moving = true; }
        if (keys['S']) { moveDir.x += fwd.x; moveDir.z += fwd.z; moving = true; }
        if (keys['A']) { moveDir.x -= right.x; moveDir.z -= right.z; moving = true; }
        if (keys['D']) { moveDir.x += right.x; moveDir.z += right.z; moving = true; }

        if (moving) {
            // Normalize moveDir
            float len = sqrtf(moveDir.x*moveDir.x + moveDir.z*moveDir.z);
            if (len > 0) { moveDir.x /= len; moveDir.z /= len; }
            
            playerPos.x += moveDir.x * speed;
            playerPos.z += moveDir.z * speed;
            
            // Qahramon yurish yo'nalishiga qarab burilishi (Assassin's Creed)
            float targetYaw = atan2f(moveDir.x, -moveDir.z) * 180.0f / 3.14159f;
            
            // Smooth rotation (silliq burilish)
            float diff = targetYaw - characterYaw;
            while (diff < -180.0f) diff += 360.0f;
            while (diff > 180.0f) diff -= 360.0f;
            characterYaw += diff * 10.0f * dt;
        }

        // Cutscene va Ovoz logikasi
        ert::CutsceneDirector::get().update(dt);
        std::string subtitle = ert::CutsceneDirector::get().getCurrentSubtitle();
        if (!subtitle.empty()) {
            // Ekranga (va Konsolga) subtitr chiqaramiz
            static std::string lastSub = "";
            if (subtitle != lastSub) {
                std::cout << "\n[OVOZ QUVVATI VA SUBTITR]: " << subtitle << "\n";
                // Ovoz o'chirildi (Beep)
                lastSub = subtitle;
            }
        }

        // Auto-play Episodes in Terminal logic
        static float storyTimer = 0.0f;
        static int currentEpIndex = 0;
        static int storyState = 0; // 0=Intro, 1=Cliffhanger, 2=Next
        
        storyTimer += dt;
        if (currentEpIndex < ert::QuestManager::get().getEpisodes().size()) {
            const auto& ep = ert::QuestManager::get().getEpisodes()[currentEpIndex];
            if (storyState == 0 && storyTimer > 3.0f) {
                std::cout << "\n\n==========================================\n";
                std::cout << "[" << ep.id << " - " << ep.title << " | Yil: " << ep.historicalYear << "]\n";
                std::cout << "-> Arxetip: " << ep.archetype << " | Qiyinlik: " << ep.difficultyTier << " Yulduz\n";
                std::cout << "-> Max Dushmanlar: " << ep.maxSimultaneousEnemies << " | Mix Turi: " << ep.mihBeatKind << "\n";
                std::cout << "[Ssenariy]: " << ep.introText << "\n";
                // Beep olib tashlandi
                storyState = 1;
                storyTimer = 0.0f;
            } else if (storyState == 1 && storyTimer > 3.0f) {
                std::cout << "[Natija]: " << ep.cliffhanger << "\n";
                // Beep olib tashlandi
                storyState = 2;
                storyTimer = 0.0f;
            } else if (storyState == 2 && storyTimer > 2.0f) {
                currentEpIndex++;
                storyState = 0;
                storyTimer = 0.0f;
            }
        }

        render();

        if (!subtitle.empty()) {
            drawText2D(subtitle, 50, 500);
        }
        if (currentEpIndex < ert::QuestManager::get().getEpisodes().size()) {
            const auto& ep = ert::QuestManager::get().getEpisodes()[currentEpIndex];
            if (storyState == 1) {
                drawText2D("EPIZOD: " + ep.id + " (" + ep.historicalYear + ")", 50, 50);
                drawText2D("SSENARIY: " + ep.introText, 50, 90);
            } else if (storyState == 2) {
                drawText2D("EPIZOD: " + ep.id + " (" + ep.historicalYear + ")", 50, 50);
                drawText2D("NATIJA: " + ep.cliffhanger, 50, 90);
            }
        }

        SwapBuffers(hdc);
    }
    return 0;
}
