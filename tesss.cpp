void computeRayT(vector<ModelTriangle> triangles,vector<ModelTexture> textureTriangles,vector<vec3> whitelight,vector<uint32_t> colours, vec3 camera){
  for(int x=0;x<WIDTH;x++){
    for(int y=0;y<HEIGHT;y++){
      vec3 dir = rayDir(x,y);
      RayTriangleIntersection intersectP = getClosestIntersection(camera, dir, triangles);

      ModelTriangle trianglex = intersectP.intersectedTriangle;
      pair<ModelTriangle,vector<vec3>> rightOne = compare(trianglex);
      vector<vec3> normals = rightOne.second;

      vec3 e0 = trianglex.vertices[1] - trianglex.vertices[0];
      vec3 e1 = trianglex.vertices[2] - trianglex.vertices[0];
      vec3 SPVector = camera - (trianglex.vertices[0]);
      mat3 DEMatrix(-dir, e0, e1);
      vec3 possibleSolution = inverse(DEMatrix) * SPVector;
      float u = possibleSolution.y;
      float v = possibleSolution.z;

      Colour c = trianglex.colour;
      vec3 normaltovertices = normalize(cross(trianglex.vertices[1]-trianglex.vertices[0],trianglex.vertices[2]-trianglex.vertices[0]));
      uint32_t colour = (255<<24) + (int(c.red )<<16) + (int(c.green)<<8) + int(c.blue);
        if(intersectP.distanceFromCamera != -INFINITY){
          if(intersectP.intersectedTriangle.name == "cornell"){
            int i = getIndex(trianglex,triangles);
            float brightness = compBrightness(intersectP.intersectionPoint, normaltovertices);
            if(brightness > 1.0f) brightness = 1.0f;
            if(brightness < 0.2f) brightness = 0.2f;
            if(inShadow(intersectP.intersectionPoint, whitelight,triangles,i)){
              brightness = 0.15f;
            }
            colour = (255<<24) + (int(c.red * brightness)<<16) + (int(c.green * brightness)<<8) + int(c.blue * brightness);
          }
          else if(intersectP.intersectedTriangle.name == "sphere"){
            int i = getIndex(trianglex,triangles);
            // GOUROUD
            // float brightness1 = compBrightness(trianglex.vertices[0],normals[0]);
            // float brightness2 = compBrightness(trianglex.vertices[1],normals[1]);
            // float brightness3 = compBrightness(trianglex.vertices[2],normals[2]);
            // float brightness = brightness1 + (u*(brightness2-brightness1)) + (v*(brightness3-brightness1));
            //PHONG
            vec3 newNormal = normals[0] + (u*(normals[1]-normals[0])) + (v*(normals[2]-normals[0]));
            float brightness = compBrightness(intersectP.intersectionPoint,newNormal);
            if(brightness > 1.0f) brightness = 1.0f;
            if(brightness < 0.2f) brightness = 0.2f;
            if(inShadow(intersectP.intersectionPoint, whitelight,triangles,i)){
              brightness = 0.15f;
            }
           colour = (255<<24) + (int(c.red * brightness)<<16) + (int(c.green * brightness)<<8) + int(c.blue * brightness);
          }
          else if(intersectP.intersectedTriangle.name == "hackspace"){
            int i = getIndex(trianglex,triangles);
            pair<ModelTriangle,ModelTexture> rightTextureP = compareTP(trianglex);
            ModelTexture textureTriangle = rightTextureP.second;
            TexturePoint p1 = textureTriangle.vertices[0];
            TexturePoint p2 = textureTriangle.vertices[1];
            TexturePoint p3 = textureTriangle.vertices[2];

            vec2 tp1 = vec2(p1.x,p1.y) * 300.0f;
            vec2 tp2 = vec2(p2.x,p2.y) * 300.0f;
            vec2 tp3 = vec2(p3.x,p3.y) * 300.0f;
            vec2 texPoint = tp1 + (u*(tp2-tp1)) + (v*(tp3-tp1));
            uint32_t tempC = colours[round(texPoint.x) + round(texPoint.y) * 300 ];
            int red = (tempC >> 16) & 255;
            int green = (tempC >> 8) & 255;
            int blue = (tempC) & 255;
            float brightness = compBrightness(intersectP.intersectionPoint, normaltovertices);
            if(brightness > 1.0f) brightness = 1.0f;
            if(brightness < 0.2f) brightness = 0.2f;
            if(inShadow(intersectP.intersectionPoint, whitelight,triangles,i)){
              brightness = 0.15f;
            }
            colour = (255<<24) + (int(red * brightness)<<16) + (int(green * brightness)<<8) + int(blue * brightness);
          }
          window.setPixelColour(x, y, colour);
        }
    }
  }
}
