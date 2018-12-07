#include <string>
#include <iostream>

#include <windows.h>
#include <shellapi.h>
#include <gl/gl.h>
#include <gl/glext.h>

#include "SOIL.h"

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND, HDC, HGLRC);

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    WNDCLASSEX wcex;
    HWND hwnd;
    HDC hDC;
    HGLRC hRC;
    MSG msg;
    BOOL bQuit = FALSE;
    float theta = 0.0f;

    // register window class
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "GLSample";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);


    if (!RegisterClassEx(&wcex))
        return 0;

    // create main window
    hwnd = CreateWindowEx(0,
                          "GLSample",
                          "SOIL Sample",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          512,
                          512,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hwnd, nCmdShow);

    //	check my error handling
    /*
    SOIL_load_OGL_texture( "img_test.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0 );
    std::cout << "'" << SOIL_last_result() << "'" << std::endl;
    */


    // enable OpenGL for the window
    EnableOpenGL(hwnd, &hDC, &hRC);

    glEnable( GL_BLEND );
    //glDisable( GL_BLEND );
    //	straight alpha
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    //	premultiplied alpha (remember to do the same in glColor!!)
    //glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );

    //	do I want alpha thresholding?
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0.5f );

    //	log what the use is asking us to load
    std::string load_me = lpCmdLine;
    if( load_me.length() > 2 )
    {
		//load_me = load_me.substr( 1, load_me.length() - 2 );
		load_me = load_me.substr( 0, load_me.length() - 0 );
    } else
    {
    	//load_me = "img_test_uncompressed.dds";
    	//load_me = "img_test_indexed.tga";
    	//load_me = "img_test.dds";
    	load_me = "img_test.png";
    	//load_me = "odd_size.jpg";
    	//load_me = "img_cheryl.jpg";
    	//load_me = "oak_odd.png";
    	//load_me = "field_128_cube.dds";
    	//load_me = "field_128_cube_nomip.dds";
    	//load_me = "field_128_cube_uc.dds";
    	//load_me = "field_128_cube_uc_nomip.dds";
    	//load_me = "Goblin.dds";
    	//load_me = "parquet.dds";
    	//load_me = "stpeters_probe.hdr";
    	//load_me = "VeraMoBI_sdf.png";

    	//	for testing the texture rectangle code
    	//load_me = "test_rect.png";
    }
	std::cout << "'" << load_me << "'" << std::endl;

	//	1st try to load it as a single-image-cubemap
	//	(note, need DDS ordered faces: "EWUDNS")
	GLuint tex_ID;
    int time_me;

    std::cout << "Attempting to load as a cubemap" << std::endl;
    time_me = clock();
	tex_ID = SOIL_load_OGL_single_cubemap(
			load_me.c_str(),
			SOIL_DDS_CUBEMAP_FACE_ORDER,
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_POWER_OF_TWO
			| SOIL_FLAG_MIPMAPS
			//| SOIL_FLAG_COMPRESS_TO_DXT
			//| SOIL_FLAG_TEXTURE_REPEATS
			//| SOIL_FLAG_INVERT_Y
			| SOIL_FLAG_DDS_LOAD_DIRECT
			);
	time_me = clock() - time_me;
	std::cout << "the load time was " << 0.001f * time_me << " seconds (warning: low resolution timer)" << std::endl;
    if( tex_ID > 0 )
    {
    	glEnable( GL_TEXTURE_CUBE_MAP );
		glEnable( GL_TEXTURE_GEN_S );
		glEnable( GL_TEXTURE_GEN_T );
		glEnable( GL_TEXTURE_GEN_R );
		glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
		glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
		glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP );
		glBindTexture( GL_TEXTURE_CUBE_MAP, tex_ID );
		//	report
		std::cout << "the loaded single cube map ID was " << tex_ID << std::endl;
		//std::cout << "the load time was " << 0.001f * time_me << " seconds (warning: low resolution timer)" << std::endl;
    } else
    {
    	std::cout << "Attempting to load as a HDR texture" << std::endl;
		time_me = clock();
		tex_ID = SOIL_load_OGL_HDR_texture(
				load_me.c_str(),
				//SOIL_HDR_RGBE,
				//SOIL_HDR_RGBdivA,
				SOIL_HDR_RGBdivA2,
				0,
				SOIL_CREATE_NEW_ID,
				SOIL_FLAG_POWER_OF_TWO
				| SOIL_FLAG_MIPMAPS
				//| SOIL_FLAG_COMPRESS_TO_DXT
				);
		time_me = clock() - time_me;
		std::cout << "the load time was " << 0.001f * time_me << " seconds (warning: low resolution timer)" << std::endl;

		//	did I fail?
		if( tex_ID < 1 )
		{
			//	loading of the single-image-cubemap failed, try it as a simple texture
			std::cout << "Attempting to load as a simple 2D texture" << std::endl;
			//	load the texture, if specified
			time_me = clock();
			tex_ID = SOIL_load_OGL_texture(
					load_me.c_str(),
					SOIL_LOAD_AUTO,
					SOIL_CREATE_NEW_ID,
					SOIL_FLAG_POWER_OF_TWO
					| SOIL_FLAG_MIPMAPS
					//| SOIL_FLAG_MULTIPLY_ALPHA
					//| SOIL_FLAG_COMPRESS_TO_DXT
					| SOIL_FLAG_DDS_LOAD_DIRECT
					//| SOIL_FLAG_NTSC_SAFE_RGB
					//| SOIL_FLAG_CoCg_Y
					//| SOIL_FLAG_TEXTURE_RECTANGLE
					);
			time_me = clock() - time_me;
			std::cout << "the load time was " << 0.001f * time_me << " seconds (warning: low resolution timer)" << std::endl;
		}

		if( tex_ID > 0 )
		{
			//	enable texturing
			glEnable( GL_TEXTURE_2D );
			//glEnable( 0x84F5 );// enables texture rectangle
			//  bind an OpenGL texture ID
			glBindTexture( GL_TEXTURE_2D, tex_ID );
			//	report
			std::cout << "the loaded texture ID was " << tex_ID << std::endl;
			//std::cout << "the load time was " << 0.001f * time_me << " seconds (warning: low resolution timer)" << std::endl;
		} else
		{
			//	loading of the texture failed...why?
			glDisable( GL_TEXTURE_2D );
			std::cout << "Texture loading failed: '" << SOIL_last_result() << "'" << std::endl;
		}
    }

    // program main loop
    const float ref_mag = 0.1f;
    while (!bQuit)
    {
        // check for messages
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // handle or dispatch messages
            if (msg.message == WM_QUIT)
            {
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            // OpenGL animation code goes here
            theta = clock() * 0.1;

            float tex_u_max = 1.0f;//0.2f;
            float tex_v_max = 1.0f;//0.2f;

            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glPushMatrix();
            glScalef( 0.8f, 0.8f, 0.8f );
            //glRotatef(-0.314159f*theta, 0.0f, 0.0f, 1.0f);
			glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
			glNormal3f( 0.0f, 0.0f, 1.0f );
            glBegin(GL_QUADS);
				glNormal3f( -ref_mag, -ref_mag, 1.0f );
                glTexCoord2f( 0.0f, tex_v_max );
                glVertex3f( -1.0f, -1.0f, -0.1f );

                glNormal3f( ref_mag, -ref_mag, 1.0f );
                glTexCoord2f( tex_u_max, tex_v_max );
                glVertex3f( 1.0f, -1.0f, -0.1f );

                glNormal3f( ref_mag, ref_mag, 1.0f );
                glTexCoord2f( tex_u_max, 0.0f );
                glVertex3f( 1.0f, 1.0f, -0.1f );

                glNormal3f( -ref_mag, ref_mag, 1.0f );
                glTexCoord2f( 0.0f, 0.0f );
                glVertex3f( -1.0f, 1.0f, -0.1f );
            glEnd();
            glPopMatrix();

			tex_u_max = 1.0f;
            tex_v_max = 1.0f;
            glPushMatrix();
            glScalef( 0.8f, 0.8f, 0.8f );
            glRotatef(theta, 0.0f, 0.0f, 1.0f);
			glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
			glNormal3f( 0.0f, 0.0f, 1.0f );
            glBegin(GL_QUADS);
                glTexCoord2f( 0.0f, tex_v_max );		glVertex3f( 0.0f, 0.0f, 0.1f );
                glTexCoord2f( tex_u_max, tex_v_max );		glVertex3f( 1.0f, 0.0f, 0.1f );
                glTexCoord2f( tex_u_max, 0.0f );		glVertex3f( 1.0f, 1.0f, 0.1f );
                glTexCoord2f( 0.0f, 0.0f );		glVertex3f( 0.0f, 1.0f, 0.1f );
            glEnd();
            glPopMatrix();

            {
				/*	check for errors	*/
				GLenum err_code = glGetError();
				while( GL_NO_ERROR != err_code )
				{
					printf( "OpenGL Error @ %s: %i", "drawing loop", err_code );
					err_code = glGetError();
				}
			}

            SwapBuffers(hDC);

            Sleep (1);
        }
    }

    //	and show off the screenshot capability
    /*
    load_me += "-screenshot.tga";
    SOIL_save_screenshot( load_me.c_str(), SOIL_SAVE_TYPE_TGA, 0, 0, 512, 512 );
    //*/
    //*
    load_me += "-screenshot.bmp";
    SOIL_save_screenshot( load_me.c_str(), SOIL_SAVE_TYPE_BMP, 0, 0, 512, 512 );
    //*/
    /*
    load_me += "-screenshot.dds";
    SOIL_save_screenshot( load_me.c_str(), SOIL_SAVE_TYPE_DDS, 0, 0, 512, 512 );
    //*/

    // shutdown OpenGL
    DisableOpenGL(hwnd, hDC, hRC);

    // destroy the window explicitly
    DestroyWindow(hwnd);

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            PostQuitMessage(0);
        break;

        case WM_DESTROY:
            return 0;

        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                break;
            }
        }
        break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC)
{
    PIXELFORMATDESCRIPTOR pfd;

    int iFormat;

    /* get the device context (DC) */
    *hDC = GetDC(hwnd);

    /* set the pixel format for the DC */
    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW |
                  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iFormat = ChoosePixelFormat(*hDC, &pfd);

    SetPixelFormat(*hDC, iFormat, &pfd);

    /* create and enable the render context (RC) */
    *hRC = wglCreateContext(*hDC);

    wglMakeCurrent(*hDC, *hRC);
}

void DisableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}

