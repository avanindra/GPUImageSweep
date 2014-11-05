/*
 * =====================================================================================
 *       Filename:  main_window_glut.cpp
 *    Description:  Static class containing callbacks and objects
 *        Created:  2014-11-03 18:17
 *         Author:  Tiago Lobato Gimenes        (tlgimenes@gmail.com)
 * =====================================================================================
 */

////////////////////////////////////////////////////////////////////////////////////////

#include "main_window_glut.hpp"

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

////////////////////////////////////////////////////////////////////////////////////////

WindowManager* MainWindowGlut::wmanager = NULL;
ImageAcquirer* MainWindowGlut::img = NULL;
CLGL* MainWindowGlut::clgl = NULL;

// Hide/Show information
bool MainWindowGlut::showInfo = true;

// FPS counter
float MainWindowGlut::fps = 0.0;

// String parameters
int MainWindowGlut::fontSize = 30;
float MainWindowGlut::stringColor[4] = {1.0,  1.0, 1.0, 1.0};
void* MainWindowGlut::font = GLUT_BITMAP_TIMES_ROMAN_24;

// Execute/Stop computation
int MainWindowGlut::play = ON;

// Mouse and orientation stuff
int MainWindowGlut::mouse_buttons = 0;
float  MainWindowGlut::translate_z = -1.f;
float  MainWindowGlut::rotate_x = 0.0;
float  MainWindowGlut::rotate_y = 0.0;
int MainWindowGlut::mouse_old_x = 0;
int MainWindowGlut::mouse_old_y = 0;
float MainWindowGlut::scale[3] = {1.7, 1.7, 0.3};

////////////////////////////////////////////////////////////////////////////////////////

typedef struct vbo_point_t
{
    vbo_point_t() : _x(0.0), _y(0.0), _z(0.0), _w(0.0) {}
    vbo_point_t(GLfloat x, GLfloat y, GLfloat z, GLfloat w) : _x(x), _y(y), _z(z), _w(w) {}
    void operator()(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
        _x = x; _y = y; _z = z; _w = w; 
    }
    GLfloat _x,_y,_z,_w;
} vbo_point;

typedef struct vbo_index_t
{
    vbo_index_t() : _i1(0), _i2(0), _i3(0) {}
    vbo_index_t(GLuint i1, GLuint i2, GLuint i3) : _i1(i1), _i2(i2), _i3(i3) {}
    void operator()(GLuint i1, GLuint i2, GLuint i3) {
        _i1 = i1; _i2 = i2; _i3 = i3;
    }
    GLuint _i1, _i2, _i3;
} vbo_index;

////////////////////////////////////////////////////////////////////////////////////////

inline vbo_point* gen_image(int width, int height, int& final_size)
{
    final_size = width*height;
    vbo_point* image = new vbo_point[final_size];
    float max = std::max(width, height);

    for(int i=0; i < height; i++)
    {
        for(int j=0; j < width; j++)
        {
            image[i*width+j](GLfloat(j-width/2)/max,GLfloat(i-height/2)/max, 0.2f, 0.0f);
        }
    }

    return image;
}

////////////////////////////////////////////////////////////////////////////////////////

#define INDEX(i,j) (i) * width + (j)

inline vbo_index* gen_index_buff(int width, int height, int& final_size)
{
    final_size = (width-1) * (height-1) * 2;
    std::vector<vbo_index>* index_buff = new std::vector<vbo_index>();
    int shift = (height - 2) * width + width - 1;

    for(int i=0; i < height-1; i++)
    {
        for(int j=0; j < width-1; j++)
        {
            index_buff->push_back(vbo_index(INDEX(i,j), INDEX(i,j+1), INDEX(i+1,j)));
            index_buff->push_back(vbo_index(INDEX(i+1,j+1), INDEX(i+1,j), INDEX(i,j+1)));
        }
    }

    return index_buff->data();
}

////////////////////////////////////////////////////////////////////////////////////////

