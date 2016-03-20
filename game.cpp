#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <sys/time.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

int score;

float xa = 1280.0f;
float ya = 720.0f;
// atrributes of the cannon ball
float vbx = 0;
float vby = 0;
float bx = xa/30;
float by = ya/30;
float br = xa/80;
int upb=0;

//zooming

float zoom = 1;
float pan = 0;
float mx=10.0f;
float my=ya/50;

int flag_inv = 0; //flag for invisible

int lives = 4;
float drag_x ;
bool drag_start = false;
bool mouse_drag = false;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
    glm::mat4 projection;
    glm::mat4 model;
    glm::mat4 view;
    GLuint MatrixID;
} Matrices;

GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open())
    {
        std::string Line = "";
        while(getline(VertexShaderStream, Line))
            VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
    }

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()){
        std::string Line = "";
        while(getline(FragmentShaderStream, Line))
            FragmentShaderCode += "\n" + Line;
        FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    printf("Compiling shader : %s\n", vertex_file_path);
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> VertexShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
    fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

    // Compile Fragment Shader
    printf("Compiling shader : %s\n", fragment_file_path);
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
    fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

    // Link the program
    fprintf(stdout, "Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
            0,                  // attribute 0. Vertices
            3,                  // size (x,y,z)
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
            );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
            1,                  // attribute 1. Color
            3,                  // size (r,g,b)
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
            );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
float circle_rot_dir = 1;
bool circle_rot_status = true;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;
// turret
bool turret_rot_status = false;
float turret_rot_dir = 1;
int tf = 0;
//block
bool block_rot_status = true;
float block_rot_dir = 1;

float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;
float circle_rotation = 0;
float turret_rotation = 0;
float block_rotation = 0;

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Function is called first on GLFW_PRESS.

    if (action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_UP:
                zoom -= 0.1;
                if( zoom <= 0.1 )
                    zoom = 0.1;
                break;
            case GLFW_KEY_DOWN:
                zoom += 0.1;
                if( zoom >= 1 )
                    zoom = 1;
                if( pan > (1.0f-zoom)*(float)xa)
                    pan = (1.0f-zoom)*(float)xa;
                break;
            case GLFW_KEY_LEFT:
                pan -= 1;
                if( pan < 0 )
                    pan = 0;
                break;
            case GLFW_KEY_RIGHT:
                pan += 1;
                if( pan > (1.0f-zoom)*(float)xa)
                    pan -= 1;
                break;
            case GLFW_KEY_C:
                rectangle_rot_status = !rectangle_rot_status;
                break;
            case GLFW_KEY_P:
                triangle_rot_status = !triangle_rot_status;
                break;
            case GLFW_KEY_A:
                turret_rot_status = true;
                turret_rot_dir = 1;
                break;
            case GLFW_KEY_B:
                turret_rot_status = true;
                turret_rot_dir = -1;
                break;
            case GLFW_KEY_F:
                if(mx<=100)
                    mx++;
                break;
            case GLFW_KEY_S:
                if(mx>=0)
                    mx--;
                break;
            default:
                break;
        }
    }
    else if (action == GLFW_RELEASE){

        switch(key){

            case GLFW_KEY_R:
                if(lives<=0)
                    break;
                lives--;
                bx=xa/30;
                by=ya/30;
                tf=0;
                vbx=0;
                vby=0;
                break;
            case GLFW_KEY_A:
                turret_rot_status = false;
                break;
            case GLFW_KEY_B:
                turret_rot_status = false;
                break;
            case GLFW_KEY_SPACE:
                if(tf==1)
                    break;
                tf = 1;
                vbx=15*cos(turret_rotation*M_PI/180.0f)*mx/100;
                vby=15*sin(turret_rotation*M_PI/180.0f)*mx/100;
                break;
            default:
                break;


        }

    }
    else if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            default:
                break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
    switch (key) {
        case 'Q':
        case 'q':
            quit(window);
            break;
        default:
            break;
    }
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_RELEASE){
                if(tf==1)
                    break;
                else{
                    tf = 1;
                    vbx=15*cos(turret_rotation*M_PI/180.0f)*mx/100;
                    vby=15*sin(turret_rotation*M_PI/180.0f)*mx/100;
                }
            }
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if( action == GLFW_PRESS )
            {
                mouse_drag = true;
                drag_start = true;
            }
            if (action == GLFW_RELEASE) 
            {
                drag_start = false;
                mouse_drag = false;
            }
            break;
        default:
            break;
    }
}

