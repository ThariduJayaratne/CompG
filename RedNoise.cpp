#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>

using namespace std;
using namespace glm;

#define WIDTH 320
#define HEIGHT 240

void draw();
void greyscale();
void update();
void handleEvent(SDL_Event event);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

vector<float> interpolation(float from, float to, int n) {
  vector<float> list;
  float step = (to - from)/(n-1);
  for(int i = 0; i < n; i++) {
    list.push_back(from + i * step);
  }
  return list;
}

vector<vec3> interpolation3(vec3 from, vec3 to, int n) {
  vector<vec3> list;
  float stepx = (to.x - from.x)/(n-1);
  float stepy = (to.y - from.y)/(n-1);
  float stepz = (to.z - from.z)/(n-1);
  for(int i = 0; i < n; i++) {
    list.push_back(vec3(from.x + i * stepx,from.y + i * stepy,from.z + i * stepz));
  }
  return list;
}
void colorscale(){
  window.clearPixels();
  vec3 r = vec3(255,0,0);
  vec3 g = vec3(0,255,0);
  vec3 b = vec3(0,0,255);
  vec3 y = vec3(255,255,0);
  vector<vec3> redtoblue = interpolation3(r, b, window.width);
  vector<vec3> yellowtogreen = interpolation3(y, g, window.width);
  for(int i=0;i<window.width;i++){
    vector<vec3> step = interpolation3(redtoblue[i],yellowtogreen[i],window.height);
    for (int y = 0; y < window.height; y++) {
      uint32_t colour = (255<<24) + (int(step.at(y).x)<<16) + (int(step.at(y).y)<<8) + int(step.at(y).z);
      window.setPixelColour(i, y, colour);
    }
  }
}

int main(int argc, char* argv[])
{
  SDL_Event event;
  while(true)
  {
    // We MUST poll for events - otherwise the window will freeze !
    if(window.pollForInputEvents(&event)) handleEvent(event);
    // update();
    // draw();
    // greyscale();
    colorscale();
    // Need to render the frame at the end, or nothing actually gets shown on the screen !
    window.renderFrame();
  }
  vector<float> test = interpolation(2.2,8.5,7);
  vector<vec3> test2 = interpolation3(vec3( 1, 4, 9.2 ),vec3( 4, 1, 9.8 ),4);
  // for(int i=0; i<test.size(); ++i) {
  //   std::cout << test[i] << ' ';
  // }
  // for(int i=0; i<test2.size(); ++i) {
  //   std::cout << test2[i].x << ' ';
  //   std::cout << test2[i].y << ' ';
  //   std::cout << test2[i].z << ' ';
  // }
}

void draw()
{
  window.clearPixels();
  for(int y=0; y<window.height ;y++) {
    for(int x=0; x<window.width ;x++) {
      float red = rand() % 255;
      float green = 0.0;
      float blue = 0.0;
      uint32_t colour = (255<<24) + (int(red)<<16) + (int(green)<<8) + int(blue);
      window.setPixelColour(x, y, colour);
    }
  }
}

void greyscale() {
    window.clearPixels();
    vector<float> values = interpolation(0, 255, window.width);
    for (int x = 0; x < window.width; x++) {
      uint32_t colour = (255<<24) + (int(255 - values[x])<<16) + (int(255 - values[x])<<8) + int(255 - values[x]);
      for (int y = 0; y < window.height; y++) {
        window.setPixelColour(x, y, colour);
      }
    }
}

void update()
{
  // Function for performing animation (shifting artifacts or moving the camera)
}

void handleEvent(SDL_Event event)
{
  if(event.type == SDL_KEYDOWN) {
    if(event.key.keysym.sym == SDLK_LEFT) cout << "LEFT" << endl;
    else if(event.key.keysym.sym == SDLK_RIGHT) cout << "RIGHT" << endl;
    else if(event.key.keysym.sym == SDLK_UP) cout << "UP" << endl;
    else if(event.key.keysym.sym == SDLK_DOWN) cout << "DOWN" << endl;
  }
  else if(event.type == SDL_MOUSEBUTTONDOWN) cout << "MOUSE CLICKED" << endl;
}