void MainWindowGlut::start(ImageAcquirer& img, CLGL& clgl, WindowManager& wmanager)
{
    /* Sets objects */
    MainWindowGlut::img = &img;
    MainWindowGlut::clgl= &clgl;
    MainWindowGlut::wmanager = &wmanager;

    /* Connect callbacks */
    // Idle function callback
    glutDisplayFunc(MainWindowGlut::glutDisplayFunc_cb);
    glutTimerFunc(30, MainWindowGlut::glutTimerFunc_cb, 30);
    glutIdleFunc(MainWindowGlut::glutIdleFunc_cb);
    glutKeyboardFunc(MainWindowGlut::glutKeyboardFunc_cb);
    glutSpecialFunc(MainWindowGlut::glutSpecialKeys_cb);
    glutMouseFunc(MainWindowGlut::glutMouse_cb);
    glutMotionFunc(MainWindowGlut::glutMotion_cb);

    glShadeModel(GL_SMOOTH);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glDisable(GL_DEPTH_TEST);

    // viewport
    glViewport(0, 0, MainWindowGlut::img->width(), MainWindowGlut::img->height());

    // projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90.0, (GLfloat)MainWindowGlut::img->width() / 
            (GLfloat)MainWindowGlut::img->height(), 0.01, 1000.0);

    // set view matrix
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, MainWindowGlut::translate_z);
    glScalef(MainWindowGlut::scale[0], MainWindowGlut::scale[1], MainWindowGlut::scale[2]);

    //cv::Mat frame0 = MainWindowGlut::img->img1();

    int final_size = 0;

    // Loads image vbos
    /* vbo_point* image = gen_image(MainWindowGlut::img->width(), MainWindowGlut::img->height(), 
            final_size);
    MainWindowGlut::clgl->clgl_load_vbo_data_to_device<GL_ARRAY_BUFFER>(
            sizeof(vbo_point)*final_size, (const void*)image, CL_MEM_READ_WRITE);
    std::cout << "pushed " << final_size << " vertex elements to the device" << std::endl;
    MainWindowGlut::clgl->clgl_load_vbo_data_to_device<GL_ARRAY_BUFFER>(
            sizeof(vbo_point)*final_size, (const void*)image, CL_MEM_READ_WRITE);
    std::cout << "pushed " << final_size << " color elements to the device" << std::endl;

    vbo_index* index = gen_index_buff(MainWindowGlut::img->width(), MainWindowGlut::img->height(), 
            final_size);
    MainWindowGlut::clgl->clgl_load_vbo_data_to_device<GL_ELEMENT_ARRAY_BUFFER>(
            sizeof(vbo_index)*final_size, (const void*)index, CL_MEM_READ_WRITE);
    std::cout << "pushed " << final_size << " index elements to the device" << std::endl;*/
    vbo_point image[] = {
        vbo_point(0,0,0,1),
        vbo_point(0,0,1,1),
        vbo_point(0,1,0,1),
        vbo_point(1,0,0,1),
     //   vbo_point(0,1,1,1),
     //   vbo_point(1,0,1,1),
     //   vbo_point(1,1,0,1),
     //   vbo_point(1,1,1,1)
    };
    vbo_point color[] = {
        vbo_point(1,1,1,1),
        vbo_point(1,1,1,1),
        vbo_point(1,1,1,1),
        vbo_point(1,1,1,1),
    };
    vbo_index index[] = {
        vbo_index(0,1,2),
        vbo_index(1,2,3),
        vbo_index(3,2,0),
        vbo_index(3,1,0)
    };

    MainWindowGlut::clgl->clgl_load_vbo_data_to_device<GL_ARRAY_BUFFER>(
            sizeof(image), (const void*)image, CL_MEM_READ_WRITE);
    std::cout << "pushed " << sizeof(image)/sizeof(vbo_point) << " vertex elements to the device" << std::endl;
    MainWindowGlut::clgl->clgl_load_vbo_data_to_device<GL_ARRAY_BUFFER>(
            sizeof(color), (const void*)color, CL_MEM_READ_WRITE);
    std::cout << "pushed " << sizeof(image)/sizeof(vbo_point) << " color elements to the device" << std::endl;

    MainWindowGlut::clgl->clgl_load_vbo_data_to_device<GL_ELEMENT_ARRAY_BUFFER>(
            sizeof(index), (const void*)index, CL_MEM_READ_WRITE);
    std::cout << "pushed " << sizeof(index)/sizeof(vbo_index) << " index elements to the device" << std::endl;

    std::cout << "width: " << MainWindowGlut::img->width() << std::endl;
    std::cout << "height " << MainWindowGlut::img->height() << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////

/* C GLUT callbacks */

////////////////////////////////////////////////////////////////////////////////////////

void MainWindowGlut::glutIdleFunc_cb()
{
    MainWindowGlut::calculateFPS();

     //   cv::imshow("frame", MainWindowGlut::img->img1());

     //   cv::waitKey(30);
}

////////////////////////////////////////////////////////////////////////////////////////