void cursorPos(GLFWwindow *window, double x_position,double y_position)
{
    float y_new = ya - y_position;

    if( drag_start )
        drag_x = x_position;

    if( mouse_drag )
    {
        drag_start = false;
        pan -= (float)(x_position - drag_x)/500.0f;
        if( pan < 0 )
            pan = 0;
        if( pan > (1.0f-zoom)*(float)xa)
            pan = (1.0f-zoom)*(float)xa;
    }
    mx = ((x_position-xa/30)*(x_position-xa/30) + (y_new-ya/30)*(y_new-ya/30))/4000;

    if(mx > 100)
        mx = 100;
    turret_rotation = atan((y_new-ya/30)/(x_position-xa/30))*180.0f/M_PI;

    if( x_position - 90 < 0 )
        turret_rotation += 180;

}

void mouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    if (yoffset > 0) 
    { 
        zoom -= 0.1;
        if( zoom <= 0.1 )
            zoom = 0.1;
    }
    else 
    {
        zoom += 0.1;
        if( zoom >= 1 )
            zoom = 1;
        if( pan > (1.0f-zoom)*(float)xa)
            pan = (1.0f-zoom)*(float)xa;
    }
}

/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
       is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

    //GLfloat fov = 90.0f;

    // sets the viewport of openGL renderer
    glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

    // set the projection matrix as perspective
    /* glMatrixMode (GL_PROJECTION);
       glLoadIdentity ();
       gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
    // Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(zoom*0.0f + pan, zoom*xa + pan, zoom*0.0f, zoom*ya, 0.1f, 500.0f);
}

VAO *triangle, *rectangle, *circle,*turret;

// making structs

typedef struct coin{

    VAO *co;
    int cx,cy,f,p;
    float r;
}coin;

typedef struct block{
    VAO *bl;
    int x,y,f,cx,cy,v;
    float a;
    float vx,vy;
}block;

coin a[6];
block b[15];

GLboolean blockcollision(int index) // AABB - Circle collision
{
    // Get center point circle first 
    glm::vec2 center(bx,by);
    // Calculate AABB info (center, half-extents)
    glm::vec2 aabb_half_extents(b[index].a / 2, b[index].a / 2);
    glm::vec2 aabb_center(
            b[index].x-b[index].a/2 + aabb_half_extents.x, 
            b[index].y-b[index].a/2 + aabb_half_extents.y
            );
    // Get difference vector between both centers
    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    // Add clamped value to AABB_center and we get the value of box closest to circle
    glm::vec2 closest = aabb_center + clamped;
    // Retrieve vector between center circle and closest point AABB and check if length <= radius
    difference = closest - center;
    b[index].cx = closest.x;
    b[index].cy = closest.y;
    return glm::length(difference) < br;
}      




// Creates the triangle object used in this sample code
void createTriangle ()
{
    /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

    /* Define vertex array as used in glBegin (GL_TRIANGLES) */
    GLfloat vertex_buffer_data [] = {
        0, 100,0, // vertex 0
        -100,-100,0, // vertex 1
        100,-100,0, // vertex 2
    };

    GLfloat color_buffer_data [] = {
        1,0,0, // color 0
        0,1,0, // color 1
        0,0,1, // color 2
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}

void createcircle(float r,float c, float para)
{
    GLfloat vertex_buffer_data[2000] = {};
    GLfloat color_buffer_data[2000] = {};
    int i,k=3;
    float x = c;
    vertex_buffer_data [0] = 0;
    color_buffer_data [0] = para;
    vertex_buffer_data [1] = 0;
    color_buffer_data [1] = para;
    vertex_buffer_data [2] = 0;
    color_buffer_data [2] = para;

    for ( i = 0; i < 361; ++i)
    {
        vertex_buffer_data [k] = r*cos(i*M_PI/180);
        color_buffer_data [k] = x;
        k++;
        vertex_buffer_data [k] = r*sin(i*M_PI/180);
        color_buffer_data [k] = 0;
        k++;
        vertex_buffer_data [k] = 0;
        color_buffer_data [k] = 0;
        k++;
    }
    circle = create3DObject(GL_TRIANGLE_FAN, 362, vertex_buffer_data, color_buffer_data, GL_FILL);

}

