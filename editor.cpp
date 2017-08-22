
#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>
#include <GL/gl.h>
#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "txt.h"

#define APPNAME "editor"

struct sConfig
{
    float fontSize;
    float margin;
    float padding;
    int split;

} _config;

char szAppName[] = APPNAME; // The name of this application
char szTitle[]   = APPNAME; // The title bar text
const char *pWindowText;
HDC hdc;
HGLRC hrc;
int windowWidth, windowHeight;
int scrollx, scrolly;

void CenterWindow(HWND hWnd);

unsigned char ttf_buffer[1<<20];
unsigned char temp_bitmap[512*512];

stbtt_bakedchar mCharData[96]; // ASCII 32..126 is 95 glyphs
GLuint mTextureId;

static TxtBuffer txt;
static int cursor = 0;
static int cursorLength = 1;

void stbtt_initfont(void)
{
    // Load font.
    FILE* fp = fopen("c:/windows/fonts/consola.ttf", "rb");
    if (!fp) return;
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    unsigned char* ttfBuffer = (unsigned char*)malloc(size);
    if (!ttfBuffer)
    {
        fclose(fp);
        return;
    }

    fread(ttfBuffer, 1, size, fp);
    fclose(fp);
    fp = 0;

    unsigned char* bmap = (unsigned char*)malloc(1024*1024);
    if (!bmap)
    {
        free(ttfBuffer);
        return;
    }

    stbtt_BakeFontBitmap(ttfBuffer,0, _config.fontSize, bmap, 512, 512, 32, 96, mCharData);

    // can free ttf_buffer at this point
    glGenTextures(1, &mTextureId);
    glBindTexture(GL_TEXTURE_2D, mTextureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512, 512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    free(ttfBuffer);
    free(bmap);
}

void getBakedQuad(int pw, int ph, int char_index, float *xpos, float *ypos, stbtt_aligned_quad *q)
{
    stbtt_bakedchar *b = mCharData + char_index;
    int round_x = STBTT_ifloor(*xpos + b->xoff);
    int round_y = STBTT_ifloor(*ypos - b->yoff);

    q->x0 = (float)round_x;
    q->y0 = (float)round_y;
    q->x1 = (float)round_x + b->x1 - b->x0;
    q->y1 = (float)round_y - b->y1 + b->y0;

    q->s0 = b->x0 / (float)pw;
    q->t0 = b->y0 / (float)pw;
    q->s1 = b->x1 / (float)ph;
    q->t1 = b->y1 / (float)ph;

    *xpos += b->xadvance;
}

void stbtt_print(float x, float y, const char *text)
{
    float curx = 0.0f;
    float cury = 0.0f;
    bool found = false;
    long cur = 0;
    float initialX = x;

    // assume orthographic projection with units = screen pixels, origin at top left
    glBindTexture(GL_TEXTURE_2D, mTextureId);

    glPushMatrix();
    glBegin(GL_TRIANGLES);

    while (text[cur])
    {
        glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

        if (cur == cursor)
        {
            found = true;
            curx = x;
            cury = y;
        }

        int c = (unsigned char)text[cur];
        if (c == '\n')
        {
            x = initialX;
            y -= _config.fontSize;
        }
        else if (c >= 32 && c < 128)
        {
            stbtt_aligned_quad q;
            getBakedQuad(512, 512, c-32, &x, &y, &q);

            glTexCoord2f(q.s0, q.t0);
            glVertex2f(q.x0, q.y0);
            glTexCoord2f(q.s1, q.t1);
            glVertex2f(q.x1, q.y1);
            glTexCoord2f(q.s1, q.t0);
            glVertex2f(q.x1, q.y0);

            glTexCoord2f(q.s0, q.t0);
            glVertex2f(q.x0, q.y0);
            glTexCoord2f(q.s0, q.t1);
            glVertex2f(q.x0, q.y1);
            glTexCoord2f(q.s1, q.t1);
            glVertex2f(q.x1, q.y1);
        }

        ++cur;
    }

    if (!found)
    {
        curx = x;
        cury = y;
    }

    glEnd();
    glPopMatrix();

    glBindTexture(GL_TEXTURE_2D, 0);

    glBegin(GL_TRIANGLES);

    glDisable(GL_TEXTURE_2D);
    glColor4f(0.0f, 0.0f, 0.0f, 0.3f);

    stbtt_aligned_quad q;
    getBakedQuad(512, 512, '|' - 32, &curx, &cury, &q);

    glVertex2f(q.x0 - 5.0f, q.y0);
    glVertex2f(q.x0 - 3.0f, q.y1);
    glVertex2f(q.x0 - 3.0f, q.y0);
    glVertex2f(q.x0 - 5.0f, q.y0);
    glVertex2f(q.x0 - 5.0f, q.y1);
    glVertex2f(q.x0 - 3.0f, q.y1);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glEnd();
}

void setupOrthoView()
{
    glViewport(0, 0, windowWidth, windowHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)windowWidth / (float)windowHeight;
    float size = windowWidth;
    glOrtho(0.0f, size, 0.0f, (size / aspect), -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void renderPanel(int x, int y, int w, int h, const float color[])
{
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    glColor3f(color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f);
    glVertex2f(x + _config.margin, y + _config.margin);
    glVertex2f(x + _config.margin, y + h - _config.margin);
    glVertex2f(x + w - _config.margin, y + _config.margin);
    glVertex2f(x + w - _config.margin, y + _config.margin);
    glVertex2f(x + w - _config.margin, y + h - _config.margin);
    glVertex2f(x + _config.margin, y + h - _config.margin);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glColor3f(236 / 255.0f, 236 / 255.0f, 236 / 255.0f);
    glVertex2f(x + _config.margin, y + _config.margin);
    glVertex2f(x + _config.margin, y + h - _config.margin);
    glVertex2f(x + w - _config.margin, y + h - _config.margin);
    glVertex2f(x + w - _config.margin, y + _config.margin);
    glEnd();
}

void renderTextArea(int left, int top, int right, int bottom)
{

}

char wParamToChar(WPARAM wParam, bool shift, bool capslock)
{
    if ((shift || capslock) && wParam >= 0x41 && wParam <= 0x5A)
    {
        return (char)wParam;
    }

    if (!(shift || capslock) && wParam >= 0x41 && wParam <= 0x5A)
    {
        return ((char)wParam) + ('a' - 'A');
    }

    if (wParam == VK_SPACE)
    {
        return (char)wParam;
    }

    if (wParam >= 0x30 && wParam <= 0x39)
    {
        return (char)wParam;
    }

    if (wParam == VK_RETURN)
    {
        return '\n';
    }

    return '\0';
}

const float white[] = { 255.0f, 255.0f, 255.0f };
const float grey[] = { 155.0f, 155.0f, 155.0f };
static bool splitter_grabbed = false;
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool ctrl = false;
    static bool shift = false;
    static bool alt = false;
    static bool capslock = false;

    switch (message)
    {
    case WM_CREATE:
    {
        CenterWindow(hwnd);
        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }

    case WM_DESTROY:
    {
        PostQuitMessage(0);
        break;
    }

    case WM_KEYUP:
    {
        if (VK_CONTROL == wParam)
        {
            ctrl = false;
        }
        else if (VK_MENU == wParam)
        {
            alt = false;
        }
        else if (VK_SHIFT == wParam)
        {
            shift = false;
        }
        break;
    }

    case WM_KEYDOWN:
    {
        if (VK_ESCAPE == wParam)
        {
            DestroyWindow(hwnd);
        }
        else if (VK_CONTROL == wParam)
        {
            ctrl = true;
        }
        else if (VK_MENU == wParam)
        {
            alt = true;
        }
        else if (VK_SHIFT == wParam)
        {
            shift = true;
        }
        else if (VK_CAPITAL == wParam)
        {
            capslock = !capslock;
        }
        else if (VK_LEFT == wParam)
        {
            if (cursor > 0)
            {
                cursor--;
            }
        }
        else if (VK_RIGHT == wParam)
        {
            if (cursor < txt.bufferSize())
            {
                cursor++;
            }
        }
        else if (VK_BACK == wParam)
        {
            if (cursor > 0)
            {
                txt.removeText(cursor, 1);
                cursor--;
            }
        }
        else if (VK_DELETE == wParam)
        {
            if (cursor >= 0 && cursor < txt.bufferSize())
            {
                txt.removeText(cursor + 1, 1);
            }
        }

        if (ctrl && !shift && 0x5A == wParam)
        {
            txt.undo();
        }
        else if (ctrl && shift && 0x5A == wParam)
        {
            txt.redo();
        }
        else
        {
            char buff[2] = { '\0' };
            buff[0] = wParamToChar(wParam, shift, capslock);
            if (buff[0] != '\0')
            {
                txt.addText(cursor, buff, 1);
                cursor++;
            }
        }
        InvalidateRect(hwnd, NULL, false);
        break;
    }

    case WM_MOUSEMOVE:
    {
        int xPos = GET_X_LPARAM(lParam);
        if (splitter_grabbed)
        {
            _config.split = xPos;
            InvalidateRect(hwnd, NULL, false);
        }
        else
        {
            if (xPos >= _config.split - 4 && xPos <= _config.split + 4)
                SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            else
                SetCursor(LoadCursor(NULL, IDC_HAND));
        }
        break;
    }
    case WM_LBUTTONDOWN:
    {
        int xPos = GET_X_LPARAM(lParam);
        splitter_grabbed = (xPos >= _config.split - 4 && xPos <= _config.split + 4);
        if (splitter_grabbed)
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
        break;
    }
    case WM_LBUTTONUP:
    {
        splitter_grabbed = false;
        SetCursor(LoadCursor(NULL, IDC_HAND));
        InvalidateRect(hwnd, NULL, false);
        break;
    }

    case WM_SIZE:
    {
        windowWidth = LOWORD(lParam);
        windowHeight = HIWORD(lParam);
        setupOrthoView();
        break;
    }

    case WM_MOUSEWHEEL:
    {
        short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        scrolly += (zDelta / WHEEL_DELTA) * _config.fontSize;
        if (scrolly > 0) scrolly = 0;
        InvalidateRect(hwnd, NULL, false);
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        hdc = BeginPaint(hwnd,&ps);
        wglMakeCurrent(hdc, hrc);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(239 / 255.0f, 239 / 255.0f, 241 / 255.0f, 0.0f);

        renderPanel(_config.split, 0, windowWidth, windowHeight, white);

        renderPanel(0, 0, _config.split, windowHeight, grey);

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor3f(89 / 255.0f, 96 / 255.0f, 105 / 255.0f);
        stbtt_print(_config.split + _config.margin + _config.padding + scrollx,
                    windowHeight - _config.fontSize - _config.margin - _config.padding - scrolly,
                    txt.buffer());

        SwapBuffers(hdc);
        wglMakeCurrent(hdc,0);
        EndPaint(hwnd, &ps);
        return 0;
    }

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

int main(int argc, char* argv[])
{
    _config.fontSize = 18.0f;
    _config.margin = 0.0f;
    _config.padding = 10.0f;
    _config.split = 200;
    scrollx = 0;
    scrolly = 0;

    MSG msg;
    WNDCLASS wc;
    HWND hwnd;
    PIXELFORMATDESCRIPTOR pfd;
    int format;

    pWindowText = "Hello Windows!";

    // Fill in window class structure with parameters that describe
    // the main window.

    ZeroMemory(&wc, sizeof wc);
    wc.hInstance     = NULL;
    wc.lpszClassName = szAppName;
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.style         = CS_DBLCLKS|CS_VREDRAW|CS_HREDRAW;
    wc.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
    wc.hIcon         = LoadIcon(NULL, MAKEINTRESOURCE(32518));
    wc.hCursor       = LoadCursor(NULL, IDC_HAND);

    if (FALSE == RegisterClass(&wc))
        return 0;

    // create the browser
    hwnd = CreateWindow(
                szAppName,
                szTitle,
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                NULL,
                0);

    if (NULL == hwnd)
        return 0;

    hdc= GetWindowDC(hwnd);

    if (NULL == hdc)
        return 0;

    ZeroMemory( &pfd, sizeof( pfd ) );
    pfd.nSize = sizeof( pfd );
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    format = ChoosePixelFormat(hdc, &pfd);

    if (format == 0)
        return 0;

    if (FALSE == SetPixelFormat(hdc, format, &pfd))
        return 0;

    hrc = wglCreateContext(hdc);

    if (NULL == hrc)
        return 0;

    wglMakeCurrent(hdc, hrc);

    stbtt_initfont();

    setupOrthoView();

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

void CenterWindow(HWND hwnd_self)
{
    HWND hwnd_parent;
    RECT rw_self, rc_parent, rw_parent;
    int xpos, ypos;

    hwnd_parent = GetParent(hwnd_self);
    if (NULL == hwnd_parent)
    {
        hwnd_parent = GetDesktopWindow();
    }

    GetWindowRect(hwnd_parent, &rw_parent);
    GetClientRect(hwnd_parent, &rc_parent);
    GetWindowRect(hwnd_self, &rw_self);

    xpos = rw_parent.left + (rc_parent.right + rw_self.left - rw_self.right) / 2;
    ypos = rw_parent.top + (rc_parent.bottom + rw_self.top - rw_self.bottom) / 2;

    windowWidth = rw_parent.right - rw_parent.left;
    windowHeight = rw_parent.bottom - rw_parent.top;

    SetWindowPos(
                hwnd_self, NULL,
                xpos, ypos, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
                );
}