void MainWindowGlut::glutDisplayFunc_cb()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

    // -------------------------- //
    // Draw information on screen //
    // -------------------------- //
    // Draw info on screen as FPS etc
    MainWindowGlut::drawInfo();

    // ------------------------------ //
    // Render the particles from VBOs //
    // ------------------------------ //
    //render the particles from VBOs
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(1.0);

    // Enable Cient State
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_INDEX_ARRAY);
    clgl_assert(glGetError());

    //Need to disable these for blender
    glDisableClientState(GL_NORMAL_ARRAY);

    for(int j=0; j < MainWindowGlut::clgl->platforms()->size(); j++)
    {
        for(int i=0; i < MainWindowGlut::clgl->vbos(j)->size()-2; i+=3)
        {

            // Binds the vertex VBO's
            glBindBuffer(GL_ARRAY_BUFFER, MainWindowGlut::clgl->vbos(j)->at(i));
            glVertexPointer(3, GL_FLOAT, sizeof(vbo_point), 0);
            clgl_assert(glGetError());

            //VBO Color must be inserted in last place
            glBindBuffer(GL_ARRAY_BUFFER, MainWindowGlut::clgl->vbos(j)->at(i+1));
            glColorPointer(3, GL_FLOAT, sizeof(vbo_point), 0);
            clgl_assert(glGetError());

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MainWindowGlut::clgl->vbos(j)->at(i+2));
            glIndexPointer(GL_INT, sizeof(vbo_index), 0);
            clgl_assert(glGetError());

            // Draw the simulation points
            glDrawArrays(GL_POINTS, 0, 
                    MainWindowGlut::clgl->clgl_get_vbo_bytes_size(
                        MainWindowGlut::clgl->vbos(j)->at(i), j)/sizeof(vbo_point));
            clgl_assert(glGetError());

            // Draw the triangles !
            glDrawElements (
                    GL_TRIANGLES,      // mode
                    MainWindowGlut::clgl->clgl_get_vbo_bytes_size(
                        MainWindowGlut::clgl->vbos(j)->at(i+2), j)/sizeof(vbo_index),    // count
                    GL_UNSIGNED_INT,   // type
                    0           // element array buffer
                    );
            clgl_assert(glGetError());
        }
    }

    glFlush();

    // Disable Client State
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_INDEX_ARRAY);
    clgl_assert(glGetError());

    // Bind the buffers to zero
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    clgl_assert(glGetError());

    glPopMatrix();

    // Swap the buffers
    glutSwapBuffers();
}

////////////////////////////////////////////////////////////////////////////////////////

/**
 * Draw info strings on screen, like FPS
 * */
void MainWindowGlut::drawInfo(void)
{
    if(MainWindowGlut::showInfo == true){
        std::stringstream fps(std::stringstream::in | std::stringstream::out);
        std::stringstream simTime(std::stringstream::in | std::stringstream::out);
        static std::stringstream pause(std::stringstream::in | std::stringstream::out);
        static std::stringstream kernel(std::stringstream::in | std::stringstream::out);

        pause.seekp(std::ios::beg);
        kernel.seekp(std::ios::beg);

        // Draw FPS
        fps << "FPS: " << MainWindowGlut::fps;
        drawString(fps.str().c_str(), 1, MainWindowGlut::img->height()-MainWindowGlut::fontSize, MainWindowGlut::stringColor, MainWindowGlut::font);

        // Draw How many particles are beeing simulated and if it is paused
        if(MainWindowGlut::play == ON){
            pause << "Simulating ";
            drawString(pause.str().c_str(), 1, 33, MainWindowGlut::stringColor, MainWindowGlut::font);
        }
        else{
            drawString("Paused", 1, 33, MainWindowGlut::stringColor, MainWindowGlut::font);
        }

        // Draw Current Kernel in Use
        kernel << "Kernel: ?" << std::endl;
        drawString(kernel.str().c_str(), 1, 7, MainWindowGlut::stringColor, MainWindowGlut::font);
    }
}

////////////////////////////////////////////////////////////////////////////////////////

/**
 * Write 2d string using GLUT
 * */
void MainWindowGlut::drawString(const char *str, int x, int y, float color[4], void *font)
{
    //backup current model-view matrix
    glPushMatrix();                     // save current modelview matrix
    glLoadIdentity();                   // reset modelview matrix

    //set to 2D orthogonal projection
    glMatrixMode(GL_PROJECTION);     // switch to projection matrix
    glPushMatrix();                  // save current projection matrix
    glLoadIdentity();                // reset projection matrix
    gluOrtho2D(0, MainWindowGlut::img->width(), 0, MainWindowGlut::img->height());  // set to orthogonal projection

    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); // lighting and color mask

    glColor4fv(color);          // set text color

    glRasterPos2i(x, y);        // place text position

    // loop all characters in the string
    while(*str){
        glutBitmapCharacter(font, *str);
        ++str;
    }

    glPopAttrib();

    // restore projection matrix
    glPopMatrix();                   // restore to previous projection matrix

    // restore modelview matrix
    glMatrixMode(GL_MODELVIEW);      // switch to modelview matrix
    glPopMatrix();                   // restore to previous modelview matrix  
}