//creating coins
void createcoin(int index, float r, float x, float y, float z)
{
    GLfloat vertex_buffer_data[2000] = {};
    GLfloat color_buffer_data[2000] = {};
    int i,k=3;
    vertex_buffer_data [0] = 0;
    color_buffer_data [0] = x;
    vertex_buffer_data [1] = 0;
    color_buffer_data [1] = y;
    vertex_buffer_data [2] = 0;
    color_buffer_data [2] = z;

    for ( i = 0; i < 361; ++i)
    {
        vertex_buffer_data [k] = r*cos(i*M_PI/180);
        color_buffer_data [k] = x;
        k++;
        vertex_buffer_data [k] = r*sin(i*M_PI/180);
        color_buffer_data [k] = y;
        k++;
        vertex_buffer_data [k] = 0;

        color_buffer_data [k] = z;
        k++;
    }
    a[index].co = create3DObject(GL_TRIANGLE_FAN, 362, vertex_buffer_data, color_buffer_data, GL_FILL);

}

// Creates the block
void createblocks(int index, float x, float a,float bi,float c)
{
    // GL3 accepts only Triangles. Quads are not supported
    GLfloat vertex_buffer_data [] = {
        -x,-x,0, // vertex 1
        x,-x,0, // vertex 2
        x, x,0, // vertex 3

        x, x,0, // vertex 3
        -x, x,0, // vertex 4
        -x,-x,0  // vertex 1
    };

    GLfloat color_buffer_data [] = {
        a,bi,c, // color 1
        a,bi,c, // color 2
        a,bi,c, // color 3

        a,bi,c, // color 3
        a,bi,c, // color 4
        a,bi,c  // color 1
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    b[index].bl = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

// Creates the rectangle object used in this sample code
void createRectangle(float x,float y,float a,float b,float c)
{
    // GL3 accepts only Triangles. Quads are not supported
    GLfloat vertex_buffer_data [] = {
        -x,-y,0, // vertex 1
        x,-y,0, // vertex 2
        x, y,0, // vertex 3

        x, y,0, // vertex 3
        -x, y,0, // vertex 4
        -x,-y,0  // vertex 1
    };

    GLfloat color_buffer_data [] = {
        a,b,c, // color 1
        a,b,c, // color 2
        a,b,c, // color 3

        a,b,c, // color 3
        a,b,c, // color 4
        a,b,c  // color 1
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createturret(float x,float y,float a,float b,float c)
{
    // GL3 accepts only Triangles. Quads are not supported
    GLfloat vertex_buffer_data [] = {
        -x,-y,0, // vertex 1
        x,-y,0, // vertex 2
        x, y,0, // vertex 3

        x, y,0, // vertex 3
        -x, y,0, // vertex 4
        -x,-y,0  // vertex 1
    };

    GLfloat color_buffer_data [] = {
        a,b,c, // color 1
        a,b,c, // color 2
        a,b,c, // color 3

        a,b,c, // color 3
        a,b,c, // color 4
        a,b,c  // color 1
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    turret = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

// checking for collision

int checkCollision1(int x,int y,int r){
    if(x+r>=xa  || (x-r)<=0)
        return 1;
    else 
        return 0;
}
int checkCollision2(int x,int y,int r){
    if(y+r>=ya || (y-r)<=1)
    {
        if(y+r>=ya)
            upb=1;
        return 1;

    }
    else 
        return 0;
}



/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw (GLFWwindow *window)
{
    reshapeWindow (window, xa, ya);

    //increasing time

    // clear the color and depth in the frame buffer
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram (programID);

    // Eye - Location of camera. Don't change unless you are sure!!
    glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (0, 0, 0);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0, 1, 0);

    // Compute Camera matrix (view)
    // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
    //  Don't change unless you are sure!!
    Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    //  Don't change unless you are sure!!
    glm::mat4 VP = Matrices.projection * Matrices.view;

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // For each model you render, since the MVP will be different (at least the M part)
    //  Don't change unless you are sure!!
    glm::mat4 MVP;	// MVP = Projection * View * Model

    // Load identity to model matrix
    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */

    //drawing the sky
    createRectangle(xa/2,ya/2,0.0f/255,191.0f/255,255.0f/255);
    glm::mat4 translateRectangle = glm::translate (glm::vec3(xa/2, ya/2, 0.0f)); // glTranslatef
    //glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
    glm::mat4 RectangleTransform = translateRectangle;
    Matrices.model *= RectangleTransform; 
    MVP = VP * Matrices.model; // MVP = p * V * M
    //  Don't change unless you are sure!!
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(rectangle);

    //drawing the ground
    createRectangle(xa/2,ya/50,0,1,0);
    Matrices.model = glm::mat4(1.0f);
    translateRectangle = glm::translate (glm::vec3(xa/2,ya/50, 0.0f)); // glTranslatef
    //glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
    RectangleTransform = translateRectangle;
    Matrices.model *= RectangleTransform; 
    MVP = VP * Matrices.model; // MVP = p * V * M
    //  Don't change unless you are sure!!
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(rectangle);

    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();
    
    // revolving block
    if(blockcollision(14)){
        b[14].f=1;
        if((b[14].x-b[14].a/2)==b[14].cx || (b[14].x+b[14].a/2)==b[14].cx)
        {   
            vbx *=-0.5f;
        }
        if((b[14].y-b[14].a/2)==b[14].cy || (b[14].y+b[14].a/2)==b[14].cy)
        {
            vby *=-0.5f;
        }

    }

    Matrices.model = glm::mat4(1.0f);

    glm::mat4 translateblock = glm::translate (glm::vec3(b[14].x, b[14].y, 0));        // glTranslatef
    glm::mat4 rotateblock = glm::rotate((float)(block_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translateblock*rotateblock );
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(b[14].bl);


    //drawing a circle
    createcircle(xa/30,0,0);

    Matrices.model = glm::mat4(1.0f);

    glm::mat4 translatecircle = glm::translate (glm::vec3(xa/30,ya/30, 0.0f));        // glTranslatef
    glm::mat4 rotatecircle = glm::rotate((float)(circle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translatecircle * rotatecircle);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(circle);


    //drawing no of shots left

    int addr=1170.0f;

    for(int i=0;i<lives;i++){


        createcircle(xa/100,1,1);

        Matrices.model = glm::mat4(1.0f);

        glm::mat4 translatecircle = glm::translate (glm::vec3(addr,700.0f, 0.0f));        // glTranslatef
        glm::mat4 rotatecircle = glm::rotate((float)(circle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
        Matrices.model *= (translatecircle * rotatecircle);
        MVP = VP * Matrices.model;
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

        // draw3DObject draws the VAO given to it using current MVP matrix
        draw3DObject(circle);
        addr+=30;
    }

    //drawing a turret
    createturret(xa/20,ya/60,165.0f/255.0f,42.0f/255.0f,42.0f/255.0f);

    Matrices.model = glm::mat4(1.0f);

    glm::mat4 translateturret = glm::translate (glm::vec3(xa/30, ya/30, 0));        // glTranslatef
    glm::mat4 rotateturret = glm::rotate((float)(turret_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translateturret*rotateturret );
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(turret);

    //drawing a ball
    createcircle(br,0,1);

    if(checkCollision1(bx,by,br))
        vbx *=-1.0f;
    if(checkCollision2(bx,by,br)){
        vby *=-2.0f/3;//1.0f/1.2;
        if(!upb)
        by += 10;
        else{
            upb=0;
            by-=10;
        }

    }

    Matrices.model = glm::mat4(1.0f);
    bx += vbx;
    by += vby;
    vby -= tf*.1;
    vbx -= tf*.001;


    glm::mat4 translateball = glm::translate (glm::vec3(bx,by, 0.0f));        // glTranslatef
    //glm::mat4 rotateball = glm::rotate((float)(circle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translateball);// * rotatecircle);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(circle);

    //drawing a meter
    createRectangle(mx/2,my,1,0,0);

    Matrices.model = glm::mat4(1.0f);

    translateRectangle = glm::translate (glm::vec3(mx/2,ya-my, 0));        // glTranslatef
    //glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translateRectangle);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(rectangle);

    //drawing a coin

    cout<<score<<endl;

    for(int index=1;index<6;index++){
        createcoin(index,a[index].r,1,1,0);

        float dist = (bx-a[index].cx)*(bx-a[index].cx)+(by-a[index].cy)*(by-a[index].cy);
        float d2 = br+a[1].r;

        if(d2*d2>=dist && !a[index].f){
            a[index].f = 1;
           score+=a[index].p;
        } 

        if(!a[index].f){
            Matrices.model = glm::mat4(1.0f);

            glm::mat4 translatecoin = glm::translate (glm::vec3(a[index].cx,a[index].cy, 0.0f));        // glTranslatef
            Matrices.model *= (translatecoin);
            MVP = VP * Matrices.model;
            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

            // draw3DObject draws the VAO given to it using current MVP matrix
            draw3DObject(a[index].co);}
    }

    //drawing a block

    for(int index=1;index<14;index++){

        if(b[index].v == 1)
            continue;


        if(blockcollision(index)){
            b[index].f=1;
            if((b[index].x-b[index].a/2)==b[index].cx || (b[index].x+b[index].a/2)==b[index].cx)
            {   b[index].vx = vbx;
                vbx *=-1;
            }
            if((b[index].y-b[index].a/2)==b[index].cy || (b[index].y+b[index].a/2)==b[index].cy)
            {
                b[index].vy = vby;
                vby *=-1;
            }

        }
        if(checkCollision1(b[index].x,b[index].y,b[index].a/2))
            b[index].vx *=-2.0f/3;
        if(checkCollision2(b[index].x,b[index].y,b[index].a/2)){
            b[index].vy *=-2.0f/3;
            b[index].y += 10;
        }
        b[index].x += b[index].vx;
        b[index].y += b[index].vy;
        b[index].vy -= b[index].f*.1;

        if(b[index].f == 1 && abs(b[index].vx)<=.5 && abs(b[index].vy)<=.5)
            b[index].v=1;

        if(true){

            Matrices.model = glm::mat4(1.0f);

            glm::mat4 translateblock = glm::translate (glm::vec3(b[index].x, b[index].y, 0));        // glTranslatef
            //glm::mat4 rotateturret = glm::rotate((float)(turret_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
            Matrices.model *= (translateblock );
            MVP = VP * Matrices.model;
            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

            // draw3DObject draws the VAO given to it using current MVP matrix
            draw3DObject(b[index].bl);
        }
    }



    // Increment angles
    float increments = 1;

    //camera_rotation_angle++; // Simulating camera rotation
    triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
    rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
    circle_rotation = circle_rotation + increments*circle_rot_dir*circle_rot_status;
    turret_rotation = turret_rotation + increments*turret_rot_dir*turret_rot_status;
    block_rotation = block_rotation + increments*block_rot_dir*block_rot_status;

    if(score==40)
        score+=lives*10;
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
       is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    glfwSetCursorPosCallback(window, cursorPos);

    glfwSetScrollCallback(window,mouseScroll);
    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
    // Create the models
    createTriangle(); // Generate the VAO, VBOs, vertices data & copy into the array buffer
    //createRectangle(xa/8,ya/8,1,1,1);
    createcircle(xa/15,0,0);
    //create blocks
    for(int index = 1;index<14;index++){
        createblocks(index, b[index].a,102.0f/255.0f, 76.0f/255.0f, 0);}

       createblocks(14, b[14].a,1,20.0f/255,147.0f/255);
    // Create and compile our GLSL program from the shaders
    programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
    // Get a handle for our "MVP" uniform
    Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


    reshapeWindow (window, width, height);

    // Background color of the scene
    glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
    glClearDepth (1.0f);

    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{

    score = 0;
    //timeofday(time,NULL);

    //initializing coins

    a[1].cx = 840;
    a[1].cy = 400;
    a[1].r = 20;
    a[1].f = 0;
    a[1].p = 10;

    a[2].cx = 1040;
    a[2].cy = ya/25+40;
    a[2].r = 20;
    a[2].f = 0;
    a[2].p = 15;

    a[3].cx = 500;
    a[3].cy = 600;
    a[3].r = 20;
    a[3].f = 0;
    a[3].p = 5;

    a[4].cx = 300;
    a[4].cy = 400;
    a[4].r = 20;
    a[4].f = 0;
    a[4].p = 5;


    a[5].cx = 500;
    a[5].cy = 200;
    a[5].r = 20;
    a[5].f = 0;
    a[5].p = 5;
    //initializing blocks


    for(int i=1;i<15;i++)
    {
        b[i].f = 0;
        b[i].a = 10;
        b[i].vx = 0;
        b[i].vy = 0;
        b[i].v=0;
    }

    b[1].x = 200;
    b[1].y = 400;

    b[2].x = 800+40;
    b[2].y = 400+40;

    b[3].x = 800+40;
    b[3].y = 400-40;

    b[4].x = 800+80;
    b[4].y = 400;

    b[5].x = 800;
    b[5].y = 400;

    b[14].x = 550;
    b[14].y = 400;
    b[14].a = 50;

    int k=6;

    int addx,addy;
    addy = ya/25;

    int yes=0;

    for(int i=0;i<3;i++){
        addx=1000.0f;
        for(int j=0;j<3;j++){
            b[k].x = addx;
            b[k].y = addy;
            if(k==10 && !yes)
            {k--;
                yes=1;
            }
            k++;
            addx+=40;
        }
        addy+=40;
    }


    int width = xa;
    int height = ya;

    GLFWwindow* window = initGLFW(width, height);

    initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time;

    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

        // OpenGL Draw commands
        draw(window);

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            last_update_time = current_time;
        }
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
