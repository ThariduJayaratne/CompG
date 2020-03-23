#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <RayTriangleIntersection.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>

using namespace std;

using namespace glm;

#define WIDTH 640
#define HEIGHT 480
#define PI 3.14159265
#define SCALETING 0.5

void handleEvent(SDL_Event event);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
int mode = 1; //1 -> Wireframe    2 -> Rasteriser    3 -> Raytracer
mat3 camOrien = mat3();
vec3 camera = vec3(0,0,6);
float angle = 0.1f;
vec3 whitelight = vec3(-0.242005 , 2.00925, -0.683984) * (float)SCALETING;


vector<CanvasPoint> interpolation2(CanvasPoint from, CanvasPoint to, int n) {
  vector<CanvasPoint> list;
  float stepx = (to.x - from.x)/n;
  float stepy = (to.y - from.y)/n;
  double stepDepth = (to.depth - from.depth)/n;
  for(int i = 0; i <= n; i++) {
    CanvasPoint pt;
    pt.x = from.x + i * stepx;
    pt.y = from.y + i * stepy;
    pt.depth = from.depth + i * stepDepth;
    list.push_back(pt);
  }
  return list;
}

double **malloc2dArray(int dim1, int dim2) {
    double **array = (double **) malloc(dim1 * sizeof(double *));
    for (int i = 0; i < dim1; i++) {
        array[i] = (double *) malloc(dim2 * sizeof(double));
    }
    return array;
}

vector<vec3> readVertices(float scale, string filename){
  ifstream myfile;
  myfile.open(filename);
  std::string line;
  vector<vec3> vertices;
  while (getline(myfile, line)) {
    std::string* lineX = split(line,' ');
    if(line[0] == 'v'){
      float xc = stof(lineX[1])*scale;
      float yc = stof(lineX[2])*scale;
      float zc = stof(lineX[3])*scale;
      vertices.push_back(vec3(xc,yc,zc));
    }
  }
  //  for(u_int i=0; i<vertices.size(); i++) {
  //    std::cout << vertices[i].x << endl;
  //    std::cout << vertices[i].y << endl;
  //    std::cout << vertices[i].z << endl;
  // }
  return vertices;
}

vector<ModelTriangle> readtriangles(string filename){
  Colour c;
  c.red = 255;
  c.green = 0;
  c.blue = 0;
  fstream myfile;
  myfile.open(filename);
  std::string line;
  vector<ModelTriangle> triangles;
  vector<vec3> readvertices = readVertices(SCALETING, filename);
  while (getline(myfile, line)) {
    string *splitLine = split(line, ' ');
    if(splitLine[0] == "f"){
      int slashpos1 =  splitLine[1].find('/');
      int slashpos2 =  splitLine[2].find('/');
      int slashpos3 =  splitLine[3].find('/');
      string point1 = splitLine[1].substr(0,slashpos1);
      string point2 = splitLine[2].substr(0,slashpos2);
      string point3 = splitLine[3].substr(0,slashpos3);
      float p1 = stof(point1) - 1;
      float p2 = stof(point2) - 1;
      float p3 = stof(point3) - 1;
      triangles.push_back(ModelTriangle(readvertices[p1], readvertices[p2], readvertices[p3], c));
    }
  }
  return triangles;
}

vector<CanvasTriangle> modelToCanvas(vec3 camera, vector<ModelTriangle> triangles, float focalDistance, float canvasScale){

  vector<CanvasTriangle> canvastriangles;
  // vector<vec3> triangletextures = readTextureIndex("logo.obj");
  // vector<TexturePoint> texturePoints = readTexturePoints("logo.obj");

  for(u_int i=0;i<triangles.size();i++){

    // int p1 = triangletextures[i].x;
    // int p2 = triangletextures[i].y;
    // int p3 = triangletextures[i].z;

    CanvasPoint test1,test2,test3;
    CanvasPoint projP1,projP2,projP3;
    float cameraP1x,cameraP1y,cameraP1z,cameraP2x,cameraP2y,cameraP2z,cameraP3x,cameraP3y,cameraP3z;
    test1.x = triangles[i].vertices[0].x;
    test2.x = triangles[i].vertices[1].x;
    test3.x = triangles[i].vertices[2].x;
    test1.y = triangles[i].vertices[0].y;
    test2.y = triangles[i].vertices[1].y;
    test3.y = triangles[i].vertices[2].y;
    test1.depth = triangles[i].vertices[0].z;
    test2.depth = triangles[i].vertices[1].z;
    test3.depth = triangles[i].vertices[2].z;

    //PERSPECTIVE
    cameraP1x = test1.x - camera.x;
    cameraP1y = test1.y - camera.y;
    cameraP1z = test1.depth - camera.z;

    vec3 camPer1 = vec3(cameraP1x,cameraP1y,cameraP1z);
    vec3 adjustedPos1 = camPer1 * camOrien;

    //PROJECTION
    float p1Screen = focalDistance/-adjustedPos1.z;
    int P1XProj = (adjustedPos1.x * p1Screen * canvasScale) + WIDTH/2;
    int P1YProj = ((1 - adjustedPos1.y) * p1Screen * canvasScale) + HEIGHT/2;
    projP1.x = P1XProj;
    projP1.y = P1YProj;
    projP1.depth = 1/-(adjustedPos1.z);
    // projP1.texturePoint = texturePoints[p1];


    cameraP2x = test2.x - camera.x;
    cameraP2y = test2.y - camera.y;
    cameraP2z = test2.depth - camera.z;

    vec3 camPer2 = vec3(cameraP2x,cameraP2y,cameraP2z);
    vec3 adjustedPos2 = camPer2 * camOrien;

    float p2Screen = focalDistance/-adjustedPos2.z;
    int P2XProj = (adjustedPos2.x * p2Screen * canvasScale) + WIDTH/2;
    int P2YProj = ((1 - adjustedPos2.y) * p2Screen  * canvasScale) + HEIGHT/2;
    projP2.x = P2XProj;
    projP2.y = P2YProj;
    projP2.depth = 1/-(adjustedPos2.z);
    // projP2.texturePoint = texturePoints[p2];


    cameraP3x = test3.x - camera.x;
    cameraP3y = test3.y - camera.y;
    cameraP3z = test3.depth - camera.z;

    vec3 camPer3 = vec3(cameraP3x,cameraP3y,cameraP3z);
    vec3 adjustedPos3 = camPer3 * camOrien;

    float p3Screen = focalDistance/-adjustedPos3.z;
    int P3XProj = (adjustedPos3.x * p3Screen  * canvasScale) + WIDTH/2;
    int P3YProj = ((1 - adjustedPos3.y) * p3Screen  * canvasScale) + HEIGHT/2;
    projP3.x = P3XProj;
    projP3.y = P3YProj;
    projP3.depth = 1/-(adjustedPos3.z);
    // projP3.texturePoint = texturePoints[p3];


    canvastriangles.push_back(CanvasTriangle(projP1,projP2,projP3));
  }
  return canvastriangles;
}