////////////////////////////////////////////////////////////////////////////////////////

/**
 * Calculates the frames per second
 * */
void MainWindowGlut::calculateFPS(void)
{
    static int frameCount = 0;
    static int previousTime = 0;
    int currentTime = 0;

    //  Increase frame count
    frameCount++;

    //  Get the number of milliseconds since glutInit called
    //  (or first call to glutGet(GLUT ELAPSED TIME)).
    currentTime = glutGet(GLUT_ELAPSED_TIME);

    //  Calculate time passed
    int timeInterval = currentTime - previousTime;

    if(timeInterval > 1000)
    {
        //  calculate the number of frames per second
        MainWindowGlut::fps = frameCount / (timeInterval / 1000.0f);

        //  Set time
        previousTime = currentTime;

        //  Reset frame count
        frameCount = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////

void MainWindowGlut::glutTimerFunc_cb(int ms)
{ 
    //this makes sure the appRender function is called every ms miliseconds
    glutTimerFunc(ms, glutTimerFunc_cb, ms);
    glutPostRedisplay();
}

////////////////////////////////////////////////////////////////////////////////////////

/*
 * Defines what each key does
 */
void MainWindowGlut::glutKeyboardFunc_cb(unsigned char key, int x, int y)
{
    //this way we can exit the program cleanly
    switch(key)
    {
        // ---------- //
        // QUIT CASES //
        // ---------- //
        case '\033': // escape quits
        case '\015': // Enter quits    
        case 'Q':    // Q quits
        case 'q':    // q (or escape) quits
            // Cleanup up and QUIT
            exit(EXIT_SUCCESS);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////

/*
 * Defines what each special key does
 */
void MainWindowGlut::glutSpecialKeys_cb(int key, int x, int y)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    switch(key)
    {
        case GLUT_KEY_LEFT:
            glRotatef(-3, 0.0, 1.0, 0.0);
            break;
        case GLUT_KEY_RIGHT:
            glRotatef(3, 0.0, 1.0, 0.0);
            break;
        case GLUT_KEY_UP:
            glRotatef(3, 1.0, 0.0, 0.0);
            break;
        case GLUT_KEY_DOWN:
            glRotatef(-3, 1.0, 0.0, 0.0);
            break;
        case GLUT_KEY_PAGE_UP:
            glTranslatef(0.0, 0.0, 0.3);
            break;
        case GLUT_KEY_PAGE_DOWN:
            glTranslatef(0.0, 0.0, -0.3);
            break;
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////////

/*
 * Define Mouse interaction
 */
void MainWindowGlut::glutMouse_cb(int button, int state, int x, int y)
{
    //handle mouse interaction for rotating/zooming the view
    if (state == GLUT_DOWN) {
        MainWindowGlut::mouse_buttons |= 1<<button;
    } else if (state == GLUT_UP) {
        MainWindowGlut::mouse_buttons = 0;
    }

    MainWindowGlut::mouse_old_x = x;
    MainWindowGlut::mouse_old_y = y;
}

////////////////////////////////////////////////////////////////////////////////////////

/*
 * Define motion of the coordinate system
 */
void MainWindowGlut::glutMotion_cb(int x, int y)
{
    //hanlde the mouse motion for zooming and rotating the view
    float dx, dy;
    dx = x - MainWindowGlut::mouse_old_x;
    dy = y - MainWindowGlut::mouse_old_y;

    if (MainWindowGlut::mouse_buttons & 1) {
        MainWindowGlut::rotate_x += dy * 0.2;
        MainWindowGlut::rotate_y += dx * 0.2;
    } else if (MainWindowGlut::mouse_buttons & 4) {
        MainWindowGlut::translate_z += dy * 0.1;
    }

    MainWindowGlut::mouse_old_x = x;
    MainWindowGlut::mouse_old_y = y;

    // set view matrix
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, MainWindowGlut::translate_z);
    glScalef(MainWindowGlut::scale[0], MainWindowGlut::scale[1], MainWindowGlut::scale[2]);
    glRotatef(MainWindowGlut::rotate_x, 1.0, 0.0, 0.0);
    glRotatef(MainWindowGlut::rotate_y, 0.0, 1.0, 0.0);
}

////////////////////////////////////////////////////////////////////////////////////////