void draw3DRasterisedTriangles(vector<CanvasTriangle> canvastriangles) {

  double **buffer = malloc2dArray(WIDTH, HEIGHT);
  for(u_int y = 0; y < HEIGHT; y++) {
    for(u_int x = 0; x < WIDTH; x++) {
      buffer[x][y] = -INFINITY;
    }
  }

  for(u_int i = 0; i < canvastriangles.size(); i++){

    CanvasPoint point1 = canvastriangles[i].vertices[0];
    CanvasPoint point2 = canvastriangles[i].vertices[1];
    CanvasPoint point3 = canvastriangles[i].vertices[2];
    Colour c = canvastriangles[i].colour;
    float red = c.red;
    float green = c.green;
    float blue = c.blue;
    uint32_t colour = (255<<24) + (int(red)<<16) + (int(green)<<8) + int(blue);

    for (int i = 0; i < 2; i++) {
      if (point3.y < point2.y) {
        CanvasPoint temp = point2;
        point2 = point3;
        point3 = temp;
      }
      if (point2.y < point1.y) {
        CanvasPoint temp = point1;
        point1 = point2;
        point2 = temp;
      }
      if (point3.y < point1.y) {
        CanvasPoint temp = point1;
        point1 = point3;
        point3 = temp;
      }
    }

    double ratio = (point3.y - point2.y)/(point3.y - point1.y);
    double point4Depth = point3.depth - ratio * (point3.depth - point1.depth);
    CanvasPoint point4 = CanvasPoint(point3.x - ratio * (point3.x - point1.x), point3.y - ratio * (point3.y - point1.y), point4Depth);
    vector<CanvasPoint> points1to4coord = interpolation2(point1,point4,(point2.y-point1.y)+1);
    vector<CanvasPoint> points1to2coord = interpolation2(point1,point2,(point2.y-point1.y)+1);
    vector<CanvasPoint> points4to3coord = interpolation2(point4,point3,(point3.y-point2.y)+1);
    vector<CanvasPoint> points2to3coord = interpolation2(point2,point3,(point3.y-point2.y)+1);

    for (u_int i = 0; i < points1to4coord.size(); i++) {
      vector<CanvasPoint> pointstofillcoord = interpolation2(points1to4coord[i],points1to2coord[i],abs(points1to4coord[i].x-points1to2coord[i].x)+1);
      for(u_int k = 0; k < pointstofillcoord.size(); k++) {
        CanvasPoint current = pointstofillcoord[k];
        if(((int)current.x >= 0) && ((int)current.x < WIDTH) && ((int)current.y >= 0) && ((int)current.y < HEIGHT)) {
          if(current.depth > buffer[(int)current.x][(int)current.y]) {
            buffer[(int)current.x][(int)current.y] = current.depth;
            window.setPixelColour((int)current.x, (int)current.y, colour);
          }
        }
      }
    }

    for (u_int i = 0; i < points4to3coord.size(); i++) {
      vector<CanvasPoint> pointstofillcoord = interpolation2(points4to3coord[i],points2to3coord[i],abs(points4to3coord[i].x-points2to3coord[i].x)+1);
      for(u_int k = 0; k < pointstofillcoord.size(); k++) {
        CanvasPoint current = pointstofillcoord[k];
        if((current.x >= 0) && ((int)current.x < WIDTH) && ((int)current.y >= 0) && ((int)current.y < HEIGHT)) {
          if(current.depth > buffer[(int)current.x][(int)current.y]) {
            buffer[(int)current.x][(int)current.y] = current.depth;
            window.setPixelColour((int)current.x, (int)current.y, colour);
          }
        }
      }
    }

  }
}

int main(int argc, char* argv[])
{
  SDL_Event event;
  vector<ModelTriangle> triangles = readtriangles("logo.obj");
  vector<CanvasTriangle> canvas = modelToCanvas(camera, triangles, 5, 100);
  draw3DRasterisedTriangles(canvas);
  while(true)
  {
    if(window.pollForInputEvents(&event)){
      // handleEvent(event);
      window.clearPixels();
    }
    window.renderFrame();
  }
}
